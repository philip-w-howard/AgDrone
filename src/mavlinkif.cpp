/*
 * mavlinkif.cpp
 *
 *  Created on: Jun 30, 2015
 *      Author: philhow
 */
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

#include "mavlink/ardupilotmega/mavlink.h"
#include "mavlinkif.h"
#include "queue.h"

static inline void byte_swap_8(void *dst, void *src)
{
	char *c_dst = (char *)dst;
	char *c_src = (char *)src;

	c_dst[0] = c_src[7];
	c_dst[1] = c_src[6];
	c_dst[2] = c_src[5];
	c_dst[3] = c_src[4];
	c_dst[4] = c_src[3];
	c_dst[5] = c_src[2];
	c_dst[6] = c_src[1];
	c_dst[7] = c_src[0];
}

static uint64_t microsSinceEpoch()
{

	struct timeval tv;
 	uint64_t micros = 0;

	gettimeofday(&tv, NULL);
	micros =  ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;

	return micros;
}

void send_msg(int fd, mavlink_message_t *msg)
{
	uint8_t buf[MAVLINK_MAX_PACKET_LEN];
	int bytes;

	// Copy the message to the send buffer
	uint16_t len = mavlink_msg_to_send_buffer(buf, msg);

	// Send the message to destination
	bytes = write(fd, buf, len);
	if (bytes != len) printf("Failed to write msg: %d %d %d\n", fd, bytes, len);
}

void queue_msg(queue_t *dest, mavlink_message_t *msg)
{
	mavlink_message_t *msg_copy;

	msg_copy = (mavlink_message_t *)malloc(sizeof(mavlink_message_t));
	assert(msg_copy != NULL);

	memcpy(msg_copy, msg, sizeof(mavlink_message_t));
	queue_insert(dest, msg_copy);

}
void send_param_request_list(queue_t *dest, uint8_t sysid, uint8_t compid)
{
	// Initialize the required buffers
	mavlink_message_t msg;

	// Pack the message
	// Ignoring return code
	mavlink_msg_param_request_list_pack(sysid, compid, &msg, 1,1);

	queue_msg(dest, &msg);
}

void send_ping(queue_t *dest, uint8_t sysid, uint8_t compid)
{
    static uint32_t seq = 0;

	// Initialize the required buffers
	mavlink_message_t msg;

    uint8_t target_system = 0;
    uint8_t target_component = 0;
    seq++;

		// Pack the message
	mavlink_msg_ping_pack(sysid, compid, &msg,
			microsSinceEpoch(), seq, target_system, target_component);

	queue_msg(dest, &msg);
}

void send_heartbeat(queue_t *dest, uint8_t sysid, uint8_t compid)
{
	// Define the system type, in this case an airplane
	uint8_t system_type = MAV_TYPE_GCS;
	uint8_t autopilot_type = MAV_AUTOPILOT_INVALID;

	uint8_t system_mode = 0;
	uint32_t custom_mode = 0;
	uint8_t system_state = 0;

	// Initialize the required buffers
	mavlink_message_t msg;

	// Pack the message
	mavlink_msg_heartbeat_pack(sysid, compid, &msg, system_type, autopilot_type, system_mode, custom_mode, system_state);

	queue_msg(dest, &msg);
}

void send_request_data_stream(queue_t *dest, uint8_t sysid, uint8_t compid,
		uint8_t target_system, uint8_t target_component, uint8_t req_stream_id,
		uint16_t req_message_rate, uint8_t start_stop)
{
	// Initialize the required buffers
	mavlink_message_t msg;

	// Pack the message
	// Ignoring return code
	mavlink_msg_request_data_stream_pack(sysid, compid, &msg,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);

	queue_msg(dest, &msg);
}

typedef struct
{
	int mav_channel;
	int fd;
	queue_t *queue;
	int bytes_at_time;
	//void (*proc_msg)(mavlink_message_t *msg, void *param);
	//void *proc_param;
	pthread_t thread_id;
	uint64_t recv_errors;
	bool stop;
} msg_params_t;

