#ifndef _XNET_MQ_H_
#define _XNET_MQ_H_

typedef struct {
	void *data;
	int size;
	int type;
} xnet_message_t;

struct xnet_mq_elem_s;

typedef struct {
	struct xnet_mq_elem_s *head;
	struct xnet_mq_elem_s *tail;
	int count;
} xnet_mq_t;

xnet_message_t *xnet_newmessage(void *data, int size, int type);
void xnet_freemessage(xnet_message_t *message);

void xnet_mq_init(xnet_mq_t *mq);
void xnet_mq_push(xnet_mq_t *mq, xnet_message_t *message);
xnet_message_t *xnet_mq_pop(xnet_mq_t *mq);
xnet_message_t *xnet_mq_peek(xnet_mq_t *mq);

#endif //_XNET_MQ_H_