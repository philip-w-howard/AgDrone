/*
 * wifiserver.cpp
 *
 *  Created on: Jul 23, 2015
 *      Author: philhow
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <assert.h>

#include "mavlinkif.h"

#include "queue.h"
#include "agdronecmd.h"
#include "connection.h"

#include "loglistcmd.h"

#define TYPE_CMD 0
#define TYPE_MSG 1

typedef struct
{
    int type;
    int msg_src;
    mavlink_message_t msg;
    char cmd[512];
} queued_cmd_t;

void *socket_thread(void *param)
{
    AgDroneCmd *agdrone = (AgDroneCmd *)param;

    agdrone->ProcessSocket();

    return NULL;
}

void *cmd_thread(void *param)
{
    AgDroneCmd *agdrone = (AgDroneCmd *)param;

    agdrone->ProcessCmds();

    return NULL;
}

//***************************************
AgDroneCmd::AgDroneCmd(queue_t *agdrone_q, int port)
{
    mPort = port;
    mIsConnected = false;
    mListenerFd = -1;
    mFileDescriptor = -1;
    m_agdrone_q = agdrone_q;
    m_cmd_q = queue_create();

    m_cmd_thread = 0;
    m_socket_thread = 0;
    m_cmd_proc = NULL;

    mListenerFd = socket(AF_INET, SOCK_STREAM, 0);
    if (mListenerFd < 0)
    {
       perror("ERROR opening AgDroneCmd socket. Terminating program.\n");
       exit(-1);
    }

}
//***************************************
AgDroneCmd::~AgDroneCmd()
{
    Stop();
}
//***************************************
void AgDroneCmd::Start()
{
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(mPort);
    if (bind(mListenerFd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding. Terminating program.\n");
        close(mListenerFd);
        mListenerFd = -1;
        exit(-1);
    }

    m_Running = true;

    listen(mListenerFd,5);
    fprintf(stderr, "Listening for WiFi connections\n");

    pthread_create(&m_cmd_thread, NULL, cmd_thread, this);
    pthread_create(&m_socket_thread, NULL, socket_thread, this);
}
//***************************************
bool AgDroneCmd::MakeConnection()
{
    socklen_t cli_len;
    struct sockaddr_in cli_addr;
    cli_len = sizeof(cli_addr);

    mFileDescriptor = -1;
    while (mFileDescriptor < 0)
    {
        mFileDescriptor = accept(mListenerFd,
                (struct sockaddr *) &cli_addr,
                &cli_len);
        if (mFileDescriptor < 0)
        {
            perror("ERROR on accept: retrying");
            sleep(1);
        }
    }

    // Set a timeout on the socket reads
    struct timeval tv;

    tv.tv_sec = 5;  // 5 Secs Timeout
    tv.tv_usec = 0;  // Not init'ing this can cause strange errors

    setsockopt(mFileDescriptor, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, 
            sizeof(struct timeval));

    fprintf(stderr, "processing AgDroneCmd data on %d\n", mFileDescriptor);

    mIsConnected = true;

    return true;
}

//***************************************
void AgDroneCmd::Stop()
{
    m_Running = false;

    queue_close(m_cmd_q);

    if (m_cmd_thread != 0) pthread_join(m_cmd_thread, NULL);
    if (m_socket_thread != 0) pthread_join(m_socket_thread, NULL);

    close(mListenerFd);
    mListenerFd = -1;
    close(mFileDescriptor);
    mIsConnected = false;
}

//***************************************
/*
bool AgDroneCmd::IsConnected()
{
    return mListenerFd >= 0 && mIsConnected;
}
*/

//***************************************
void AgDroneCmd::QueueMsg(mavlink_message_t *msg, int msg_src)
{
    queued_cmd_t *item;

    // ignore messages if we aren't currently processing a command
    if (CommandIsActive()) 
    {
        item = (queued_cmd_t *)malloc(sizeof(queued_cmd_t));
        assert(item != NULL);

        memcpy(&item->msg, msg, sizeof(mavlink_message_t));

        item->msg_src = msg_src;
        item->type = TYPE_MSG;

        queue_insert(m_cmd_q, item);
    }
}

//***************************************
void AgDroneCmd::QueueCmd(const char *cmd, int cmd_src)
{
    queued_cmd_t *item;

    item = (queued_cmd_t *)malloc(sizeof(queued_cmd_t));
    assert(item != NULL);

    strncpy(item->cmd, cmd, sizeof(item->cmd));
    item->cmd[sizeof(item->cmd)-1] = 0;

    item->msg_src = cmd_src;
    item->type = TYPE_CMD;

    queue_insert(m_cmd_q, item);
}
//***************************************
void AgDroneCmd::ProcessCommand(char *command)
{
    if (m_cmd_proc != NULL)
    {
        m_cmd_proc->Abort();
        delete m_cmd_proc;
        m_cmd_proc = NULL;
    }

    if (strcmp(command, "loglist") == 0)
    {
        m_cmd_proc = new LogListCmd(m_agdrone_q);
        m_cmd_proc->Start();
    }
}
//***************************************
void AgDroneCmd::ProcessMessage(mavlink_message_t *msg, int msg_src)
{
    if (CommandIsActive()) m_cmd_proc->ProcessMessage(msg, msg_src);
}
//***************************************
void AgDroneCmd::ProcessCmds()
{
    queued_cmd_t *item;

    while(queue_is_open(m_cmd_q) && m_Running)
    {
        item = (queued_cmd_t *)queue_remove(m_cmd_q);
        if (item != NULL)
        {
            if (item->type == TYPE_CMD)
            {
                ProcessCommand(item->cmd);
            }
            else if (CommandIsActive())
            {
                ProcessMessage(&item->msg, item->msg_src);
            }
            else
            {
                // simply ignore the message
            }

            free(item);
        }
    }
}
//***************************************
void AgDroneCmd::ProcessSocket()
{
}
//***************************************
bool AgDroneCmd::CommandIsActive()
{
    return (m_cmd_proc != NULL) && !m_cmd_proc->IsFinished();
}
