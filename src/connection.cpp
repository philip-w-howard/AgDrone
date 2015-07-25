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
	mSent = 0;
}

Connection::~Connection()
{
	queue_mark_closed(mInpQueue);
	queue_close(mInpQueue);
}

void Connection::Start()
{
	printf("Starting connection for channel %d\n", mMavChannel);

	int result = pthread_create(&mReader, NULL, reading_thread, this);

	if (result != 0)
	{
		perror("Unable to start mavlink reading thread");
		return;
	}
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
	int num_msgs = 0;
	uint8_t input_buff[mBytesAtATime];
	int length;

	memset(&msg, 0, sizeof(msg));
	memset(&status, 0, sizeof(status));

	printf("Reading data on %d\n", mFileDescriptor);

	while(mRunning)
	{
		length = read(mFileDescriptor, input_buff, mBytesAtATime);
		if (length < 0 && errno != EINTR) break;

		if (length < 0) perror("Error reading file descriptor");

		if (length == 0) perror("Received zero bytes");

		for (int ii=0; ii<length; ii++)
		{
			// Try to get a new message
			if(mavlink_parse_char(mMavChannel, input_buff[ii], &msg, &status))
			{
				num_msgs++;

				QueueToDest(&msg, mMsgSrc);
				mRecvErrors += status.packet_rx_drop_count;
			}
		}
	}

	if (mRunning)
		printf("Stopped reading from %d because of error\n", mFileDescriptor);
	else
		printf("Stopped reading from %d because was told to stop\n", mFileDescriptor);
}

void Connection::WriteMsgs()
{
	uint8_t buf[MAVLINK_MAX_PACKET_LEN];
	uint16_t len;
	int size;
	queued_msg_t *item;

	printf("Writing data to %d\n", mFileDescriptor);

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
				mSent++;
			}
			else if (size < 0)
			{
				if (errno == EPIPE)
				{
					printf("Pipe closed on other end %d\n", mFileDescriptor);
					break;
				} else {
					perror("Error writing message");
				}
			}
			else
			{
				printf("Wrote too few bytes to channel %d (%d, %d)\n", mMavChannel, size, len);
			}
		}
	}

	printf("Done writing data to %d %p\n", mFileDescriptor, mInpQueue);
	if (!queue_is_open(mInpQueue)) printf("Done writing because queue is not open\n");
	if (!mRunning || mStopWriter) printf("Done writing because told to stop\n");

	close(mFileDescriptor);
}

void Connection::Process()
{
	printf("Processing connection for channel %d\n", mMavChannel);

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

	printf("Done processing connection for channel %d\n", mMavChannel);

}

