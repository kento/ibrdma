#include "rdma-common.h"

//static const int RDMA_BUFFER_SIZE = 1024;
static const int RDMA_BUFFER_SIZE = 1000000;



struct context {
  struct ibv_context *ctx;
  struct ibv_pd *pd;
  struct ibv_cq *cq;
  struct ibv_comp_channel *comp_channel;

  pthread_t cq_poller_thread;
};

struct connection {
  struct rdma_cm_id *id;
  struct ibv_qp *qp;

  int connected;
      
  struct ibv_mr *recv_mr;
  struct ibv_mr *send_mr;
  struct ibv_mr *rdma_msg_mr;

  struct ibv_mr peer_mr;

  struct mr_message *recv_msg;
  struct mr_message *send_msg;

  char *rdma_msg_region;

  enum {
    SS_INIT,
    SS_MR_SENT,
    SS_RDMA_SENT,
    SS_DONE_SENT
  } send_state;

  enum {
    RS_INIT,
    RS_MR_RECV,
    RS_DONE_RECV
  } recv_state;
};

static void build_context(struct ibv_context *verbs);
static void build_context_s(struct ibv_context *verbs);
static void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
static char * get_peer_message_region(struct connection *conn);
static void on_completion(struct ibv_wc *);
static void on_completion_s(struct ibv_wc *);
static void * poll_cq(void *);
static void * poll_cq_s(void *);
static void register_memory(struct connection *conn);
static void send_message(struct connection *conn);
static void post_receives(struct connection *conn);

int build_send_msg(struct connection *conn, char* msg, int size); 
//void send_mem(void * conn, int type, void* data, uint64_t size );
static void register_rdma_region(struct connection *conn, void* addr, uint64_t size);

static struct context *s_ctx = NULL;
static enum mode s_mode = M_WRITE;
struct transfer_file *tfile = NULL;
struct transfer_info *tinfo = NULL;

const char *event_type_str(enum rdma_cm_event_type event)
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

void die(const char *reason)
{
  fprintf(stderr, "%s\n", reason);
  exit(EXIT_FAILURE);
}

void init_tfile(void* data, int size) {
  tfile = (struct transfer_file *)malloc(sizeof(struct transfer_file));
  tfile->data = data;
  tfile->send_base_addr = data;
  tfile->tsize = 0;
  tfile->size = size;
}

void build_connection(struct rdma_cm_id *id)
{
  struct connection *conn;
  struct ibv_qp_init_attr qp_attr;

  build_context(id->verbs);
  build_qp_attr(&qp_attr);

  TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

  id->context = conn = (struct connection *)malloc(sizeof(struct connection));

  conn->id = id;
  conn->qp = id->qp;

  conn->send_state = SS_INIT;
  conn->recv_state = RS_INIT;
  
  conn->rdma_msg_mr = NULL;

  conn->connected = 0;

  register_memory(conn);
}

void build_connection_s(struct rdma_cm_id *id)
{
  struct connection *conn;
  struct ibv_qp_init_attr qp_attr;

  build_context_s(id->verbs);
  build_qp_attr(&qp_attr);

  TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

  id->context = conn = (struct connection *)malloc(sizeof(struct connection));

  conn->id = id;
  conn->qp = id->qp;

  conn->send_state = SS_INIT;
  conn->recv_state = RS_INIT;

  conn->connected = 0;

  register_memory(conn);
  post_receives(conn);
}

void build_context_s(struct ibv_context *verbs)
{
  if (s_ctx) {
    if (s_ctx->ctx != verbs)
      die("cannot handle events in more than one context.");

    return;
  }

  s_ctx = (struct context *)malloc(sizeof(struct context));

  s_ctx->ctx = verbs;

  TEST_Z(s_ctx->pd = ibv_alloc_pd(s_ctx->ctx));
  TEST_Z(s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx));
  TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0)); /* cqe=10 is arbitrary */
  TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));

  TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq_s, NULL));
}



void send_memr(void * context, int type, struct ibv_mr *data, uint64_t size ) {

    struct connection *conn = (struct connection *)context;
    conn->send_msg->type = type;
    if (data != NULL) {
      memcpy(&conn->send_msg->data.mr, data, sizeof(struct ibv_mr));
    }

    conn->send_msg->size = size;
    struct ibv_send_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = (uintptr_t)conn;
    wr.opcode = IBV_WR_SEND;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    sge.addr = (uintptr_t)conn->send_msg;
    sge.length = sizeof(struct mr_message);
    sge.lkey = conn->send_mr->lkey;
  
    while (!conn->connected);

    TEST_NZ(ibv_post_send(conn->qp, &wr, &bad_wr));
    printf("Post Send: TYPE=%d\n", conn->send_msg->type);
}



void set_mode(enum mode m)
{
  s_mode = m;
}


