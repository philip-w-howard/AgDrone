/*
 * ag.cpp
 *
 *  Created on: Jun 30, 2015
 *      Author: philhow
 *  TODO:
 *      better detection of lost connections
 *      in wifi server mode, handle socket closed
 *      check logfile format
 *      create startup mechanism
 *      search for USB port (ACM0 vs ACM1 vs ACM?)
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
#include "agdronecmd.h"
#include "gettimecmd.h"
#include "log.h"
#include "heartbeat.h"

#define USB_PORT        "/dev/ttyACM0"
#define RADIO_PORT      "/dev/ttyUSB0"
//#define UART_PORT       "/dev/ttyMFD1"
#define UART_PORT       "uart"
#define CONSOLE_PORT    "/dev/ttyMFD2"
#define WIFI            "wifi"

static bool Wifi_Server = true;
static int Wifi_Port = 2002;
static char Wifi_Addr[256] = "192.168.2.3";
static bool Do_Echo = false;
static bool Do_TX = false;
static bool g_GetPixhawkTime = false;
static char g_echoname[256] = CONSOLE_PORT;
static char g_pixhawk[256] = USB_PORT;
static char g_mission[256] = UART_PORT;
static time_t g_StartTime = 0;

/*
static void signal_handler(int sig)
{
    printf("**************************** Received signal %d\n", sig);
}
*/

static const char *port_name(char *tag)
{
    if (strcmp(tag, "usb") == 0)
        return USB_PORT;
    else if (strcmp(tag, "radio") == 0)
        return RADIO_PORT;
    else if (strcmp(tag, "uart") == 0)
        return UART_PORT;
    else if (strcmp(tag, "console") == 0)
        return CONSOLE_PORT;
    else
    {
        return tag;
    }
}
static void process_args(int argc, char **argv)
{
    for (int ii=1; ii<argc; ii++)
    {
        if (strncmp(argv[ii], "-echo:", 6) == 0)
        {
            Do_Echo = true;
            strcpy(g_echoname, port_name(&argv[ii][6]));
        }
        else if (strncmp(argv[ii], "-gettime", 8) == 0)
        {
            g_GetPixhawkTime = true;
        }
        else if (strncmp(argv[ii], "-mission:", 9) == 0)
        {
            strcpy(g_mission, port_name(&argv[ii][9]));
        }
        else if (strncmp(argv[ii], "-pixhawk:", 9) == 0)
        {
            strcpy(g_pixhawk, port_name(&argv[ii][9]));
        }
        else if (strcmp(argv[ii], "-wifi:client") == 0)
        {
            strcpy(g_mission, "wifi");
            Wifi_Server = false;
        }
        else if (strcmp(argv[ii], "-wifi:server") == 0)
        {
            strcpy(g_mission, "wifi");
            Wifi_Server = true;
        }
        else if (strncmp(argv[ii], "-wifiport:", 10) == 0)
            Wifi_Port = atoi(&argv[ii][10]);
        else if (strncmp(argv[ii], "-wifiaddr:", 10) == 0)
            strcpy(Wifi_Addr, &argv[ii][10]);
        else if (strcmp(argv[ii], "-txtest") == 0)
            Do_TX = true;
        else
        {
            fprintf(stderr, "Unknown argument: %s\n", argv[ii]);
            exit(-1);
        }
    }
}

static void print_stats(int total, Connection *pixhawk, Connection *mission)
{
    uint64_t p_sent, p_recv, p_csent, p_crecv;
    uint64_t m_sent, m_recv, m_csent, m_crecv;

    mission->GetStats(&m_sent, &m_recv, &m_csent, &m_crecv);
    pixhawk->GetStats(&p_sent, &p_recv, &p_csent, &p_crecv);

    fprintf(stderr, "processed %d pix: %lld %lld %lld %lld "
                                 "mis: %lld %lld %lld %lld\n",
            total,
            p_sent, p_recv, p_csent, p_crecv,
            m_sent, m_recv, m_csent, m_crecv);
    WriteLog("processed %d pix: %lld %lld %lld %lld "
                                 "mis: %lld %lld %lld %lld\n",
            total,
            p_sent, p_recv, p_csent, p_crecv,
            m_sent, m_recv, m_csent, m_crecv);
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
            count++;
            if (count < 10000 && (count%100 == 0)) 
                printf("sent %d chars to %s\n", count, ports->name);
            else if (count >= 10000 && (count%1000 == 0)) 
                printf("sent %d chars to %s\n", count, ports->name);
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

    printf("Starting echo on %s and %s\n", g_pixhawk, g_echoname);

    ports1.output = PixhawkConnection::ConnectSerial(g_echoname);
    if (ports1.output < 0)
    {
        printf("Failed to open %s\n", g_echoname);
    }
    else
    {
        printf("Opened %s\n", g_echoname);
    }

    ports1.input = PixhawkConnection::ConnectSerial(g_pixhawk);
    if (ports1.input < 0)
    {
        printf("Failed to open %s\n", g_pixhawk);
        exit(-5);
    }
    printf("Opened %s\n", g_pixhawk);

    if (ports1.output < 0)
    {
        printf("Failed to open %s\n", g_echoname);
        exit(-5);
    }
    strcpy(ports1.name, g_echoname);
    strcpy(ports2.name, g_pixhawk);

    ports2.input = ports1.output;
    ports2.output = ports2.input;

    pthread_create(&tid, NULL, echo_port, &ports1);
    pthread_create(&tid, NULL, echo_port, &ports2);

    pthread_join(tid, NULL);
}

