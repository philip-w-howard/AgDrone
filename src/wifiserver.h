/*
 * wifiserver.h
 *
 *  Created on: Jul 23, 2015
 *      Author: philhow
 */

#ifndef WIFISERVER_H_
#define WIFISERVER_H_

#include "queue.h"
#include "connection.h"

class WifiServerConnection : public Connection
{
public:
	WifiServerConnection(queue_t *destQueue, int port);
	virtual ~WifiServerConnection();
	virtual void MakeConnection();
	virtual void Disconnect();
	virtual bool IsConnected();
protected:
	int  mPort;
	bool mIsConnected;
	int mListenerFd;
};

#endif /* WIFISERVER_H_ */
