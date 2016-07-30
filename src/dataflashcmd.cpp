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
    m_receivedLastBlock = false;
    m_detailLog = false;
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

        if (m_detailLog)
        {
            printf("Received id %d offset %d size %d\n",
                log_data.id, log_data.ofs, log_data.count);
        }

        if (m_data.Insert(log_data.ofs, log_data.count, log_data.data))
        {
            m_dataPackets++;
            m_dataBytes += log_data.count;
        }
        else
            m_dupPackets++;

        if (m_dataPackets % 1000 == 0) LogProgress();

        if (log_data.ofs + log_data.count > m_highwater)
            m_highwater = log_data.ofs + log_data.count;

        if (log_data.count < sizeof(log_data.data))
        {
            m_receivedLastBlock = true;

            if (!LookForHoles())
            {
                m_finished = true;
                m_data.ListAllBlocks();
                printf("LOG_DATA command is finished\n");
            }
        }

        m_otherCount = 0;
    }
    else
    {
        m_otherPackets++;

        if (++m_otherCount > OTHERS_PER_RETRY)
        {
            m_otherCount = 0;

            if (m_highwater == m_lastHighwater)
            {
                if (!LookForHoles())
                {
                    m_finished = true;
                    if (m_receivedLastBlock)
                    {
                        m_data.ListAllBlocks();
                        printf("LOG_DATA command is finished\n");
                    }
                    else
                        printf("LOG_DATA command is being aborted %d %d\n",
                                m_highwater, m_lastHighwater);
                }
            } 
            else 
            {
                m_lastHighwater = m_highwater;
            }
        }
        //printf("Received %d\n", msg->msgid);
    }
}

bool DataFlashCmd::LookForHoles()
{
    int start = -1;
    int size;
    bool requestedMore = false;
    static uint64_t lastHolesSize = 0;

    //printf("Looking for holes\n");
    fflush(stdout);

    if (m_detailLog) m_data.ListAllBlocks();

    m_numHoles = 0;
    m_holesBytes = 0;

    while (m_data.GetHole(&start, &size, start))
    {
        m_numHoles++;
        m_holesBytes += size;

        if (m_numHoles <= 5)
        {
            if (m_detailLog)
                printf("DATA_FLASH filling hole: %d %d\n", start, size);
            send_log_request_data(m_agdrone_q, 0x45, 0x67, 
                    m_logId, start, size);
        }
        requestedMore = true;
    }

    // handle the end-of-file
    if (size == -1 && !m_receivedLastBlock)
    {
        if (m_detailLog)
            printf("DATA_FLASH filling hole: %d %d\n", start, size);
        send_log_request_data(m_agdrone_q, 0x45, 0x67, m_logId, start, size);
        requestedMore = true;
    }

    LogProgress();
    if (lastHolesSize == m_holesBytes)
        m_detailLog = true;
    else
        m_detailLog = false;

    lastHolesSize = m_holesBytes;


    if (!requestedMore) printf("didn't find any holes\n");
    return requestedMore;
}

//***********************************************
void DataFlashCmd::LogProgress()
{
    printf("Pkts: %lld %lld %lld Bytes: %d %lld Holes: %lld %lld\n",
            m_dataPackets, m_otherPackets, m_dupPackets,
            m_highwater, m_dataBytes,
            m_numHoles, m_holesBytes);
    fflush(stdout);
}

