#include "rdma-client.h"
#include <sys/time.h>
#include <stdio.h>

double get_dtime(void);

int main(int argc, char **argv)
{
  char* host;
  char* data;
  uint64_t size;
  //  int flag1, flag2;
  int flag1;
  double s,e;

  host = argv[1];
  size = atoi(argv[2]);
  
  struct  RDMA_communicator comm;
  struct  RDMA_param param;
  param.host = host;

  s = get_dtime();
  RDMA_Active_Init(&comm, &param);
  e = get_dtime();

  /* ===== */
  data = (char*)malloc(size);
  int i;
  flag1 = 0;
  for (i=size-2; i >= 0; i--) {
    data[i] = (char) (i % 26 + 'a');
  }
  data[size-1] += '\0';
  printf("%f\n",e - s);
  

  s = get_dtime();
  RDMA_Isendr(data, size, 1015, &flag1, &comm);
  /*=======*/
  //  data = (char*)malloc(size);
  //  flag2 = 0;
  //  for (i=size-2; i >= 0; i--) {
  //    data[i] = (char) (i % 26 + 'a');
  //  }
  //  data[size-1] += '\0';
  //  RDMA_Isendr(data, size, 1015, &flag2, &comm);
  /* ===== */
  RDMA_Wait (&flag1);
  e = get_dtime();
  //  RDMA_Wait (&flag2) ;
  printf("%f\n", e - s);
  return 0;
  //  RDMA_Active_Finalize(&comm);

}

double get_dtime(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
}


