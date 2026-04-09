#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#include "common.h"

ssize_t recv_all(int fd, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        // Use read() to read data from the file descriptor
        ssize_t n = read(fd, (char *)buf + total, len - total);

        // Check for errors (n < 0) or end of file (n == 0)
        if (n <= 0) {
            return n; // Return error or EOF indicator
        }

        // Accumulate the total number of bytes read
        total += n;
    }
    // Return total bytes read upon successful completion (which equals len)
    return total;
}


int send_add_employee(int fd, char* addstr){
  char buf[4096] = {0};

  dbproto_hdr_t hdr;
  hdr.type = htons(MSG_ADD_REQ);
  hdr.len = htons(1);

  dbproto_add_req add_req;
  strcpy(add_req.data,addstr);

  memcpy(buf,&hdr,sizeof(dbproto_hdr_t));

  memcpy(buf+sizeof(dbproto_hdr_t),&add_req,sizeof(dbproto_add_req));

  write(fd,buf,sizeof(dbproto_hdr_t)+sizeof(dbproto_add_req));

  ssize_t n = read(fd, buf, sizeof(buf));
    if (n < sizeof(dbproto_hdr_t)) {
        printf("Short read or connection closed\n");
        return STATUS_ERROR;
    }

    memcpy(&hdr, buf, sizeof(dbproto_hdr_t));
    hdr.type = ntohs(hdr.type);
    hdr.len  = ntohs(hdr.len);

    printf("Header type: %d\n", hdr.type);

    if (hdr.type == MSG_ERR) {
        printf("Cannot add employee %s\n",addstr);
        return STATUS_ERROR;
    }

    printf("Employee added successfully.\n");
    return STATUS_SUCCESS;


}

int send_list_req(int fd){
  char buf[4096] = {0};

  dbproto_hdr_t hdr;
  hdr.type = htons(MSG_LIST_REQ);
  hdr.len = htons(0);

  memcpy(buf, &hdr, sizeof(hdr));
  write(fd, buf, sizeof(dbproto_hdr_t));

   // 2. Read response header fully
  recv_all(fd, &hdr, sizeof(hdr));

  hdr.type = ntohs(hdr.type);
  hdr.len  = ntohs(hdr.len);

  printf("Header type: %d\n", hdr.type);

  if(hdr.type == MSG_ERR){
    printf("Failed to get employees\n");
    return STATUS_ERROR;
  }


 // 3. Read each employee exactly once
  for (int i = 0; i < hdr.len; i++) {
      dbproto_employee_list_resp emp;

      recv_all(fd, &emp, sizeof(emp));

      emp.hours = ntohs(emp.hours);

      printf("Employee %d\n", i);
      printf("  Name: %s\n", emp.name);
      printf("  Address: %s\n", emp.address);
      printf("  Hours: %u\n", emp.hours);
  }

  return STATUS_SUCCESS;
}

int send_remove_req(int fd,char* removearg){
  char buf[4096] = {0};

  dbproto_hdr_t hdr;
  hdr.type = htons(MSG_DELETE_REQ);
  hdr.len = htons(1);

  dbproto_delete_req del_req;

  strcpy(del_req.name,removearg);

  memcpy(buf,&hdr,sizeof(dbproto_hdr_t));

  memcpy(buf+sizeof(dbproto_hdr_t),&del_req,sizeof(dbproto_delete_req));

  write(fd,buf,sizeof(dbproto_hdr_t)+sizeof(dbproto_delete_req));

  recv_all(fd, &hdr, sizeof(hdr));

  hdr.type = ntohs(hdr.type);
  hdr.len  = ntohs(hdr.len);

  printf("Header type: %d\n", hdr.type);

  if (hdr.type == MSG_ERR) {
      printf("Cannot delete employee %s\n",removearg);
      return STATUS_ERROR;
  }

  printf("Employee deleted successfully.\n");
  return STATUS_SUCCESS;
}


int send_hello(int fd) {
    char buf[4096] = {0};

    dbproto_hdr_t hdr;
    hdr.type = htons(MSG_HELLO_REQ);
    hdr.len  = htons(1);

    dbproto_hello_req hello_req;
    hello_req.proto = htons(PROTO_VERS);

    memcpy(buf, &hdr, sizeof(hdr));
    memcpy(buf + sizeof(dbproto_hdr_t),
           &hello_req,
           sizeof(dbproto_hello_req));

    write(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_req));

    ssize_t n = read(fd, buf, sizeof(buf));
    if (n < sizeof(dbproto_hdr_t)) {
        printf("Short read or connection closed\n");
        return STATUS_ERROR;
    }

    memcpy(&hdr, buf, sizeof(dbproto_hdr_t));
    hdr.type = ntohs(hdr.type);
    hdr.len  = ntohs(hdr.len);

    printf("Header type: %d\n", hdr.type);

    if (hdr.type == MSG_ERR) {
        printf("Protocol mismatch\n");
        return STATUS_ERROR;
    }

    printf("Server connected, protocol v1.\n");
    return STATUS_SUCCESS;
}

int main(int argc, char *argv[]) {
  char* addarg = NULL;
  char* portarg = NULL, *hostarg = NULL;
  unsigned short port = 0;
  bool list = false;
  char* removearg = NULL;

  int c;
  while ((c = getopt(argc, argv, "p:h:a:lr:")) != -1) {
    switch (c) {
      case 'a':
        addarg = optarg;
        break;
      case 'l':
        list = true;
        break;
      case 'p':
        portarg = optarg;
        port = atoi(portarg);
        break;
      case 'h':
        hostarg = optarg;
        break;
      case 'r':
        removearg = optarg;
        break;
      case '?':
        printf("Unknown option -%c\n", c);
      default:
        return -1;
    }
  }


  if (port == 0) {
    printf("Bad port: %s\n", portarg);
    return -1;
  }

  if (hostarg == NULL) {
    printf("Must specify host with -h\n");
    return -1;
  }

  struct sockaddr_in serverInfo = {0}; 
  serverInfo.sin_family = AF_INET;
  serverInfo.sin_addr.s_addr = inet_addr(hostarg);
  serverInfo.sin_port = htons(port);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    return -1;
  }

  if (connect(fd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1) {
    perror("connect");
    close(fd);
    return 0;
  }

  if (send_hello(fd) != STATUS_SUCCESS) {
    return -1;
  }

  char input[256];

while (true) {
    printf("Enter command (add NAME, list, remove NAME, quit): \n");
    if (!fgets(input, sizeof(input), stdin)) break;

    // Remove newline
    input[strcspn(input, "\n")] = 0;

    if (strcmp(input, "quit") == 0) break;
    else if (strncmp(input, "add ", 4) == 0) send_add_employee(fd, input+4);
    else if (strcmp(input, "list") == 0) send_list_req(fd);
    else if (strncmp(input, "remove ", 7) == 0) send_remove_req(fd, input+7);
    else printf("Unknown command\n");
}

  close(fd);

}
