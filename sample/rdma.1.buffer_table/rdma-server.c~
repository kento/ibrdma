
#include "rdma-server.h"
#include "buffer_table.h"
#include <time.h>

/*
static int on_connect_request(struct rdma_cm_id *id);
static int on_connection(struct rdma_cm_id *id);
static int on_disconnect(struct rdma_cm_id *id);
static int on_event(struct rdma_cm_event *event);
*/

static void accept_connection(struct rdma_cm_id *id);
static void *poll_cq(void *ctx);
static void build_connection(struct rdma_cm_id *id);
static void build_context(struct ibv_context *verbs);
static void build_params(struct rdma_conn_param *params);
static void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
static void register_memory(struct connection *conn);
static void register_rdma_region(struct connection *conn,  void* addr, uint64_t size);
static void append_rdma_msg(uint64_t conn_id, struct RDMA_message *msg);
static void* passive_init(void * arg /*(struct RDMA_communicator *comm)*/) ;


//int RDMA_Passive_Init(struct RDMA_communicator *comm);
//int RDMA_Irecv(char* buff, int* size, int* tag, struct RDMA_communicator *comm);
//int RDMA_Passive_Finalize(struct RDMA_communicator *comm);

static struct context *s_ctx = NULL;
static int connections = 0;
pthread_t listen_thread;

/*
int main(int argc, char **argv) {
  struct RDMA_communicator comm;
  RDMA_Passive_Init(&comm);
  return 0;
}
*/

int RDMA_Irecvr(char** buff, uint64_t* size, int* tag, struct RDMA_communicator *comm)
{
  struct RDMA_message *rdma_msg;
  rdma_msg = get_current();
  *buff = rdma_msg->buff;
  *size = rdma_msg->size;
  *tag = rdma_msg->tag;
  return 0;
  return 0;
}

int RDMA_Recvr(char** buff, uint64_t* size, int* tag, struct RDMA_communicator *comm)
{
  struct RDMA_message *rdma_msg;
  while ((rdma_msg = get_current()) == NULL);
  *buff = rdma_msg->buff;
  *size = rdma_msg->size;
  *tag = rdma_msg->tag;
  return 0;
}


int RDMA_Passive_Init(struct RDMA_communicator *comm) 
{
  TEST_NZ(pthread_create(&listen_thread, NULL, (void *) passive_init, comm));
  return 0;
}


static void* passive_init(void * arg /*(struct RDMA_communicator *comm)*/)  
{
  struct RDMA_communicator *comm;
  struct sockaddr_in addr;
    struct rdma_cm_event *event = NULL;
  //  struct rdma_cm_id *listener = NULL;
  //  struct rdma_event_channel *ec = NULL;
  uint16_t port = 0;
  
  comm = (struct RDMA_communicator *) arg;
  create_hashtable(HASH_TABLE_LEN);

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(RDMA_PORT);

  TEST_Z(comm->ec = rdma_create_event_channel());
  TEST_NZ(rdma_create_id(comm->ec, &(comm->cm_id), NULL, RDMA_PS_TCP));
  TEST_NZ(rdma_bind_addr(comm->cm_id, (struct sockaddr *)&addr));
  TEST_NZ(rdma_listen(comm->cm_id, 10)); /* backlog=10 is arbitrary */
  port = ntohs(rdma_get_src_port(comm->cm_id));

  debug(printf("listening on port %d.\n", port), 1);

  while (1) {
    //    pthread_t          thread_id;
    //    pthread_attr_t     thread_attr;
    //    simple_context_t  *context = NULL;
    //    struct rdma_cm_id *id = NULL;
    int rc =0;
    debug(printf("Waiting for cm_event... "),1);
    if ((rc = rdma_get_cm_event(comm->ec, &event))){
      debug(printf("get event failed : %d\n", rc), 1);
      break;
    }
    debug(printf("\"%s\"\n", event_type_str(event->event)), 1);
    switch (event->event){
    case RDMA_CM_EVENT_CONNECT_REQUEST:
      accept_connection(event->id);
      break;
    case RDMA_CM_EVENT_ESTABLISHED:
      debug(printf("Establish: host_id=%lu\n", (uintptr_t)event->id), 1);
      
      //  on_connect(event->id->context);
      //      pthread_attr_init(&thread_attr);
      //      pthread_create(&thread_id,
      //                       &thread_attr,
      //                       handle_server_cq,
      //                       (void *)(event->id->context));
      break;
    case RDMA_CM_EVENT_DISCONNECTED:
      debug(printf("Disconnect from id : %p \n", event->id), 1);
      //      rdma_disconnect(comm->cm_id);
      //      rdma_destroy_qp(comm->cm_id);
      //      rdma_destroy_id(comm->cm_id);
      //        fprintf(stderr, "Disconnect from id : %p (total connections %d)\n",
      //         event->id, connections);
      //      context = (simple_context_t *)(event->id->context);
      //      id = event->id;
      break;
    default:
      break;
    }
    rdma_ack_cm_event(event);
    //    if (context){
    //    context->quit_cq_thread = 1;
    //    pthread_join(thread_id, NULL);
    //    rdma_destroy_id(id);
    //    free_connection(context);
    //    context = NULL;
    //    }
  }

  //  rdma_destroy_id(listener);
  //  rdma_destroy_event_channel(ec);


  return 0;
}

