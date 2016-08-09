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
#include "connection.h"
#include "wifiserver.h"

#include "log.h"

WifiServerConnection::WifiServerConnection(queue_t *destQueue, int port)
    : Connection(1, 256, destQueue, MSG_SRC_MISSION_PLANNER)
{
    mPort = port;
    mIsConnected = false;
    mListenerFd = -1;

    struct sockaddr_in serv_addr;

    mListenerFd = socket(AF_INET, SOCK_STREAM, 0);
    if (mListenerFd < 0)
    {
       perror("ERROR opening WiFi socket. Terminating program.\n");
       exit(-1);
    }

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
    WriteLog("Listening for WiFi connections\n");
}
WifiServerConnection::~WifiServerConnection()
{
    close(mListenerFd);
    mListenerFd = -1;
    close(mFileDescriptor);
    mIsConnected = false;
}
bool WifiServerConnection::MakeConnection()
{
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    clilen = sizeof(cli_addr);

    mFileDescriptor = -1;
    while (mFileDescriptor < 0)
    {
        mFileDescriptor = accept(mListenerFd,
                (struct sockaddr *) &cli_addr,
                &clilen);
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

    setsockopt(mFileDescriptor, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

    WriteLog("processing wifi data on %d\n", mFileDescriptor);

    mIsConnected = true;

    return true;
}

void WifiServerConnection::Disconnect()
{
    close(mListenerFd);
    mListenerFd = -1;
    close(mFileDescriptor);
    mIsConnected = false;
}

bool WifiServerConnection::IsConnected()
{
    return mListenerFd >= 0 && mIsConnected;
}


