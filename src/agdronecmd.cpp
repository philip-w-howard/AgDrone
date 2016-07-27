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

#include "queue.h"
#include "agdronecmd.h"

#define TYPE_CMD 0
#define TYPE_MSG 1

typedef struct
{
    int type;
    int msg_src;
    mavlink_msg_t *msg;
    char command[512];
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

void *msg_thread(void *param)
{
    AgDroneCmd *agdrone = (AgDroneCmd *)param;
    agdrone->ProcessMsgs();

    return NULL;
}

//***************************************
AgDroneCmd::AgDroneCmd(queue_t *pixhawk_q, int port)
{
    mPort = port;
    mIsConnected = false;
    mListenerFd = -1;
    m_pixhawk_q = pixhawk_q;
    m_cmd_q = init_queue();

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
    close(mListenerFd);
    mListenerFd = -1;
    close(mFileDescriptor);
    mIsConnected = false;
}
//***************************************
AgDroneCmd::Start()
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

    listen(mListenerFd,5);
    fprintf(stderr, "Listening for WiFi connections\n");

    int result;
    result = pthread_create(&m_msg_thread, NULL, msg_thread, this);
    result = pthread_create(&m_cmd_thread, NULL, cmd_thread, this);
    result = pthread_create(&m_socket_thread, NULL, socket_thread, this);
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
void AgDroneCmd::Disconnect()
{
    close(mListenerFd);
    mListenerFd = -1;
    close(mFileDescriptor);
    mIsConnected = false;
}

//***************************************
bool AgDroneCmd::IsConnected()
{
    return mListenerFd >= 0 && mIsConnected;
}

//***************************************
void AgDroneCmd::QueueMsg(mavlink_message_t *msg, int msg_src)
{
    queued_cmd_t *item;

    item = (queued_cmd_t *)malloc(sizeof(queued_cmd_t));
    assert(item != NULL);

    item->msg = (mavlink_message_t *)malloc(sizeof(mavlink_message_t));
    assert(item->msg != NULL);
    memcpy(item->msg, msg, sizeof(mavlink_message_t));

    item->msg_src = msg_src;
    item->type = TYPE_MSG;

    queue_insert(m_cmd_q, item);
}

//***************************************
void AgDroneCmd::QueueCmd(char *cmd, int cmd_src)
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

