/*
 * Author: Philip Howard <phil.w.howard@gmail.com>
 */

//#include "grove.h"
//#include "jhd1313m1.h"
//#include <mraa/uart.hpp>
//#include <mraa/gpio.hpp>

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#include "pixhawk.h"
#include "mavlinkif.h"

int open_pixhawk(char *portname)
{
	int fd;

	fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
	{
		perror ("error opening serial port");
		return fd;
	}

	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		perror("error %d from tcgetattr");
		return -1;
	}


	cfsetispeed(&tty, B57600);
	cfsetospeed(&tty, B57600);

	tty.c_cflag &= ~CSIZE; /* Mask the character size bits */
	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS ;

	tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	tty.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
	tty.c_iflag = 0;  // IGNPAR;
	tty.c_oflag &= ~OPOST;		// turn off output processing

	tty.c_cc[VTIME] = 0;	// block until read is ready
	tty.c_cc[VMIN] = 1;		// wait for one character


	tcflush(fd, TCIFLUSH);
	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
		perror ("error %d from tcsetattr");
		return -1;
	}
	return fd;
}

