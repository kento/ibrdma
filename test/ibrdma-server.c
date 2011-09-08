#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rdma/rdma_cma.h>

#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned non-zero." ); } while (0)

struct connection {
  struct ibv_qp *qp;
 
  struct ibv_mr *recv_mr;
  struct ibv_mr *send_mr;
 
  char *recv_region;
  char *send_region;
};

struct context {
  struct ibv_context *ctx;
  struct ibv_pd *pd;
  struct ibv_cq *cq;
  struct ibv_comp_channel *comp_channel;
  
  pthread_t cq_poller_thread;
};

static void die(const char *reason);

static int on_event(struct rdma_cm_event *event);

static int on_connect_request(struct rdma_cm_id *id);
static int on_connection(void *context);
static int on_disconnect(struct rdma_cm_id *id);

static void build_context(struct ibv_context *verbs);
static void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
static void post_receives(struct connection *conn);
static void register_memory(struct connection *conn);

static void* poll_cq(void *);
static struct context *s_ctx = NULL;

static void on_completion(struct ibv_wc *wc);

int main (int argc, char **reason)
{
  /*
  struct sockaddr_in{
   short sin_family;
   unsigned short sin_port;
   struct in_addr sin_addr;
   char sin_zero[8];
  };
   */
  struct sockaddr_in addr;

  
  struct rdma_cm_event *event = NULL;


  /*
  struct rdma_cm_id {
    struct ib_device        *device;
    void                    *context;
    struct ib_qp            *qp;
    rdma_cm_event_handler    event_handler;
    struct rdma_route        route;
    enum rdma_port_space     ps;
    enum ib_qp_type          qp_type;
    u8                       port_num;
   };
  */
  struct rdma_cm_id *listener = NULL;
  struct rdma_event_channel *ec = NULL;
  uint16_t port = 0;

  // By setting addr.sin_port to zero, 
  //we instruct rdmacm to pick an available port
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;

  // 1. Create an event channel so that we can receive rdmacm events, 
  // such as connection-request and connection-established notifications.
  TEST_Z(ec = rdma_create_event_channel());

  // 2. Create a listener
  TEST_NZ(rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP));

  // 3. Bind to an address and get the address/port
  TEST_NZ(rdma_bind_addr(listener, (struct sockaddr *) &addr));
  port = ntohs(rdma_get_src_port(listener));

  // 4. Wait for a connection request.
  printf("listening on port %d.\n", port);
  while (rdma_get_cm_event(ec, &event) == 0) {
    struct rdma_cm_event event_copy;
    memcpy(&event_copy, event, sizeof(*event));
    rdma_ack_cm_event(event);
    if (on_event(&event_copy))
      break;
  }

  rdma_destroy_id(listener);
  rdma_destroy_event_channel(ec);

//   7. Wait for the connection to be established.
//  8. Post operations as appropriate.
  return 0;
}

int on_event(struct rdma_cm_event *event)
{
  int r = 0;
  
  if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST)
    r = on_connect_request(event->id);
  else if (event->event == RDMA_CM_EVENT_ESTABLISHED)
    r = on_connection(event->id->context);
  else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
    r = on_disconnect(event->id);
  else 
    die("on_event: unknown event.");

  return r;
}

int on_connect_request(struct rdma_cm_id *id)
{
  struct ibv_qp_init_attr qp_attr;
  struct rdma_conn_param cm_params;
  struct connection *conn;

  printf("received connection request.\n");

  // 5. Create a protection domain, completion queue, and send-receive queue pair.
  build_context(id->verbs);
  build_qp_attr(&qp_attr);

  TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

  id->context = conn = (struct connection *)malloc(sizeof(struct connection));
  conn->qp = id->qp;

  register_memory(conn);
  post_receives(conn);

  memset(&cm_params, 0, sizeof(cm_params));
  // 6. Accept the connection request.
  TEST_NZ(rdma_accept(id, &cm_params));

  return 0;

}

void buidl_context(struct ibv_context *verbs)
{
  if (s_ctx) {
    if (s_ctx->ctx != verbs)
      die("cannot handle events in more than one context.");
    return;
  }
  
  s_ctx = (struct context*)malloc(sizeof(struct context));
  s_ctx->ctx = verbs;

  TEST_Z(s_ctx->pd = ib_alloc_pd(s_ctx->ctx));
  TEST_Z(s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx));
  TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0));
  TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));

  TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL));
}

void die(const char *reason)
{
  fprintf(stderr, "%s\n", reason);
  exit(EXIT_FAILURE);
}

void * poll_cq(void *ctx)
{
  struct ibv_cq *cq;
  struct ibv_wc wc;
 
  while (1) {
    TEST_NZ(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
    ibv_ack_cq_events(cq, 1);
    TEST_NZ(ibv_req_notify_cq(cq, 0));
 
    while (ibv_poll_cq(cq, 1, &wc))
      on_completion(&wc);
  }
 
  return NULL;
}


void build_qp_attr(struct ibv_qp_init_attr *qp_attr)
{
  memset(qp_attr, 0, sizeof(*qp_attr));
 
  qp_attr->send_cq = s_ctx->cq;
  qp_attr->recv_cq = s_ctx->cq;
  qp_attr->qp_type = IBV_QPT_RC;
 
  qp_attr->cap.max_send_wr = 10;
  qp_attr->cap.max_recv_wr = 10;
  qp_attr->cap.max_send_sge = 1;
  qp_attr->cap.max_recv_sge = 1;
}
