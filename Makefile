all : test


test : test/test.c src/xnet_mq.c
	gcc -o $@ $^

xnet_main : src/xnet.c src/xnet_socket.c src/xnet_main.c
	gcc -o $@ $^ -std=gnu99 -lws2_32