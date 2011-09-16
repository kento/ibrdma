#ifndef RDMA_COMMON_H
#define RDMA_COMMON_H

#define MAX_PATH_LEN (1024) /*1024 bytes is max in linux*/

#ifndef TRANSFER_PORT
#define TRANSFER_PORT (10150)
#endif

#ifndef NUM_FILE_BUF_C
#define NUM_FILE_BUF_C (2)
#endif

#ifndef FILE_BUF_SIZE_C
#define FILE_BUF_SIZE_C (4*1000*1000)
#endif

#ifndef NUM_MR_C
#define NUM_MR_C (2)
#endif

#ifndef MR_SIZE_C
#define MR_SIZE_C (256*1000)
#endif

#ifndef NUM_MR_S
#define NUM_MR_S (2)
#endif

#ifndef MR_SIZE_S
#define MR_SIZE_S (256*1000)
#endif



#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>

#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)



enum mode {
  M_WRITE,
  M_READ
};

struct mr_message {
  enum {
    MR_INIT, /*Inform which file to be sent*/
    /*RDMA: read the information and prepare*/
    MR_INIT_ACK, /*Reply ack*/
    /*Wait a chunk of the file*/
    MR_CHUNK, /*Inform Memory region(MR) of the chunk*/
    /*Send file chunk*/
    MR_CHUNK_ACK,
    /*one file transfer done*/
    MR_FILE_DONE,
    MR_FILE_DONE_ACK,
    /*all files transfer done*/
    MR_FIN,
    MR_FIN_ACK
  } type;

  union {
    struct ibv_mr mr;
  } data;
  char path[MAX_PATH_LEN];
  uint64_t size;
};


struct transfer_info {
  struct transfer_file *tfiles;
  int num_ffile;
  char *ib_host; /*IP address in IPoIB*/
  int   ib_port;
  //  struct file_buf fbufs[NUM_FILE_BUF_C];
  struct file_buf *fbufs;
};

struct transfer_file {
  char* path;
  void* data;
  void* send_base_addr;
  uint64_t size;
  uint64_t tsize;
};



struct file_buf{
  char *fbuf;
  int size;/*size of data to transfer. 0 means buffer is free*/
};

void die(const char *reason);

void build_connection(struct rdma_cm_id *id);
void build_params(struct rdma_conn_param *params);
void destroy_connection(void *context);
void * get_local_message_region(void *context);
void on_connect(void *context);
void send_mr(void *context, int size);
void set_mode(enum mode m);
const char *event_type_str(enum rdma_cm_event_type event);
void send_memr(void * conn, int type, struct ibv_mr *data, uint64_t size );
void send_init(void *conn);
void init_tfile(void* data, int size);


#endif
