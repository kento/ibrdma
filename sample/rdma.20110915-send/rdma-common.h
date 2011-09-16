#ifndef RDMA_COMMON_H
#define RDMA_COMMON_H

#ifndef NUM_FILE_BUF
#define NUM_FILE_BUF (2)
#endif

#ifndef FILE_BUF_SIZE
#define FILE_BUF_SIZE (4*1000*1000)
#endif

#ifndef CLIENT_NUM_MR
#define CLIENT_NUM_MR (2)
#endif

#ifndef CLIENT_MR_SIZE
#define CLIENT_MR_SIZE (256*1000)
#endif

#ifndef SERVER_NUM_MR
#define SERVER_NUM_MR (2)
#endif

#ifndef SERVER_MR_SIZE
#define SERVER_MR_SIZE (256*1000)
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

struct flush_file {
  char *path;
};

struct transfer_info {
  struct flush_file *ffiles;
  char *ib_host; /*IP address in IPoIB*/
  int   ib_port;
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
