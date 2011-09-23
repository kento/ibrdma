#include "rdma-common.h"

int RDMA_Active_Init(struct RDMA_communicator *comm, struct RDMA_param *param);
int RDMA_Sendr (char *buff, uint64_t size, int tag, struct RDMA_communicator *comm);
int RDMA_Isendr(char *buff, uint64_t size, int tag,  int *flag, struct RDMA_communicator *comm);
int RDMA_Wait(int *flag);
int RDMA_Active_Finalize(struct RDMA_communicator *comm);
