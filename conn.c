#include "conn.h"
#include "debug.h"
#include "interface.h"


extern ctx_t g_ctx;

static  conn_tab_t conn_tab;

int conn_tab_init(conn_tab_t*  conn_tab){
	int i=0;
	g_ctx.conn_tab.size=0;
	for(i=0; i<MAX_CONN; i++){
		g_ctx.conn_tab.ctab[i] = NULL;
	}
	pthread_mutex_init(&conn_tab->lock, 0);
}



conn_t*  create_conn(int node_idx, int sockfd, struct sockaddr_in addr){
	static int conn_id = 0;
	conn_t* conn =  my_malloc(sizeof(conn_t));
	conn->node_idx = node_idx;
	conn->id = conn_id++;
	conn->addr = addr;
	conn->sockfd = sockfd;
	conn->recving_msg = NULL;
	conn->sending_msg = NULL;
	conn->snd_state = IDEL;
	conn->rcv_state  = IDEL;
	conn->conn_state = CLOSED;
	conn->conn_type = NONE;
	
	pthread_mutex_init(&conn->lock, NULL);
	INIT_LIST_HEAD(&conn->reconn_link);
	
	queue_init(&conn->q_recv, 0);
	queue_init(&conn->q_send, MAX_SEND_QUEUE_SIZE);
	queue_init(&conn->q_ack, 0);
	
	return conn;
}


int destroy_conn(conn_t* conn){
	my_free(conn);
}


int set_conn_state(conn_t* conn, int state){
	pthread_mutex_lock(&conn->lock);
		conn->conn_state = state;
	pthread_mutex_unlock(&conn->lock);
}

int get_conn_state(conn_t* conn){
	int state;
	pthread_mutex_lock(&conn->lock);
		state = conn->conn_state ;
	pthread_mutex_unlock(&conn->lock);
	return state;
}
int conn_tab_add(conn_t* conn){
	int i;
	int rc = -1;
	pthread_mutex_lock(&g_ctx.conn_tab.lock);
	for(i=0; i<MAX_CONN; i++){
		if(NULL == g_ctx.conn_tab.ctab[i]  ){
			g_ctx.conn_tab.ctab[i] = conn;
			g_ctx.conn_tab.size++;
			rc = 0;
			break;
		}
	}
	pthread_mutex_unlock(&g_ctx.conn_tab.lock);
	return rc; 
}

int conn_tab_del(conn_t* conn){
	int i;
	int rc = -1;
	pthread_mutex_lock(&g_ctx.conn_tab.lock);
	for(i=0; i<MAX_CONN; i++){
		if(g_ctx.conn_tab.ctab[i] == conn  ){
			g_ctx.conn_tab.ctab[i] = NULL;
			g_ctx.conn_tab.size--;
			rc =0;
			break;
		}
	}
	pthread_mutex_unlock(&g_ctx.conn_tab.lock);
	return  rc; // conn tab is find
	
}

conn_t* __conn_tab_find_from_fd(int fd){
	int i;
	for(i=0; i<MAX_CONN; i++){
		if(g_ctx.conn_tab.ctab[i] != NULL && g_ctx.conn_tab.ctab[i]->sockfd == fd){
			return g_ctx.conn_tab.ctab[i];
		}
	}
	debug_err("can't find conn from fd[%d]\n", fd);
	return NULL; // conn tab is find
}

conn_t* conn_tab_find_from_fd(int fd){
	conn_t* conn;
	pthread_mutex_lock(&g_ctx.conn_tab.lock);
	conn = __conn_tab_find_from_fd(fd);
	pthread_mutex_unlock(&g_ctx.conn_tab.lock);
	return conn;
}

static conn_t* __conn_tab_find_from_node(node_t* node){
	int i;	

	for(i=0; i<g_ctx.conf.node_num; i++){
		
		if(g_ctx.conn_tab.ctab[i] != NULL && cmp_ip(g_ctx.conn_tab.ctab[i]->addr ,node->addr)){
			return g_ctx.conn_tab.ctab[i];
		}
	}
	return NULL; // conn tab is find
}

conn_t* conn_tab_find_from_node(node_t* node){
	conn_t* conn;
	pthread_mutex_lock(&g_ctx.conn_tab.lock);
	conn = __conn_tab_find_from_node(node);
	pthread_mutex_unlock(&g_ctx.conn_tab.lock);
	return conn;
	
}
	


