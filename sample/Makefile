.PHONY: clean

#CFLAGS  := -Wall -Werror -g
CFLAGS  := -Wall -g
LD      := gcc
LDFLAGS := ${LDFLAGS} -lrdmacm 

#APPS    := rdma-client rdma-server buffer_table_test rdma-client-test rdma-server-test
APPS    := clean  scr_rdma_transfer_send


all: ${APPS}

scr_rdma_transfer_send:./rdma.1/rdma-common.o ./rdma.1/rdma-client.o ./rdma.1/scr_rdma_transfer_send.o ./rdma.1/common.o
	${LD} -o $@ $^ ${LDFLAGS} 



clean:
	rm -f *.o ${APPS}


# -----
rdma-client: rdma-common.o rdma-client.o
	${LD} -o $@ $^ ${LDFLAGS}

rdma-server: buffer_table.o rdma-common.o rdma-server.o
	${LD} -o $@ $^ ${LDFLAGS}

buffer_table_test: buffer_table.o buffer_table_test.c
	${LD} -o $@ $^ ${LDFLAGS}

