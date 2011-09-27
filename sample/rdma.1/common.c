#include "common.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

int file_dump(char * path, char *content)
{
  int fd;
  int len;

  fd = open(path, O_WRONLY | O_CREAT);
  len = strlen(content);
  write(fd, content, len);
  close(fd);
  return 0;
}

int get_pid(void)
{ 
  return (int)getpid();
}

int get_rand(void) {
  return rand();
}


double get_dtime(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
}

