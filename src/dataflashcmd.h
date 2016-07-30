#pragma once

#include <list>

#include "cmdprocessor.h"
#include "loglistcmd.h"
#include "mavlinkif.h"
#include "datalist.h"
#include "queue.h"

class DataFlashCmd : public CommandProcessor
{
    public:
        DataFlashCmd(queue_t *agdrone_q, int log_id);
        virtual ~DataFlashCmd();

        virtual void Start();
        virtual void Abort();
        virtual void ProcessMessage(mavlink_message_t *msg, int msg_src);
    protected:
        static const int OTHERS_PER_RETRY = 10;

        int m_otherCount;
        int m_logId;
        bool m_receivedLastBlock;
        uint32_t m_highwater;
        uint32_t m_lastHighwater;
        DataList m_data;

        LogListCmd m_logList;
        uint32_t m_logTime;
        uint32_t m_logEstSize;

        uint64_t m_dataPackets;
        uint64_t m_otherPackets;
        uint64_t m_dupPackets;
        uint64_t m_dataBytes;
        uint64_t m_numHoles;
        uint64_t m_holesBytes;

        bool m_detailLog;

        bool LookForHoles();
        void LogProgress();
};
