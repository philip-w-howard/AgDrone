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
#include "heartbeat.h"

#define USB_PORT "/dev/ttyACM0"
#define RADIO_PORT "/dev/ttyUSB0"
#define UART_PORT "/dev/ttyMFD1"
#define CONSOLE_PORT "/dev/MFD2"

static char portname[256] = UART_PORT;
static bool Wifi_Server = true;
static int Wifi_Port = 2002;
static char Wifi_Addr[256] = "192.168.2.5";
static bool Do_Echo = false;
static char echoname[256] = CONSOLE_PORT;

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
		if (strncmp(argv[ii], "-echo:", 6) == 0)
		{
			Do_Echo = true;
			if (strcmp(&argv[ii][6], "usb") == 0)
				strcpy(echoname, USB_PORT);
			else if (strcmp(&argv[ii][6], "radio") == 0)
				strcpy(echoname, RADIO_PORT);
			else if (strcmp(&argv[ii][6], "uart") == 0)
				strcpy(echoname, UART_PORT);
			else
			{
				strcpy(echoname, &argv[ii][6]);
			}
		}
		else if (strncmp(argv[ii], "-port:", 6) == 0)
		{
			if (strcmp(&argv[ii][6], "usb") == 0)
				strcpy(portname, USB_PORT);
			else if (strcmp(&argv[ii][6], "radio") == 0)
				strcpy(portname, RADIO_PORT);
			else if (strcmp(&argv[ii][6], "uart") == 0)
				strcpy(portname, UART_PORT);
			else
			{
				strcpy(portname, &argv[ii][6]);
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

static void print_stats(int total, int pixhawk, int wifi_in, int wifi_out)
{
	fprintf(stderr, "processed %d msgs %d %d %d\n", total, pixhawk, wifi_in, wifi_out);
}

typedef struct
{
	int input;
	int output;
	char name[256];
} ports_t;

void *echo_port(void *p)
{
	ports_t *ports = (ports_t *)p;
	int count = 0;
	uint8_t input_char;
	int length;

	printf("Echoing to %s\n", ports->name);

	while ( 1 )
	{
		length = read(ports->input, &input_char, 1);
		if (length > 0)
		{
			write(ports->output, &input_char, 1);
			if ((++count % 1000) == 0) printf("sent %d chars to %s\n", count, ports->name);
		}
	}

	printf("Done echoing to %s\n", ports->name);

	return NULL;
}

void do_echo()
{
	ports_t ports1;
	ports_t ports2;
	pthread_t tid;

	printf("Starting echo on %s and %s\n", portname, echoname);

	ports1.output = PixhawkConnection::ConnectSerial(echoname);
	if (ports1.output < 0)
	{
		printf("Failed to open %s\n", echoname);
	}
	else
	{
		printf("Opened %s\n", echoname);
	}

	ports1.input = PixhawkConnection::ConnectSerial(portname);
	if (ports1.input < 0)
	{
		printf("Failed to open %s\n", portname);
		exit(-5);
	}
	printf("Opened %s\n", portname);

	if (ports1.output < 0)
	{
		printf("Failed to open %s\n", echoname);
		exit(-5);
	}
	strcpy(ports1.name, echoname);
	strcpy(ports2.name, portname);

	ports2.input = ports1.output;
	ports2.output = ports2.input;

	pthread_create(&tid, NULL, echo_port, &ports1);
	pthread_create(&tid, NULL, echo_port, &ports2);

	pthread_join(tid, NULL);
}

int main(int argc, char **argv)
{
	printf("AgDrone V%s\n", __DATE__);
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
	    perror("can't ignore SIGPIPE");
	    return -1;
	}

	if (Do_Echo)
	{
		do_echo();
		printf("Done with echo\n");
		return 0;
	}

	queue_t *pixhawk_q = queue_create();
	if (pixhawk_q == NULL)
	{
		perror("Error opening pixhawk queue");
		return -2;
	}

	queue_t *mission_q = queue_create();
	if (mission_q == NULL)
	{
		perror("Error opening mission planner queue");
		return -3;
	}

	queue_t *agdrone_q = queue_create();
	if (agdrone_q == NULL)
	{
		perror("Error opening mission planner queue");
		return -4;
	}

	char logname[256];
	time_t now = time(NULL);
	sprintf(logname, "/media/sdcard/pixhawk_%ld.tlog", now);
	int logfile = open(logname, O_RDWR | O_CREAT | O_TRUNC);
	if (logfile < 0)
	{
		perror("Unable to open log file");
		return -5;
	}

	Connection *wifi;

	if (Wifi_Server)
	{
		wifi = new WifiServerConnection(agdrone_q, Wifi_Port);
	} else {
		wifi = new WifiClientConnection(agdrone_q, Wifi_Addr, Wifi_Port);
	}

	if (wifi->Start() != 0)
	{
		fprintf(stderr, "Unable to start wifi process\n");
		return -6;
	}

	PixhawkConnection pixhawk(agdrone_q, portname);
	if (pixhawk.Start() != 0)
	{
		fprintf(stderr, "Unable to start pixhawk process\n");
		return -7;
	}

	Start_Heartbeat();

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
				fprintf(stderr, "Received msg from unknown source: %d\n", item->msg_src);

			free(item->msg);
			free(item);
		}

		num_msgs++;
		if (num_msgs < 100 && num_msgs % 10 == 0) 	print_stats(num_msgs, num_pixhawk, num_mission_planner, wifi->NumSent());
		if (num_msgs < 1000 && num_msgs % 100 == 0) print_stats(num_msgs, num_pixhawk, num_mission_planner, wifi->NumSent());
		if (num_msgs % 1000 == 0) 					print_stats(num_msgs, num_pixhawk, num_mission_planner, wifi->NumSent());

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
