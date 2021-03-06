#include "scr_rdma_transfer_common.h"
#include "rdma-client.h"
#include "common.h"
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



int RDMA_transfer(char* src, char* dst, int buf_size, int count, struct RDMA_communicator *comm) ;
ssize_t buf_read(int fd, void* buf, size_t size);
int get_tag(void);

struct file_buffer {
  char *buf;
};

struct RDMA_communicator rdma_comm;
struct RDMA_param rdma_param = {"192.168.117.44"};

/*
int main(int argc, char **argv)
{
  char* host;
  char* src;
  char *dst;
  //  uint64_t size;
  //  int flag1, flag2;
  double st, et;
  int i;

  host = argv[1];
  src = argv[2];
  dst = argv[3];

  
  TEST_RDMA_transfer (host, src, dst);
  return 0;
}
*/

int RDMA_transfer_init(void) 
{
  char *ip;
  char *prefix;
  char tfnfile[1024];
  int fd;
  char tfn[16];
  /*TODO: remove hard cored arg*/
  ip = get_ip_addr("ib0");
  prefix = getenv("SCR_PREFIX");
  sprintf(tfnfile,"%s/transfer/%s", prefix, ip);
  fd = open(tfnfile, O_RDONLY);
  read(fd, tfn, 16);
  close(fd);
  rdma_param.host = tfn;
  RDMA_Active_Init(&rdma_comm, &rdma_param);
  return 0;
}

int file_send_count = 1;

int RDMA_file_transfer(char* src, char* dst, int buf_size, int count) 
{
  int i;
  int fd;
  int read_size;
  int tag;
  int ctl_msg_size;
  struct file_buffer *fbufs;
  int fbuf_index = 0;
  int *flags;
  int flag_init=0;
  char ctl_msg[1536];

  double e,s;
  int send_buf = 0;

  s = get_dtime();

  fbufs = (struct file_buffer *)malloc(sizeof(struct file_buffer) * count);
  flags = (int *)malloc(sizeof(int) * count);
  for (i = 0; i < count; i++) {
    fbufs[i % count].buf =  (char *)malloc(buf_size);
    flags[i % count] = 1;
  }

  //  fprintf(stderr, "ACT lib: SEND: %d: src=%s dst=%s \n", file_send_count, src, dst);

  /*send init*/
  tag=get_tag();
  sprintf(ctl_msg, "%d\t%s\t", tag, dst);
  //  fprintf(stderr, "ACT lib: SEND: %d: ctlmsg=%s\n", file_send_count,  ctl_msg);
  file_send_count++;
  //printf(ctl_msg);
  ctl_msg_size =  strlen(ctl_msg);
  flag_init = 0;
  RDMA_Isendr(ctl_msg, ctl_msg_size, TRANSFER_INIT, &flag_init, &rdma_comm);
  //  fprintf(stderr, "ACT lib: SEND: %d: DONE ctlmsg=%s\n", file_send_count,  ctl_msg);
 
 /*---------*/
  /*send file*/
  fd = open(src, O_RDONLY);
  read_size = buf_read(fd, fbufs[fbuf_index].buf, buf_size);

  RDMA_Wait (&flag_init);
 

  do {
    //    printf("sent fbuf_index=%d\n", fbuf_index);
    //    fprintf(stderr, "ACT lib: SEND: Befor RDNA Isendr call: ctlmsg=%s\n", ctl_msg);
    flags[fbuf_index] = 0;
    RDMA_Isendr(fbufs[fbuf_index].buf, read_size, tag, &flags[fbuf_index], &rdma_comm);
    send_buf += read_size;
    //    fprintf(stderr, "ACT lib: SEND: After RDNA Isendr call: ctlmsg=%s\n", ctl_msg);
    //flags[fbuf_index] = 1;
    //    printf("... sent done\n");

    read_size = buf_read(fd, fbufs[(fbuf_index + 1) % count].buf, buf_size);

    //    fprintf(stderr, "ACT lib: SEND: read time = %f secs, read size = %f MB, throughput = %f MB/s: ctlmsg=%s\n", e - s, read_size/1000000.0, read_size/(e - s)/1000000.0, ctl_msg);
    //fprintf(stderr, "ACT lib: SEND: Before wait: ctlmsg=%s\n",  ctl_msg);
    RDMA_Wait (&flags[fbuf_index]);
    //    fprintf(stderr, "ACT lib: SEND: After wait: ctlmsg=%s\n",  ctl_msg);
    fbuf_index = (fbuf_index + 1) % count;
  } while (read_size > 0);

  /*---------*/

  /*send fin*/
  sprintf(ctl_msg, "%d\t%s", tag, dst);
  ctl_msg_size =  strlen(ctl_msg);
  RDMA_Sendr(ctl_msg, ctl_msg_size, TRANSFER_FIN, &rdma_comm);
  /*---------*/
  close(fd);
  for (i = 0; i < count; i++) {
    free(fbufs[i % count].buf);
  }
  free(flags);
  free(fbufs);

  e = get_dtime();
  fprintf(stderr, "ACT lib: SEND: file send: Time= %f , size= %d \n",  e - s, send_buf);

  return 0;
}


