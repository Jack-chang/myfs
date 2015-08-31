#ifndef  __HELP__H__
#define  __HELP__H__

#include "sys.h"
/*
#define my_malloc(len) ({  \
		void* p = malloc(len) ;    \
		p;             \
		})
*/
		

#define my_malloc(len)  ({\
	void* p;           \
	do{           \
		p = malloc(len);         \
		if(p==NULL){                   \
			debug_err("malloc failed! len=%d\n", len);    \
		}   \
	}while(0); \
	p;    \
})


#define my_free(ptr) \
	do{                 \
		if(ptr){       \
			free(ptr);           \
			ptr = NULL;	 \
		}		\
		else                           \
			debug_err("free a zero point!\n");     \
	}while(0)


typedef  struct _rval{
	union{
		int v;
		void* p;
	};
	int err;
}rval_t;


#define check(rc) \
	do{               \
		if(rc < 0){        \
			debug_err("call fun failed! err[%d]:%s\n",  errno, strerror(errno)); \
			assert(0);             \
		}             \
	}while(0)          




#define success(rval) (rval.err==0)
#define failed(rval)  (rval.err!=0)


static inline int cmp_addr(struct sockaddr_in addr1, struct sockaddr_in addr2){
		char abuf1[INET_ADDRSTRLEN];
		char abuf2[INET_ADDRSTRLEN];
		const char* info1;
		const char* info2;
		info1  = inet_ntop(AF_INET, &addr1.sin_addr, abuf1,INET_ADDRSTRLEN);
		info2  = inet_ntop(AF_INET, &addr2.sin_addr, abuf2,INET_ADDRSTRLEN);
	
		if(ntohs(addr1.sin_port) != ntohs(addr2.sin_port))
				return 0;
		if(strcmp(info1,info2)==0)
				return 1;
		else
				return 0;
}

static inline int cmp_ip(struct sockaddr_in addr1, struct sockaddr_in addr2){
		char abuf1[INET_ADDRSTRLEN];
		char abuf2[INET_ADDRSTRLEN];
		const char* info1;
		const char* info2;
		info1  = inet_ntop(AF_INET, &addr1.sin_addr, abuf1,INET_ADDRSTRLEN);
		info2  = inet_ntop(AF_INET, &addr2.sin_addr, abuf2,INET_ADDRSTRLEN);

		if(strcmp(info1,info2)==0)
				return 1;
		else
				return 0;
}




#endif
