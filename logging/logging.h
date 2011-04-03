
#ifndef LOGGING_H
#define LOGGING_H

void log_open(char* log_file);
void log_msg(char* msg, ...);
void log_perror(char* msg);

#endif



