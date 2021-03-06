
#include "rdma-common.h"
#include "assert.h"
#include "arpa/inet.h"
#include "time.h"

const int TIMEOUT_IN_MS = 500; /* ms */

//static int on_addr_resolved(struct rdma_cm_id *id);
//nstatic int on_connection(struct rdma_cm_id *id);
//static int on_disconnect(struct rdma_cm_id *id);
//static int on_event(struct rdma_cm_event *event);
//static int on_route_resolved(struct rdma_cm_id *id);
static void usage(const char *argv0);


//static int run(int ac, char **av);
static void print_path_rec(struct rdma_cm_id *id);
//static const char *event_type_str(enum rdma_cm_event_type event);
//int build_send_msg(struct connection *conn, char* addr, int size);
static char *sprint_gid(union ibv_gid *gid, char *str, size_t len);
static int print_device_info(void) ;
static int init_test_data(char *data, uint64_t size);
int ibrdma_send(char* host, char* port, void* data, uint64_t size);
int ibrdma_transfer(struct transfer_info *tfi);
uint64_t get_file_size(const char *file_name);
ssize_t file_read(int fd, void* buf, size_t size);

int main(int argc, char **argv)
{
  char* host;
  char port[128];
  char* data;
  uint64_t size;
  struct transfer_info tfi;
  int n = 3;
  struct transfer_file tf[n];

  if (argc != 3)
    usage(argv[0]);

  host = argv[1];
  size = atoi(argv[2]);

  data = (char*)malloc(size);
  init_test_data(data, size);
  sprintf(port, "%d", TRANSFER_PORT);

  // INIT transfer_info
  tf[0].path_src = "/g/g90/sato5/Programs/ibrdma/sample/rdma/tmp/b.1000000";
  tf[0].path_dst = "/p/lscratchc/sato5/b.1000000";
  tf[1].path_src = "/g/g90/sato5/Programs/ibrdma/sample/rdma/tmp/b.1000";
  tf[1].path_dst = "/p/lscratchc/sato5/b.1000";
  tf[2].path_src = "/g/g90/sato5/Programs/ibrdma/sample/rdma/tmp/b.1000.1";
  tf[2].path_dst = "/p/lscratchc/sato5/b.1000.1";

  tfi.tfiles = tf;
  tfi.num_tfile = 1;
  tfi.ib_host = host;

  //ibrdma_send(host, port, data, size);
  ibrdma_transfer(&tfi);
  return 0;
}

static int init_test_data(char* data, uint64_t size) {
  int i;

  for (i=size-2; i >= 0; i--) {
    data[i] = (char) (i % 26 + 'a');
    //    printf("%c",data[i]);
  }
  data[size-1] += '\0';
  return 0;
}

static char *sprint_gid(union ibv_gid *gid, char *str, size_t len)
{
  char *rc = NULL;
  assert(str != NULL);
  assert(gid != NULL);
  rc = (char *)inet_ntop(AF_INET6, gid->raw, str, len);

  if (!rc) {
    perror("inet_ntop failed\n");
    exit(-1);
  }
  return (rc);
}


static void print_path_rec (struct rdma_cm_id *id)
{
  struct rdma_route *route = &(id->route);
  int   i = 0;
  char  str[128];
  printf("   Path Information:\n");
  for (i = 0; i < route->num_paths; i++) {
    struct ibv_sa_path_rec *rec = &(route->path_rec[i]);
    printf("      Record           : %d\n", i);
    printf("         dgid          : %s\n", sprint_gid(&(rec->dgid), str, 128));
    printf("         sgid          : %s\n", sprint_gid(&(rec->sgid), str, 128));
    printf("         dlid          : %d\n", ntohs(rec->dlid));
    printf("         slid          : %d\n", ntohs(rec->slid));
    printf("         raw           : %d\n", rec->raw_traffic);
    printf("         flow_label    : %d\n", rec->flow_label);
    printf("         hop_limit     : %d\n", rec->hop_limit);
    printf("         traffic_class : %d\n", rec->traffic_class);
    printf("         reversible    : %d\n", ntohl(rec->reversible));
    printf("         numb_path     : %d\n", rec->numb_path);
    printf("         pkey          : 0x%04X\n", rec->pkey);
    printf("         sl            : %d\n", rec->sl);
    printf("         mtu_selector  : %d\n", rec->mtu_selector);
    printf("         mtu           : %d\n", rec->mtu);
    printf("         rate_selector : %d\n", rec->rate_selector);
    printf("         rate          : %d\n", rec->rate);
    printf("         packet_lts    : %d\n", rec->packet_life_time_selector);
    printf("         packet_lt     : %d\n", rec->packet_life_time);
    printf("         preference    : %d\n", rec->preference);
  }
}

