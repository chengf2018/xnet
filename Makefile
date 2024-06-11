CFLAGS = -std=gnu99
BASE_SRC_C = src/xnet.c src/xnet_socket.c src/xnet_timeheap.c \
		src/xnet_util.c src/malloc_ref.c src/xnet_packer.c \
		src/xnet_string.c

alltest : test test_packer test_udp_client test_udp_server \
		test_client test_server

allexample : http_server

win : CFLAGS += -pthread -lws2_32
win : allexample alltest
linux : CFLAGS += -pthread
linux : allexample alltest

test_server : $(BASE_SRC_C) test/test_server.c src/xnet_config.c
	gcc -o $@ $^ $(CFLAGS)

test_client : $(BASE_SRC_C) test/test_client.c
	gcc -o $@ $^ $(CFLAGS)

test_udp_client : $(BASE_SRC_C) test/test_udp_client.c
	gcc -o $@ $^ $(CFLAGS)

test_udp_server : $(BASE_SRC_C) test/test_udp_server.c
	gcc -o $@ $^ $(CFLAGS)

test : test/test.c src/xnet_timeheap.c src/xnet_config.c src/xnet_util.c
	gcc -o $@ $^ $(CFLAGS)

test_packer : test/test_packer.c src/xnet_packer.c src/xnet_string.c
	gcc -o $@ $^ $(CFLAGS)

http_server : $(BASE_SRC_C) example/http_server.c
	gcc -o $@ $^ $(CFLAGS)