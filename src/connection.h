/*
 * connection.h
 *
 *  Created on: Jul 23, 2015
 *      Author: philhow
 */

#pragma once

#include <assert.h>

#include "queue.h"
#include "mavlinkif.h"

#define MSG_SRC_SELF 0
#define MSG_SRC_PIXHAWK 1
#define MSG_SRC_MISSION_PLANNER 2
#define MSG_SRC_AGDRONE_CONTROL 3


#define TYPE_CMD 0
#define TYPE_MSG 1

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

    void GetStats(uint64_t *msgsSent, uint64_t *msgsRevd, 
                  uint64_t *charsSent, uint64_t *charsRevd); 

    virtual void Process();
    virtual void ReadMsgs();
    virtual void WriteMsgs();

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
    uint64_t mMsgsSent;
    uint64_t mCharsSent;
    uint64_t mMsgsReceived;
    uint64_t mCharsReceived;

    //friend void *reading_thread(void *param);
    //friend void *writing_thread(void *param);

};

