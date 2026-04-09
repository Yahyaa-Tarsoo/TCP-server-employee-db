#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "servpoll.h"
#include "common.h"
#include <parse.h>

void fsm_resp_hello_err(clientstate_t* client, dbproto_hdr_t* hdr){
	hdr->type = htons(MSG_ERR);
	hdr->len = htons(0);

	write(client->fd,hdr,sizeof(dbproto_hdr_t));
}

void fsm_resp_hello(clientstate_t* client){
	dbproto_hdr_t hdr;
	hdr.type = htons(MSG_HELLO_RESP);
	hdr.len  = htons(1);

	dbproto_hello_resp hello_resp;
	hello_resp.proto = PROTO_VERS;
	hello_resp.proto = htons(hello_resp.proto);

	char sendbuf[sizeof(dbproto_hdr_t)+sizeof(dbproto_hello_resp)];

	memcpy(sendbuf,&hdr,sizeof(dbproto_hdr_t));
	memcpy(sendbuf + sizeof(dbproto_hdr_t), &hello_resp,sizeof(dbproto_hello_resp));

	send(client->fd,sendbuf,sizeof(sendbuf),MSG_NOSIGNAL);	
}

void fsm_resp_add(clientstate_t* client){
	dbproto_hdr_t hdr;

	hdr.type = htons(MSG_ADD_RESP);
	hdr.len = htons(0);

	write(client->fd,&hdr,sizeof(dbproto_hdr_t));
}

void fsm_resp_add_err(clientstate_t* client){
	dbproto_hdr_t hdr;

	hdr.type = htons(MSG_ERR);
	hdr.len = htons(0);

	write(client->fd,&hdr,sizeof(dbproto_hdr_t));
}

void fsm_resp_list(clientstate_t* client,struct dbheader_t* dbhdr,struct employee_t* employees){
    dbproto_hdr_t hdr;

    hdr.type = htons(MSG_LIST_RESP);
    hdr.len  = htons(dbhdr->count);

    write(client->fd, &hdr, sizeof(hdr));

    for (int i = 0; i < dbhdr->count; i++) {
        dbproto_employee_list_resp list_resp;

        memset(&list_resp, 0, sizeof(list_resp));

        strcpy(list_resp.name, employees[i].name);
        strcpy(list_resp.address, employees[i].address);

        list_resp.hours = htons(employees[i].hours);

        write(client->fd, &list_resp, sizeof(list_resp));
    }
}

void fsm_resp_list_err(clientstate_t* client){
	dbproto_hdr_t hdr;

	hdr.type = htons(MSG_ERR);
	hdr.len = htons(0);

	write(client->fd,&hdr,sizeof(dbproto_hdr_t));
}

void fsm_resp_delete(clientstate_t* client){
	dbproto_hdr_t hdr;

	hdr.type = htons(MSG_DELETE_RESP);
	hdr.len = htons(0);

	write(client->fd,&hdr,sizeof(dbproto_hdr_t));
}

void fsm_resp_delete_err(clientstate_t* client){
	dbproto_hdr_t hdr;

	hdr.type = htons(MSG_ERR);
	hdr.len = htons(0);

	write(client->fd,&hdr,sizeof(dbproto_hdr_t));
}

void handle_client_fsm(struct dbheader_t* dbhdr, struct employee_t** employees, clientstate_t* client, int dbfd){

	if(client->state == STATE_CONNECTED){
		client->state = STATE_HELLO;
	}

	dbproto_hdr_t hdr;

	memcpy(&hdr,client->buffer,sizeof(hdr));
		        // use memcpy instead of casting to prevent alignment issues on different CPUs

	hdr.len = ntohs(hdr.len);
	hdr.type = ntohs(hdr.type);

	if(client->state == STATE_HELLO){
		if(hdr.type != MSG_HELLO_REQ || hdr.len != 1){
			// TODO send err msg
		}else{
			dbproto_hello_req hello_req;
	        memcpy(&hello_req,client->buffer + sizeof(dbproto_hdr_t),sizeof(hello_req));

	        hello_req.proto = ntohs(hello_req.proto);

	        if (hello_req.proto != PROTO_VERS) {
	            fsm_resp_hello_err(client,&hdr);
	            return;
	        }
	        fsm_resp_hello(client);
	        printf("Client upgraded to msg state\n");
	        client->state = STATE_MSG;
	        return;
		}
	}

	if(client->state == STATE_MSG){
		if(hdr.type == MSG_ADD_REQ){
			if(hdr.len != 1){
				fsm_resp_add_err(client);
				return;
			}else{
				dbproto_add_req add_req;
				memcpy(&add_req,client->buffer+sizeof(dbproto_hdr_t),sizeof(dbproto_add_req));
				if(add_employee(dbhdr,employees,add_req.data) != STATUS_SUCCESS){
					fsm_resp_add_err(client);
					return;
				}else{
					output_file(dbfd,dbhdr,*employees);
					fsm_resp_add(client);
					return;
				}
			}
		}

		if(hdr.type == MSG_LIST_REQ){
			if(hdr.len != 0){
				fsm_resp_list_err(client);
				return;
			}else{
				fsm_resp_list(client,dbhdr,*employees);
				return;
			}

		}

		if(hdr.type == MSG_DELETE_REQ){
			if(hdr.len != 1){
				fsm_resp_delete_err(client);
				return;
			}else{
				//int remove_employee(struct dbheader_t* dbhdr, struct employee_t** employees, char* removestr)
				dbproto_delete_req del_req;
				memcpy(&del_req,client->buffer+sizeof(dbproto_hdr_t),sizeof(dbproto_delete_req));
				if(remove_employee(dbhdr,employees,del_req.name) != STATUS_SUCCESS){
					fsm_resp_delete_err(client);
					return;
				}else{
					output_file(dbfd,dbhdr,*employees);
					fsm_resp_delete(client);
					return;
				}
			
			}
		}

		
	}
}

void init_clients(clientstate_t* states){
	for(int i=0;i<MAX_CLIENTS;i++){
		states[i].fd = -1;
		states[i].state = STATE_NEW;
		memset(&states[i].buffer,'\0',BUFFER_SIZE);
	}
}

int find_free_slot(clientstate_t* states){
	for(int i=0;i<MAX_CLIENTS;i++){
		if(states[i].fd == -1){
			return i;
		}
	}
	return -1;
}

int find_slot_by_fd(clientstate_t* states, int fd){
	for(int i=0;i<MAX_CLIENTS;i++){
		if(states[i].fd == fd){
			return i;
		}
	}
	return -1;
}