#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "log.h"

static FILE *g_log = NULL;

int OpenLog()
{
    char logName[256];
    time_t now = time(NULL);

    sprintf(logName, "/media/sdcard/agdrone_%ld.log", now);
    g_log = fopen(logName, "w");

    if (g_log ==  NULL)
    {
        fprintf(stderr, "Error opening log file: %s\n", logName);
        g_log = stderr;
    
        return -1;
    }

    return 0;
}

int WriteLog(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vfprintf(g_log, format, args);
    fflush(g_log);

    return 0;
}
int CloseLog()
{
    if (g_log != NULL) fclose(g_log);
    g_log = NULL;

    return 0;
}

