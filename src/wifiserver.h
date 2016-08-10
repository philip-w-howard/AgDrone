/*
 * wifiserver.h
 *
 *  Created on: Jul 23, 2015
 *      Author: philhow
 */

#pragma once

#include "queue.h"
#include "connection.h"

class WifiServerConnection : public Connection
{
public:
    WifiServerConnection(queue_t *destQueue, int port);
    virtual ~WifiServerConnection();
    virtual bool MakeConnection();
    virtual void Disconnect();
    virtual bool IsConnected();
protected:
    int  mPort;
    bool mIsConnected;
    int mListenerFd;
};

