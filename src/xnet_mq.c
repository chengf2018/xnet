#include "xnet_mq.h"
#include <stdlib.h>

typedef struct xnet_mq_elem_s {
	xnet_message_t *message;
	struct xnet_mq_elem_s *next;
} xnet_mq_elem_t;


xnet_message_t *
xnet_newmessage(void *data, int size, int type) {
	xnet_message_t *new = (xnet_message_t *)malloc(sizeof(xnet_message_t));
	new->data = data;
	new->size = size;
	new->type = type;
	return new;
}

void
xnet_freemessage(xnet_message_t *message) {
	free(message);
}

void
xnet_mq_init(xnet_mq_t *mq) {
	mq->head = mq->tail = NULL;
	mq->count = 0;
}

void
xnet_mq_push(xnet_mq_t *mq, xnet_message_t *message) {
	xnet_mq_elem_t *new = (xnet_mq_elem_t *)malloc(sizeof(xnet_mq_elem_t));
	new->message = message;
	new->next = NULL;

	if (mq->head == NULL) {
		mq->head = mq->tail = new;
		return;
	}
	mq->count++;
	mq->tail->next = new;
	mq->tail = new;
}

xnet_message_t *
xnet_mq_pop(xnet_mq_t *mq) {
	xnet_mq_elem_t *mq_head;
	xnet_message_t *head;
	
	if (!mq->tail) return NULL;

	mq_head = mq->head;
	if (mq->head == mq->tail) {
		mq->head = mq->tail = NULL;
	} else {
		mq->head = mq->head->next;
	}
	head = mq_head->message;
	free(mq_head);
	mq->count--;
	return head;
}

xnet_message_t *
xnet_mq_peek(xnet_mq_t *mq) {
	if (!mq->tail) return NULL;
	return mq->tail->message;
}