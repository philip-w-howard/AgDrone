#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "dataflashcmd.h"

//**********************************************
DataFlashCmd::DataFlashCmd(queue_t *q, int log_id) : CommandProcessor(q)
{
    m_otherCount = 0;
    m_highwater = 0;
    m_logId = log_id;
    m_lastHighwater = 0;
}

//**********************************************
DataFlashCmd::~DataFlashCmd()
{
}

//**********************************************
void DataFlashCmd::Start()
{
    printf("Starting DATA_FLASH command for id %d\n", m_logId);
    send_log_request_data(m_agdrone_q, 0x45, 0x67, m_logId, 0, -1);
    m_finished = false;
}
//**********************************************
void DataFlashCmd::Abort()
{
}
//**********************************************
void DataFlashCmd::ProcessMessage(mavlink_message_t *msg, int msg_src)
{
    if (m_finished) return;

    if (msg->msgid == MAVLINK_MSG_ID_LOG_DATA) 
    {
        mavlink_log_data_t log_data;
        mavlink_msg_log_data_decode(msg, &log_data);

        printf("Received id %d offset %d size %d\n",
                log_data.id, log_data.ofs, log_data.count);
        m_data.Insert(log_data.ofs, log_data.count, log_data.data);

        if (log_data.ofs + log_data.count > m_highwater)
            m_highwater = log_data.ofs + log_data.count;

        if (log_data.count < sizeof(log_data.data))
        {
            m_finished = true;
            printf("LOG_DATA command is finished\n");
        }
    }
    else
    {
        if (++m_otherCount > OTHERS_PER_RETRY)
        {
            if (m_highwater == m_lastHighwater)
            {
                m_finished = true;
                printf("LOG_DATA command is being aborted %d %d\n",
                        m_highwater, m_lastHighwater);
            } 
            else 
            {
                m_highwater = m_lastHighwater;
                m_otherCount = 0;
            }
                /*
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
                */
            }
        printf("Received %d\n", msg->msgid);
    }
}
