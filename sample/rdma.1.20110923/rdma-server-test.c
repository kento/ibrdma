#include "rdma-server.h"


int main(int argc, char **argv) {
  struct RDMA_communicator comm;
  char *data;
  uint64_t size;
  int ctl_tag;

  RDMA_Passive_Init(&comm);
  while (1) {
    RDMA_Recvr(&data, &size, &ctl_tag, &comm);
    //printf("%d: size=%lu: %s\n", ctl_tag, size, data);
    printf("%d: size=%lu: \n", ctl_tag, size);
  }
  //  RDMA_show_buffer();  
  return 0;
}
