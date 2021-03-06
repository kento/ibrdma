#include "scr_rdma_transfer.h"
#include "rdma-client.h"
#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>

#include <unistd.h> /* for close */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>


double get_dtime(void);
int RDMA_transfer(char* src, char* dst, int buf_size, int count, struct RDMA_communicator *comm) ;
ssize_t scr_read(int fd, void* buf, size_t size);
int get_tag(void);

struct file_buffer {
  char *buf;
};

int main(int argc, char **argv)
{
  char* host;
  char* src, *dst;
  //  uint64_t size;
  //  int flag1, flag2;
  double s,e;

  host = argv[1];
  src = argv[2];
  dst = argv[3];
  
  struct  RDMA_communicator comm;
  struct  RDMA_param param;
  param.host = host;

  s = get_dtime();
  RDMA_Active_Init(&comm, &param);
  e = get_dtime();
  printf("%f\n", e - s);

  s = get_dtime();
  RDMA_transfer(src, dst, 200000000, 1, &comm);
  e = get_dtime();
  printf("%f\n", e - s);

  //  RDMA_Active_Finalize(&comm);
  return 0;
}

char* get_ip_addr (char* interface) {
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

int RDMA_transfer(char* src, char* dst, int buf_size, int count, struct RDMA_communicator *comm) 
{
  int i;
  int fd;
  int read_size;
  int tag;
  int ctl_msg_size;
  struct file_buffer *fbufs;
  int fbuf_index = 0;
  int *flags;
  char ctl_msg[1536];

  fbufs = (struct file_buffer *)malloc(sizeof(struct file_buffer) * count);
  flags = (int *)malloc(sizeof(int) * count);
  for (i = 0; i < count; i++) {
    fbufs[i % count].buf =  (char *)malloc(buf_size);
    flags[i % count] = 1;
  }

  /*send init*/
  tag=get_tag();
  sprintf(ctl_msg, "%d\t%s", tag, dst);
  printf(ctl_msg);
  ctl_msg_size =  strlen(ctl_msg);
  RDMA_Sendr(ctl_msg, ctl_msg_size, TRANSFER_INIT, comm);
  /*---------*/

  /*send file*/
  fd = open(src, O_RDONLY);
  printf("read fbuf_index=%d\n", fbuf_index);
  read_size = scr_read(fd, fbufs[fbuf_index].buf, buf_size);

  do {
    printf("sent fbuf_index=%d\n", fbuf_index);
    flags[fbuf_index] = 0;
    RDMA_Isendr(fbufs[fbuf_index].buf, read_size, tag, &flags[fbuf_index], comm);
    printf("... sent done\n");
    fbuf_index = (fbuf_index + 1) % count;

    RDMA_Wait (&flags[fbuf_index]);
    printf("read fbuf_index=%d\n", fbuf_index);
    read_size = scr_read(fd, fbufs[fbuf_index].buf, buf_size);
    printf("... read done\n");
  } while (read_size > 0);
  /*---------*/

  /*send fin*/
  sprintf(ctl_msg, "%d\t%s", tag, dst);
  ctl_msg_size =  strlen(ctl_msg);
  RDMA_Sendr(ctl_msg, ctl_msg_size, TRANSFER_FIN, comm);
  /*---------*/
  

  return 0;
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

ssize_t scr_read(int fd, void* buf, size_t size)
{
  ssize_t n = 0;
  int retries = 10;
  while (n < size)
    {
      int rc = read(fd, (char*) buf + n, size - n);
      if (rc  > 0) {
	n += rc;
      } else if (rc == 0) {
	/* EOF */
	return n;
      } else { /* (rc < 0) */
	/* got an error, check whether it was serious */
	//	if (errno == EINTR || errno == EAGAIN) {
	//	  continue;
	//	}
	
	/* something worth printing an error about */
	retries--;
	if (retries) {
	  /* print an error and try again */
	  printf("Error reading: read(%d, %s, %ld)   @ %s:%d",
		  fd, (char*) buf + n, size - n,  __FILE__, __LINE__
		  );
	} else {
	  /* too many failed retries, give up */
	  printf("Giving up read: read(%d, %s, %ld)  @ %s:%d",
		  fd, (char*) buf + n, size - n, __FILE__, __LINE__
		  );
	  exit(1);
	}
      }
    }
  return n;
}




double get_dtime(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
}


