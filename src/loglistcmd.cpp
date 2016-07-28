#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "loglistcmd.h"

//**********************************************
LogListCmd::LogListCmd(queue_t *q) : CommandProcessor(q)
{
    m_other_count = 0;
    m_filled_entries = 0;
    m_entries_capacity = 0;
    m_entries = NULL;
    m_finished = false;
}

//**********************************************
LogListCmd::~LogListCmd()
{
    if (m_entries != NULL) free(m_entries);
}

//**********************************************
void LogListCmd::Start()
{
    printf("Starting LOG_LIST command\n");
    send_log_request_list(m_agdrone_q, 0x45, 0x67, 0, -1);
    m_finished = false;
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
            printf("Invalid log entry************************\n");
            printf("Log entry: id %d size %d num %d last %d\n",
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
            printf("Log entry: id %d size %d num %d last %d\n",
                        log_entry.id, log_entry.size, log_entry.num_logs,
                        log_entry.last_log_num);
        }

        if (m_filled_entries == log_entry.num_logs)
        {
            m_finished = true;
            printf("LOG_LIST command is finished\n");
        }
        else
            printf("filled: %d expecting %d\n", 
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
        printf("Received %d\n", msg->msgid);
    }
}
