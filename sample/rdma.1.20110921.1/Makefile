.PHONY: clean

#CFLAGS  := -Wall -Werror -g
CFLAGS  := -Wall -g
LD      := gcc
LDFLAGS := ${LDFLAGS} -lrdmacm 

#APPS    := rdma-client rdma-server buffer_table_test rdma-client-test rdma-server-test
APPS    := clean  rdma-client-test rdma-server-test

all: ${APPS}

rdma-client-test:rdma-common.o rdma-client.o rdma-client-test.o
	${LD} -o $@ $^ ${LDFLAGS}

rdma-server-test:rdma-common.o buffer_table.o rdma-server.o rdma-server-test.o
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

