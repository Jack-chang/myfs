#ifndef __TPOOL_H__
#define __TPOOL_H__

#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include "queue.h"

typedef void* (*TFUN)(void*);

typedef struct _thread{
	pthread_t pid;
	int myid; 
	queue_t q;
	sem_t ready;	
}thread_t;

typedef struct tpool_t{
    char* name;// thread name, for debug
    int num;  //thread pool num
    thread_t** threads;
}tpool_t;


typedef struct _task{
		unsigned long long id; //for test
		struct list_head   link;   //process task link
		TFUN fun;
		void* arg;
		void* rval;
}task_t;


extern task_t*  new_task(TFUN fun, void* arg);
extern void destroy_task(task_t* task);


extern thread_t*   new_thread(int myid);
extern void start(tpool_t* tp);
extern int add_task(task_t* task);
extern void wait_all(tpool_t* tp);
extern int schedule(tpool_t* tp,task_t* task);
                                               
#endif
