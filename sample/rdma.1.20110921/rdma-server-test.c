#include "rdma-server.h"


int main(int argc, char **argv) {
  struct RDMA_communicator comm;
  RDMA_Passive_Init(&comm);
  return 0;
}
