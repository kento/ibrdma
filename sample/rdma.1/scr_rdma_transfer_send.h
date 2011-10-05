
int RDMA_transfer_init(void);
int RDMA_file_transfer(char* src, char* dst, int buf_size, int count);


struct RDMA_tf {
  char* src;
  char* dst;
};
