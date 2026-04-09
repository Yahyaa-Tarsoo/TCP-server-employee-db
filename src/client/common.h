#ifndef COMMON_H
#define COMMON_H

#include<stdint.h>

#define STATUS_ERROR -1
#define STATUS_SUCCESS 0

#define PROTO_VERS 100

typedef enum
{
	MSG_HELLO_REQ = 1,
	MSG_HELLO_RESP = 2,
	MSG_ADD_REQ = 3,
	MSG_ADD_RESP = 4,
	MSG_LIST_REQ = 5,
	MSG_LIST_RESP = 6,
	MSG_DELETE_REQ = 7,
	MSG_DELETE_RESP = 8,
	MSG_ERR = 9
}dbproto_type_e;

typedef struct __attribute__((packed)){  // prevent padding since enum is compiler defined
	uint16_t type;
	uint16_t len;
}dbproto_hdr_t;

typedef struct{
	uint16_t proto;
}dbproto_hello_req;

typedef struct{
	uint16_t proto;
}dbproto_hello_resp;

typedef	struct{
	uint8_t data[1024];
}dbproto_add_req;

typedef struct {
	char name[256];
	char address[256];
	unsigned int hours;
}dbproto_employee_list_resp;

typedef struct{
	char name[256];
}dbproto_delete_req;




#endif