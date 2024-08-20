CC = gcc
CFLAGS = -std=gnu99 -pthread -Wall -g
BASE_SRC_C = src/xnet.c src/xnet_socket.c src/xnet_timeheap.c \
		src/xnet_util.c src/malloc_ref.c src/xnet_packer.c \
		src/xnet_string.c
SUFFIX=.exe
LUA_INC ?= 3rd/lua
LUA_STATICLIB := 3rd/lua/liblua.a

win : CFLAGS += -lws2_32
win : all
linux : all

alltest = test$(SUFFIX) test_packer$(SUFFIX) test_udp_client$(SUFFIX) \
		test_udp_server$(SUFFIX) test_client$(SUFFIX) test_server$(SUFFIX)

allexample = http_server$(SUFFIX) control_server$(SUFFIX)

all : $(allexample) $(alltest) xnet$(SUFFIX)

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

#main
xnet$(SUFFIX) : $(BASE_SRC_C) src/xnet_main.c src/xnet_config.c $(LUA_STATICLIB)
	$(CC) -o $@ $^ $(CFLAGS) -I$(LUA_INC) -lm -ldl

#example

http_server$(SUFFIX) : $(BASE_SRC_C) example/http_server.c
	$(CC) -o $@ $^ $(CFLAGS)

control_server$(SUFFIX) : $(BASE_SRC_C) example/control_server.c
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clear
clear :
	rm -rf *$(SUFFIX)
