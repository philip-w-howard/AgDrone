#pragma once

#include "cmdprocessor.h"
#include "mavlinkif.h"
#include "queue.h"

class GetTimeCmd : public CommandProcessor
{
    public:
        GetTimeCmd(queue_t *agdrone_q);
        virtual ~GetTimeCmd();

        virtual void Start();
        virtual void Abort();
        virtual void ProcessMessage(mavlink_message_t *msg, int msg_src);

        time_t GetTime();

    protected:
        time_t m_time;
};
