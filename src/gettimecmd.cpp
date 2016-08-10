#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "gettimecmd.h"

#include "log.h"

//**********************************************
GetTimeCmd::GetTimeCmd(queue_t *q) : CommandProcessor(q)
{
    m_finished = false;
    m_time = 0;
}

//**********************************************
GetTimeCmd::~GetTimeCmd()
{
}

//**********************************************
void GetTimeCmd::Start()
{
    WriteLog("Starting GETTIME command\n");
    m_finished = false;
}
//**********************************************
void GetTimeCmd::Abort()
{
}
//**********************************************
void GetTimeCmd::ProcessMessage(mavlink_message_t *msg, int msg_src)
{
    if (m_finished) return;

    if (msg->msgid == MAVLINK_MSG_ID_SYSTEM_TIME) 
    {
        mavlink_system_time_t time_msg;
        mavlink_msg_system_time_decode(msg, &time_msg);

        m_time = time_msg.time_unix_usec/1000000;
        m_finished = true;
    }
}
//**********************************************
time_t GetTimeCmd::GetTime()
{
    return m_time;
}
