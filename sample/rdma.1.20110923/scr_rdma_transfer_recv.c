#include "scr_rdma_transfer.h"
#include "rdma-server.h"
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>


#ifndef TABLE_LEN
#define TABLE_LEN (1000)
#endif

static double get_dtime(void);

struct file_path {
  int tag;
  char path[1024];
  int flag;
  int fd;
};

struct file_path fp[TABLE_LEN];

static int get_index(uint64_t id);
static int hash(uint64_t id);
static double get_dtime(void);


void RDMA_transfer_recv(void);


int main(int argc, char **argv) {
  RDMA_transfer_recv();
  return 0;
}


void RDMA_transfer_recv(void)
{
  struct RDMA_communicator comm;
  char* data;
  //  char* path;
  uint64_t size;
  int ctl_tag;
  int file_tag;
  int id;
  int i;
  double s,e;


  RDMA_Passive_Init(&comm);

  for (i = 0; i < TABLE_LEN; i++) {
    fp[i].flag = 0;
  }

  while (1) {


    RDMA_Recvr(&data, &size, &ctl_tag, &comm);
    if (ctl_tag == TRANSFER_INIT) {
      s = get_dtime();
      printf("%d: size=%lu: %s\n", ctl_tag, size, data);
      file_tag = atoi(strtok(data, "\t"));
      id = get_index(file_tag);
      fp[id].tag = file_tag;
      fp[id].fd = open(strtok(NULL, "\t"), O_WRONLY | O_CREAT);
      fp[id].flag = 1;
    } else if (ctl_tag == TRANSFER_FIN) {
      printf("%d: size=%lu: %s\n", ctl_tag, size, data);
      file_tag = atoi(strtok(data, "\t"));
      id = get_index(file_tag);
      fp[id].tag = -1;
      close(fp[id].fd);
      fp[id].flag = 0;
      e = get_dtime();
      printf("time: %f\n", e - s);
    } else {
      printf("%d: size=%lu\n", ctl_tag, size);
      id = get_index(file_tag);
      write(fp[id].fd, data, size);
      free(data);
    }
  }
  //  RDMA_show_buffer();
  return ;
}

static double get_dtime(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
}



static int get_index(uint64_t id)
{
  int i;
  int index;
  int offset = 0;
  index = hash(id + offset);
  i = 0;
  for (i = 0; i < TABLE_LEN; i++) {
    if (fp[index].flag == 0) {
      return index;
    } else if (fp[index].tag == id) {
      return index;
    }
    index = (index + 1) % TABLE_LEN;
    i++;
  }
  printf("Error:Hashtable: Enough size of hashtable needed\n");
  exit(0);
  return 0;
}

static int hash(uint64_t id)
{
  unsigned int b    = 378551;
  unsigned int a    = 63689;
  unsigned int hash = 0;
  unsigned int i    = 0;
  unsigned int l    = 10;
  for(i = 0; i < l; i++)
    {
      hash = hash * a + id;
      a    = a * b;
    }

  return hash % TABLE_LEN;
}
