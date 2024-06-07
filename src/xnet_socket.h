#ifndef _XNET_SOCKET_H_
#define _XNET_SOCKET_H_

#include <stdint.h>
#include <stdbool.h>

#define POLL_EVENT_MAX 64

#ifdef _WIN32
    //windows head
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define SOCKET_TYPE SOCKET

    typedef int socklen_t;
#else
    //linux head
    #include <sys/socket.h>
    #include <sys/epoll.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <netdb.h>

    #define SOCKET_TYPE int
#endif

#define MIN_READ_SIZE 512
#define MAX_CLIENT_NUM 65536
#define MAX_UDP_PACKAGE 65535

//socket type:
#define SOCKET_TYPE_INVALID 0
#define SOCKET_TYPE_LISTENING 1
#define SOCKET_TYPE_ACCEPTED 2
#define SOCKET_TYPE_CONNECTING 3
#define SOCKET_TYPE_CONNECTED 4

//protocol type:
#define SOCKET_PROTOCOL_TCP 1
#define SOCKET_PROTOCOL_UDP 2
#define SOCKET_PROTOCOL_UDP_IPV6 3

//addr type:
#define SOCKET_ADDR_TYPE_IPV4 1
#define SOCKET_ADDR_TYPE_IPV6 2

union sockaddr_all {
    struct sockaddr s;
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
};

typedef struct {
    uint8_t type;//ipv4 ipv6
    uint16_t port;
    uint8_t addr[16];
} xnet_addr_t;

typedef struct write_buffer {
    struct write_buffer *next;
    char *buffer;
    char *ptr;
    int sz;
} xnet_write_buff_t;

typedef struct {
    xnet_write_buff_t wb;
    xnet_addr_t udp_addr;
} xnet_udp_wirte_buff_t;

typedef struct {
    xnet_write_buff_t *head;
    xnet_write_buff_t *tail;
} xnet_wb_list_t;

typedef struct {
	SOCKET_TYPE fd;
	int type;
	int id;
    int read_size;
    uint8_t protocol;//1tcp 2udp 3udp_v6
    bool reading;
    bool writing;
    bool closing;

    xnet_wb_list_t wb_list;
    int64_t wb_size;
    xnet_addr_t addr_info;

    //保留给用户
    void *unpacker;
    void *user_ptr;
} xnet_socket_t;

typedef struct {
    xnet_socket_t *s[POLL_EVENT_MAX];
    bool read[POLL_EVENT_MAX];
    bool write[POLL_EVENT_MAX];
    bool error[POLL_EVENT_MAX];
#ifndef _WIN32
    bool eof[POLL_EVENT_MAX];
#endif
    int n;
} xnet_poll_event_t;

typedef struct dlink_node {
    int sock_id;
    SOCKET_TYPE fd;
    struct dlink_node *last;
    struct dlink_node *next;
} dlink_node_t;

typedef struct {
#ifdef _WIN32
    fd_set readfds;
    fd_set writefds;
    fd_set errorfds;

    dlink_node_t *socket_list;
    /*todo：select模型需要记录socket列表，检测socket触发只能遍历整个socket列表*/
#else
    SOCKET_TYPE epoll_fd;
    struct epoll_event event[POLL_EVENT_MAX];
#endif
    xnet_socket_t slots[MAX_CLIENT_NUM];
    int slot_index;
    xnet_poll_event_t poll_event;

    //其他线程的操作通过管道发送异步进行
	SOCKET_TYPE recv_fd;
	SOCKET_TYPE send_fd;
    fd_set rfds;

    char udp_buffer[MAX_UDP_PACKAGE];
} xnet_poll_t;


int get_last_error();
int xnet_socket_init();
int xnet_socket_deinit();
int xnet_poll_init(xnet_poll_t *poll);
int xnet_poll_deinit(xnet_poll_t *poll);
int xnet_poll_addfd(xnet_poll_t *poll, SOCKET_TYPE fd, int id);
int xnet_poll_closefd(xnet_poll_t *poll, xnet_socket_t *s);
int xnet_poll_wait(xnet_poll_t *poll, int timeout);//进行io等待，触发后返回触发的socket列表，保存在poll->event中

int xnet_enable_read(xnet_poll_t *poll, xnet_socket_t *s, bool enable);
int xnet_enable_write(xnet_poll_t *poll, xnet_socket_t *s, bool enable);

int xnet_recv_data(xnet_poll_t *poll, xnet_socket_t *s, char **out_data);
int xnet_recv_udp_data(xnet_poll_t *poll, xnet_socket_t *s, xnet_addr_t *addr_out);
int xnet_send_data(xnet_poll_t *poll, xnet_socket_t *s);

int xnet_listen_tcp_socket(xnet_poll_t *poll, const char *host, int port, int backlog, xnet_socket_t **socket_out);
int xnet_accept_tcp_socket(xnet_poll_t *poll, xnet_socket_t *listen_s, xnet_socket_t **socket_out);
int xnet_connect_tcp_socket(xnet_poll_t *poll, const char *host, int port, xnet_socket_t **socket_out);

int xnet_listen_udp_socket(xnet_poll_t *poll, const char *host, int port, xnet_socket_t **socket_out);
int xnet_create_udp_socket(xnet_poll_t *poll, int protocol, xnet_socket_t **socket_out);
int xnet_set_udp_socket_addr(xnet_poll_t *poll, xnet_socket_t *s, const char *host, int port);
int xnet_udp_connect(xnet_poll_t *poll, xnet_socket_t * s, const char *host, int port);

void xnet_addrtoa(xnet_addr_t *addr, char str[64]);

//以下接口不希望对外提供
bool wb_list_empty(xnet_socket_t *s);
void set_nonblocking(SOCKET_TYPE fd);
void set_keepalive(SOCKET_TYPE fd);
void append_send_buff(xnet_poll_t *poll, xnet_socket_t *s, const char *buffer, int sz);
void append_udp_send_buff(xnet_poll_t *poll, xnet_socket_t *s, xnet_addr_t *addr, const char *buffer, int sz);
void block_recv(SOCKET_TYPE fd, void *buffer, int sz);
void block_send(SOCKET_TYPE fd, void *buffer, int sz);
int get_sockopt(SOCKET_TYPE fd, int level, int optname, int *optval, int *optlen);


#endif //_XNET_SOCKET_H_