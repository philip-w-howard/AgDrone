/*
 * mavlinkif.h
 *
 *  Created on: Jun 30, 2015
 *      Author: philhow
 */

#pragma once

#include "mavlink/ardupilotmega/mavlink.h"
#include "queue.h"

void send_param_request_list(queue_t *dest, uint8_t sysid, uint8_t compid);
void send_heartbeat(queue_t *dest, uint8_t sysid, uint8_t compid);
void send_ping(queue_t *dest, uint8_t sysid, uint8_t compid);
void send_request_data_stream(queue_t *dest, uint8_t sysid, uint8_t compid,
        uint8_t target_system, uint8_t target_component, uint8_t req_stream_id,
        uint16_t req_message_rate, uint8_t start_stop);
void send_mav_cmd(queue_t *dest, uint8_t sysid, uint8_t compid,
        int cmd, int param1, int param2, int param3, int param4);
void send_log_request_list(queue_t *dest, uint8_t sysid, uint8_t compid, 
        uint16_t start, uint16_t end);
void send_log_request_data(queue_t *dest, uint8_t sysid, uint8_t compid, 
        uint16_t id, uint32_t offset, uint32_t count);

void queue_msg(queue_t *dest, int msg_src, mavlink_message_t *msg);
void write_tlog(int fd, mavlink_message_t *msg);

