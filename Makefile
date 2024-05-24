BASE_SRC_C = src/xnet.c src/xnet_socket.c src/util_time.c src/xnet_timeheap.c\
		src/util.c src/malloc_ref.c
SERVER_SRC_C = $(BASE_SRC_C) src/xnet_main.c src/xnet_config.c
CLIENT_SRC_C = $(BASE_SRC_C) test/test_client.c
TEST_SRC_C = test/test.c src/xnet_timeheap.c src/xnet_config.c src/util.c

TEST_UDP_CLIENT_SRC_C = $(BASE_SRC_C) test/test_udp_client.c
TEST_UDP_SERVER_SRC_C = $(BASE_SRC_C) test/test_udp_server.c

all : test

test : $(TEST_SRC_C)
	gcc -o $@ $^ -std=gnu99

win : xnet_main.exe client.exe test_udp_client.exe test_udp_server.exe
linux : xnet_main client test_udp_client test_udp_server

xnet_main.exe : $(SERVER_SRC_C)
	gcc -o $@ $^ -std=gnu99 -lws2_32 -pthread

xnet_main : $(SERVER_SRC_C)
	gcc -o $@ $^ -std=gnu99 -pthread

client.exe : $(CLIENT_SRC_C)
	gcc -o $@ $^ -std=gnu99 -lws2_32 -pthread

client : $(CLIENT_SRC_C)
	gcc -o $@ $^ -std=gnu99 -pthread

test_udp_client.exe : $(TEST_UDP_CLIENT_SRC_C)
	gcc -o $@ $^ -std=gnu99 -lws2_32 -pthread

test_udp_server.exe : $(TEST_UDP_SERVER_SRC_C)
	gcc -o $@ $^ -std=gnu99 -lws2_32 -pthread

test_udp_client : $(TEST_UDP_CLIENT_SRC_C)
	gcc -o $@ $^ -std=gnu99 -pthread

test_udp_server : $(TEST_UDP_SERVER_SRC_C)
	gcc -o $@ $^ -std=gnu99 -pthread