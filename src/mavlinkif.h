/*
 * mavlinkif.h
 *
 *  Created on: Jun 30, 2015
 *      Author: philhow
 */

#ifndef AGDRONE_SRC_MAVLINKIF_H_
#define AGDRONE_SRC_MAVLINKIF_H_

#include "mavlink/ardupilotmega/mavlink.h"
#include "queue.h"

void send_param_request_list(queue_t *dest, uint8_t sysid, uint8_t compid);
void send_heartbeat(queue_t *dest, uint8_t sysid, uint8_t compid);
void send_ping(queue_t *dest, uint8_t sysid, uint8_t compid);
void send_request_data_stream(queue_t *dest, uint8_t sysid, uint8_t compid,
		uint8_t target_system, uint8_t target_component, uint8_t req_stream_id,
		uint16_t req_message_rate, uint8_t start_stop);

void *start_message_read_thread(int mav_channel, int fd, int bytes_at_time,
		queue_t *queue);
		//void (*proc_msg)(mavlink_message_t *msg, void *param), void *proc_param);

void *start_message_write_thread(int fd, queue_t *queue);

void stop_message_thread(void *p);
void wait_for_message_thread(void *p);

void write_tlog(int fd, mavlink_message_t *msg);
void queue_msg(queue_t *dest, mavlink_message_t *msg);

#endif /* AGDRONE_SRC_MAVLINKIF_H_ */
