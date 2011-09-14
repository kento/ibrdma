#include "rdma-common-client.h"
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

int main(int argc, char **argv)
{
  char* host;
  char* port;
  char* data;
  uint64_t size;

  if (argc != 4)
    usage(argv[0]);

  host = argv[1];
  port = argv[2];
  size = atoi(argv[3]);
  data = (char*)malloc(size);

  init_test_data(data, size);
  
  ibrdma_send(host, port, data, size);
  return 0;
}

static int init_test_data(char* data, uint64_t size) {
  int i;

  for (i=size-2; i >= 0; i--) {
    data[i] = (char) (i % 26 + 'a');
    printf("%c",data[i]);
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
    fprintf(stderr, "get event failed : %d\n", rc);
    return (rc);
  }
  fprintf(stderr, "got \"%s\" event\n", event_type_str(event->event));
  
  if (event->event == requested_event)
    rv = 0;
  rdma_ack_cm_event(event);
  return (rv);
}







//static int run(int argc, char **argv)
int ibrdma_send(char* host, char* port, void* data, uint64_t size)
{
  struct addrinfo *addr;
  //struct rdma_cm_event *event = NULL;
  struct rdma_cm_id *cmid= NULL;
  struct rdma_event_channel *ec = NULL;
  struct rdma_conn_param cm_params;


  //print_device_info();



  TEST_NZ(getaddrinfo(host, port, NULL, &addr));

  TEST_Z(ec = rdma_create_event_channel());
  /*create rdma socket*/
  TEST_NZ(rdma_create_id(ec, &cmid, NULL, RDMA_PS_TCP));

  /* int rdma_resolve_addr (struct rdma_cm_id *id, struct sockaddr *src_addr, 
                            struct sockaddr dst_addr, int timeout_ms)
       id          RDMA identifier
       src_addr    Source address information. This parameter may be NULL.
       dst_addr    Destination address information
       timeout_ms  Time to wait for resolution to complete
     Description:
       Resolve destination and optional source addresses from IP addresses 
       to an RDMA address. If suc- cessful, 
       the specified rdma_cm_id will be bound to a local device.
  */
  TEST_NZ(rdma_resolve_addr(cmid, NULL, addr->ai_addr, TIMEOUT_IN_MS));
  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_ADDR_RESOLVED));
  freeaddrinfo(addr);
  build_connection(cmid);
  

  /*--------------------*/

  /* int rdma_resolve_route (struct rdma_cm_id *id, int timeout_ms); 
       id            RDMA identifier
       timeout_ms    Time to wait for resolution to complete
     Description:
       Resolves an RDMA route to the destination address in order 
       to establish a connection. The destination address must have 
       already been resolved by calling rdma_resolve_addr. 
   */
  TEST_NZ(rdma_resolve_route(cmid, TIMEOUT_IN_MS));
  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_ROUTE_RESOLVED));
  /* -------------------- */
  
  //  print_path_rec(cmid);

  /* int rdma_connect (struct rdma_cm_id *id, struct rdma_conn_param *conn_param); 
       id            RDMA identifier
       conn_param    connection parameters

       Description:
       For an rdma_cm_id of type RDMA_PS_TCP, this call initiates a connection 
       request to a remote destination. For an rdma_cm_id of type RDMA_PS_UDP, 
       it initiates a lookup of the remote QP providing the datagram service
  */
  build_params(&cm_params);
  TEST_NZ(rdma_connect(cmid, &cm_params));
  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_ESTABLISHED));
  printf("Connected !\n");
  /* --------------------- */
  
  /*TODO: do something */
  on_connect(cmid->context);
  //  char* msg = (char* ) malloc(size);
  //  sprintf(msg,"sze=%d",size);
  //  printf(msg);

  init_tfile(data,  size);

  //  build_send_msg(cmid->context, data, size);
  //  send_mr(cmid->context, size);
  send_init(cmid->context);
  /*--------------------*/

  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_DISCONNECTED));
  rdma_destroy_id(cmid);
  rdma_destroy_event_channel(ec);

  return 0;
}


void usage(const char *argv0)
{
  fprintf(stderr, "usage: %s <server-address> <server-port> <data size(bytes)>\n", argv0);
  exit(1);
}


