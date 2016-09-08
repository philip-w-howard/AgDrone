#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "sendfile.h"
#include "log.h"

//**********************************************
SendFile::SendFile()
{
    MD5_Init(&m_md5Summer);
    memset(m_md5Sum, 0, sizeof(m_md5Sum));
}

//**********************************************
SendFile::~SendFile()
{
}
//**********************************************
void SendFile::SendTo(char *destname)
{
    strcpy(m_destname, destname);
}
//**********************************************
void SendFile::Send(char *srcname)
{
    int input;
    unsigned char block[512];
    int size;

    strcpy(m_srcname, srcname);
    WriteLog("Sending %s\n", m_srcname);
    input = open(m_srcname, O_RDONLY);
    if (input < 0)
    {
        WriteLog("Unable to open %s\n", m_srcname);
        Finish();
    }

    size = read(input, block, sizeof(block));
    while (size > 0)
    {
        Send(block, size);
        size = read(input, block, sizeof(block));
    }

    close(input);
    Finish();
}
//**********************************************
void SendFile::Send(unsigned char buff, int size)
{
    MD5_Update(&m_md5Summer, buff, size);

}
//**********************************************
void SendFile::Abort()
{
}
//**********************************************
void SendFile::Finish()
{
    int ii;

    MD5_Final(m_md5Sum, &m_md5Summer);

    strcpy(m_md5CharSum, "");
    for (ii=0; ii<MD5_DIGEST_LENGTH; ii++)
    {
        strcat(m_md5CharSum, "%02x");
        sprintf(m_md5CharSum, m_md5Sum[ii]);
    }

    WriteLog("MD5 Sum: %s\n", m_md5CharSum);
}

void Start(char *dstFile, int size)
{
    // open socket
    struct sockaddr_in serv_addr;
    m_data_server = socket(AF_INET, SOCK_STREAM, 0);
    if (m_data_server < 0)
    {
        WriteLog("Failed to open data server");
        // Report to AgDroneCtrl
        m_finished = true;
        return;
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(DATA_PORT);
    if (bind(m_data_server, (struct sockaddr *) &serv_addr,
               sizeof(serv_addr)) < 0)
    {
        WriteLog("Failed to bind to data server");
        // Report to AgDroneCtrl
        m_finished = true;
        return;
    }

    listen(m_data_server, 5);

    WriteLog("Starting file transfer: %s\n", dstFile);

    char msg[200];
    sprintf(msg, "filename %s\nfilesize %d\n", dstFile, size)
    write(m_client_socket, msg, strlen(msg));

    struct sockaddr_in data_addr;
    socklen_t data_len;
    data_len = sizeof(data_addr);
    m_data_socket = accept(m_data_server, 
            (struct sockaddr *)&data_addr, &data_len);

    if (m_data_socket < 0)
    {
        WriteLog("Failed to connect to data client");
        // Report to AgDroneCtrl
        m_finished = true;
        return;
    }
}
