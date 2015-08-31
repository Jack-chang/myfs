#ifndef __MSG_H__
#define __MSG_H__

#include "sys.h"
#include "debug.h"
#include "help.h"

#define MSG_MAGIC  0x1237f

//extern ctx_t g_ctx;

typedef unsigned long long msg_id_t;

typedef enum msg_type{
    UN_MSG,
    ACK_MSG,
    REQ_MSG,
}msg_type_t;


typedef struct _msg_hr{
	msg_id_t id;
	msg_type_t type;
	int magic;
	int len;
	int from;
	int to;
}msg_hr;

#define MSG_HR_LEN  sizeof(msg_hr)

typedef struct _msg{
	msg_hr hr;
	
	char* buf;
	int curr_len;
	
	struct list_head  link;   //send link, wait-ack link, resend link
	sem_t sem_send;
	
}msg_t;


static inline  msg_id_t get_new_msg_id(){
	static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
	
	static  msg_id_t  msg_id = 0;
	msg_id_t new_id;
	pthread_mutex_lock(&mutex);
	new_id =  msg_id++;
	pthread_mutex_unlock(&mutex);
	return new_id;
	
}


extern int get_local_id();

static inline  msg_t* new_msg(msg_type_t type, int to_id,char* buf, int len){

	msg_t* msg = (msg_t*)my_malloc(sizeof(msg_t));

	msg->hr.id = get_new_msg_id();
	
	msg->hr.type = type;
	msg->hr.from = get_local_id();
	msg->hr.to = to_id;
	msg->hr.magic = MSG_MAGIC;
	msg->hr.len   = len;
	msg->buf = buf;
	msg->curr_len= 0;  
	INIT_LIST_HEAD(&msg->link);
	sem_init(&msg->sem_send, 0, 0 );

	return msg;
	
}

static inline void free_msg(msg_t* msg){
	if(msg->buf)
		my_free(msg->buf);
	my_free(msg);
}


#endif
