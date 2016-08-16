#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <errno.h>

#include "gettlogscmd.h"

#include "log.h"

//**********************************************
GetTlogsCmd::GetTlogsCmd(queue_t *q, int client_socket, int msg_src) :
    CommandProcessor(q, client_socket, msg_src) 
{
    m_data_socket = -1;
    m_data_server = -1;
}

//**********************************************
GetTlogsCmd::~GetTlogsCmd()
{
}

//**********************************************
void GetTlogsCmd::Start()
{
    m_finished = false;

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
        WriteLog("Failed to bind to  data server");
        // Report to AgDroneCtrl
        m_finished = true;
        return;
    }

    listen(m_data_server, 5);

    WriteLog("Processing GetTlogsCmd\n");

    GetFileList();
}
//**********************************************
void GetTlogsCmd::Abort()
{
}
//**********************************************
void GetTlogsCmd::ProcessMessage(mavlink_message_t *msg, int msg_src)
{
    if (m_finished) return;
}

//***********************************************
void GetTlogsCmd::MakeDataConnection()
{
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

//***********************************************
void GetTlogsCmd::LogProgress()
{
    WriteLog("gettlogs files left: %d\n", m_fileList.size());
}

#include <errno.h>
#include <stdio.h>
#include <string.h>

void GetTlogsCmd::GetFileList()
{
    DIR *dirp;
    struct dirent *dp;

    if ((dirp = opendir("/media/sdcard")) == NULL) 
    {
        perror("couldn't open '/media/sdcard'");
        WriteLog("Couldn't open directory: /media/sdcard\n");
        m_finished = true;
        return;
    }

    do 
    {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL) 
        {
            WriteLog("Found file: %s\n", dp->d_name);
            if (strstr(dp->d_name, ".tlog") != NULL)
            {
                WriteLog("Adding to list: %s\n", dp->d_name);
                m_fileList.push_back(element_t(dp->d_name, 0));
            }
        }
    } while (dp != NULL);


    if (errno != 0)
    {
        WriteLog("Error reading log directory\n");
        m_finished = true;
    }

    (void) closedir(dirp);

    std::list<element_t>::iterator it;

    WriteLog("Found %d tlogs\n", m_fileList.size());

    int index = 1;
    for (it=m_fileList.begin(); it != m_fileList.end(); it++)
    {
        WriteLog("Log: %d %s\n", index++, it->m_filename);
    }

    m_finished = true;
}
