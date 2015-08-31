#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "conn.h"
#include "interface.h"
#include "debug.h"
#include "help.h"
#include  "error.h"

extern ctx_t g_ctx;


msg_t*  find_ack(conn_t* conn, msg_t* ack){
	queue_t* q = &conn->q_ack;
	struct list_head* head = &q->head;
	struct list_head* pos;
	struct list_head* tmp;
	msg_t* msg;
	
	list_for_each_safe(pos,tmp ,head ){
		msg = list_entry(pos, msg_t, link);
		if(msg->hr.id == ack->hr.id && msg->hr.to == ack->hr.from){
			list_del_init(pos);
			break;
		}
	}

	return msg;
}

int handle_ack_msg(conn_t* conn, msg_t* ack){
			msg_t* msg = find_ack(conn, ack);
			sem_post(&msg->sem_send);
			free_msg(ack);
}
int recv_callback(conn_t* conn, msg_t* msg){
	switch(msg->hr.type ){
		case ACK_MSG :
			handle_ack_msg(conn, msg);
			break;
		case REQ_MSG:
			enque(&g_ctx.recv_msg_q, &msg->link);
			break;
		default:
			debug_info("wrong msg type!\n");
			break;
	}
	       
}


/*
 * rval.err ==0 , send success, rval.v == buflen
 * rval.err == EAGAIN, rval.v is len that has send
 * rval.err == other ,send failed, rval.v is len that has send 
 */


rval_t recv_buf(conn_t* conn, char* buf, int buflen){
		rval_t rval; 
		int  recv_len = 0;

		rval.err = 0; 
		assert(buflen >0);

		while(recv_len  < buflen){
				rval.v = recv(conn->sockfd, buf, buflen-recv_len, 0);
				if(rval.v ==-1){
						rval.err = errno;
						if(rval.err == EINTR){
							debug_warn("recv fun get EINTR signal!\n");
							continue;
						}
						/*
						if(rval.err ==ECONNRESET)
						if(rval.err  !=  EAGAIN)
							debug_err("recv buf failed! errno:%d, has recved len[%d], want recv len[%d]\n",
									rval.err, recv_len, buflen-recv_len); 
						*/
						break;
				}
				if(rval.v == 0){ //  peer has performed an orderly shutdown.
					debug_warn("peer has shutdown rval.v ==0 \n");
					rval.err = ECONNRESET;
					break;
				}
				
				recv_len += rval.v;
		}

		rval.v = recv_len;
		return rval;
}


void* recv_fun(void* arg){
		conn_t* conn = (conn_t*)arg;
		msg_t* msg ;
		rval_t rval;
		char* buf;
		int len;
		int off=0;

		rval.err = 0;
		msg  = conn->recving_msg;

		/*
		pthread_mutex_lock(&conn->lock);
			conn->rcv_state = RCVING;
		pthread_mutex_unlock(&conn->lock);
		*/
		
		do{
				if(msg==NULL){
						int node_idx  = conn->node_idx;
						int id = g_ctx.conf.node[node_idx].id;
						msg = new_msg(UN_MSG, id,  NULL, 0);
				}

				if(msg->curr_len  <  MSG_HR_LEN){
						buf = (char*)&msg->hr + msg->curr_len;
						len = MSG_HR_LEN - msg->curr_len;

						rval = recv_buf(conn,buf,len);
						msg->curr_len += rval.v;

						if(failed(rval)){
							goto l_exit;
						}
						
						assert(len == rval.v);

						assert(msg->curr_len == MSG_HR_LEN);
						
						if(msg->hr.magic !=  MSG_MAGIC){
							debug_info("recv msg magic failed! msg id[%llu] \n", msg->hr.id);
							rval.err = EMSG_MAGIC;
							goto l_exit;
						}
					
					    	debug_info("recv msg hr ok, msg id[%llu], hr len[%d],"
									" actual recv len[%d]\n",
								msg->hr.id, msg->hr.len, rval.v);
						
				}				

				assert(msg->hr.len >0);
				
				if(!msg->buf )
					msg->buf = my_malloc(msg->hr.len);

				buf = msg->buf +  msg->curr_len  - MSG_HR_LEN;
				len = msg->hr.len + MSG_HR_LEN - msg->curr_len;

				rval = recv_buf(conn, buf, len);
				msg->curr_len += rval.v;

				if(failed(rval)){
					goto l_exit;
				}

				debug_info("recv msg buf ok!  msg id[%llu], buf len[%d], msg->curr_len[%d]\n",
							msg->hr.id,  msg->hr.len, msg->curr_len);

				assert(msg->curr_len == msg->hr.len + MSG_HR_LEN);
				
				recv_callback(conn,msg);
			

				if(conn->recving_msg == msg){
						conn->recving_msg = NULL;
				}
				msg = NULL;

		}while(1);
		
l_exit:
		switch(rval.err){
			case EAGAIN:
				debug_info("recving msg block, msg id[%llu], msg->curr_len[%d], msg->hr.len[%d]\n",msg->hr.id, msg->curr_len, msg->hr.len);

				conn->recving_msg = msg;
				break;
			case  EMSG_MAGIC:
			case ECONNRESET:
			case  ESHUTDOWN:
				debug_warn("recv msg id[%llu], buf len[%d],msg->curr_len[%d], failed, errno[%d]\n",
								msg->hr.id, msg->hr.len, msg->curr_len, rval.err);
				debug_warn("start reconn!\n");

				free_msg(msg);
				
				conn->recving_msg = msg = NULL;
				
				add_reconn(conn);
				break;
			default:
				debug_err("recv msg id[%llu], buf len[%d],msg->curr_len[%d], failed, errno[%d]\n",
						msg->hr.id, msg->hr.len, msg->curr_len, rval.err);
				break;
		}

		/*
		pthread_mutex_lock(&conn->lock);
			conn->rcv_state = IDEL;
		pthread_mutex_unlock(&conn->lock);
		*/
		
		return 0;
		
}

