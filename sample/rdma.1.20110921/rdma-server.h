#include "rdma-common.h"

int RDMA_Passive_Init(struct RDMA_communicator *comm);
int RDMA_Irecv(char* buff, int* size, int* tag, struct RDMA_communicator *comm);
int RDMA_Recvw(char* buff, int* size, int* tag, struct RDMA_communicator *comm);
int RDMA_Passive_Finalize(struct RDMA_communicator *comm);