int TEST_RDMA_transfer (char* host, char* src, char* dst) 
{
  //  uint64_t size;
  //  int flag1, flag2;
  struct  RDMA_communicator comm;
  struct  RDMA_param param;

  double st, et;
  int i;

  param.host = host;

  RDMA_Active_Init(&comm, &param);
  st = get_dtime();
  for (i = 0; i < 12; i++){
    char path[1024];
    sprintf(path, "%s.%d", dst, i);
    RDMA_transfer(src, path, 200000000, 2, &comm);
  }
  et = get_dtime();
  printf("ACT: Total time= %f\n", et - st);
  //  RDMA_Active_Finalize(&comm);
  return 0;
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
  double e,s;

  fbufs = (struct file_buffer *)malloc(sizeof(struct file_buffer) * count);
  flags = (int *)malloc(sizeof(int) * count);
  for (i = 0; i < count; i++) {
    fbufs[i % count].buf =  (char *)malloc(buf_size);
    flags[i % count] = 1;
  }

  /*send init*/
  tag=get_tag();
  sprintf(ctl_msg, "%d\t%s", tag, dst);
  //printf(ctl_msg);
  ctl_msg_size =  strlen(ctl_msg);
  RDMA_Sendr(ctl_msg, ctl_msg_size, TRANSFER_INIT, comm);
  /*---------*/

  /*send file*/
  fd = open(src, O_RDONLY);
  //  printf("read fbuf_index=%d\n", fbuf_index);
  read_size = buf_read(fd, fbufs[fbuf_index].buf, buf_size);

  do {
    //    printf("sent fbuf_index=%d\n", fbuf_index);

    RDMA_Isendr(fbufs[fbuf_index].buf, read_size, tag, &flags[fbuf_index], comm);
    //flags[fbuf_index] = 1;
    //    printf("... sent done\n");
    //    printf("read fbuf_index=%d\n", fbuf_index);
    s = get_dtime();
    read_size = buf_read(fd, fbufs[(fbuf_index + 1) % count].buf, buf_size);
    e = get_dtime();
    //    printf("ACT lib: read time = %fsecs, read size = %d MB, throughput = %f MB/s\n", e - s, read_size/1000000, read_size/(e - s)/1000000.0);
    RDMA_Wait (&flags[fbuf_index]);

    fbuf_index = (fbuf_index + 1) % count;
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
  /*TODO: Remove hard cored arg*/
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

ssize_t buf_read(int fd, void* buf, size_t size)
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
	  printf("Error reading: read(%d, %s, %ld)   @ %s:%d\n",
		  fd, (char*) buf + n, size - n,  __FILE__, __LINE__
		  );
	} else {
	  /* too many failed retries, give up */
	  printf("Giving up read: read(%d, %s, %ld)  @ %s:%d\n",
		  fd, (char*) buf + n, size - n, __FILE__, __LINE__
		  );
	  exit(1);
	}
      }
    }
  return n;
}





