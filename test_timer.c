#include "timer.h"


int timer_cb(timer_id id, void *user_data, int len){
	printf("time out!\n");
}

int main(){
	int intval = 1;
	timer_id  id;
	int i;

	init_timer(1024);
	for(i=0; i<10; i++){	
		id = add_timer(intval+i ,timer_cb, NULL, 0);
	}

	sleep(100000);
	destroy_timer();
}
