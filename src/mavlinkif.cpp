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
#include "connection.h"

#include "log.h"

// use MAV_TYPE_ONBOARD_CONTROLLER in heartbeat for type
// use MAV_COMP_ID_SYSTEM_CONTROL for compid
// use 1 for sysid (same as pixhawk)

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
    if (bytes != len) WriteLog("Failed to write msg: %d %d %d\n", fd, bytes, len);
}

void queue_msg(queue_t *dest, int msg_src, mavlink_message_t *msg)
{
    queued_msg_t *item;

    item = (queued_msg_t *)malloc(sizeof(queued_msg_t));
    assert(item != NULL);

    item->msg = (mavlink_message_t *)malloc(sizeof(mavlink_message_t));
    assert(item->msg != NULL);

    item->msg_src = msg_src;

    memcpy(item->msg, msg, sizeof(mavlink_message_t));
    queue_insert(dest, item);

}
void send_param_request_list(queue_t *dest, uint8_t sysid, uint8_t compid)
{
    // Initialize the required buffers
    mavlink_message_t msg;

    // Pack the message
    // Ignoring return code
    mavlink_msg_param_request_list_pack(sysid, compid, &msg, 1,1);

    queue_msg(dest, MSG_SRC_SELF, &msg);
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

    queue_msg(dest, MSG_SRC_SELF, &msg);
}

void send_mav_cmd(queue_t *dest,
        uint8_t sysid, uint8_t compid, uint8_t target_sys, uint8_t target_comp, 
        int command, float param1, float param2, float param3, float param4)
{
    // Initialize the required buffers
    mavlink_message_t msg;
    uint8_t confirmation = 0;

    // Pack the message
    mavlink_msg_command_long_pack(sysid, compid, &msg,
            target_sys, target_comp,
            command, confirmation, param1, param2, param3, param4,
            0.0, 0.0, 0.0);

    queue_msg(dest, MSG_SRC_SELF, &msg);
}

void send_log_request_list(queue_t *dest, uint8_t sysid, uint8_t compid,
        uint16_t start, uint16_t end)
{
    // Initialize the required buffers
    mavlink_message_t msg;

    // Pack the message
    mavlink_msg_log_request_list_pack(sysid, compid, &msg,
            1, 1,       // target system, target component
            start, end);

    queue_msg(dest, MSG_SRC_SELF, &msg);
}
void send_log_request_data(queue_t *dest, uint8_t sysid, uint8_t compid, 
        uint16_t id, uint32_t offset, uint32_t count)
{
    // Initialize the required buffers
    mavlink_message_t msg;

    // Pack the message
    mavlink_msg_log_request_data_pack(sysid, compid, &msg,
            1, 1,       // target system, target component
            id, offset, count);

    queue_msg(dest, MSG_SRC_SELF, &msg);
}
void send_heartbeat(queue_t *dest, uint8_t sysid, uint8_t compid)
{
    // Define the system type, in this case an airplane
    uint8_t system_type = MAV_TYPE_ONBOARD_CONTROLLER;
    uint8_t autopilot_type = MAV_AUTOPILOT_INVALID;

    uint8_t system_mode = 0;
    uint32_t custom_mode = 0;
    uint8_t system_state = 0;

    // Initialize the required buffers
    mavlink_message_t msg;

    // Pack the message
    mavlink_msg_heartbeat_pack(sysid, compid, &msg, system_type, autopilot_type, system_mode, custom_mode, system_state);

    queue_msg(dest, MSG_SRC_SELF, &msg);
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

    queue_msg(dest, MSG_SRC_SELF, &msg);
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
