#include "assert.h"
#include "tpool.h"
#include "stdio.h"
#include "debug.h"
#include "help.h"
#include "interface.h"
#include "conn.h"

thread_t*  new_thread(int myid){
		thread_t* t = my_malloc(sizeof(thread_t));
		assert(t);

		t->myid = myid;
	    sem_init(&t->ready,0,0);
		queue_init(&t->q, 1024);
	
		return t;
}
	
void start(tpool_t* tp){
    int i;
    for(i=0; i<tp->num; i++){
        sem_post(&tp->threads[i]->ready);
    }
}

void stop(tpool_t* tp){
}

void* warp_fun(void* arg){
    thread_t* th = (thread_t*)arg;
    task_t* task;
    struct list_head* list;
	
	debug_info("thread[%d] on ready\n",th->myid);
    sem_wait(&th->ready);
	debug_info("thread[%d] on start\n",th->myid);

    while(1){
	   	list = deque(&th->q);
		task = list_entry(list, task_t, link);
		debug_info("begin execute task[%lu], on thread[%d]\n", task->id, th->myid);
		task->rval = task->fun(task->arg);
		debug_info("complete execute task[%lu], on thread[%d]\n", task->id, th->myid);
		destroy_task(task);
   }
}

void set_detach(tpool_t* tp){

}

void wait_all(tpool_t* tp){
    int i;
    for(i=0; i<tp->num; i++){
       	 pthread_join(tp->threads[i]->pid, NULL);
    }
}

/*
*	@char* name : thread name, for test
*	@n:int		thread pool init num
*
*/

int tpool_init(tpool_t* tp, char* name, int n){

    int i;
    int re;
    tp->name = name;
    tp->num =n;
   
    tp->threads= my_malloc(sizeof(thread_t*) * n);
	
    for(i=0; i<tp->num; i++){
		tp->threads[i] = new_thread(i);
		re = pthread_create(&tp->threads[i]->pid, NULL, warp_fun,(void*)tp->threads[i]);
       	assert(re==0);
    }
    return 0;
}


task_t*  new_task(TFUN fun, void* arg){
	static unsigned long long task_id = 0;
	task_t* task = (task_t*)my_malloc(sizeof(task_t));
	task->id = task_id++;
	task->fun = fun;
	task->arg  = arg;
	INIT_LIST_HEAD(&task->link);
	task->rval = NULL;
}

void destroy_task(task_t* task){
	my_free(task);
}

/*
 *  round robin schedule
*/
int schedule(tpool_t* tp,task_t* task){
	
	int thread_id;
	conn_t* conn =(conn_t*)task->arg;

	thread_id = conn->id % tp->num;
	
	debug_info("schedule task[%lu], on thread[%d]\n", task->id, tp->threads[thread_id]->myid);
	enque(&tp->threads[thread_id]->q, &task->link); 

}
