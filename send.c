#include "sys.h"

#include "conn.h"
#include "interface.h"
#include "debug.h"
#include "help.h"
#include "timer.h"

/*
 * if rva.err =0 ,success, else rval.err = errno,
 *   rval.v  send len 
 * */

rval_t send_buf(conn_t* conn, char*buf, int buflen){
	int left = buflen;
	int send_len = 0;
	rval_t rval;

	rval.err  = 0;	
	while(left){
			//send hr first.
			rval.v = send(conn->sockfd, buf, left, 0);
			if(rval.v == -1){
				rval.err = errno;
				goto l_exit;
			}
			left -= rval.v;
			send_len += rval.v;
	}

l_exit:
	rval.v = send_len;

	switch(rval.err){
		case 0:
			debug_info("send_buf success! send_len[%d]\n",  rval.v);
			break;
		case EAGAIN:
			debug_info("send_buf blocked! send_len[%d]\n",  rval.v); 
			break;
		default:
			debug_info("send_buf failed! errno[%d]\n",  rval.err); 
			break;
	}
	
	return rval;		
	
}


rval_t send_one_msg(conn_t* conn, msg_t* msg){
		int len ;
		int send_len = 0;
		rval_t rval;
		int err = 0;
		char* buf;

		debug_info("start send msg_id[%llu], msg->hr.len[%d], msg->curr_len[%d]\n", 
				msg->hr.id, msg->hr.len, msg->curr_len);
		
		
			//send hr first.
		if(msg->curr_len  <  MSG_HR_LEN){
				buf = (char*)&msg->hr +  msg->curr_len;
				len = MSG_HR_LEN - msg->curr_len;

				rval= send_buf(conn, buf , len);

				msg->curr_len += rval.v;
				send_len += rval.v;

				if(rval.err !=0)
					return rval;
				
				debug_info("send msg id[%llu] hr len[%d] ok!\n",
								msg->hr.id, msg->curr_len);
				assert(msg->curr_len == MSG_HR_LEN);
		}

		//send data 
		buf  = msg->buf + msg->curr_len - MSG_HR_LEN;
		len = msg->hr.len + MSG_HR_LEN - msg->curr_len;

		rval  = send_buf(conn,buf ,len );

		msg->curr_len += rval.v;
		send_len += rval.v;

		if(rval.err	!=0){
			return rval;
		}

		debug_info("send msg id[%llu] buf len[%d] ok!\n",
						msg->hr.id, msg->hr.len);
		assert(msg->hr.len + MSG_HR_LEN == msg->curr_len);
		rval.v = send_len;
		return rval;
}


int timeout_cb(void* arg){
	cb_arg_t* cb = arg;
	conn_t* conn = cb->conn;
	msg_t* msg =   cb->msg;

	debug_info("msg[%llu] timeout, and to resend!\n", msg->hr.id);

	list_del_init(&msg->link);
	
	enque(&conn->q_send,  &msg->link);

	my_free(cb);
}

void send_before(conn_t* conn, msg_t* msg){
		cb_arg_t* cb;
		int intval = 1000;

	//only request msg need to add to ack queue
	if(msg->hr.type == REQ_MSG){
		//add ack queue
		enque(&conn->q_ack,  &msg->link);
		//add timeout timer
		cb = my_malloc(sizeof(cb_arg_t));
		cb->conn = conn;
		cb->msg  = msg;
		add_timer(intval, timeout_cb, cb);
	}
}

void send_success(conn_t* conn, msg_t* msg){
	if(msg->hr.type == ACK_MSG){
			 free_msg( msg);
	}
}

void* send_fun(void* arg){

		conn_t* conn = (conn_t*)arg;
		msg_t* msg ;
		struct list_head* list;
		rval_t rval;
		
/*
		pthread_mutex_lock(&conn->lock);
		conn->snd_state = SNDING;
		pthread_mutex_unlock(&conn->lock);
		*/
		
		
		//first send sending_msg
		if(conn->sending_msg){
			msg = conn->sending_msg;
			rval = send_one_msg(conn,conn->sending_msg);
			if(rval.err !=0){
				goto l_exit;
			}
			msg = conn->sending_msg = NULL;
		}

l_continue:			
		while(!is_empty(&conn->q_send)){
				list = deque(&conn->q_send);
				assert(list != NULL);
				msg = list_entry(list, msg_t, link);
				assert(msg->curr_len ==0); //start send a new message
				
				//first add ack queue before send
				send_before(conn,msg);
				
				rval = send_one_msg(conn,msg);
				if(rval.err !=0){
					goto l_exit;
				}
				
				//send ok, call callback_fun
				
				send_success(conn, msg);
		}

	    
		{ //send empty, complete this task
			pthread_mutex_lock(&conn->lock);
				if(is_empty(&conn->q_send))
					conn->snd_state = IDEL;
				else{
					pthread_mutex_unlock(&conn->lock);
					goto l_continue;
				}
			pthread_mutex_unlock(&conn->lock);
		} 
	
		return 0;
l_exit:
	assert(rval.err!=0);
	switch(rval.err){
		case EAGAIN: //if not send complete
			conn->sending_msg = msg;
			debug_info("send msg id[%llu] blocked, msg->hr.len[%d], curr_len[%d], this send len[%d]\n",
						msg->hr.id, msg->hr.len, msg->curr_len,rval.v);
			break;
		case ECONNRESET:  //shutdown by remote peer
		case ENOTCONN:
		case EPIPE:  //this socket has been shutdown by recv threads on this socket
			debug_warn("add reconn conn[%d]\n", conn->id);
			msg->curr_len = 0;
			conn->sending_msg = msg;
			add_reconn(conn);
			break;
	
		default:
			debug_err("send msg id[%llu] failed!  curr_len[%d], msg_len[%d],send len[%d], errno[%d]\n",
						msg->hr.id, msg->curr_len, msg->hr.len,rval.v, rval.err);
			break;
	}
	
	return 0;
		

}

