#pragma once

#include <list>
#include <time.h>

#include "cmdprocessor.h"
#include "loglistcmd.h"
#include "mavlinkif.h"
#include "datalist.h"
#include "queue.h"
#include "sendfile.h"

class GetImagesCmd : public CommandProcessor
{
    public:
        GetImagesCmd(queue_t *agdrone_q, int client_socket, int msg_src); 
        virtual ~GetImagesCmd();

        virtual void Start();
        virtual void Abort();
        virtual void ProcessMessage(mavlink_message_t *msg, int msg_src);
    protected:
        class element_t
        {
            public:
                element_t(char *folder, char *filename, int size)
                {
                    strcpy(m_filename, filename);
                    m_size = size;
                }
                char m_folder[256];
                char m_filename[256];
                int m_size;
        };

        std::list<element_t> m_fileList;
        SendFile m_sender;

        void GetFileList();
};
