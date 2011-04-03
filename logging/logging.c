
#include "logging.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <time.h>
#include <errno.h>


int log_fd;

void log_open(char* log_file)
{
  log_fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT, 0644);
  if (log_fd < 0) {
    printf("logfile %s kann nicht geoefnet werden.\n", log_file);
    exit(EXIT_FAILURE);
  }
}

void log_msg(char* msg, ...)
{
  time_t tm;
  struct tm *ptr;
  tm = time(NULL);
  ptr = localtime(&tm);

  dprintf(log_fd, "%04i-%02i-%02i %02i:%02i:%02i - ",
    ptr->tm_year + 1900, ptr->tm_mon + 1, ptr->tm_mday,
    ptr->tm_hour, ptr->tm_min, ptr->tm_sec);

  va_list ap;
  va_start(ap, msg);
  vdprintf(log_fd, msg, ap);
  va_end(ap);

  dprintf(log_fd, "\n");
}

void log_perror(char* msg)
{
  log_msg("%s: %s", msg, strerror(errno));
}


