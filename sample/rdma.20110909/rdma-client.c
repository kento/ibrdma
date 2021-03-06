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


static int run(int ac, char **av);
static void print_path_rec(struct rdma_cm_id *id);
//static const char *event_type_str(enum rdma_cm_event_type event);
static char *sprint_gid(union ibv_gid *gid, char *str, size_t len);


int main(int argc, char **argv)
{
  return run(argc, argv);
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


static int run(int argc, char **argv)
{
  struct addrinfo *addr;
  //struct rdma_cm_event *event = NULL;
  struct rdma_cm_id *cmid= NULL;
  struct rdma_event_channel *ec = NULL;
  struct rdma_conn_param cm_params;

  if (argc != 4)
    usage(argv[0]);

  if (strcmp(argv[1], "write") == 0)
    set_mode(M_WRITE);
  else if (strcmp(argv[1], "read") == 0)
    set_mode(M_READ);
  else
    usage(argv[0]);

  TEST_NZ(getaddrinfo(argv[2], argv[3], NULL, &addr));

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
  sprintf(get_local_message_region(cmid->context), "message from active/client side with pid %d", getpid());
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
  
  print_path_rec(cmid);

  /* int rdma_connect (struct rdma_cm_id *id, struct rdma_conn_param *conn_param); 
       id            RDMA identifier
       conn_param    connection parameters

       Description:
       For an rdma_cm_id of type RDMA_PS_TCP, this call initiates a connection 
       request to a remote destination. For an rdma_cm_id of type RDMA_PS_UDP, 
       it initiates a lookup of the remote QP providing the datagram service
  */
  build_params(&cm_params);
  printf("Connecting ...\n");
  TEST_NZ(rdma_connect(cmid, &cm_params));
  TEST_NZ(wait_for_event(ec, RDMA_CM_EVENT_ESTABLISHED));
  printf("Connected !\n");
  /* --------------------- */
  
  /*TODO: do something */
  on_connect(cmid->context);
  send_mr(cmid->context);
  /*--------------------*/

  rdma_disconnect(cmid);
  rdma_destroy_id(cmid);
  rdma_destroy_event_channel(ec);

  return 0;
  /*=================*/
  /*=================*/

  /*
  while (rdma_get_cm_event(ec, &event) == 0) {


    memcpy(&event_copy, event, sizeof(*event));
    rdma_ack_cm_event(event);

    if (on_event(&event_copy))
      break;
  }
  */
}

/*
int on_addr_resolved(struct rdma_cm_id *id)
{
  printf("address resolved.\n");

  build_connection(id);
  sprintf(get_local_message_region(id->context), "message from active/client side with pid %d", getpid());
  TEST_NZ(rdma_resolve_route(id, TIMEOUT_IN_MS));

  return 0;
  }*/

/*
int on_connection(struct rdma_cm_id *id)
{
  on_connect(id->context);
  send_mr(id->context);

  return 0;
}
*/

/*
int on_disconnect(struct rdma_cm_id *id)
{
  printf("disconnected.\n");

  destroy_connection(id->context);
  return 1; 
}
*/

/*
int on_event(struct rdma_cm_event *event)
{
  int r = 0;

  if (event->event == RDMA_CM_EVENT_ADDR_RESOLVED)
    r = on_addr_resolved(event->id);
  else if (event->event == RDMA_CM_EVENT_ROUTE_RESOLVED)
    r = on_route_resolved(event->id);
  else if (event->event == RDMA_CM_EVENT_ESTABLISHED)
    r = on_connection(event->id);
  else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
    r = on_disconnect(event->id);
  else
    die("on_event: unknown event.");

  return r;
}
*/

 /*
int on_route_resolved(struct rdma_cm_id *id)
{
  struct rdma_conn_param cm_params;

  printf("route resolved.\n");
  build_params(&cm_params);
  TEST_NZ(rdma_connect(id, &cm_params));

  return 0;
  }
*/

void usage(const char *argv0)
{
  fprintf(stderr, "usage: %s <mode> <server-address> <server-port>\n  mode = \"read\", \"write\"\n", argv0);
  exit(1);
}