static void * poll_cq(void *ctx)
{  

  struct ibv_cq *cq;
  struct ibv_wc wc;
  struct ibv_send_wr wr, *bad_wr = NULL;
  struct ibv_sge sge;

  struct connection *conn;
  struct control_msg cmsg;
  struct RDMA_message *rdma_msg;

  char* buff;
  uint64_t buff_size;
  int tag;

  uint64_t mr_size;
  char* recv_base_addr;
  uint64_t recv_size;
  uint64_t conn_id;
  
  while (1) {
    TEST_NZ(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
    ibv_ack_cq_events(cq, 1);
    TEST_NZ(ibv_req_notify_cq(cq, 0));

    while (ibv_poll_cq(cq, 1, &wc)) {
      conn = (struct connection *)(uintptr_t)wc.wr_id;
      debug(printf("Control MSG from: %lu\n", (uintptr_t)conn->id), 1);
      if (wc.status != IBV_WC_SUCCESS) {
	die("on_completion: status is not IBV_WC_SUCCESS.");
      }
      if (wc.opcode == IBV_WC_RECV) {
	switch (conn->recv_msg->type)
	  {
	  case MR_INIT:
	    debug(printf("Recieved: MR_INT: Type=%u, buff size=%lu\n", conn->recv_msg->type, conn->recv_msg->data1.buff_size), 1);
	    /*Allocat buffer for the client msg*/ 
	    buff_size =conn->recv_msg->data1.buff_size;
	    conn-> rdma_msg_region = buff = (char *)malloc(conn->recv_msg->data1.buff_size);
	    recv_base_addr = buff;
	    recv_size  = 0;
	    mr_size= 0;
	    conn = (struct connection *)(uintptr_t)wc.wr_id;
	    conn_id = (uint64_t)conn->id;
	    /**/
	    cmsg.type=MR_INIT_ACK;
	    send_control_msg(conn, &cmsg);
	    break;
	  case MR_CHUNK:
	    debug(printf("Recived: MR_CHUNK: Type=%d, mr size=%lu\n", conn->recv_msg->type, conn->recv_msg->data1.mr_size), 1);
	    mr_size= conn->recv_msg->data1.mr_size;
	    memcpy(&conn->peer_mr, &conn->recv_msg->data.mr, sizeof(conn->peer_mr));
	    register_rdma_region(conn, recv_base_addr, conn->recv_msg->data1.mr_size);

	    /* !! memset must be called !!*/
	    memset(&wr, 0, sizeof(wr));

	    //      wr.wr_id = (uintptr_t)conn;
	    wr.wr_id =  (uintptr_t)conn->rdma_msg_region;
	    wr.opcode = IBV_WR_RDMA_READ;
	    wr.sg_list = &sge;
	    wr.num_sge = 1;
	    wr.send_flags = IBV_SEND_SIGNALED;
	    wr.wr.rdma.remote_addr = (uintptr_t)conn->peer_mr.addr;
	    wr.wr.rdma.rkey = conn->peer_mr.rkey;

	    //sge.addr = (uintptr_t)conn->rdma_msg_region;
	    sge.addr = (uintptr_t)recv_base_addr;
	    sge.length = mr_size;
	    sge.lkey = conn->rdma_msg_mr->lkey;

	    TEST_NZ(ibv_post_send(conn->qp, &wr, &bad_wr));
	    debug(printf("Post send: RDMA: id=%lu\n", wr.wr_id), 1);

	    recv_base_addr += mr_size;
	    recv_size += mr_size;
	    cmsg.type=MR_CHUNK_ACK;
	    send_control_msg(conn, &cmsg);
	    break;
	  case MR_FIN:
	    tag = conn->recv_msg->data1.tag;
	    debug(printf("RDMA transfer finished: Tag=%d\n",tag), 1);
	    cmsg.type=MR_FIN_ACK;
	    send_control_msg(conn, &cmsg);
	    /*Post reveived data*/
	    rdma_msg = (struct RDMA_message*) malloc(sizeof(struct RDMA_message));
	    rdma_msg->buff = buff;
	    rdma_msg->size = buff_size;
	    rdma_msg->tag = tag;
	    append_rdma_msg(conn_id, rdma_msg);
	    //	    rdma_disconnect(conn->id);
	    break;
	  default:
	    debug(printf("Unknown TYPE"), 1);
	    break;
	  }

	post_receives(conn);
      } else if (wc.opcode == IBV_WC_SEND) {
	debug(printf("Sent: Sent out: TYPE=%d\n", conn->send_msg->type), 1);
      } else if (wc.opcode == IBV_WC_RDMA_READ) {
	debug(printf("Sent: RDMA: IBV_WC_RDMA_READ "), 1);
      } else {
	die("unknow opecode.");
      }
    }
  }
    return NULL;
}


static void append_rdma_msg(uint64_t conn_id, struct RDMA_message *msg)
{
  append(conn_id, msg);
  return;
}

void RDMA_show_buffer(void)
 {
  show();
};

static void register_rdma_region(struct connection *conn,  void* addr, uint64_t size)
{
  if (conn->rdma_msg_mr != NULL) {
    ibv_dereg_mr(conn->rdma_msg_mr);
  }
                                                                                                                                                                                               
  TEST_Z(conn->rdma_msg_mr = ibv_reg_mr(
                                        s_ctx->pd,
                                        addr,
                                        size,
                                    IBV_ACCESS_LOCAL_WRITE
                                      | IBV_ACCESS_REMOTE_READ
                                        | IBV_ACCESS_REMOTE_WRITE));
  return;
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

  //  conn->send_state = SS_INIT;
  //  conn->recv_state = RS_INIT;

  conn->connected = 0;

  register_memory(conn);

}


static void build_context(struct ibv_context *verbs)
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

static void build_params(struct rdma_conn_param *params)
{
  memset(params, 0, sizeof(*params));

  params->initiator_depth = params->responder_resources = 1;
  params->rnr_retry_count = 7; /* infinite retry */
}

static void build_qp_attr(struct ibv_qp_init_attr *qp_attr)
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

void register_memory(struct connection *conn)
{
  conn->send_msg = malloc(sizeof(struct control_msg));
  conn->recv_msg = malloc(sizeof(struct control_msg));

  //  conn->rdma_local_region = malloc(RDMA_BUFFER_SIZE);
  //  conn->rdma_remote_region = malloc(RDMA_BUFFER_SIZE);
  //  conn->rdma_msg_region = malloc(RDMA_BUFFER_SIZE);

  TEST_Z(conn->send_mr = ibv_reg_mr(
				    s_ctx->pd, 
				    conn->send_msg, 
				    sizeof(struct control_msg), 
				    IBV_ACCESS_LOCAL_WRITE));

  TEST_Z(conn->recv_mr = ibv_reg_mr(
				    s_ctx->pd, 
				    conn->recv_msg, 
				    sizeof(struct control_msg), 
				    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ));

  //  TEST_Z(conn->rdma_msg_mr = ibv_reg_mr(
  //    s_ctx->pd, 
  //    conn->rdma_msg_region, 
  //    RDMA_BUFFER_SIZE, 
  //    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ));

  //    IBV_ACCESS_LOCAL_WRITE | ((s_mode == M_WRITE) ? IBV_ACCESS_REMOTE_WRITE : IBV_ACCESS_REMOTE_READ)));

  /*
    TEST_Z(conn->rdma_local_mr = ibv_reg_mr(
    s_ctx->pd, 
    conn->rdma_local_region, 
    RDMA_BUFFER_SIZE, 
    IBV_ACCESS_LOCAL_WRITE));


  TEST_Z(conn->rdma_remote_mr = ibv_reg_mr(
    s_ctx->pd, 
    conn->rdma_remote_region, 
    RDMA_BUFFER_SIZE, 
    IBV_ACCESS_LOCAL_WRITE | ((s_mode == M_WRITE) ? IBV_ACCESS_REMOTE_WRITE : IBV_ACCESS_REMOTE_READ)));
  */
}

static void accept_connection(struct rdma_cm_id *id)
{
  struct rdma_conn_param   conn_param;
  /*
  simple_context_t        *context = malloc(sizeof(*context));
  if (!context)
    {
       perror("failed to malloc context for connection\n");
        rdma_reject(id, NULL, 0);
        return;
      }

  // associate this context with this id. 
  context->id = id;
  id->context = context;

  context->quit_cq_thread = 0;

  if (allocate_server_resources(context))
    {
      fprintf(stderr, "failed to allocate resources\n");
      rdma_reject(id, NULL, 0);
      return;
    }

  post_server_rec_work_req(context);
  */
  debug(printf("Accepting connection on id == %p (total connections %d)\n", id, ++connections), 1);


  build_connection(id);

  //  memset(&conn_param, 0, sizeof(conn_param));
  //  conn_param.responder_resources = 1;
  //  conn_param.initiator_depth = 1;
  build_params(&conn_param);

  //  sprintf(get_local_message_region(id->context), "message from passive/server side with pid %d", getpid());
  //  rdma_accept(context->id, &conn_param);
  TEST_NZ(rdma_accept(id, &conn_param));


  post_receives(id->context);
  //if (query_qp_on_alloc)
  //    {
  //      debug_print_qp(context);
  //    }
}


int RDMA_Passive_Finalize(struct RDMA_communicator *comm)
{
  rdma_destroy_id(comm->cm_id);
  rdma_destroy_event_channel(comm->ec);
  return 0;
}