/**************************************************************************
 * helper functions to print nice pretty strings...
 */
/*

static const char *event_type_str(enum rdma_cm_event_type event)
{
  switch (event)
    {
    case RDMA_CM_EVENT_ADDR_RESOLVED: return ("Addr resolved");
    case RDMA_CM_EVENT_ADDR_ERROR: return ("Addr Error");
    case RDMA_CM_EVENT_ROUTE_RESOLVED: return ("Route resolved");
    case RDMA_CM_EVENT_ROUTE_ERROR: return ("Route Error");
    case RDMA_CM_EVENT_CONNECT_REQUEST: return ("Connect request");
    case RDMA_CM_EVENT_CONNECT_RESPONSE: return ("Connect response");
    case RDMA_CM_EVENT_CONNECT_ERROR: return ("Connect Error");
    case RDMA_CM_EVENT_UNREACHABLE: return ("Unreachable");
    case RDMA_CM_EVENT_REJECTED: return ("Rejected");
    case RDMA_CM_EVENT_ESTABLISHED: return ("Established");
    case RDMA_CM_EVENT_DISCONNECTED: return ("Disconnected");
    case RDMA_CM_EVENT_DEVICE_REMOVAL: return ("Device removal");
    default: return ("Unknown");
    }
  return ("Unknown");
  }
*/

static int print_device_info(void) {
  struct ibv_device ** ibv_devs;
  int i = 0;
  /*TODO: get num_devs automatically*/
  int num_devs = 1;
  /*NULL => get all devices*/

  ibv_devs = ibv_get_device_list(NULL);

  for (i = 0; i < num_devs; i++) {
    struct ibv_context *ibv_contxt;
    struct ibv_device_attr device_attr;
    char *dev_name;
    uint64_t dev_guid;

    ibv_contxt = ibv_open_device (ibv_devs[i]);

    dev_name = ibv_get_device_name(ibv_devs[i]);
    dev_guid = ibv_get_device_guid(ibv_devs[i]);
    printf("%s (%d):\n", dev_name, dev_guid);
    ibv_query_device (ibv_contxt, &device_attr);
    printf("      Record           : %d\n", i);
    printf("         max_mr_size   : %llu\n", device_attr.max_mr_size);
    printf("         max_mr        : %llu\n", device_attr.max_mr);

    ibv_close_device (ibv_contxt);
  }

  ibv_free_device_list(ibv_devs);
  return 0;
}

/*************************************************************************
 * Wait for the rdma_cm event specified.
 * If another event comes in return an error.
 */
static int
wait_for_event(struct rdma_event_channel *channel, enum rdma_cm_event_type requested_event)
{
  struct rdma_cm_event *event;
  int                   rc = 0;
  int                   rv = -1;

  if ((rc = rdma_get_cm_event(channel, &event)))
  {
    fprintf(stdout, "get event failed : %d\n", rc);
    return (rc);
  }
  fprintf(stdout, "got \"%s\" event\n", event_type_str(event->event));
  
  if (event->event == requested_event)
    rv = 0;
  rdma_ack_cm_event(event);
  return (rv);
}

