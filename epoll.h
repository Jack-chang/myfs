#ifndef __EPOLL_H__
#define __EPOLL_H__


extern void add_event(conn_t* conn);
extern void del_event(conn_t* conn);

extern conn_t* active_connect(int id);

#endif