void build_context(struct ibv_context *verbs)
{
  if (s_ctx) {
    if (s_ctx->ctx != verbs)
      die("cannot handle events in more than one context.");

    return;
  }

  s_ctx = (struct context *)malloc(sizeof(struct context));

  s_ctx->ctx = verbs;

  TEST_Z(s_ctx->pd = ibv_alloc_pd(s_ctx->ctx));
  TEST_Z(s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx));
  TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0)); /* cqe=10 is arbitrary */
  TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));

  TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL));
}

void build_params(struct rdma_conn_param *params)
{
  memset(params, 0, sizeof(*params));

  //  params->initiator_depth = params->responder_resources = 1;
  params->initiator_depth = params->responder_resources = 10;
  params->rnr_retry_count = 7; /* infinite retry */
}

void build_qp_attr(struct ibv_qp_init_attr *qp_attr)
{
  memset(qp_attr, 0, sizeof(*qp_attr));

  qp_attr->send_cq = s_ctx->cq;
  qp_attr->recv_cq = s_ctx->cq;
  qp_attr->qp_type = IBV_QPT_RC;

  qp_attr->cap.max_send_wr = 10;
  qp_attr->cap.max_recv_wr = 10;
  //  qp_attr->cap.max_send_sge = 1;
  //  qp_attr->cap.max_recv_sge = 1;
  qp_attr->cap.max_send_sge = 10;
  qp_attr->cap.max_recv_sge = 10;
}

void destroy_connection(void *context)
{
  struct connection *conn = (struct connection *)context;

  rdma_destroy_qp(conn->id);

  ibv_dereg_mr(conn->send_mr);
  ibv_dereg_mr(conn->recv_mr);
  ibv_dereg_mr(conn->rdma_msg_mr);

  free(conn->send_msg);
  free(conn->recv_msg);
  free(conn->rdma_msg_region);

  rdma_destroy_id(conn->id);

  free(conn);
}

void * get_local_message_region(void *context)
{
  return ((struct connection *)context)->rdma_msg_region;
}

char * get_peer_message_region(struct connection *conn)
{
  return conn->rdma_msg_region;
}

void on_completion(struct ibv_wc *wc)
{
  struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;

  if (wc->status != IBV_WC_SUCCESS)
    die("on_completion: status is not IBV_WC_SUCCESS.");

  if (wc->opcode == IBV_WC_RECV) {
    switch (conn->recv_msg->type)
    {
      int send_size=0;
    case MR_INIT_ACK: 
    case MR_CHUNK_ACK: 
      printf("Recived: Type=%d\n",  conn->recv_msg->type);
      if (tfile->tsize == tfile->size) {
      /*sent all data*/
	send_memr(conn, MR_FIN, NULL, tfile->size);
      } else {
      /*not sent all data yet*/
	if (tfile->tsize + RDMA_BUFFER_SIZE > tfile->size) {
	  send_size = tfile->size - tfile->tsize;
	} else {
	  send_size = RDMA_BUFFER_SIZE;
	}
	printf("Send size=%d\n", send_size);
	register_rdma_region(conn, tfile->send_base_addr, send_size);
	send_memr(conn, MR_CHUNK, conn->rdma_msg_mr, send_size);
	tfile->send_base_addr += send_size;
	tfile->tsize += send_size;
      }
      break;
    case MR_FIN_ACK: 
      printf("Recived: Type=%d\n",  conn->recv_msg->type);
      rdma_disconnect(conn->id);
      /*TODO: remove exit();*/
      exit(0);
      return;
    default: 
      printf("Unknown TYPE");
    }
    post_receives(conn);
    return;
  } else if (wc->opcode == IBV_WC_SEND) {
     printf("Send: TYPE=%d\n", conn->send_msg->type);
  } else {
     die("unknow opecode.");
  }
  return;
  /*============end ==================*/
}

static void register_rdma_region(struct connection *conn, void* addr, uint64_t size)
{
  if (conn->rdma_msg_mr != NULL) {
    ibv_dereg_mr(conn->rdma_msg_mr);
  }
  //  printf("Regist size=%d\n", size);
  TEST_Z(conn->rdma_msg_mr = ibv_reg_mr(
				    s_ctx->pd, 
				    addr, 
				    size, 
				    IBV_ACCESS_LOCAL_WRITE
				      | IBV_ACCESS_REMOTE_READ 
				      | IBV_ACCESS_REMOTE_WRITE));
  return;
}

