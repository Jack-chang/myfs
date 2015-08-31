#include "timer.h"

static timer_list_t timer_list;


  /* Tick Bookkeeping */ 

static void tick(){
	 struct list_head* head  = &timer_list.header;
        struct list_head* pos;
        struct timer *node ;

        list_for_each(pos,head){
                node = list_entry(pos, struct  timer, entries);
                node->elapse++;
                if(node->elapse >= node->interval) {
                        node->elapse = 0;
                        node->cb(node->user_data);
                }
        }

}
static void*  timeout_func(void* arg){ 
	int ret;
	while(1){
		do{
			ret = sleep(1);
		}while(ret!=0);
		
		tick();
 	}
}

int init_timer(int count) 
{ 
		int ret = 0; 

		memset(&timer_list, 0, sizeof(struct timer_list)); 
		INIT_LIST_HEAD(&timer_list.header); 
		timer_list.max_num = count; 

		ret  = pthread_create(&timer_list.pid, NULL, timeout_func, NULL);
		check(ret);

		return ret; 
} 


/** 
 * Destroy the timer list. 
 * 
 * @return 		 0 means ok, the other means fail. 
 */ 
int destroy_timer(void) 
{ 
		struct timer *node = NULL; 
		struct list_head* head  = &timer_list.header;
		struct list_head* pos;
		struct list_head* tmp;

		pthread_join(timer_list.pid);

		list_for_each_safe(pos,tmp, head){
				node = list_entry(pos, struct timer, entries);
				printf("Remove id %d\n", node->id); 
				if(node->user_data)
						free(node->user_data); 
				free(node); 

		}

		memset(&timer_list, 0, sizeof(struct timer_list)); 
	

		return 0; 
} 




int	add_timer(int interval, timer_expiry *cb, void *user_data) 
{ 
		struct timer *node = NULL; 

		if (cb == NULL || interval <= 0) { 
				return EINVAL; 
		} 
 
		  timer_list.num++; 
 
	  node = my_malloc(sizeof(struct timer));
	  INIT_LIST_HEAD(&node->entries);

	  node->user_data = user_data;
 
	  node->cb = cb; 
	  node->interval = interval; 
	  node->elapse = 0; 
	  node->id = timer_list.num; 
 
	  list_add_tail(&node->entries,&timer_list.header); 
 
	 return ;
  } 




