#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/socket.h>
#include <string.h>      
#include <arpa/inet.h>   
#include <unistd.h>      

#include "common.h"
#include "parse.h"
#include "file.h"
#include "servpoll.h"

clientstate_t clientStates[MAX_CLIENTS];

void print_usage(char* argv[]){
	printf("Usage: %s -n -f <database_file>\n",argv[0]);
	printf("\t -n  -create a new database file\n");
	printf("\t -f  -(required) path to db file\n");
	return;
}



void poll_loop(unsigned short port, struct dbheader_t* dbhdr, struct employee_t** employees,int dbfd){
	int listen_fd,client_fd,freeSlot;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_length = sizeof(client_addr);

	struct pollfd fds[MAX_CLIENTS+1];
	int nfds = 1;
	int n_events;

	init_clients(&clientStates);

	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		return;
	}

	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if(bind(listen_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1){
		perror("bind");
		return;
	}

	if(listen(listen_fd,10)==-1){
		perror("listen");
		return;
	}

	printf("SERVER LISTENING ON PORT %d\n", port);

	memset(&fds,0,sizeof(fds));
	fds[0].fd = listen_fd;
	fds[0].events = POLLIN;

	while(1){

		if((n_events=poll(&fds,nfds,-1))==-1){
			perror("poll");
			return;
		}

		if(fds[0].revents & POLLIN){
			client_fd = accept(listen_fd,(struct sockaddr*)&client_addr,&client_length);
			if(client_fd==-1) { return; }
			int slot = find_free_slot(&clientStates);
			clientStates[slot].fd = client_fd;
			clientStates[slot].state = STATE_CONNECTED;

			fds[nfds].fd = client_fd;
			fds[nfds].events = POLLIN;
			nfds++;
		}

		for(int i=1;i<nfds;i++){
			if(fds[i].revents & POLLIN){
				int fd = fds[i].fd;
				int slot = find_slot_by_fd(&clientStates,fd);
				ssize_t bytes_read = read(fd, clientStates[slot].buffer, sizeof(clientStates[slot].buffer));
		        if (bytes_read <= 0) { 
		            close(fd);
		            clientStates[slot].fd = -1;
		            clientStates[slot].state = STATE_DISCONNECTED;
	            
		            fds[i] = fds[nfds - 1];
		            nfds--;
		            i--; 
		        }else{
		            handle_client_fsm(dbhdr, employees, &clientStates[slot],dbfd);
		        }
			}
		}
	}
}

int main(int argc, char* argv[]){	
	int c;
	bool newfile = false;
	char* filepath = NULL;
	char* addstr = NULL;
	char* removestr = NULL;
	bool list = false;
	char* updatehr = NULL;
	char* portarg = NULL;
	int portnum;

	int dbfd = -1;
	struct dbheader_t* dbhdr = NULL;

	struct employee_t* employees = NULL;

	while((c=getopt(argc,argv,"nf:a:lr:h:p:")) != -1){
		switch(c){
			case 'n': 
				newfile = true;
				break;
			case 'f': 
				filepath = optarg;
				break;
			case 'p':
				portarg = optarg;
				portnum = atoi(portarg);
				if(portnum==0){
					printf("BAD PORT %s\n",portarg);
				}
				break;
			case '?' :
				printf("Invalid option -%c\n", c);
				break;
			default: return -1;
		}
	}

	if (filepath == NULL)
	{
		printf("File path is required\n");
		print_usage(argv);
		return 0;
	}

	if(newfile){
		dbfd = create_db_file(filepath);
		if(dbfd	== STATUS_ERROR){
			printf("failed to create db file\n");
			return -1;
		}
		create_db_header(dbfd,&dbhdr);
	}else{
		dbfd = open_db_file(filepath);
		if(dbfd	== STATUS_ERROR){
			printf("failed to open db file\n");
			return -1;
		}
		if(validate_db_header(dbfd,&dbhdr) == STATUS_ERROR){
			printf("Failed to validate db header!\n");
			return STATUS_ERROR;
		}
	}

	if(read_employees(dbfd,dbhdr,&employees) != STATUS_SUCCESS){
		printf("Failed to read employees\n");
		return 0;
	}

	poll_loop(portnum,dbhdr,&employees,dbfd);

	

	return 0;
}