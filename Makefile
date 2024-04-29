SRC_C = src/xnet.c src/xnet_socket.c src/xnet_main.c src/util_time.c \
		src/xnet_timeheap.c

all : test


test : test/test.c src/xnet_mq.c src/xnet_timeheap.c
	gcc -o $@ $^ -std=gnu99
win : xnet_main.exe
linux : xnet_main
xnet_main.exe : $(SRC_C)
	gcc -o $@ $^ -std=gnu99 -lws2_32

xnet_main : $(SRC_C)
	gcc -o $@ $^ -std=gnu99