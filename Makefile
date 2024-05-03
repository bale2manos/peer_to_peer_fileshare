CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g -I/usr/include/tirpc
LDFLAGS = -L. -Wl,-rpath,'$$ORIGIN' -ldl -ltirpc -lpthread

SRC_DIR = ./test
TEST_SOURCES = $(wildcard $(SRC_DIR)/*.c)
TEST_EXECUTABLES = $(patsubst $(SRC_DIR)/%.c, $(SRC_DIR)/%, $(TEST_SOURCES))

all: server $(TEST_EXECUTABLES)

server: servidor.o servidor_handle.o comm.o  servidor_rpc_clnt.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

$(SRC_DIR)/%: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

servidor.o: servidor.c servidor_rpc.h
	$(CC) $(CFLAGS) -c servidor.c -o servidor.o

clean:
	rm -f *.o *.so servidor cliente $(TEST_EXECUTABLES)

.PHONY: all clean servidor cliente
