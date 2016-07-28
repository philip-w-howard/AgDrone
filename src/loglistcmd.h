#ifndef _LOGLISTCMD_H_
#define _LOGLISTCMD_H_

#include "cmdprocessor.h"
#include "mavlinkif.h"
#include "queue.h"

class LogListCmd : public CommandProcessor
{
    public:
        LogListCmd(queue_t *agdrone_q);
        virtual ~LogListCmd();

        virtual void Start();
        virtual void Abort();
        virtual void ProcessMessage(mavlink_message_t *msg, int msg_src);
    protected:
        static const int OTHERS_PER_RETRY = 20;
        typedef struct
        {
            bool filled;
            mavlink_log_entry_t entry;
        } entry_t;
        int m_other_count;
        int m_filled_entries;
        int m_entries_capacity;
        entry_t *m_entries;

};
#endif // _LOGLISTCMD_H_
