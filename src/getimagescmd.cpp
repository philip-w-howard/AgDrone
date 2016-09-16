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

#include "getimagescmd.h"

#include "log.h"
#include "sendfile.h"

//**********************************************
GetImagesCmd::GetImagesCmd(queue_t *q, int client_socket, int msg_src) :
    CommandProcessor(q, client_socket, msg_src),
    m_sender(m_client_socket)
{
}

//**********************************************
GetImagesCmd::~GetImagesCmd()
{
}

//**********************************************
void GetImagesCmd::Start()
{
    m_finished = false;
    WriteLog("Processing GetImagesCmd\n");

    GetFileList();
}
//**********************************************
void GetImagesCmd::Abort()
{
}
//**********************************************
void GetImagesCmd::ProcessMessage(mavlink_message_t *msg, int msg_src)
{
    if (!m_finished && m_fileList.size() > 0)
    {
        element_t file = m_fileList.front();
        m_fileList.pop_front();

        WriteLog("Pretending to retrieve %s/%s %d\n",
                file.m_folder, file.m_filename, file.m_size);
        //write(m_client_socket, "sendingfile\n", strlen("sendingfile\n"));
        //m_sender.Start(file.m_filename, filesize);
        //m_sender.Send(filename);

    }
    else if (!m_finished)
    {
        write(m_client_socket, "imagesdone\n", strlen("imagesdone\n"));
        m_finished = true;
    }
}

void GetImagesCmd::GetFileList()
{
    char *file_p;
    char *folder_p;
    char folder[256] = "";
    char *size_p;
    int size;
    char input_line[512];
    char *token;

    FILE *dir_list = popen("gphoto2 --list-files", "r");
    if (dir_list == NULL) 
    {
        WriteLog("Unable to get file list\n");
        m_finished = true;
        return;
    }

    while (fgets(input_line, sizeof(input_line), dir_list) != NULL)
    {
        token = strtok(input_line, " \n");
        if (strcmp(token, "There") == 0 &&
            strcmp(strtok(NULL, " \n"), "are") == 0)
        {
            strtok(NULL, "'");
            folder_p = strtok(NULL, "'");
            if (folder_p != NULL) 
            {
                strcpy(folder, folder_p);
                WriteLog("Looking in folder %s\n", folder);
            }
        }
        else if (token[0] == '#')
        {
            file_p = strtok(NULL, " \n");
            if (file_p != NULL && strlen(file_p) > 0)
            {
                strtok(NULL, " \n");
                size_p = strtok(NULL, " \n");
                if (size_p == NULL || strlen(size_p) == 0) 
                    size = 0;
                else
                    size = atoi(size_p);
                WriteLog("Adding to list: %s\n", file_p);
                m_fileList.push_back(element_t(folder, file_p, size));
            }
        }
    }

    pclose(dir_list);

    std::list<element_t>::iterator it;

    WriteLog("Found %d images\n", m_fileList.size());
}
