 /*
 * pixhawk.h
 *
 *  Created on: Jun 30, 2015
 *      Author: philhow
 */

#ifndef AGDRONE_SRC_PIXHAWK_H_
#define AGDRONE_SRC_PIXHAWK_H_

#include "queue.h"
#include "connection.h"

class PixhawkConnection : public Connection
{
public:
    PixhawkConnection(queue_t *destQueue, char *portName);
    virtual ~PixhawkConnection();
    virtual bool MakeConnection();
    virtual int Start();
    virtual void Disconnect();
    virtual bool IsConnected();
    static int ConnectSerial(char *port_name);

    /*
    virtual int WriteData(char *buff, int len);
    virtual int ReadData(char *buff, int len);
    */
protected:
    char *mPortName;
    bool mIsConnected;
};
#define EDISON_SYSID        0x41
#define EDISON_COMPID       0x41

/*
typedef struct pixhawk_proc_msg_s
{
    int pixhawk_fd;
    int log_fd;
    queue_t *send_q;
    int num_msgs;
} pix_proc_msg_t;

int open_pixhawk(char *portname);

void pixhawk_proc_msg(mavlink_message_t *msg, void *param);
*/

#endif /* AGDRONE_SRC_PIXHAWK_H_ */
