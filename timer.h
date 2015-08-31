#include <sys/time.h> 

#include "sys.h"
#include "list.h"
#include "debug.h"
#include "help.h"

#define MAX_TIMER_NUM 10000

#define TIMER_START 1
#define TIMER_TICK 10000000

#define INVALID_TIMER_ID 1
 typedef int timer_id; 

 /** 
 * The type of callback function to be called by timer scheduler when a timer 
 * has expired. 
 * 
 * @param id                The timer id. 
 * @param user_data        The user data. 
 * $param len               The length of user data. 
 */ 
 typedef int timer_expiry(void *user_data); 

 /** 
 * The type of the timer 
 */ 
 struct timer { 
        struct list_head  entries;/**< list entry               */ 

        timer_id id;                /**< timer id                  */ 

        int interval;               /**< timer interval(second) */ 
        int elapse;                 /**< 0 -> interval             */ 

        timer_expiry *cb;          /**< call if expiry            */ 
        void *user_data;           /**< callback arg               */ 
 };

 				
/** 
 * The timer list 
 */ 
 typedef struct timer_list { 
 	pthread_t pid;
        struct list_head  header;  /**< list header         */ 
        int num; 	                                   /**< timer entry number */ 
        int max_num;                               /**< max entry number    */ 
 }timer_list_t;

 extern int init_timer(int count) ;
 extern int destroy_timer(void) ;
 extern int add_timer(int interval, timer_expiry *cb, void *user_data) ;

  
