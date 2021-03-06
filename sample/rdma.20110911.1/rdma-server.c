#include "rdma-common.h"
#include <time.h>

/*
static int on_connect_request(struct rdma_cm_id *id);
static int on_connection(struct rdma_cm_id *id);
static int on_disconnect(struct rdma_cm_id *id);
static int on_event(struct rdma_cm_event *event);
*/
static void usage(const char *argv0);

static void accept_connection(struct rdma_cm_id *id);

int main(int argc, char **argv)
{
  struct sockaddr_in addr;
  struct rdma_cm_event *event = NULL;
  struct rdma_cm_id *listener = NULL;
  struct rdma_event_channel *ec = NULL;
  uint16_t port = 0;

  if (argc != 2)
    usage(argv[0]);

  if (strcmp(argv[1], "write") == 0)
    set_mode(M_WRITE);
  else if (strcmp(argv[1], "read") == 0)
    set_mode(M_READ);
  else
    usage(argv[0]);

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;

  TEST_Z(ec = rdma_create_event_channel());
  TEST_NZ(rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP));
  TEST_NZ(rdma_bind_addr(listener, (struct sockaddr *)&addr));
  TEST_NZ(rdma_listen(listener, 10)); /* backlog=10 is arbitrary */

  port = ntohs(rdma_get_src_port(listener));

  printf("listening on port %d.\n", port);
  

  while (1) {
    //    pthread_t          thread_id;
    //    pthread_attr_t     thread_attr;
    //    simple_context_t  *context = NULL;
    //    struct rdma_cm_id *id = NULL;
    int rc =0;
    fprintf(stderr, "Waiting for cm_event... ");
    if ((rc = rdma_get_cm_event(ec, &event))){
      fprintf(stderr, "get event failed : %d\n", rc);
      break;
    }
    fprintf(stderr, "\"%s\"\n", event_type_str(event->event));
    switch (event->event){
      case RDMA_CM_EVENT_CONNECT_REQUEST:
	accept_connection(event->id);
	break;
      case RDMA_CM_EVENT_ESTABLISHED:
	//	pthread_attr_init(&thread_attr);
	//	pthread_create(&thread_id,
	//			 &thread_attr,
	//			 handle_server_cq,
	//			 (void *)(event->id->context));
	break;
    case RDMA_CM_EVENT_DISCONNECTED:
      fprintf(stderr, "Disconnect from id : %p \n", event->id);
      //	fprintf(stderr, "Disconnect from id : %p (total connections %d)\n",
      //	 event->id, connections);
	//	context = (simple_context_t *)(event->id->context);
	//	id = event->id;
	break;
      default:
	break;
    }
    rdma_ack_cm_event(event);
    //    if (context){
    //	  context->quit_cq_thread = 1;
    //	  pthread_join(thread_id, NULL);
    //	  rdma_destroy_id(id);
    //	  free_connection(context);
    //	  context = NULL;
    //    }
  }

  rdma_destroy_id(listener);
  rdma_destroy_event_channel(ec);

  return 0;


  /*
  while (rdma_get_cm_event(ec, &event) == 0) {
    struct rdma_cm_event event_copy;

    memcpy(&event_copy, event, sizeof(*event));
    rdma_ack_cm_event(event);

    if (on_event(&event_copy))
      break;
  }

  rdma_destroy_id(listener);
  rdma_destroy_event_channel(ec);

  return 0;
  */
}

/*************************************************************************
 * Accept the connection from the client.  This includes allocating a qp and
 * other resources to talk to.
 */
static int connections = 0;
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
  printf("Accepting connection on id == %p (total connections %d)\n",
	 id, ++connections);

  build_connection(id);

  //  memset(&conn_param, 0, sizeof(conn_param));
  //  conn_param.responder_resources = 1;
  //  conn_param.initiator_depth = 1;
  build_params(&conn_param);

  sprintf(get_local_message_region(id->context), "message from passive/server side with pid %d", getpid());
  //  rdma_accept(context->id, &conn_param);
  TEST_NZ(rdma_accept(id, &conn_param));


  //if (query_qp_on_alloc)
  //    {
  //      debug_print_qp(context);
  //    }
}

/*
int on_connect_request(struct rdma_cm_id *id)
{
  struct rdma_conn_param cm_params;

  printf("received connection request.\n");
  build_connection(id);
  build_params(&cm_params);
  sprintf(get_local_message_region(id->context), "message from passive/server side with pid %d", getpid());
  TEST_NZ(rdma_accept(id, &cm_params));

  return 0;
}


int on_connection(struct rdma_cm_id *id)
{
  on_connect(id->context);

  return 0;
}

int on_disconnect(struct rdma_cm_id *id)
{
  printf("peer disconnected.\n");

  destroy_connection(id->context);
  return 0;
}

int on_event(struct rdma_cm_event *event)
{
  int r = 0;
  printf("Got Event: %s\n", event_type_str(event->event));
  if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST) {
    r = on_connect_request(event->id);
  } else if (event->event == RDMA_CM_EVENT_ESTABLISHED) {
    r = on_connection(event->id);
  } else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
    r = on_disconnect(event->id);
  else
    die("on_event: unknown event.");

  return r;
}
*/

void usage(const char *argv0)
{
  fprintf(stderr, "usage: %s <mode>\n  mode = \"read\", \"write\"\n", argv0);
  exit(1);
}
