#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

#include "loglistcmd.h"

#include "log.h"

//**********************************************
LogListCmd::LogListCmd(queue_t *q, int client_socket, int msg_src, int id) 
    : CommandProcessor(q, client_socket, msg_src)
{
    m_other_count = 0;
    m_filled_entries = 0;
    m_entries_capacity = 0;
    m_entries = NULL;
    m_finished = false;
    m_id = id;
}

//**********************************************
LogListCmd::~LogListCmd()
{
    if (m_entries != NULL) free(m_entries);
}

//**********************************************
void LogListCmd::Start()
{
    if (m_id == 0)
    {
        WriteLog("Starting LOG_LIST command\n");
        send_log_request_list(m_agdrone_q, 0x45, 0x67, 0, -1);
    }
    else
    {
        WriteLog("Starting LOG_LIST %d command\n", m_id);
        send_log_request_list(m_agdrone_q, 0x45, 0x67, m_id, m_id);
    }
    m_finished = false;
}
//**********************************************
void LogListCmd::Start(int id)
{
    m_id = id;
    Start();
}
//**********************************************
void LogListCmd::Abort()
{
}
//**********************************************
void LogListCmd::ProcessMessage(mavlink_message_t *msg, int msg_src)
{
    if (m_finished) return;

    if (msg->msgid == MAVLINK_MSG_ID_LOG_ENTRY) 
    {
        mavlink_log_entry_t log_entry;
        mavlink_msg_log_entry_decode(msg, &log_entry);
        if (log_entry.last_log_num > m_entries_capacity)
        {
            m_entries = (entry_t *)realloc(m_entries, 
                    sizeof(entry_t)*(log_entry.last_log_num + 1));
            assert(m_entries != NULL);

            for (int ii=m_entries_capacity; ii<=log_entry.last_log_num; ii++)
            {
                m_entries[ii].filled = false;
            }

            m_entries_capacity = log_entry.last_log_num + 1;
        }

        if (log_entry.id >= m_entries_capacity)
        {
            WriteLog("Invalid log entry************************\n");
            WriteLog("Log entry: id %d size %d num %d last %d\n",
                log_entry.id, log_entry.size, log_entry.num_logs,
                log_entry.last_log_num);
        }
        else if (m_id != 0 && log_entry.id == m_id)
        {
            m_entries[0].entry = log_entry;
            if (!m_entries[0].filled)
            {
                m_entries[0].filled = true;
                m_filled_entries++;
            }
            WriteLog("Log entry[0]: id %d size %d num %d last %d\n",
                        log_entry.id, log_entry.size, log_entry.num_logs,
                        log_entry.last_log_num);
        }
        else
        {
            m_entries[log_entry.id].entry = log_entry;
            if (!m_entries[log_entry.id].filled)
            {
                m_entries[log_entry.id].filled = true;
                m_filled_entries++;
            }
            WriteLog("Log entry: id %d size %d num %d last %d\n",
                        log_entry.id, log_entry.size, log_entry.num_logs,
                        log_entry.last_log_num);
        }

        if (m_filled_entries == log_entry.num_logs ||
            (m_id != 0 && log_entry.id == m_id))
        {
            m_finished = true;
            WriteLog("LOG_LIST command is finished\n");
            SendEntries();
        }
        else
            WriteLog("filled: %d expecting %d\n", 
                    m_filled_entries, log_entry.num_logs);
    }
    else
    {
        if (++m_other_count > OTHERS_PER_RETRY)
        {
            if (m_filled_entries == 0)
            {
                send_log_request_list(m_agdrone_q, 0x45, 0x67, 0, -1);
            }
            else
            {
                for (int ii=1; ii<m_entries_capacity; ii++)
                {
                    if (!m_entries[ii].filled)
                    {
                        send_log_request_list(m_agdrone_q, 0x45, 0x67, ii, -1);
                        break;
                    }
                }
            }
        }
        WriteLog("Received %d\n", msg->msgid);
    }
}
//**********************************************
int LogListCmd::NumLogs()
{
    return m_entries_capacity;
}
//**********************************************
int LogListCmd::ValidLog(int index)
{
    if (index >= m_entries_capacity) return false;
    return m_entries[index].filled;
}
//**********************************************
mavlink_log_entry_t *LogListCmd::LogEntry(int index)
{
    if (index >= m_entries_capacity) return NULL;
    if (!m_entries[index].filled) return NULL;

    return &m_entries[index].entry;
}
//**********************************************
void LogListCmd::SendEntries()
{
    int ii;
    time_t log_time;
    mavlink_log_entry_t *entry;
    char buff[500];

    for (ii=0; ii<NumLogs(); ii++)
    {
        if (ValidLog(ii))
        {
            entry = LogEntry(ii);
            log_time = entry->time_utc;
            WriteLog("Log Entry: %d %d %ld %s", 
                    entry->id, entry->size, log_time, ctime(&log_time));
            if (m_msg_src == MSG_SRC_AGDRONE_CONTROL && m_client_socket >= 0)
            {
                sprintf(buff, "LogEntry %d %d %ld %s\n",
                        entry->id, entry->size, log_time, ctime(&log_time));
                write(m_client_socket, buff, strlen(buff));
            }
        }
    }
    if (m_msg_src == MSG_SRC_AGDRONE_CONTROL && m_client_socket >= 0)
    {
        sprintf(buff, "LogEntryDone\n");
        write(m_client_socket, buff, strlen(buff));
    }
}


