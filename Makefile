SERVER_SRC_C = src/xnet.c src/xnet_socket.c src/xnet_main.c src/util_time.c \
		src/xnet_timeheap.c src/xnet_config.c
CLIENT_SRC_C = src/xnet.c src/xnet_socket.c src/util_time.c src/xnet_timeheap.c\
		test/test_client.c
TEST_SRC_C = test/test.c src/xnet_timeheap.c src/xnet_config.c

all : test

test : $(TEST_SRC_C)
	gcc -o $@ $^ -std=gnu99

win : xnet_main.exe client.exe
linux : xnet_main client

xnet_main.exe : $(SERVER_SRC_C)
	gcc -o $@ $^ -std=gnu99 -lws2_32 -pthread

xnet_main : $(SERVER_SRC_C)
	gcc -o $@ $^ -std=gnu99 -pthread

client.exe : $(CLIENT_SRC_C)
	gcc -o $@ $^ -std=gnu99 -lws2_32 -pthread

client : $(CLIENT_SRC_C)
	gcc -o $@ $^ -std=gnu99 -pthread