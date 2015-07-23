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
       perror("ERROR opening socket");
       return;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(mPort);
    if (bind(mListenerFd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        close(mListenerFd);
        mListenerFd = -1;
        return;
    }

    listen(mListenerFd,5);
    printf("Listening for WiFi connections\n");
}
WifiServerConnection::~WifiServerConnection()
{
	close(mListenerFd);
	mListenerFd = -1;
	close(mFileDescriptor);
	mIsConnected = false;
}
void WifiServerConnection::MakeConnection()
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
    	}
    }

    printf("processing wifi data on %d\n", mFileDescriptor);
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

/*
//*************************************************
typedef struct
{
	queue_t *wifi_q;
	queue_t *agdrone_q;
	int     sockfd;
} wifi_param_t;

typedef struct
{
	char host[256];
	int portno;
	queue_t *wifi_q;
	queue_t *agdrone_q;
} wifi_client_param_t;

static void *wifi_listener(void *param)
{
	wifi_param_t *wifi = (wifi_param_t *)param;

	int wififd;

	socklen_t clilen;
    struct sockaddr_in cli_addr;
    clilen = sizeof(cli_addr);
    printf("Listening for WiFi connections\n");

    while (1)
    {
    	wififd = accept(wifi->sockfd,
    			(struct sockaddr *) &cli_addr,
                &clilen);
    	if (wififd < 0)
    	{
    		perror("ERROR on accept");
    	}

    	printf("processing wifi data on %d\n", wififd);

    	start_message_read_thread(0, wififd, 200, wifi->agdrone_q, MSG_SRC_MISSION_PLANNER);
    	start_message_write_thread(wififd, wifi->wifi_q);
    }

    return NULL;
}

int listen_to_wifi(int portno, queue_t *wifi_q, queue_t *agdrone_q)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    pthread_t thread_id;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
       perror("ERROR opening socket");
       return -1;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        close(sockfd);
        return -1;
    }

    listen(sockfd,5);

    wifi_param_t *params;
    params = (wifi_param_t *)malloc(sizeof(wifi_param_t));
    params->wifi_q = wifi_q;
    params->agdrone_q = agdrone_q;
    params->sockfd = sockfd;

    pthread_create(&thread_id, NULL, wifi_listener, params);

    return 0;
}


static void *connect_to_wifi(void *p)
{
	wifi_client_param_t *params = (wifi_client_param_t *)p;

	while (1)
	{
		int sockfd;
		struct sockaddr_in serv_addr;
		struct hostent *server;

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
			perror("ERROR opening socket");
		server = gethostbyname(params->host);
		if (server == NULL)
		{
			fprintf(stderr,"ERROR, no such host: %s\n", params->host);
			return (void *)-1;
		}
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr,
			 (char *)&serv_addr.sin_addr.s_addr,
			 server->h_length);
		serv_addr.sin_port = htons(params->portno);
		while (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		{
			perror("ERROR connecting, retrying");
		}

		void *reader, *writer;

		reader = start_message_read_thread(0, sockfd, 200, params->agdrone_q, MSG_SRC_MISSION_PLANNER);
		writer = start_message_write_thread(sockfd, params->wifi_q);

		// wait for reader to finish then make a new connection
		wait_for_message_thread(reader);
		stop_message_thread(writer);
	}

    return NULL;
}

int start_wifi(char *host, int portno, queue_t *wifi_q, queue_t *agdrone_q)
{
    pthread_t thread_id;
    wifi_client_param_t *param;
    param = (wifi_client_param_t *)malloc(sizeof(wifi_client_param_t));
    assert(param != NULL);
    bzero((char *) param, sizeof(wifi_client_param_t));
	strcpy(param->host, host);
	param->portno = portno;
	param->wifi_q = wifi_q;
	param->agdrone_q = agdrone_q;

	pthread_create(&thread_id, NULL, connect_to_wifi, param);

	return 0;
}




*/
