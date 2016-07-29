#pragma once

#include <list>

#include "cmdprocessor.h"
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
        static const int OTHERS_PER_RETRY = 20;

        int m_otherCount;
        int m_logId;
        uint32_t m_highwater;
        uint32_t m_lastHighwater;
        DataList m_data;
};
