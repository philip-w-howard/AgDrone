/*
 * connection.h
 *
 *  Created on: Jul 23, 2015
 *      Author: philhow
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <assert.h>

#include "queue.h"
#include "mavlinkif.h"

#define MSG_SRC_SELF 0
#define MSG_SRC_PIXHAWK 1
#define MSG_SRC_MISSION_PLANNER 2


typedef struct
{
    int msg_src;
    mavlink_message_t *msg;
} queued_msg_t;


class Connection
{
public:
    Connection(int mavChannel, int bytesAtATime, queue_t *destQueue, int msgSrc);
    virtual ~Connection();

    virtual bool MakeConnection() = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() = 0;

    virtual int Start();
    void Stop();
    void QueueToDest(mavlink_message_t *msg, int msg_src);
    void QueueToSource(mavlink_message_t *msg, int msg_src);
    int NumSent() { return (int) mSent; }

protected:
    queue_t *mDestQueue;
    queue_t *mInpQueue;
    bool mRunning;
    bool mStopWriter;
    int mFileDescriptor;
    int mMavChannel;
    pthread_t mWriter;
    pthread_t mReader;

    int mBytesAtATime;
    int mMsgSrc;
    uint64_t mRecvErrors;
    uint64_t mSent;


    void Process();
    void ReadMsgs();
    void WriteMsgs();

    //virtual int WriteData(char *buff, int len);
    //virtual int ReadData(char *buff, int len);

    friend void *reading_thread(void *param);
    friend void *writing_thread(void *param);

};

#endif /* CONNECTION_H_ */
