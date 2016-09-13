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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "gettlogscmd.h"

#include "log.h"
#include "sendfile.h"

//**********************************************
GetTlogsCmd::GetTlogsCmd(queue_t *q, int client_socket, int msg_src) :
    CommandProcessor(q, client_socket, msg_src) 
{
}

//**********************************************
GetTlogsCmd::~GetTlogsCmd()
{
}

//**********************************************
void GetTlogsCmd::Start()
{
    m_finished = false;
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
    if (!m_finished && m_fileList.size() > 0)
    {
        element_t file = m_fileList.front();
        m_fileList.pop_front();

        char filename[256];
        strcpy(filename, "/media/sdcard/");
        strcat(filename, file.m_filename);

        write(m_client_socket, "sendingfile\n", strlen("sendingfile\n"));
        SendFile sender(m_client_socket);
        sender.Start(file.m_filename, file.m_size);
        sender.Send(filename);

    }
    else if (!m_finished)
    {
        write(m_client_socket, "tlogsdone\n", strlen("sendingfile\n"));
        m_finished = true;
    }

}

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
}
