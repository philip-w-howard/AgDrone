/*
 * connection.cpp
 *
 *  Created on: Jul 23, 2015
 *      Author: philhow
 */
#include <unistd.h>
#include <stdlib.h>
//#include <sys/time.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
//#include <assert.h>

#include "mavlinkif.h"
#include "connection.h"

#include "log.h"

void *writing_thread(void *param)
{
    Connection *conn = (Connection *)param;

    conn->WriteMsgs();

    return NULL;
}

void *reading_thread(void *param)
{
    Connection *conn = (Connection *)param;
    conn->Process();

    return NULL;
}

Connection::Connection(int mavChannel, int bytesAtATime, queue_t *destQueue, int msgSrc)
{
    mMavChannel = mavChannel;
    mBytesAtATime = bytesAtATime;
    mDestQueue = destQueue;
    mMsgSrc = msgSrc;
    mInpQueue = queue_create();

    mRunning = true;
    mStopWriter = false;
    mFileDescriptor = -1;
    mWriter = 0;
    mReader = 0;
    mRecvErrors = 0;
    mMsgsSent = 0;
    mMsgsReceived = 0;
    mCharsSent = 0;
    mCharsReceived = 0;
}

Connection::~Connection()
{
    queue_mark_closed(mInpQueue);
    queue_close(mInpQueue);
}

int Connection::Start()
{
    WriteLog("Starting connection for channel %d\n", mMavChannel);

    int result = pthread_create(&mReader, NULL, reading_thread, this);

    if (result != 0)
    {
        perror("Unable to start mavlink reading thread");
        return -1;
    }

    return 0;
}

void Connection::Stop()
{
    mRunning = false;
    pthread_join(mReader, NULL);
}

void Connection::QueueToDest(mavlink_message_t *msg, int msg_src)
{
    queued_msg_t *item;

    item = (queued_msg_t *)malloc(sizeof(queued_msg_t));
    assert(item != NULL);

    item->msg = (mavlink_message_t *)malloc(sizeof(mavlink_message_t));
    assert(item->msg != NULL);

    item->msg_src = msg_src;

    memcpy(item->msg, msg, sizeof(mavlink_message_t));
    queue_insert(mDestQueue, item);
}

void Connection::QueueToSource(mavlink_message_t *msg, int msg_src)
{
    queued_msg_t *item;

    item = (queued_msg_t *)malloc(sizeof(queued_msg_t));
    assert(item != NULL);

    item->msg = (mavlink_message_t *)malloc(sizeof(mavlink_message_t));
    assert(item->msg != NULL);

    item->msg_src = msg_src;

    memcpy(item->msg, msg, sizeof(mavlink_message_t));
    queue_insert(mInpQueue, item);
}

void Connection::ReadMsgs()
{
    mavlink_message_t msg;
    mavlink_status_t status;
    uint8_t input_buff[mBytesAtATime];
    int length;

    memset(&msg, 0, sizeof(msg));
    memset(&status, 0, sizeof(status));

    WriteLog("Reading data on %d\n", mFileDescriptor);

    while(mRunning)
    {
        length = read(mFileDescriptor, input_buff, mBytesAtATime);
        if (length < 0 && errno != EINTR) break;

        if (length < 0) perror("Error reading file descriptor");

        if (length == 0) 
        {
            fprintf(stderr, "Received zero bytes on channel %d\n", mMavChannel);
            WriteLog("Received zero bytes on channel %d\n", mMavChannel);
        }

        for (int ii=0; ii<length; ii++)
        {
            mCharsReceived++;

            // Try to get a new message
            if(mavlink_parse_char(mMavChannel, input_buff[ii], &msg, &status))
            {
                mMsgsReceived++;

                QueueToDest(&msg, mMsgSrc);
                mRecvErrors += status.packet_rx_drop_count;
            }
        }
    }

    if (mRunning)
    {
        perror("Error reading");
        fprintf(stderr, "Stopped reading from %d because of error\n", 
                mFileDescriptor);
        WriteLog("Stopped reading from %d because of error\n", mFileDescriptor);
        //fprintf(stderr, "Exiting process\n");
        //exit(-1);
    }
    else
        WriteLog("Stopped reading from %d because was told to stop\n", 
                mFileDescriptor);
}

void Connection::WriteMsgs()
{
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len;
    int size;
    queued_msg_t *item;

    WriteLog("Writing data to %d\n", mFileDescriptor);

    while(queue_is_open(mInpQueue) && mRunning && !mStopWriter)
    {
        item = (queued_msg_t *)queue_remove(mInpQueue);
        if (item != NULL)
        {
            // Copy the message to the send buffer
            len = mavlink_msg_to_send_buffer(buf, item->msg);
            free(item->msg);
            free(item);

            // write the msg to the destination
            size = write(mFileDescriptor, buf, len);
            if (size == len)
            {
                mMsgsSent++;
                mCharsSent += len;
            }
            else if (size < 0)
            {
                if (errno == EPIPE)
                {
                    fprintf(stderr, "Pipe closed on other end %d\n", 
                            mFileDescriptor);
                    WriteLog("Pipe closed on other end %d\n", 
                            mFileDescriptor);
                    break;
                } else {
                    perror("Error writing message");
                }
            }
            else
            {
                WriteLog("Wrote too few bytes to channel %d (%d, %d)\n", 
                        mMavChannel, size, len);
            }
        }
    }

    WriteLog("Done writing data to %d %p\n", mFileDescriptor, mInpQueue);
    if (!queue_is_open(mInpQueue)) 
        WriteLog("Done writing because queue is not open\n");
    if (!mRunning || mStopWriter) 
        WriteLog("Done writing because told to stop\n");

    close(mFileDescriptor);
}

void Connection::Process()
{
    WriteLog("Processing connection for channel %d\n", mMavChannel);

    while (mRunning)
    {
        MakeConnection();

        mStopWriter = false;
        int result = pthread_create(&mWriter, NULL, writing_thread, this);

        if (result != 0)
        {
            perror("Unable to start mavlink write thread");
            return;
        }

        ReadMsgs();
        mStopWriter = true;
        pthread_join(mWriter, NULL);
    }

    WriteLog("Done processing connection for channel %d\n", mMavChannel);

}

void Connection::GetStats(uint64_t *msgsSent, uint64_t *msgsRevd,
                          uint64_t *charsSent, uint64_t *charsRevd)
{
    if (msgsSent != NULL) *msgsSent = mMsgsSent;
    if (msgsRevd != NULL) *msgsRevd = mMsgsReceived;
    if (charsSent != NULL) *charsSent = mCharsSent;
    if (charsRevd != NULL) *charsRevd = mCharsReceived;
}

