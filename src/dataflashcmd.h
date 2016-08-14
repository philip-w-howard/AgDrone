#pragma once

#include <list>
#include <time.h>

#include "cmdprocessor.h"
#include "loglistcmd.h"
#include "mavlinkif.h"
#include "datalist.h"
#include "queue.h"

class DataFlashCmd : public CommandProcessor
{
    public:
        DataFlashCmd(queue_t *agdrone_q, int client_socket, int msg_src, 
                int log_id);
        virtual ~DataFlashCmd();

        virtual void Start();
        virtual void Abort();
        virtual void ProcessMessage(mavlink_message_t *msg, int msg_src);
    protected:
        static const int OTHERS_PER_RETRY = 10;
        static const int DATA_PORT = 2004;

        int m_otherCount;
        int m_logId;
        bool m_receivedLastBlock;
        uint32_t m_highwater;
        uint32_t m_lastHighwater;
        DataList m_data;
        int m_msg_src;
        int m_data_socket;
        int m_data_server;

        LogListCmd m_logList;
        time_t m_logTime;
        uint32_t m_logEstSize;
        char m_logName[256];

        uint64_t m_dataPackets;
        uint64_t m_otherPackets;
        uint64_t m_dupPackets;
        uint64_t m_dataBytes;
        uint64_t m_numHoles;
        uint64_t m_holesBytes;

        bool m_detailLog;

        bool LookForHoles();
        void LogProgress();
        bool WriteLogFile();
};
