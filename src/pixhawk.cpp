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
#include "connection.h"

PixhawkConnection::PixhawkConnection(queue_t *destQueue, char *portName)
  : Connection(0, 1, destQueue, MSG_SRC_PIXHAWK)
{
	mPortName = portName;
	mIsConnected = false;
}

PixhawkConnection::~PixhawkConnection()
{
	Disconnect();
}

int PixhawkConnection::Start()
{
	if (!MakeConnection()) return -1;

	return Connection::Start();
}

bool PixhawkConnection::MakeConnection()
{
	if (!mIsConnected)
	{
		fprintf(stderr, "Connecting to Pixhawk on port %s\n", mPortName);

		mFileDescriptor = open (mPortName, O_RDWR | O_NOCTTY | O_SYNC);
		if (mFileDescriptor < 0)
		{
			perror ("error opening serial port");
			return false;
		}

		struct termios tty;
		memset (&tty, 0, sizeof tty);
		if (tcgetattr (mFileDescriptor, &tty) != 0)
		{
			perror("error %d from tcgetattr");
			return false;
		}


		/*
		tty.c_cflag &= ~CSIZE; // Mask the character size bits
		tty.c_cflag &= ~PARENB;
		tty.c_cflag &= ~CSTOPB;
		tty.c_cflag &= ~CRTSCTS ;
		tty.c_cflag &= ~CBAUD;

		tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

		tty.c_cflag |= B57600 | CS8 | CLOCAL | CREAD;
		tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
				| INLCR | IGNCR | ICRNL | IXON);  // IGNPAR;
		tty.c_oflag &= ~OPOST;		// turn off output processing
		*/

		tty.c_cc[VTIME] = 0;	// block until read is ready
		tty.c_cc[VMIN] = 1;		// wait for one character

		cfmakeraw(&tty);
		cfsetispeed(&tty, B57600);
		cfsetospeed(&tty, B57600);

		tcflush(mFileDescriptor, TCIFLUSH);
		if (tcsetattr (mFileDescriptor, TCSANOW, &tty) != 0)
		{
			perror ("error %d from tcsetattr");
			return false;
		}

		fprintf(stderr, "Connected to Pixhawk on FD %d\n", mFileDescriptor);

		mIsConnected = true;
		return true;
	}

	return true;
}
void PixhawkConnection::Disconnect()
{
	close(mFileDescriptor);
	mIsConnected = false;
}

bool PixhawkConnection::IsConnected()
{
	return mIsConnected;
}
