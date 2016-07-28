#ifndef _CMDPROCESSOR_H_
#define _CMDPROCESSOR_H_

#include "queue.h"
#include "mavlinkif.h"

class CommandProcessor
{
    public:
        CommandProcessor(queue_t *agdrone_q) 
        {
            m_agdrone_q = agdrone_q;
            m_finished = false;
        }
        virtual ~CommandProcessor() {}

        virtual void Start() = 0;
        virtual void Abort()        {}
        virtual void ProcessMessage(mavlink_message_t *msg, int msg_src)    {}
        virtual bool IsFinished()   { return m_finished; }
    protected:
        queue_t *m_agdrone_q;
        bool    m_finished;
};
#endif // _CMDPROCESSOR_H_
