.PHONY: clean

#CFLAGS  := -Wall -Werror -g
CFLAGS  := -Wall -g
LD      := gcc
LDFLAGS := ${LDFLAGS} -lrdmacm 

#APPS    := rdma-client rdma-server buffer_table_test rdma-client-test rdma-server-test
APPS    := clean  scr_rdma_transfer_send  scr_rdma_transfer_recv rdma-client-test rdma-server-test

all: ${APPS}

scr_rdma_transfer_send:rdma-common.o rdma-client.o scr_rdma_transfer_send.o common.o
	${LD} -o $@ $^ ${LDFLAGS}

scr_rdma_transfer_recv:rdma-common.o buffer_table.o rdma-server.o scr_rdma_transfer_recv.o  common.o
	${LD} -o $@ $^ ${LDFLAGS}

rdma-client-test:rdma-common.o rdma-client.o rdma-client-test.o  common.o
	${LD} -o $@ $^ ${LDFLAGS}

rdma-server-test:rdma-common.o buffer_table.o rdma-server.o rdma-server-test.o  common.o
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

