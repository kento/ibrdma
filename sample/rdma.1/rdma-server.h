#include "rdma-common.h"


//int alloc_size = 0;



int RDMA_Passive_Init(struct RDMA_communicator *comm);
int RDMA_Irecvr(char** buff, uint64_t* size, int* tag, struct RDMA_communicator *comm);
int RDMA_Recvr(char** buff, uint64_t* size, int* tag, struct RDMA_communicator *comm);
int RDMA_Passive_Finalize(struct RDMA_communicator *comm);
void RDMA_show_buffer(void);
