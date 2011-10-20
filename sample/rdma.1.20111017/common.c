#include "common.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>


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

char* get_ip_addr (char* interface)
{
  char *ip;
  int fd;
  struct ifreq ifr;
  fd = socket(AF_INET, SOCK_STREAM, 0);
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
  ioctl(fd, SIOCGIFADDR, &ifr);
  //  printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
  ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
  return ip;
}

void write_log(char* log)
{
  int fd;
  char *ip;
  char *prefix;
  char logfile[1024];

  ip = get_ip_addr("ib0");
  prefix = getenv("SCR_PREFIX");
  //sprintf(logfile,"%s/transfer/%s.log", prefix, ip);

  //sprintf(logfile,"/g/g90/sato5/log");
  fd = open("~/log",  O_WRONLY |O_APPEND| O_CREAT, 0660);
  write(fd, log, strlen(log));
  close(fd);
}

/*
void print_rdma_err_msg(enum ibv_wc_status status) {
  switch (status){
  case IBV_WC_LOC_LEN_ERR:
  case IBV_WC_LOC_QP_OP_ERR:
  case IBV_WC_LOC_EEC_OP_ERR:
  case IBV_WC_LOC_PROT_ERR:
  case IBV_WC_WR_FLUSH_ERR:
  case IBV_WC_MW_BIND_ERR:
  case IBV_WC_BAD_RESP_ERR:
  case IBV_WC_LOC_ACCESS_ERR:
  case IBV_WC_REM_INV_REQ_ERR:
  case IBV_WC_REM_ACCESS_ERR:
  case IBV_WC_REM_OP_ERR:
  case IBV_WC_RETRY_EXC_ERR:
  case IBV_WC_RNR_RETRY_EXC_ERR:
  case IBV_WC_LOC_RDD_VIOL_ERR:
  case IBV_WC_REM_INV_RD_REQ_ERR:
  case IBV_WC_REM_ABORT_ERR:
  case IBV_WC_INV_EECN_ERR:
  case IBV_WC_INV_EEC_STATE_ERR:
  case IBV_WC_FATAL_ERR:
  case IBV_WC_RESP_TIMEOUT_ERR:
  case IBV_WC_GENERAL_ERR:
  }
}
*/
