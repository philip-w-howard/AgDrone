/*
 * agdronecmd.h - command processing for AgDrone Edison
 *
 * Created on: Jul 27, 2016
 * Author: philhow
 */

#ifndef AGDRONECMD_H_
#define AGDRONECMD_H_

#include <pthread.h>

#include "mavlinkif.h"
#include "queue.h"

class AgDroneCmd
{
public:
    AgDroneCmd(queue_t *agdrone_q, int port);
    virtual ~AgDroneCmd();
    void Start();
    void Stop();
    void QueueCmd(const char *cmd, int cmd_src);
    void QueueMsg(mavlink_message_t *msg, int msg_src);
    void ProcessCmds();
    void ProcessSocket();
protected:
    enum COMMAND_T {NONE, LOG_LIST};

    bool m_Running;
    int  mPort;
    bool mIsConnected;
    int mListenerFd;
    int mFileDescriptor;
    COMMAND_T m_active_cmd;
    queue_t *m_cmd_q;
    queue_t *m_agdrone_q;
    pthread_t m_cmd_thread;
    pthread_t m_socket_thread;

    bool MakeConnection();
    void ProcessCommand(char *command);
    void ProcessMessage(mavlink_message_t *msg, int msg_src);
    void ProcessLogListMsg(mavlink_message_t *msg);
};

#endif /* AGDRONECMD_H_ */
