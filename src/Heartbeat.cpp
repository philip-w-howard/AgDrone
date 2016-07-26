/*
 * Heartbeat.cpp
 *
 *  Created on: Jul 27, 2015
 *      Author: philhow
 */

#include <pthread.h>
#include "mraa.hpp"

static volatile bool HeartbeatRunning = false;
static pthread_t Heartbeat_Thread;

static void *heartbeat(void *param)
{
	mraa::Gpio* d_pin = NULL;
	d_pin = new mraa::Gpio(13, true, false);

	// set the pin as output
	if ((int)(d_pin->dir(mraa::DIR_OUT)) != (int)MRAA_SUCCESS)
	{
		fprintf(stderr, "Can't set digital pin as output, no heartbeat signal\n");
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

void Start_Heartbeat()
{
	HeartbeatRunning = true;
	pthread_create(&Heartbeat_Thread, NULL, heartbeat, NULL);

}

void Stop_Heartbeat()
{
	if (HeartbeatRunning)
	{
		HeartbeatRunning = false;
		pthread_join(Heartbeat_Thread, NULL);
	}
}

