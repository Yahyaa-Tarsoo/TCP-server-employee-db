#ifndef SERVPOLL_H
#define SERVPOLL_H

#include <poll.h>

#define PORT 8080
#define MAX_CLIENTS 256
#define BUFFER_SIZE 4096

typedef enum{
	STATE_NEW,
	STATE_CONNECTED,
	STATE_DISCONNECTED,
	STATE_HELLO,
	STATE_MSG,
	STATE_GOODBYE
}state_e;

typedef struct{
	int fd;
	state_e state;
	char buffer[BUFFER_SIZE];
} clientstate_t;


void init_clients(clientstate_t* states);

int find_free_slot(clientstate_t* states);

int find_slot_by_fd(clientstate_t* states, int fd);

#endif