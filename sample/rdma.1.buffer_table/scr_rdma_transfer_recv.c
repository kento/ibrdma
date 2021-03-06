#include "scr_rdma_transfer.h"
#include "rdma-server.h"

void RDMA_transfer_recv(void);

int main(int argc, char **argv) {
  RDMA_transfer_recv();
  return 0;
}


void RDMA_transfer_recv(void)
{
  struct RDMA_communicator comm;
  char * data;
  uint64_t size;
  int tag;

  data=NULL;

  RDMA_Passive_Init(&comm);

  while (1) {
    RDMA_Recvr(&data, &size, &tag, &comm);
    if (data != NULL) {
      printf("%d: size=%lu\n", tag, size);
      //      free(data);
      data=NULL;
    }
    //    RDMA_show_buffer();
  }
  //  RDMA_show_buffer();
  return ;
}
