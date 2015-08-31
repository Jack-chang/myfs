#include "interface.h"
#include "conf.h"


void set_config(int id, char* addr, int port){
	int n = g_ctx.conf.node_num++;
	node_t* node = &g_ctx.conf.node[n];
	
	node->id = id;
	node->addr.sin_family = AF_INET;
	node->addr.sin_addr.s_addr = inet_addr(addr);
    	node->addr.sin_port = htons(port); 
}


node_t* find_node_from_id(int server_id){
	int i;
	for(i=0; i<MAX_NODE; i++){
		if(g_ctx.conf.node[i].id == server_id)
			return &g_ctx.conf.node[i];
	}

	debug_err("can't find this server! server_id[%d]\n", server_id);
	return NULL;
}

int find_node_from_addr(struct sockaddr_in addr){
	int i;
	for(i=0; i<MAX_NODE; i++){
		if(cmp_ip(g_ctx.conf.node[i].addr ,addr))
			return i;
	}
	debug_err("can't find this server! server_addr:%s\n", dump_addr(addr)   );
	
	return -1;
}

int find_node_from_addr1(struct sockaddr_in addr){
	int i;
	for(i=0; i<MAX_NODE; i++){
		if(cmp_ip(g_ctx.conf.node[i].addr ,addr))
			return i;
	}
	debug_err("can't find this server! server_addr:%s\n", dump_addr(addr)   );

	return -1;
}




void read_conf(char* file_name){
	set_config(0,"115.29.42.68",10000);
	set_config(1,"114.215.104.108",10001);
	
}

