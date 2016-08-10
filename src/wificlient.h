/*
 * wifi.h
 *
 *  Created on: Jun 30, 2015
 *      Author: philhow
 */

#pragma once

#include "queue.h"
#include "connection.h"

class WifiClientConnection : public Connection
{
public:
    WifiClientConnection(queue_t *destQueue, char *host, int port);
    virtual ~WifiClientConnection();
    virtual bool MakeConnection();
    virtual void Disconnect();
    virtual bool IsConnected();
protected:
    char *mHost;
    int  mPort;
    bool mIsConnected;
};

