#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>

#include "sendfile.h"
#include "log.h"

//**********************************************
SendFile::SendFile(int client_socket)
{
    m_client_socket = client_socket;

    m_data_server = -1;
    m_data_socket = -1;
    m_finished = false;

    // open socket
    struct sockaddr_in serv_addr;
    m_data_server = socket(AF_INET, SOCK_STREAM, 0);
    if (m_data_server < 0)
    {
        WriteLog("Failed to open data server\n");
        perror("Failed to open data server\n");
        // Report to AgDroneCtrl
        m_finished = true;
        m_data_server = -1;
        m_data_socket = -1;

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
}

//**********************************************
SendFile::~SendFile()
{
    if (m_data_server != -1) 
    {
        WriteLog("Closing data server\n");
        close(m_data_server);
    }
    if (m_data_socket != -1) 
    {
        WriteLog("Closing data socket\n");
        close(m_data_socket);
    }
    m_data_server = -1;
    m_data_socket = -1;
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
void SendFile::Send(unsigned char *buff, int size)
{
    MD5_Update(&m_md5Summer, buff, size);

    if (m_data_socket >= 0) write(m_data_socket, buff, size);
}
//**********************************************
void SendFile::Abort()
{
}
//**********************************************
void SendFile::Finish()
{
    int ii;
    char charBuff[4];

    MD5_Final(m_md5Sum, &m_md5Summer);

    strcpy(m_md5CharSum, "");
    for (ii=0; ii<MD5_DIGEST_LENGTH; ii++)
    {
        sprintf(charBuff, "%02x", m_md5Sum[ii]);
        strcat(m_md5CharSum, charBuff);
    }

    WriteLog("MD5 Sum: %s\n", m_md5CharSum);

    if (m_data_socket != -1) 
    {
        WriteLog("Closing data socket\n");
        close(m_data_socket);
    }
    m_data_socket = -1;

    char md5buff[sizeof(m_md5CharSum) + 20];
    sprintf(md5buff, "md5sum %s\n", m_md5CharSum);
    write(m_client_socket, md5buff, strlen(md5buff));
}
//**********************************************
void SendFile::Start(char *dstFile, int size)
{
    m_finished = false;

    MD5_Init(&m_md5Summer);
    memset(m_md5Sum, 0, sizeof(m_md5Sum));

    WriteLog("Starting file transfer: %s\n", dstFile);

    char msg[200];
    sprintf(msg, "filename %s\nfilesize %d\n", dstFile, size);
    write(m_client_socket, msg, strlen(msg));

    struct sockaddr_in data_addr;
    socklen_t data_len;
    data_len = sizeof(data_addr);
    m_data_socket = accept(m_data_server, 
            (struct sockaddr *)&data_addr, &data_len);

    if (m_data_socket < 0)
    {
        WriteLog("Failed to connect to data client\n");
        // Report to AgDroneCtrl
        m_data_socket = -1;
        m_finished = true;
        return;
    }
}
