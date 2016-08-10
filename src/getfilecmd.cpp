#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

#include "getfilecmd.h"

#include "log.h"

//**********************************************
GetFileCmd::GetFileCmd(queue_t *q, int socket_fd, char *command) 
    : CommandProcessor(q)
{
    m_finished = false;
    m_socket_fd = socket_fd;

    strncpy(m_command, command, sizeof(m_command));
    m_command[sizeof(m_command)-1] = 0;
}

//**********************************************
GetFileCmd::~GetFileCmd()
{
}

//**********************************************
void GetFileCmd::Start()
{
    char buff[sizeof(m_command)];
    char *filename;
    int blocks = 0;

    WriteLog("Starting %s command\n", m_command);
    m_finished = false;

    strcpy(buff, m_command);
    filename = strtok(buff, " \r\n\t");
    filename = strtok(NULL, " \r\n\t");

    if (filename == NULL)
    {
        fprintf(stderr, "No filename specified in GETFILE command\n");
        WriteLog("No filename specified in GETFILE command\n");

        // FIX THIS: need to inform remote I/F

        return;
    }

    int file_fd;
    file_fd = open(filename, O_RDONLY);
    if (file_fd < 0)
    {
        fprintf(stderr, "Error opening %s\n", filename);
        WriteLog("Error opening %s for GETFILE command\n", filename);

        // FIX THIS: need to inform remote I/F

        return;
    }

    int filesize = lseek(file_fd, 0, SEEK_END);
    lseek(file_fd, 0, SEEK_SET);
    unsigned int count;

    sprintf(buff, "%12d", filesize);
    count = write(m_socket_fd, buff, strlen(buff));
    if (count != strlen(buff))
    {
        fprintf(stderr, "Error writing file length in GETFILE command\n");
        WriteLog("Error writing file length in GETFILE command\n");

        close(file_fd);

        // FIX THIS: need to inform remote I/F

        return;
    }

    while ((count = read(file_fd, m_buffer, sizeof(m_buffer))) > 0)
    {
        write(m_socket_fd, m_buffer, count);
        filesize -= count;
        if (++blocks % 20 == 0) 
        {
            WriteLog("GETFILE sent %d blocks %d bytes remaining\n", 
                    blocks, filesize);
        }
    }

    if (filesize != 0)
    {
        fprintf(stderr, "Error sending file in GETFILE command\n");
        WriteLog("Error sending file in GETFILE command: %d\n", filesize);
    }
    else
    {
        WriteLog("Finished sinding file\n");
    }

    close(file_fd);
    m_finished = true;
}
//**********************************************
void GetFileCmd::Abort()
{
}
