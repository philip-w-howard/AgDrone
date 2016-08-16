#pragma once

#include <list>
#include <time.h>

#include "cmdprocessor.h"
#include "loglistcmd.h"
#include "mavlinkif.h"
#include "datalist.h"
#include "queue.h"

class GetTlogsCmd : public CommandProcessor
{
    public:
        GetTlogsCmd(queue_t *agdrone_q, int client_socket, int msg_src); 
        virtual ~GetTlogsCmd();

        virtual void Start();
        virtual void Abort();
        virtual void ProcessMessage(mavlink_message_t *msg, int msg_src);
    protected:
        static const int DATA_PORT = 2004;
        static const int BLOCKS_PER_MSG = 2;

        class element_t
        {
            public:
                element_t(char *filename, int size)
                {
                    strcpy(m_filename, filename);
                    m_size = size;
                }
                char m_filename[256];
                int m_size;
        };

        std::list<element_t> m_fileList;

        int m_data_socket;
        int m_data_server;
        int m_logfile;
        int64_t m_bytesSent;
        int64_t m_totalBytesSent;
        char m_buffer[1024*1024];

        void LogProgress();
        void MakeDataConnection();
        void GetFileList();
};