void do_tx_test()
{
    int pixhawk;
    int count = 0;

    printf("Starting tx test on %s\n", g_pixhawk);

    pixhawk = PixhawkConnection::ConnectSerial(g_pixhawk);
    if (pixhawk < 0)
    {
        printf("Failed to open %s\n", g_pixhawk);
        exit(-5);
    }
    printf("Opened %s\n", g_pixhawk);

    while (1)
    {
        write(pixhawk, "this is a test\n", 15);
        if (++count %1000 == 0) printf("Sent %d\n", count);
    }
}

int main(int argc, char **argv)
{
    fprintf(stderr, "AgDrone Vs_%s_%s\n", __DATE__, __TIME__);

    OpenLog();
    WriteLog("AgDrone Vs_%s_%s\n", __DATE__, __TIME__);

    // check that we are running on Galileo or Edison
    mraa_platform_t platform = mraa_get_platform_type();
    if (platform != MRAA_INTEL_EDISON_FAB_C) {
        fprintf(stderr, "Unsupported platform, exiting\n");
        return MRAA_ERROR_INVALID_PLATFORM;
    }

    WriteLog("pixhawk interface running on %s\n", mraa_get_version());

    process_args(argc, argv);

    //if (signal(SIGPIPE, signal_handler) == SIG_ERR)
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        perror("can't ignore SIGPIPE");
        CloseLog();
        return -1;
    }

    if (Do_Echo)
    {
        do_echo();
        fprintf(stderr, "Done with echo\n");
        CloseLog();
        return 0;
    }
    else if (Do_TX)
    {
        do_tx_test();
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
        perror("Error opening AgDrone queue");
        return -4;
    }

    AgDroneCmd *agdrone_cmd = new AgDroneCmd(agdrone_q, 2003);
    agdrone_cmd->Start();

    char logname[256];
    time_t now = time(NULL);
    sprintf(logname, "/media/sdcard/pixhawk_%ld.tlog", now);
    int logfile = open(logname, O_RDWR | O_CREAT | O_TRUNC);
    if (logfile < 0)
    {
        perror("Unable to open log file");
        return -5;
    }

    fprintf(stderr, "Mission: %s Pixhawk: %s\n", g_mission, g_pixhawk);

    Connection *mission;

    if (strcmp(g_mission, "wifi") == 0)
    {
        if (Wifi_Server)
        {
            mission = new WifiServerConnection(agdrone_q, Wifi_Port);
        } else {
            mission = new WifiClientConnection(agdrone_q, Wifi_Addr, Wifi_Port);
        }
    }
    else
    {
        mission = new PixhawkConnection(1, agdrone_q, 
                g_mission, MSG_SRC_MISSION_PLANNER);
    }

    if (mission->Start() != 0)
    {
        fprintf(stderr, "Unable to start Mission Planner process\n");
        WriteLog("Unable to start Mission Planner process\n");
        return -6;
    }

    Connection *pixhawk = new PixhawkConnection(0, agdrone_q, 
            g_pixhawk, MSG_SRC_PIXHAWK);
    if (pixhawk->Start() != 0)
    {
        fprintf(stderr, "Unable to start pixhawk process\n");
        WriteLog("Unable to start pixhawk process\n");
        return -7;
    }

    Start_Heartbeat(agdrone_cmd);

    GetTimeCmd getTimeCmd(agdrone_q);

    if (g_GetPixhawkTime)
    {
        getTimeCmd.Start();
    }
    else
    {
        time(&g_StartTime);
        WriteLog("Start time: %s\n", ctime(&g_StartTime));
    }

    int num_msgs = 0;
    int num_pixhawk = 0;
    int num_mission_planner = 0;

    queued_msg_t      *item;

    agdrone_cmd->QueueCmd("getimages", MSG_SRC_SELF);
    while (1)
    {
        item = (queued_msg_t *)queue_remove(agdrone_q);
        if (item != NULL)
        {
            write_tlog(logfile, item->msg);

            if (item->msg_src == MSG_SRC_PIXHAWK)
            {
                if (g_GetPixhawkTime)
                {
                    getTimeCmd.ProcessMessage(item->msg, item->msg_src);
                    if (getTimeCmd.IsFinished())
                    {
                        g_GetPixhawkTime = false;
                        g_StartTime = getTimeCmd.GetTime();
                        WriteLog("Initial pixhawk time: %s\n", 
                                ctime(&g_StartTime));
                    }
                }
                mission->QueueToSource(item->msg, item->msg_src);
                agdrone_cmd->QueueMsg(item->msg, item->msg_src);
                num_pixhawk++;
            }
            else if (item->msg_src == MSG_SRC_MISSION_PLANNER ||
                     item->msg_src == MSG_SRC_SELF)
            {
                pixhawk->QueueToSource(item->msg, item->msg_src);
                num_mission_planner++;
            }
            else
                WriteLog("Received msg from unknown source: %d\n", item->msg_src);

            free(item->msg);
            free(item);
        }

        num_msgs++;
        if (num_msgs < 100 && num_msgs % 10 == 0)
            print_stats(num_msgs, pixhawk, mission);
        if (num_msgs < 1000 && num_msgs % 100 == 0)
            print_stats(num_msgs, pixhawk, mission);
        if (num_msgs % 1000 == 0)
        {
            print_stats(num_msgs, pixhawk, mission);
        }
        //if (num_msgs == 500) agdrone_cmd->QueueCmd("gettlogs", MSG_SRC_SELF);
    }

    Stop_Heartbeat();

    WriteLog("AgDrone exiting\n");
    close(logfile);
    CloseLog();

    printf("AgDrone exiting\n");
    return MRAA_SUCCESS;
}