static void *read_msgs(void *p)
{
	msg_params_t *params = (msg_params_t *)p;

	mavlink_message_t msg;
	mavlink_status_t status;
	int num_msgs = 0;
	uint8_t input_buff[params->bytes_at_time];
	int length;

	memset(&msg, 0, sizeof(msg));
	memset(&status, 0, sizeof(status));

	printf("Reading data on %d\n", params->fd);

	while(!params->stop)
	{
		length = read(params->fd, input_buff, params->bytes_at_time);
		if (length < 0 && errno != EINTR) break;

		if (length < 0) perror("Error reading file descriptor");

		for (int ii=0; ii<length; ii++)
		{
			// Try to get a new message
			if(mavlink_parse_char(params->mav_channel, input_buff[ii], &msg, &status))
			{
				num_msgs++;

				queue_msg(params->queue, &msg);
				params->recv_errors += status.packet_rx_drop_count;
			}
		}
	}

	printf("Done reading from %d\n", params->fd);

	return NULL;
}

static void *write_msgs(void *p)
{
	msg_params_t *params = (msg_params_t *)p;

	uint8_t buf[MAVLINK_MAX_PACKET_LEN];
	uint16_t len;
	int size;

	mavlink_message_t *msg;

	printf("Writing data to %d\n", params->fd);

	while(queue_is_open(params->queue))
	{
		msg = (mavlink_message_t *)queue_remove(params->queue);
		if (msg != NULL)
		{
			// Copy the message to the send buffer
			len = mavlink_msg_to_send_buffer(buf, msg);
			free(msg);

			// write the msg to the destination
			size = write(params->fd, buf, len);
			if (size < 0)
			{
				if (errno == EPIPE)
				{
					printf("Pipe closed on other end %d\n", params->fd);
					break;
				} else {
					perror("Error writing message");
				}
			}
		}
	}

	printf("Done writing data to %d %p\n", params->fd, params->queue);
	close(params->fd);

	return NULL;
}

void *start_message_read_thread
		(int mav_channel, int fd, int bytes_at_time, queue_t *queue)
{
	msg_params_t *params;

	params = (msg_params_t *)malloc(sizeof(msg_params_t));
	if (params == NULL)
	{
		perror("Unable to malloc read_params");
		return NULL;
	}

	memset(params, 0, sizeof(msg_params_t));

	params->mav_channel 	= mav_channel;
	params->fd     			= fd;
	params->bytes_at_time	= bytes_at_time;
	params->queue           = queue;
	params->stop 			= false;

	int result = pthread_create(&params->thread_id, NULL, read_msgs, params);

	if (result != 0)
	{
		perror("Unable to start mavlink read thread");
		free(params);
		return NULL;
	}

	return params;
}

void *start_message_write_thread(int fd, queue_t *queue)
{
	msg_params_t *params;

	params = (msg_params_t *)malloc(sizeof(msg_params_t));
	if (params == NULL)
	{
		perror("Unable to malloc read_params");
		return NULL;
	}

	memset(params, 0, sizeof(msg_params_t));

	params->fd     			= fd;
	params->queue			= queue;

	int result = pthread_create(&params->thread_id, NULL, write_msgs, params);

	if (result != 0)
	{
		perror("Unable to start mavlink write thread");
		free(params);
		return NULL;
	}

	return params;
}

void stop_message_thread(void *p)
{
	msg_params_t *params = (msg_params_t *)p;

	params->stop = true;
	if (params->queue != NULL) queue_mark_closed(params->queue);

	pthread_join(params->thread_id, NULL);
	free(params);
}

void wait_for_message_thread(void *p)
{
	msg_params_t *params = (msg_params_t *)p;

	pthread_join(params->thread_id, NULL);
	free(params);
}

void write_tlog(int fd, mavlink_message_t *msg)
{
	static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

	struct timeval  tv;
	uint64_t time_in_mill;
	uint8_t timestamp[sizeof(uint64_t)];

	pthread_mutex_lock(&log_lock);

	if (gettimeofday(&tv, NULL) != 0)
	{
		perror("Unable to get time of day\n");
	}

	// convert to time in microseconds
	time_in_mill = tv.tv_sec;
	time_in_mill *= 1000000;
	time_in_mill += tv.tv_usec;

	// get network byte order
	byte_swap_8(timestamp, &time_in_mill);


	// write timestamp
	write(fd, timestamp, sizeof(timestamp));

	// Copy the message to the send buffer
	uint8_t buf[MAVLINK_MAX_PACKET_LEN];
	uint16_t len = mavlink_msg_to_send_buffer(buf, msg);

	// write the msg to the log file
	write(fd, buf, len);

	/*
	char str[256];
	sprintf(str, "%X %X %X %X %X %X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	write(fd, str, strlen(str));
	*/

	pthread_mutex_unlock(&log_lock);
}
