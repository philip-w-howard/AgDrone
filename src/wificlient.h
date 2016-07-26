/*
 * wifi.h
 *
 *  Created on: Jun 30, 2015
 *      Author: philhow
 */

#ifndef AGDRONE_SRC_WIFI_H_
#define AGDRONE_SRC_WIFI_H_

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

#endif /* AGDRONE_SRC_WIFI_H_ */
