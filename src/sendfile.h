#pragma once

#include <openssl/md5.h>

class SendFile 
{
    public:
        SendFile(int client_socket);
        virtual ~SendFile();

        void SendTo(char *destname);
        void Send(char *srcname);
        void Send(unsigned char *buff, int size);
        void Abort();
        void Finish();
    protected:
        MD5_CTX m_md5Summer;
        unsigned char m_md5Sum[MD5_DIGEST_LENGTH];
        unsigned char m_md5CharSum[MD5_DIGEST_LENGTH*2 + 5];
        char m_destname[512];
        char m_srcname[512];

        int m_client_socket;
        int m_data_server;
        int m_data_socket;

        bool m_finished;
};
