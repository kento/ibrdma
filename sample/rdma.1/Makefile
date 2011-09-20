.PHONY: clean

#CFLAGS  := -Wall -Werror -g
CFLAGS  := -Wall -g
LD      := gcc
LDFLAGS := ${LDFLAGS} -lrdmacm

APPS    := rdma-client rdma-server

all: ${APPS}

rdma-client: rdma-common.o rdma-client.o
	${LD} -o $@ $^ ${LDFLAGS}

rdma-server: hashtable.o rdma-common.o rdma-server.o
	${LD} -o $@ $^ ${LDFLAGS}

clean:
	rm -f *.o ${APPS}

