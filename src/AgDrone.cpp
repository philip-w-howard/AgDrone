/*
 * ag.cpp
 *
 *  Created on: Jun 30, 2015
 *      Author: philhow
 */
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include<signal.h>

#include <mraa/gpio.hpp>
#include "mavlink/ardupilotmega/mavlink.h"

#include "pixhawk.h"
#include "connection.h"
#include "wificlient.h"
#include "wifiserver.h"

#define USB_PORT "/dev/ttyACM0"
#define RADIO_PORT "/dev/ttyUSB0"
#define UART_PORT "/dev/ttyMFD1"
#define CONSOLE_PORT "/dev/MFD2"

static char portname[256] = USB_PORT;
static bool Wifi_Server = true;
static int Wifi_Port = 2002;
static char Wifi_Addr[256] = "192.168.2.5";

/*
static void signal_handler(int sig)
{
	printf("**************************** Received signal %d\n", sig);
}
*/

static void process_args(int argc, char **argv)
{
	for (int ii=1; ii<argc; ii++)
	{
		if (strncmp(argv[ii], "-port:", 6) == 0)
		{
			if (strcmp(&argv[ii][6], "usb") == 0)
				strcpy(portname, USB_PORT);
			else if (strcmp(&argv[ii][6], "radio") == 0)
				strcpy(portname, RADIO_PORT);
			else if (strcmp(&argv[ii][6], "uart") == 0)
				strcpy(portname, UART_PORT);
			else
			{
				fprintf(stderr, "Unknown port: %s\n", &argv[ii][6]);
				exit(-1);
			}
		}
		else if (strcmp(argv[ii], "-wifi:client") == 0)
			Wifi_Server = false;
		else if (strcmp(argv[ii], "-wifi:server") == 0)
			Wifi_Server = true;
		else if (strncmp(argv[ii], "-wifiport:", 10) == 0)
			Wifi_Port = atoi(&argv[ii][10]);
		else if (strncmp(argv[ii], "-wifiaddr:", 10) == 0)
			strcpy(Wifi_Addr, &argv[ii][10]);
		else
		{
			fprintf(stderr, "Unknown argument: %s\n", argv[ii]);
			exit(-1);
		}
	}
}

int main(int argc, char **argv)
{
	// check that we are running on Galileo or Edison
	mraa_platform_t platform = mraa_get_platform_type();
	if (platform != MRAA_INTEL_EDISON_FAB_C) {
		std::cerr << "Unsupported platform, exiting" << std::endl;
		return MRAA_ERROR_INVALID_PLATFORM;
	}

	std::cout << "pixhawk interface running on " << mraa_get_version() << std::endl;

    process_args(argc, argv);

	//if (signal(SIGPIPE, signal_handler) == SIG_ERR)
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
	    printf("\ncan't ignore SIGPIPE\n");
	}

	queue_t *pixhawk_q = queue_create();
	if (pixhawk_q == NULL)
	{
		perror("Error opening pixhawk queue");
		return -1;
	}

	queue_t *mission_q = queue_create();
	if (mission_q == NULL)
	{
		perror("Error opening mission planner queue");
		return -1;
	}

	queue_t *agdrone_q = queue_create();
	if (agdrone_q == NULL)
	{
		perror("Error opening mission planner queue");
		return -1;
	}

	int logfile = open("/media/sdcard/pixhawk.tlog", O_RDWR | O_CREAT | O_TRUNC);
	if (logfile < 0)
	{
		perror("Unable to open log file");
		return -1;
	}

	Connection *wifi;

	if (Wifi_Server)
	{
		wifi = new WifiServerConnection(agdrone_q, Wifi_Port);
	} else {
		wifi = new WifiClientConnection(agdrone_q, Wifi_Addr, Wifi_Port);
	}

	wifi->Start();

	PixhawkConnection pixhawk(agdrone_q, portname);
	pixhawk.Start();
/*
	uint8_t data[256];
	memset(data, 0x55, 256);
	while (1)
	{
		printf("1's\n");
		memset(data, 0xFF, 256);
		for (int ii=0; ii<1000; ii++)
		{
			if (write(pixhawk, data, 256) != 256)
			{
				perror("Error writing ones");
			}
		}
		printf("0's\n");
		memset(data, 0x00, 256);
		for (int ii=0; ii<1000; ii++)
		{
			if (write(pixhawk, data, 256) != 256)
			{
				perror("Error writing zeros");
			}
		}
	}
*/

	int num_msgs = 0;
	int num_pixhawk = 0;
	int num_mission_planner = 0;

	queued_msg_t      *item;

	while (1)
	{
		item = (queued_msg_t *)queue_remove(agdrone_q);
		if (item != NULL)
		{
			write_tlog(logfile, item->msg);

			if (item->msg_src == MSG_SRC_PIXHAWK)
			{
				wifi->QueueToSource(item->msg, item->msg_src);
				num_pixhawk++;
			}
			else if (item->msg_src == MSG_SRC_MISSION_PLANNER)
			{
				pixhawk.QueueToSource(item->msg, item->msg_src);
				num_mission_planner++;
			}
			else
				printf("Received msg from unknown source: %d\n", item->msg_src);

			free(item->msg);
			free(item);
		}

		num_msgs++;
		if (num_msgs < 100 && num_msgs % 10 == 0) printf("processed %d msgs %d %d\n", num_msgs, num_pixhawk, num_mission_planner);
		if (num_msgs < 1000 && num_msgs % 100 == 0) printf("processed %d msgs %d %d\n", num_msgs, num_pixhawk, num_mission_planner);
		if (num_msgs % 1000 == 0) printf("processed %d msgs %d %d\n", num_msgs, num_pixhawk, num_mission_planner);

/*
		if (num_msgs == 100)
		{
			send_param_request_list(pixhawk_q, EDISON_SYSID, EDISON_COMPID);
		}
		if (num_msgs == 1000 || num_msgs == 1005)
		{
			send_request_data_stream(pixhawk_q, EDISON_SYSID, EDISON_COMPID,
					1, 1, MAV_DATA_STREAM_POSITION, 1, 1);
		}
		if (num_msgs == 2000 || num_msgs == 2005)
		{
			send_request_data_stream(pixhawk_q, EDISON_SYSID, EDISON_COMPID,
					1, 1, MAV_DATA_STREAM_POSITION, 20, 1);
		}
*/
	}

	close(logfile);

	std::cout <<  "Exiting\n";
	return MRAA_SUCCESS;
}


/*
	int target_system = 1;
	int target_component = 1;
	int start_stop = 1;
	int req_stream_id = MAV_DATA_STREAM_EXTENDED_STATUS;
	int req_message_rate = 2;
*/

	//send_heartbeat(pixhawk, logfile);

	/*
	req_stream_id = MAV_DATA_STREAM_EXTENDED_STATUS;
	req_message_rate = 2;
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	req_stream_id = MAV_DATA_STREAM_POSITION;
	req_message_rate = 3;
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	req_stream_id = MAV_DATA_STREAM_EXTRA1;
	req_message_rate = 10;
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	req_stream_id = MAV_DATA_STREAM_EXTRA3;
	req_message_rate = 2;
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	req_stream_id = MAV_DATA_STREAM_RAW_SENSORS;
	req_message_rate = 2;
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	req_stream_id = MAV_DATA_STREAM_RC_CHANNELS;
	req_message_rate = 10;
	start_stop = 0;
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	send_request_data_stream(pixhawk, logfile,
			target_system, target_component, req_stream_id, req_message_rate, start_stop);
	*/
