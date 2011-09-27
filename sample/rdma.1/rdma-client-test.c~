#include "rdma-client.h"
#include <sys/time.h>
#include <stdio.h>

#include <unistd.h> /* for close */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

double get_dtime(void);
int get_tag(void);
char* get_ip_addr (char* interface);

int main(int argc, char **argv)
{
  char* host;
  char* data;
  uint64_t size;
  //  int flag1, flag2;
  int flag1;
  double s,e;

  host = argv[1];
  size = atoi(argv[2]);
  
  struct  RDMA_communicator comm;
  struct  RDMA_param param;
  param.host = host;

  s = get_dtime();
  RDMA_Active_Init(&comm, &param);
  e = get_dtime();
  /* ===== */
  data = (char*)malloc(size);
  int i;
  flag1 = 0;
  for (i=size-2; i >= 0; i--) {
    data[i] = (char) (i % 26 + 'a');
  }
  data[size-1] += '\0';
  printf("%f\n",e - s);
  
  s = get_dtime();
  RDMA_Isendr(data, size, get_tag(), &flag1, &comm);
  /*=======*/
  //  data = (char*)malloc(size);
  //  flag2 = 0;
  //  for (i=size-2; i >= 0; i--) {
  //    data[i] = (char) (i % 26 + 'a');
  //  }
  //  data[size-1] += '\0';
  //  RDMA_Isendr(data, size, 1015, &flag2, &comm);
  /* ===== */
  RDMA_Wait (&flag1);
  e = get_dtime();
  //  RDMA_Wait (&flag2) ;
  printf("%f\n", e - s);
  return 0;
  //  RDMA_Active_Finalize(&comm);

}

int get_tag(void)
{
  char *ip;
  int tag = 0;
  int i;
  ip = get_ip_addr("ib0");
  /*use last three ip octet for the message tag.                                                                                                                                                
    Fisrt octet is passed.                                                                                                                                                                      
  */
  atoi(strtok(ip, "."));
  tag = atoi(strtok(NULL, "."));
  for (i = 0; i < 2; i++) {
    tag = tag * 1000;
    tag = tag + atoi(strtok(NULL, "."));
  }
  return tag;
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
  printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
  ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
  return ip;
}

double get_dtime(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
}


