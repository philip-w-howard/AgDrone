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

#include "mraa.hpp"

#include "pixhawk.h"
#include "mavlinkif.h"
#include "connection.h"

#include "log.h"

PixhawkConnection::PixhawkConnection(
        int mavChannel, queue_t *destQueue, char *portName, int msg_src)
  : Connection(mavChannel, 1, destQueue, msg_src)
{
    WriteLog("PixhawkConnection: %s\n", portName);

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


int PixhawkConnection::ConnectSerial(char *port_name)
{
    const char *uart_name = port_name;
    int fileDescr;

    if (strcmp(port_name, "uart") == 0)
    {
        // If you have a valid platform configuration use numbers to represent uart
        // device. If not use raw mode where std::string is taken as a constructor
        // parameter
        mraa_uart_context dev;
        try {
            dev = mraa_uart_init(0);
        } catch (std::exception& e) {
            std::cerr << e.what() << ", likely invalid platform config" << std::endl;
            exit(-1);
        }

        uart_name = mraa_uart_get_dev_path(dev);
    }

    WriteLog("Opening serial port on %s\n", uart_name);

    //fileDescr = open (port_name, O_RDWR | O_NOCTTY | O_SYNC);
    fileDescr = open (uart_name, O_RDWR );
    if (fileDescr < 0)
    {
        perror ("error opening serial port");
        return -1;
    }

    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fileDescr, &tty) != 0)
    {
        perror("error %d from tcgetattr");
        return -1;
    }

    tty.c_cc[VTIME] = 0;    // block until read is ready
    tty.c_cc[VMIN] = 1;     // wait for one character

    cfmakeraw(&tty);
    cfsetispeed(&tty, B57600);
    cfsetospeed(&tty, B57600);

    //tty.c_oflag |= CLOCAL;

    //tcflush(fileDescr, TCIFLUSH);
    if (tcsetattr (fileDescr, TCSAFLUSH, &tty) != 0)
    {
        perror ("error %d from tcsetattr");
        return -1;
    }

        //tcflow(fileDescr, TCOON);

    return fileDescr;
}

bool PixhawkConnection::MakeConnection()
{
    if (!mIsConnected)
    {
        WriteLog("Connecting to Pixhawk on port %s\n", mPortName);

        mFileDescriptor = ConnectSerial(mPortName);

        if (mFileDescriptor < 0)
        {
            //perror ("error opening serial port");
            return false;
        }

        WriteLog("Connected to Pixhawk on FD %d\n", mFileDescriptor);

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
