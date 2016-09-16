/*
 * Heartbeat.cpp
 *
 *  Created on: Jul 27, 2015
 *      Author: philhow
 */

#include <pthread.h>
#include "mraa.hpp"

#include "log.h"
#include "agdronecmd.h"
#include "connection.h"

static volatile bool g_HeartbeatRunning = false;
static pthread_t Heartbeat_Thread;
static AgDroneCmd *g_agdrone;

/*
static void *heartbeat(void *param)
{
    mraa::Gpio* d_pin = NULL;
    d_pin = new mraa::Gpio(13, true, false);

    // set the pin as output
    if ((int)(d_pin->dir(mraa::DIR_OUT)) != (int)MRAA_SUCCESS)
    {
        fprintf(stderr, "Can't set digital pin as output, no heartbeat signal\n");
        WriteLog("Can't set digital pin as output, no heartbeat signal\n");
        return NULL;
    }

    // loop forever toggling the on board LED every second
    for (;;) {
        d_pin->write(1);
        sleep(1);
        d_pin->write(0);
        sleep(1);
    }

    return NULL;
}
*/

static void *heartbeat(void *param)
{
    mavlink_message_t msg;

    memset(&msg, 0, sizeof(msg));

    // loop forever toggling the on board LED every second
    while (g_HeartbeatRunning)
    {
        sleep(1);
        g_agdrone->QueueMsg(&msg, MSG_SRC_SELF);
    }

    return NULL;
}
void Start_Heartbeat(AgDroneCmd *agdrone)
{
    g_agdrone = agdrone;
    g_HeartbeatRunning = true;
    pthread_create(&Heartbeat_Thread, NULL, heartbeat, NULL);

}

void Stop_Heartbeat()
{
    if (g_HeartbeatRunning)
    {
        g_HeartbeatRunning = false;
        pthread_join(Heartbeat_Thread, NULL);
    }
}

