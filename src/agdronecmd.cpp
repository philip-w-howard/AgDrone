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
    m_active_cmd = NONE;

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
    if (m_active_cmd == NONE) return;

    item = (queued_cmd_t *)malloc(sizeof(queued_cmd_t));
    assert(item != NULL);

    memcpy(&item->msg, msg, sizeof(mavlink_message_t));

    item->msg_src = msg_src;
    item->type = TYPE_MSG;

    queue_insert(m_cmd_q, item);
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
    if (strcmp(command, "loglist") == 0)
    {
        m_active_cmd = LOG_LIST;
        printf("Starting LOG_LIST command\n");
        send_log_request_list(m_agdrone_q, 0x45, 0x67, 0, -1);
    }
}
//***************************************
void AgDroneCmd::ProcessMessage(mavlink_message_t *msg, int msg_src)
{
    switch (m_active_cmd)
    {
        case LOG_LIST:
            ProcessLogListMsg(msg);
            break;
        case NONE:
            // ignore the message
            break;
    }
}
//***************************************
void AgDroneCmd::ProcessLogListMsg(mavlink_message_t *msg)
{
    typedef struct
    {
        uint16_t filled;
        mavlink_log_entry_t entry;
    } entry_t;

    static int other_count = 0;
    static int filled_entries = 0;
    static int entries_capacity = 0;
    static entry_t *entries = NULL;

    if (msg->msgid == MAVLINK_MSG_ID_LOG_ENTRY) 
    {
        if (rand() % 100 < 33) return;  //************************************

        mavlink_log_entry_t log_entry;
        mavlink_msg_log_entry_decode(msg, &log_entry);
        if (log_entry.last_log_num > entries_capacity)
        {
            entries = (entry_t *)realloc(entries, sizeof(entry_t)*(log_entry.last_log_num + 1));
            assert(entries != NULL);

            for (int ii=entries_capacity; ii<log_entry.last_log_num; ii++)
            {
                entries[ii].filled = 0;
            }

            entries_capacity = log_entry.last_log_num + 1;
        }

        if (log_entry.id >= entries_capacity)
        {
            printf("Invalid log entry************************\n");
            printf("Log entry: id %d size %d num %d last %d\n",
                log_entry.id, log_entry.size, log_entry.num_logs,
                log_entry.last_log_num);
        }
        else
        {
            entries[log_entry.id].entry = log_entry;
            if (!entries[log_entry.id].filled)
            {
                entries[log_entry.id].filled = 1;
                filled_entries++;
                printf("Log entry: id %d size %d num %d last %d\n",
                        log_entry.id, log_entry.size, log_entry.num_logs,
                        log_entry.last_log_num);
            }
        }

        if (filled_entries == log_entry.num_logs)
        {
            filled_entries = 0;
            entries_capacity = 0;
            entries = NULL;
            
            printf("Finished LOG_LIST command\n");
            m_active_cmd = NONE;
        }
    }
    else
    {
        if (++other_count > 20)
        {
            if (filled_entries == 0)
            {
                send_log_request_list(m_agdrone_q, 0x45, 0x67, 0, -1);
            }
            else
            {
                for (int ii=1; ii<entries_capacity; ii++)
                {
                    if (!entries[ii].filled)
                    {
                        send_log_request_list(m_agdrone_q, 0x45, 0x67, ii, -1);
                        break;
                    }
                }
            }
        }
        printf("Received %d\n", msg->msgid);
    }
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
            else if (m_active_cmd != NONE)
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

