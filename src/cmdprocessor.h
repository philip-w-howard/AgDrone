#pragma once

#include "queue.h"
#include "mavlinkif.h"
#include "connection.h"

class CommandProcessor
{
    public:
        CommandProcessor(queue_t *agdrone_q, 
                int client_socket = -1, 
                int msg_src = MSG_SRC_SELF) 
        {
            m_agdrone_q = agdrone_q;
            m_client_socket = client_socket;
            m_msg_src = msg_src;
            m_finished = false;
        }
        virtual ~CommandProcessor() {}

        virtual void Start() = 0;
        virtual void Abort()        {}
        virtual void ProcessMessage(mavlink_message_t *msg, int msg_src)    {}
        virtual bool IsFinished()   { return m_finished; }
    protected:
        queue_t *m_agdrone_q;
        int     m_client_socket;
        int     m_msg_src;
        bool    m_finished;
};
