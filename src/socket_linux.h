#ifndef _SOCKET_LINUX_H_
#define _SOCKET_LINUX_H_

#define closesocket close
#define XNET_EINTR EINTR

#if (EAGAIN != EWOULDBLOCK)
	#define XNET_HAVR_WOULDBLOCK(err) ((err) == EWOULDBLOCK || (err) == EAGAIN)
#else
	#define XNET_HAVR_WOULDBLOCK(err) ((err) == EWOULDBLOCK)
#endif

static int
poll_add(SOCKET_TYPE efd, SOCKET_TYPE fd, void *ud) {
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.ptr = ud;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		return 1;
	}
	return 0;
}

static void 
poll_del(SOCKET_TYPE efd, SOCKET_TYPE fd) {
	epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
}

static int
poll_wait(xnet_poll_t *poll, int timeout) {
    struct epoll_event ev[POLL_EVENT_MAX];
    int i;
    xnet_poll_event_t *poll_event = &poll->poll_event;
	int n = epoll_wait(poll->epoll_fd, ev, POLL_EVENT_MAX, timeout);
	
	for (i=0;i<n;i++) {
		poll_event->s[i] = ev[i].data.ptr;
		unsigned flag = ev[i].events;
		poll_event->write[i] = (flag & EPOLLOUT) != 0;
		poll_event->read[i] = (flag & EPOLLIN) != 0;
		poll_event->error[i] = (flag & EPOLLERR) != 0;
		poll_event->eof[i] = (flag & EPOLLHUP) != 0;

	}

	poll_event->n = n;
	return n;
}

static int
poll_enable_write(xnet_poll_t *poll, xnet_socket_t *s, bool enable) {
    struct epoll_event ev;

    if (s->writing == enable) return 0;
    s->writing = enable;

	ev.events = (s->reading ? EPOLLIN : 0) | (enable ? EPOLLOUT : 0);
	ev.data.ptr = s;
printf("socket [%d] poll_enable_write, [%d,%d]\n", s->id, s->reading, enable);
	if (epoll_ctl(poll->epoll_fd, EPOLL_CTL_MOD, s->fd, &ev) == -1) {
		return 1;
	}
    return 0;
}

static int
poll_enable_read(xnet_poll_t *poll, xnet_socket_t *s, bool enable) {
	struct epoll_event ev;

	if (s->reading == enable) return 0;
	s->reading = enable;    
	ev.events = (enable ? EPOLLIN : 0) | (s->writing ? EPOLLOUT : 0);
	ev.data.ptr = s;
printf("socket [%d] poll_enable_read, [%d,%d]\n", s->id, enable, s->writing);
	if (epoll_ctl(poll->epoll_fd, EPOLL_CTL_MOD, s->fd, &ev) == -1) {
		return 1;
	}
    return 0;
}

static void
poll_set_nonblocking(SOCKET_TYPE fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if ( -1 == flag )
        return;
    if (!(flag & O_NONBLOCK))
        fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

#endif //_SOCKET_LINUX_H_