#include "rdma-common.h"

#include <unistd.h>



void die(const char *reason)
{
  debug(printf("%s\n", reason), 1);
  exit(EXIT_FAILURE);
}

int get_pid(void)
 {
  return (int)getpid();
}


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

int send_control_msg (struct connection *conn, struct control_msg *cmsg)
{
  struct ibv_send_wr wr, *bad_wr = NULL;
  struct ibv_sge sge;

  //  struct connection *conn = (struct connection *)context;
  conn->send_msg->type = cmsg->type;
  //  memcpy(&conn->send_msg->data.mr, conn->rdma_msg_mr, sizeof(struct ibv_mr));
  memcpy(&conn->send_msg->data.mr, &cmsg->data.mr, sizeof(struct ibv_mr));
  conn->send_msg->data1 = cmsg->data1;

  memset(&wr, 0, sizeof(wr));

  wr.wr_id = (uintptr_t)conn;
  wr.opcode = IBV_WR_SEND;
  wr.sg_list = &sge;
  wr.num_sge = 1;
  wr.send_flags = IBV_SEND_SIGNALED;

  sge.addr = (uintptr_t)conn->send_msg;
  sge.length = sizeof(struct control_msg);
  sge.lkey = conn->send_mr->lkey;
  
  //  while (!conn->connected);

  TEST_NZ(ibv_post_send(conn->qp, &wr, &bad_wr));
  debug(printf("Post Ssend: TYPE=%d\n", conn->send_msg->type), 1);
  return 0;
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
  sge.length = sizeof(struct control_msg);
  sge.lkey = conn->recv_mr->lkey;

  TEST_NZ(ibv_post_recv(conn->qp, &wr, &bad_wr));
  debug(printf("Post Recive: id=%lu\n", wr.wr_id), 1);
}
