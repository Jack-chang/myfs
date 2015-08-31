#include "sys.h"

#include "errno.h"
#include "conn.h"
#include "debug.h"
#include "help.h"
#include "interface.h"

extern ctx_t g_ctx;


int add_reconn(conn_t* conn){
	
	int sockfd = conn->sockfd;
	int need = 0;
	int original_state ;
	int rc;
	
	pthread_mutex_lock(&conn->lock);
	original_state = conn->conn_state;
	if(conn->conn_state == CONNECTED){
		debug_info("closed sockfd[%d]\n", sockfd);
		rc = close(sockfd);
		check(rc);
		/*
		if(conn->conn_type == ACCEPT){
			rc = close(sockfd);
			check(rc);
		}else{
			rc =shutdown(sockfd,SHUT_RDWR);
			check(rc);
		}*/
		need = 1;
	}else if(conn->conn_state == CLOSED){  //when active connect failed
		need = 1;	
	}//other COONECING

	conn->conn_state = CONNECTING;
	pthread_mutex_unlock(&conn->lock);

	assert(conn->conn_state == CONNECTING);
	//
	del_event(conn);

	
	if(need && conn->conn_type == ACTIVE){
		debug_info("add to reconn queue, conn->conn_type[%d], original state[%d]\n",
					conn->conn_type, original_state);
		enque(&g_ctx.reconn_q, &conn->reconn_link);
	}else{		//accept connect, do nothing ,but wait for be connected
		debug_info("accept connect do nothing, conn->conn_type[%d], original state[%d]\n",
					conn->conn_type, original_state);
	}
}


static rval_t re_connect(conn_t* conn){
		rval_t rval;

		rval.err = 0;

		if((conn->sockfd = socket(PF_INET,SOCK_STREAM,0))<0){
			  debug_err(" create socket failed! errno[%d]\n", errno);
			  rval.err = errno;
			  return rval;
		}
		sock_general_init(conn->sockfd);
		
		rval.v = connect(conn->sockfd,(struct sockaddr *)&conn->addr, sizeof(struct sockaddr));
		if(rval.v == -1){
				rval.err = errno;
				close(conn->sockfd);
		}
		setblock(conn->sockfd, 0);  //not block

		return rval;

}

static void* reconn_fun(void* arg){
		conn_t* conn;
		queue_t* q = &g_ctx.reconn_q;
		rval_t rval;
		int  times = 1;
		while(1){
				conn = list_entry(deque(q), conn_t,  reconn_link);
				assert(conn->conn_state == CONNECTING);
				assert(conn->conn_type == ACTIVE);
				debug_info("start reconn conn[%d], fd[%d],%s \n",
						   conn->id, conn->sockfd, dump_addr(conn->addr));
				rval =  re_connect(conn);
				if(success(rval)){
					debug_info("reconn conn[%d], fd[%d] success! \n", conn->id, conn->sockfd);
					set_conn_state(conn, CONNECTED);
					add_event(conn);

				}else{
						debug_warn("reconn conn[%d], fd[%d] failed! errno[%d]\n",
								conn->id, conn->sockfd, rval.err);
						enque(q, &conn->reconn_link); //continue reconn
				}
				sleep(times);
		}
}

void start_reconn_thread(){
		int re;
		re = pthread_create(&g_ctx.reconn_pid, NULL, reconn_fun, NULL);
		assert(re==0);
		debug_info("start reconn thread success!\n");
}

void stop_reconn_thread(){
		pthread_join(g_ctx.reconn_pid, NULL);
}


