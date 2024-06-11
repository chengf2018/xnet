CC = gcc
CFLAGS = -std=gnu99 -pthread
BASE_SRC_C = src/xnet.c src/xnet_socket.c src/xnet_timeheap.c \
		src/xnet_util.c src/malloc_ref.c src/xnet_packer.c \
		src/xnet_string.c
SUFFIX=.exe

win : CFLAGS += -lws2_32
win : all
linux : all

alltest = test$(SUFFIX) test_packer$(SUFFIX) test_udp_client$(SUFFIX) \
		test_udp_server$(SUFFIX) test_client$(SUFFIX) test_server$(SUFFIX)

allexample = http_server$(SUFFIX) control_server$(SUFFIX)

all : $(allexample) $(alltest)

#test

test_server$(SUFFIX) : $(BASE_SRC_C) test/test_server.c src/xnet_config.c
	$(CC) -o $@ $^ $(CFLAGS)

test_client$(SUFFIX) : $(BASE_SRC_C) test/test_client.c
	$(CC) -o $@ $^ $(CFLAGS)

test_udp_client$(SUFFIX) : $(BASE_SRC_C) test/test_udp_client.c
	$(CC) -o $@ $^ $(CFLAGS)

test_udp_server$(SUFFIX) : $(BASE_SRC_C) test/test_udp_server.c
	$(CC) -o $@ $^ $(CFLAGS)

test$(SUFFIX) : test/test.c src/xnet_timeheap.c src/xnet_config.c src/xnet_util.c
	$(CC) -o $@ $^ $(CFLAGS)

test_packer$(SUFFIX) : test/test_packer.c src/xnet_packer.c src/xnet_string.c
	$(CC) -o $@ $^ $(CFLAGS)

#example

http_server$(SUFFIX) : $(BASE_SRC_C) example/http_server.c
	$(CC) -o $@ $^ $(CFLAGS)

control_server$(SUFFIX) : $(BASE_SRC_C) example/control_server.c
	$(CC) -o $@ $^ $(CFLAGS)