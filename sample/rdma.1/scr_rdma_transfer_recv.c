#include "common.h"
#include "scr_rdma_transfer_common.h"
#include "scr_rdma_transfer_send.h"
#include "rdma-server.h"
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>



#ifndef TABLE_LEN
#define TABLE_LEN (1000)
#endif

#ifndef MAX_WRITER
#define MAX_WRITER (10)
#endif



struct file_path {
  int tag;
  char path[1024];
  int flag;
  int fd;
};

struct thread_arg {
  int fd;
  char *data;
  uint64_t size;
  int id;
};

struct file_path fp[TABLE_LEN];
char cpath[TABLE_LEN][1024];
int fp_flag[TABLE_LEN];
pthread_t writer_thread[TABLE_LEN];
struct thread_arg writer_args[TABLE_LEN];
double start[TABLE_LEN];
double end[TABLE_LEN];

pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;
int thread_count = 0;


static int get_index(uint64_t id);
static int hash(uint64_t id);



void RDMA_transfer_recv(void);
void *writer(void *args);


int main(int argc, char **argv) {
  RDMA_transfer_recv();
  return 0;
}


int count = 1;

void RDMA_transfer_recv(void)
{
  struct RDMA_communicator comm;
  char* data;
  char* path;
  uint64_t size;
  int ctl_tag;
  int file_tag;
  int id;
  int i;
  char log[1024];

  RDMA_Passive_Init(&comm);

  for (i = 0; i < TABLE_LEN; i++) {
    fp[i].flag = 0;
    fp_flag[i] = 0;
  }
  
  int a = 0;
  double s;
  while (1) {
    RDMA_Recvr(&data, &size, &ctl_tag, &comm);
    if (a == 0) {
      s = get_dtime();
      a = 1;
    }
    if (ctl_tag == TRANSFER_INIT) {
      sprintf(log, "ACT lib: RECV: INI: %s\n", data);
      //printf(log);
      file_tag = atoi(strtok(data, "\t"));
      id = get_index(file_tag);
      fp[id].tag = file_tag;
      path = strtok(NULL, "\t");
      //      write_log(log);
      sprintf(cpath[id],"%s",path);
      fp[id].fd = open(path, O_WRONLY | O_CREAT, 0660);
      fp[id].flag = 1;
      start[id] = get_dtime();
    } else if (ctl_tag == TRANSFER_FIN) {
      //printf("%d: size=%lu: %s\n", ctl_tag, size, data);
      //
      file_tag = atoi(strtok(data, "\t"));
      id = get_index(file_tag);
      while (fp_flag[id] != 0) {
	usleep(1);
      }
      fp[id].tag = -1;
      close(fp[id].fd);
      fp[id].flag = 0;
      end[id] = get_dtime();

      sprintf(log, "ACT lib: RECV: FIN: %s: count=%d  Elapse_time=%f \n", cpath[id], count, end[id] - start[id]);
      count++;
      //write_log(log);
      //      printf(log);
      debug(printf("ACT lib: Recv: FIN: Time stamp= %f\n",  get_dtime() - s), 1);
      //RDMA_show_buffer();
    } else {
      id = get_index(ctl_tag);
      while (fp_flag[id] != 0) {
	usleep(1);
      }
      fp_flag[id] = 1;
      writer_args[id].id = id;
      writer_args[id].fd = fp[id].fd;
      writer_args[id].data = data;
      writer_args[id].size = size;
      while (thread_count >= MAX_WRITER) {
	usleep(10);
      }
      pthread_mutex_lock(&fastmutex);
      thread_count++;
      pthread_mutex_unlock(&fastmutex);
      pthread_create(&writer_thread[id], NULL, writer, &writer_args[id]);
     
      

      //id = get_index(ctl_tag);
      //            ws = get_dtime();
      //	    write(fp[id].fd, data, size);
      //    we = get_dtime();
      //	    printf("ACT lib: write time=%fsecs, write size=%d MB, throughput=%f MB/s\n", we - ws, size/1000000, size/(we - ws)/1000000.0);
      //	    //            printf("%d:id %d: fd %d: size=%lu DONE\n", ctl_tag, id, (int)fp[id].fd, size);
      //
    }
  }
  //  RDMA_show_buffer();
  return ;
}

void *writer(void *args)
{
  struct thread_arg *wargs;
  int fd;
  int id;
  char* data;
  uint64_t size;
  double ws, we;

  int n_write = 0;
  int n_write_sum = 0;

  wargs = (struct thread_arg *)args;
  id = wargs->id;
  fd = wargs->fd;
  data = wargs->data;
  size = wargs->size;
  //  printf("ACT lib: %d : start\n", fd);
  ws = get_dtime();

  do
    {
      n_write = write(fd, data, size);

      if(n_write < 0)
	{
	  perror("ACT lib: ERROR: RECV: write error\n");
	  exit(EXIT_FAILURE);
	}

      n_write_sum = n_write_sum + n_write;

      if(n_write_sum >= size)
	{
	  break;
	}
      fprintf(stderr, "ACT lib: RECV: partial write %d / %lu \n", n_write, size);
    } while( n_write > 0);
  fsync(fd);
  we = get_dtime();
  printf("ACT lib: RECV: writer ends: %d : write time = %f secs, write size= %f MB , throughput= %f MB/s\n", fd, we - ws, size/1000000.0, size/(we - ws)/1000000.0);
  //  printf("%d:id %d: fd %d: size=%lu DONE\n", ctl_tag, id, (int)fp[id].fd, size);
  fp_flag[id] = 0;
  free(data);
  
  pthread_mutex_lock(&fastmutex);
  thread_count--;
  pthread_mutex_unlock(&fastmutex);
  return NULL;
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
  unsigned int l    = 3;
  for(i = 0; i < l; i++)
    {
      hash = hash * a + id;
      a    = a * b;
    }

  return hash % TABLE_LEN;
}
