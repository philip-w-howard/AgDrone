/*
 * agdronecmd.h - command processing for AgDrone Edison
 *
 * Created on: Jul 27, 2016
 * Author: philhow
 */

#ifndef AGDRONECMD_H_
#define AGDRONECMD_H_

#include <pthread.h>

#include "queue.h"

class AgDroneCmd
{
public:
    AgDroneCmd(queue_t *pixhawk_q, int port);
    virtual ~AgDroneCmd();
    void Start();
    void QueueCmd(char *cmd, int cmd_src);
    void QueueMsg(mavlink_message_t *msg, int msg_src);
    void ProcessCmds();
    void ProcessMsgs();
    void ProcessSocket();
protected:
    int  mPort;
    bool mIsConnected;
    int mListenerFd;
    queue_t *m_cmd_q;
    queue_t *m_pixhawk_q;
    pthread_t m_msg_thread;
    pthread_t m_cmd_thread;
    pthread_t m_socket_thread;

    bool MakeConnection();
};

#endif /* AGDRONECMD_H_ */
