CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g -fPIC
LDFLAGS = -L. -Wl,-rpath,'$$ORIGIN' -lclaves

SRC_DIR = ./test
TEST_SOURCES = $(wildcard $(SRC_DIR)/*.c)
TEST_EXECUTABLES = $(patsubst $(SRC_DIR)/%.c, $(SRC_DIR)/%, $(TEST_SOURCES))

all: servidor libclaves.so cliente $(TEST_EXECUTABLES)

servidor: servidor.o servidor_handle.o comm.o
	$(CC) $(CFLAGS) $^ -o $@

libclaves.so: claves.o comm.o
	$(CC) -shared -o $@ $^

cliente: cliente.o
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(SRC_DIR)/%: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f *.o *.so servidor cliente $(TEST_EXECUTABLES)

.PHONY: all clean servidor cliente