int ibrdma_transfer(struct transfer_info *tfi) {
  struct addrinfo *addr;
  struct rdma_cm_id *cmid= NULL;
  struct rdma_event_channel *ec = NULL;
  struct rdma_conn_param cm_params;
  int fd;
  int i,j;

  /*Allocation buffer space for reading from local fs to memory*/
  struct transfer_file *tfs = tfi->tfiles;
  struct file_buf *fbs;
  int nf = tfi->num_tfile;
  char* host = tfi->ib_host;
  char port[16];
  sprintf(port, "%d", TRANSFER_PORT);

  TEST_NZ(getaddrinfo(host, port, NULL, &addr));
  TEST_Z(ec = rdma_create_event_channel());
  TEST_NZ(rdma_create_id(ec, &cmid, NULL, RDMA_PS_TCP));
  TEST_NZ(rdma_resolve_addr(cmid, NULL, addr->ai_addr, TIMEOUT_IN_MS));
  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_ADDR_RESOLVED));
  freeaddrinfo(addr);
  build_connection(cmid);
  TEST_NZ(rdma_resolve_route(cmid, TIMEOUT_IN_MS));
  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_ROUTE_RESOLVED));
  build_params(&cm_params);
  TEST_NZ(rdma_connect(cmid, &cm_params));
  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_ESTABLISHED));
  on_connect(cmid->context);
  

  tfi->fbufs = fbs= (struct file_buf*)malloc(sizeof(struct file_buf) * NUM_FILE_BUF_C);
  for (i = 0; i < NUM_FILE_BUF_C; i++) {
    (fbs + i)->fbuf = (char *)malloc(FILE_BUF_SIZE_C);
  }

  j = 0;
  for (i = 0; i < nf; i++) {
    uint64_t size =  (uint64_t) get_file_size(tfs->path_src);
    printf("Transfer: File=%s, Size=%d\n",tfs->path_src, size);
    fd = open(tfs->path_src, O_RDONLY);
    uint64_t tsize = 0;
    while (tsize < size) {
      j = j % NUM_FILE_BUF_C;
      struct file_buf *fb = fbs + j;
      uint64_t read_size = FILE_BUF_SIZE_C;
      if (tsize + read_size > size) {
	read_size = size - tsize;
      }
      fb->size = read_size;
      printf("Chunk size =%d\n",read_size);
      file_read(fd, fb->fbuf, read_size);
      send_init(cmid->context, fb);
      start_thread(fb);
      tsize += read_size;
      j++;
    }
    tfs = tfs + 1;
  }

    /*----------------------------*/

    //    TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL));   

    TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_DISCONNECTED));
    rdma_destroy_id(&cmid);
    rdma_destroy_event_channel(&ec);


  return 0;
}

uint64_t get_file_size(const char *file_name)
{
  uint64_t fsize = 0;

  FILE *fp = fopen(file_name, "rb"); 

  fseek(fp,0,SEEK_END); 
  fsize = ftell(fp); 
 
  fclose(fp);
 
  return fsize;
}

ssize_t file_read(int fd, void* buf, size_t size)
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
       //       if (errno == EINTR || errno == EAGAIN) {
       //         continue;
       //       }
       /* something worth printing an error about */
       retries--;
       if (retries) {
         /* print an error and try again */
         printf("Error reading: read(%d, %x, %ld)  %m @ %s:%d",
                 fd, (char*) buf + n, size - n,  __FILE__, __LINE__
		 );
       } else {
         /* too many failed retries, give up */
         printf("Giving up read: read(%d, %x, %ld)  %m @ %s:%d",
                 fd, (char*) buf + n, size - n, __FILE__, __LINE__
		 );
         exit(1);
       }
     }
   }
   return n;
 }


//static int run(int argc, char **argv)
int ibrdma_send(char* host, char* port, void* data, uint64_t size)
{
  
  struct addrinfo *addr;
  struct rdma_cm_id *cmid= NULL;
  struct rdma_event_channel *ec = NULL;
  struct rdma_conn_param cm_params;
  TEST_NZ(getaddrinfo(host, port, NULL, &addr));
  TEST_Z(ec = rdma_create_event_channel());
  TEST_NZ(rdma_create_id(ec, &cmid, NULL, RDMA_PS_TCP));
  TEST_NZ(rdma_resolve_addr(cmid, NULL, addr->ai_addr, TIMEOUT_IN_MS));
  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_ADDR_RESOLVED));
  freeaddrinfo(addr);
  build_connection(cmid);
  TEST_NZ(rdma_resolve_route(cmid, TIMEOUT_IN_MS));
  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_ROUTE_RESOLVED));
  build_params(&cm_params);
  TEST_NZ(rdma_connect(cmid, &cm_params));
  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_ESTABLISHED));
  on_connect(cmid->context);

  /* Init MSG send to start RDMA*/
  struct file_buf *fbuf = (struct file_buf*)malloc(sizeof(struct file_buf));
  //  init_tfile(data,  size);
  fbuf->fbuf  = data;
  fbuf->size  = size;
  send_init(cmid->context, fbuf);
  start_thread(fbuf);
  //  struct file_buf *fbuf2 = (struct file_buf*)malloc(sizeof(struct file_buf));
  //  fbuf2->fbuf  = data;
  //  fbuf2->size  = size;
  //  send_init(cmid->context, fbuf2);
  //  start_thread(fbuf2);
  //  start_thread(fbuf);
  /*----------------------------*/

  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_DISCONNECTED));
  rdma_destroy_id(cmid);
  rdma_destroy_event_channel(ec);

  return 0;
}


void usage(const char *argv0)
{
  fprintf(stdout, "usage: %s <server-address> <server-port> <data size(bytes)>\n", argv0);
  exit(1);
}


