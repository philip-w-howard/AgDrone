/*
 * wifi.cpp
 *
 *  Created on: Jun 30, 2015
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
//#include "mavlinkif.h"
#include "connection.h"
#include "wificlient.h"

#include "log.h"

WifiClientConnection::WifiClientConnection(queue_t *destQueue, char *host, int port)
        : Connection(1, 256, destQueue, MSG_SRC_MISSION_PLANNER)

{
    mHost = host;
    mPort = port;
    mIsConnected = false;
}

WifiClientConnection::~WifiClientConnection()
{
    Disconnect();
}

bool WifiClientConnection::MakeConnection()
{
    struct sockaddr_in serv_addr;
    struct hostent *server;

    mIsConnected = false;

    WriteLog("Waiting for WiFi connection\n");

    mFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (mFileDescriptor < 0)
    {
        perror("ERROR opening socket");
    }

    server = gethostbyname(mHost);
    if (server == NULL)
    {
        WriteLog("ERROR, no such host: %s\n", mHost);
        return false;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(mPort);
    while (connect(mFileDescriptor, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting, retrying");
        sleep(1);

        server = gethostbyname(mHost);
        if (server == NULL)
        {
            WriteLog("ERROR, no such host: %s\n", mHost);
            return false;
        }

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
             (char *)&serv_addr.sin_addr.s_addr,
             server->h_length);
        serv_addr.sin_port = htons(mPort);
    }

    // Set a timeout on the socket reads
    struct timeval tv;

    tv.tv_sec = 5;  // 5 Secs Timeout
    tv.tv_usec = 0;  // Not init'ing this can cause strange errors

    setsockopt(mFileDescriptor, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

    WriteLog("Connected to WiFi on FD %d\n", mFileDescriptor);

    mIsConnected = true;

    return true;
}

void WifiClientConnection::Disconnect()
{
    close(mFileDescriptor);
    mIsConnected = false;
}
bool WifiClientConnection::IsConnected()
{
    return mIsConnected;
}
