.PHONY: clean

#CFLAGS  := -Wall -Werror -g
CFLAGS  := -Wall -g
LD      := gcc
LDFLAGS := ${LDFLAGS} -lrdmacm

APPS    := ibrdma-server

all: ${APPS}

#ibrdma-client: rdma-common.o rdma-client.o
#	${LD} -o $@ $^ ${LDFLAGS}

ibrdma-server: ibrdma-server.o
	${LD} -o $@ $< ${LDFLAGS}

clean:
	rm -f *.o ${APPS}

