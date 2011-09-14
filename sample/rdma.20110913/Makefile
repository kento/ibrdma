.PHONY: clean

#CFLAGS  := -Wall -Werror -g
CFLAGS  := -Wall -g
LD      := gcc
LDFLAGS := ${LDFLAGS} -lrdmacm

APPS    := rdma-client rdma-server

all: ${APPS}

rdma-client: rdma-common-client.o rdma-client.o
	${LD} -o $@ $^ ${LDFLAGS}

rdma-server: rdma-common-client.o rdma-server.o
	${LD} -o $@ $^ ${LDFLAGS}

clean:
	rm -f *.o ${APPS}

