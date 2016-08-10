#pragma once

#include "cmdprocessor.h"
#include "mavlinkif.h"
#include "queue.h"

class GetFileCmd : public CommandProcessor
{
    public:
        GetFileCmd(queue_t *agdrone_q, int socket_fd, char *command);
        virtual ~GetFileCmd();

        virtual void Start();
        virtual void Abort();

    protected:
        char m_command[256];
        int m_socket_fd;
        char m_buffer[1024*1024];
};
