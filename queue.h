#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include "list.h"
#include "help.h"

typedef struct _queue{
	struct list_head head;         
	int size;             
	int capacity;        
	sem_t sem_space;          
    	sem_t sem_resource;
    pthread_mutex_t mutex;
}queue_t;


//if capacity == 0, it does't limit queue length
static inline int queue_init(queue_t* q, int capacity){
		q->capacity = capacity;
		q->size = 0;

		INIT_LIST_HEAD(&q->head);
	
		pthread_mutex_init(&q->mutex,NULL);
		if(capacity>0)
			sem_init(&q->sem_space,0,q->capacity);
		sem_init(&q->sem_resource, 0, 0 );
		return 0;
		
}
static inline queue_t* create_queue(int capacity){
		queue_t* q = (queue_t*)my_malloc(sizeof(queue_t));
		assert(q!=NULL);
		queue_init(q,capacity); 
		return q;
}


static inline int enque_head(queue_t* q, struct list_head* element){
	
		pthread_mutex_lock(&q->mutex);

	       if(q->capacity>0){
			while(q->size == q->capacity){
					pthread_mutex_unlock(&q->mutex);
					sem_wait(&q->sem_space);
					pthread_mutex_lock(&q->mutex);
			}
	       }

		list_add(element, &q->head);
		q->size++;
		pthread_mutex_unlock(&q->mutex);
		sem_post(&q->sem_resource);

		return 0;
}



static inline int enque(queue_t* q, struct list_head* element){
	
		pthread_mutex_lock(&q->mutex);

		 if(q->capacity>0){
			while(q->size == q->capacity){
					pthread_mutex_unlock(&q->mutex);
					sem_wait(&q->sem_space);
					pthread_mutex_lock(&q->mutex);
			}
		 }

		list_add_tail(element, &q->head);
		q->size++;
		pthread_mutex_unlock(&q->mutex);
		sem_post(&q->sem_resource);

		return 0;
}

static inline struct list_head* deque_noblock(queue_t* q){
	struct list_head* element =NULL;

	pthread_mutex_lock(&q->mutex);
	if(q->size != 0){
	        	element = q->head.next;
			list_del_init(element);
			q->size--;
	}
	pthread_mutex_unlock(&q->mutex);
	
	 if(q->capacity>0 && element!=NULL){
		sem_post(&q->sem_space);
	 }
	return element;
}

static inline struct list_head* deque(queue_t* q){
		struct list_head* element;

		pthread_mutex_lock(&q->mutex);
		while(q->size == 0){
				pthread_mutex_unlock(&q->mutex);
				sem_wait(&q->sem_resource);
				pthread_mutex_lock(&q->mutex);
		}

		element = q->head.next;
		list_del_init(element);
		q->size--;
		pthread_mutex_unlock(&q->mutex);
		
		 if(q->capacity>0){
			sem_post(&q->sem_space);
		 }

		return element;
}

static inline int is_empty(queue_t* q){
		int re;
		pthread_mutex_lock(&q->mutex);
		re = q->size==0 ? 1:0;
		pthread_mutex_unlock(&q->mutex);
		return re;

}

#endif
