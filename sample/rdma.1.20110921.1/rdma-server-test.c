#include "rdma-server.h"


int main(int argc, char **argv) {
  struct RDMA_communicator comm;
  char * data;
  uint64_t size;
  int tag;

  data=NULL;

  RDMA_Passive_Init(&comm);

  while (1) {
    RDMA_Recvr(&data, &size, &tag, &comm);
    if (data != NULL) {
      printf("==%d\n", tag);
      free(data);
      data=NULL;
    }
    //    RDMA_show_buffer();
  }

  exit(0);
  RDMA_show_buffer();
  return 0;
}