void on_completion_s(struct ibv_wc *wc)
{

  struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;
  struct ibv_send_wr wr, *bad_wr = NULL;
  struct ibv_sge sge;

  if (wc->status != IBV_WC_SUCCESS)
    die("on_completion: status is not IBV_WC_SUCCESS.");

  if (wc->opcode == IBV_WC_RECV) {
    switch (conn->recv_msg->type)
      {
      case MR_INIT:
	printf("Recived: MR_INIT: Type=%d, size=%d\n", conn->recv_msg->type, conn->recv_msg->size);
	/* ===== RDMA (allocation of RDMA transfer file) ==========*/
	tfile = (struct transfer_file*)malloc(sizeof(struct transfer_file));
	conn-> rdma_msg_region = tfile->data = malloc(conn->recv_msg->size);
	tfile->send_base_addr = tfile->data;
	tfile->tsize = 0;
	tfile->size = conn->recv_msg->size;
	/* ============================================= ==========*/
        send_memr (conn, MR_INIT_ACK, NULL, NULL);
	break;
      case MR_CHUNK:

	printf("Recived: MR_CHUNK: Type=%d, size=%d\n", conn->recv_msg->type, conn->recv_msg->size);
	/*======= RDMA (read from remote memory region in client side)=======*/

	memcpy(&conn->peer_mr, &conn->recv_msg->data.mr, sizeof(conn->peer_mr));
	
	/*Remove below 6 lines*/
		free(tfile);
		tfile = (struct transfer_file*)malloc(sizeof(struct transfer_file));
		conn-> rdma_msg_region = tfile->data = malloc(conn->recv_msg->size);
		tfile->send_base_addr = tfile->data;
		tfile->tsize = 0;
		tfile->size = conn->recv_msg->size;
	/*Remove above 6 lines*/
	

	register_rdma_region(conn, tfile->send_base_addr, conn->recv_msg->size);

	/* !! memset must be called !!*/
	memset(&wr, 0, sizeof(wr));

	//	wr.wr_id = (uintptr_t)conn;
	wr.wr_id =  (uintptr_t)conn->rdma_msg_region;
	wr.opcode = IBV_WR_RDMA_READ;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;
	wr.wr.rdma.remote_addr = (uintptr_t)conn->peer_mr.addr;
	wr.wr.rdma.rkey = conn->peer_mr.rkey;

	sge.addr = (uintptr_t)conn->rdma_msg_region;
	sge.length = conn->recv_msg->size;
	sge.lkey = conn->rdma_msg_mr->lkey;
	
	/*
	int a;
	if((a = ibv_post_send(conn->qp, &wr, &bad_wr)) != 0) {
	  printf("%d: by post_send\n", a);
	}
	*/
	TEST_NZ(ibv_post_send(conn->qp, &wr, &bad_wr));
    
	printf("Post send: RDMA: id=%d\n", wr.wr_id);

	tfile->send_base_addr += conn->recv_msg->size;
	tfile->tsize += conn->recv_msg->size;

	/* ==================================================================*/
        send_memr (conn, MR_CHUNK_ACK, NULL, NULL);
	break;
      case MR_FIN:
	printf("Recived: MR_FIN: Type=%d, size=%d\n", conn->recv_msg->type, conn->recv_msg->size);
	//	printf("DATA:\n%s\n", tfile->data);
	ibv_dereg_mr(conn->rdma_msg_mr);
        send_memr(conn, MR_FIN_ACK, NULL, NULL);
	break;
      default:
        printf("Unknown TYPE");
	break;
      }
    post_receives(conn);
  } else if (wc->opcode == IBV_WC_SEND) {
    printf("Send: TYPE=%d\n", conn->send_msg->type);
  } else if (wc->opcode == IBV_WC_RDMA_READ) {
    printf("Send: RDMA: IBV_WC_RDMA_READ ");
  } else {
    die("unknow opecode.");
  }
  return;
}

void send_init(void *conn) {
  
  send_memr(conn, MR_INIT, NULL, NULL);
  post_receives(conn);
}

void on_connect(void *context)
{
  ((struct connection *)context)->connected = 1;
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



void * poll_cq_s(void *ctx)
{
  struct ibv_cq *cq;
  struct ibv_wc wc;

  

  while (1) {
    TEST_NZ(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
    ibv_ack_cq_events(cq, 1);
    TEST_NZ(ibv_req_notify_cq(cq, 0));
    printf("SIZE=====%d\n", sizeof(struct mr_message));

    while (ibv_poll_cq(cq, 1, &wc))
      on_completion_s(&wc);
  }

  return NULL;
}

void post_receives(struct connection *conn)
{
  struct ibv_recv_wr wr, *bad_wr = NULL;
  struct ibv_sge sge;

  wr.wr_id = (uintptr_t)conn;
  wr.next = NULL;
  wr.sg_list = &sge;
  wr.num_sge = 1;

  sge.addr = (uintptr_t)conn->recv_msg;
  sge.length = sizeof(struct mr_message);
  sge.lkey = conn->recv_mr->lkey;

  TEST_NZ(ibv_post_recv(conn->qp, &wr, &bad_wr));
  printf("Post Recive: id=%d\n", wr.wr_id);

}

void register_memory(struct connection *conn)
{
  conn->send_msg = malloc(sizeof(struct mr_message));
  conn->recv_msg = malloc(sizeof(struct mr_message));

  TEST_Z(conn->send_mr = ibv_reg_mr(
    s_ctx->pd, 
    conn->send_msg, 
    sizeof(struct mr_message), 
    IBV_ACCESS_LOCAL_WRITE));

  TEST_Z(conn->recv_mr = ibv_reg_mr(
    s_ctx->pd, 
    conn->recv_msg, 
    sizeof(struct mr_message), 
    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ));
}

