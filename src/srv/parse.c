#include <stdio.h>
#include<stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "parse.h"
#include "common.h"

int update_hour(struct dbheader_t* dbhdr, struct employee_t* employees, char* updatehr){
	 char* name = strtok(updatehr,",");
	 int newHr = atoi(strtok(NULL,","));
	 int i=0;
	 for(;i<dbhdr->count;i++){
	 	if(strcmp(employees[i].name,name)==0){
	 		employees[i].hours = newHr;
	 		break;
	 	}
	 }

	 if(i==dbhdr->count){
	 	printf("Invalid name given\n");
	 	return STATUS_ERROR;
	 }

	 return STATUS_SUCCESS;
}

int remove_employee(struct dbheader_t* dbhdr, struct employee_t** employees, char* removestr){
	int place = -1;
	for(int i=0;i<dbhdr->count;i++){
		if(strcmp((*employees)[i].name,removestr)==0){
			place = i;
			break;
		}
	}

	if(place==-1){
		printf("Cannot found this employee\n");
		return STATUS_ERROR;
	}

	printf("Removing \n");

	for(int i=place;i<dbhdr->count-1;i++){
		(*employees)[i] = (*employees)[i+1];
	}

	dbhdr->count--;

	if(dbhdr->count == 0){
        free(*employees);
        *employees = NULL;
    }
/*
	struct employee_t* tmp = realloc(*employees,dbhdr->count*sizeof(struct employee_t));

	if(tmp!=NULL || dbhdr->count==0){
		*employees = tmp;
	}*/
    printf("removed\n");
	return STATUS_SUCCESS;
}

void list_employees(struct dbheader_t* dbhdr, struct employee_t* employees){
	for(int i=0;i<dbhdr->count;i++){
		printf("Employee %d\n",i);
		printf("\tName: %s\n",employees[i].name);
		printf("\tAddress: %s\n",employees[i].address);
		printf("\tHours: %d\n",employees[i].hours);
	}
}

int add_employee(struct dbheader_t* dbhdr,struct employee_t** employees,char* strtoadd){
	int coma_count = 0;
	for(int i=0;i<strlen(strtoadd);i++){
		if(strtoadd[i] == ','){
			coma_count++;
		}
	}

	if(coma_count != 2){
		return STATUS_ERROR;
	}
	dbhdr->count++;
	int count = (dbhdr)->count;

	printf("Adding \n");

	*employees = realloc(*employees,sizeof(struct employee_t) * count);
	
	if(*employees==NULL){
		printf("Malloc failed\n");
		return STATUS_ERROR;
	}

	

	char* name = strtok(strtoadd,",");
	char* address = strtok(NULL,",");
	int hours = atoi(strtok(NULL,","));

	if (!name || !address || !hours) {
    	return STATUS_ERROR;
	}

	strcpy((*employees)[count-1].name, name);
	strcpy((*employees)[count-1].address, address);
	(*employees)[count-1].hours	= hours;

	printf("Added \n");
  
	return STATUS_SUCCESS;
}


int read_employees(int fd,struct dbheader_t* dbhdr, struct employee_t** employeesOut){
	if(fd<0){
		printf("Invalid fd provided!\n");
		return STATUS_ERROR;
	}
	int count = dbhdr->count;

	struct employee_t* employees = calloc(count,sizeof(struct employee_t));
	if(employees == NULL){
		printf("Malloc failed\n");
		return STATUS_ERROR;
	}

	read(fd,employees,count*sizeof(struct employee_t));

	for(int i=0;i<count;i++){
		employees[i].hours = ntohl(employees[i].hours);
	}

	*employeesOut = employees;

	return STATUS_SUCCESS;
}

void output_file(int fd, struct dbheader_t* dbhdr, struct employee_t* employees){
	if(fd<0){
		printf("Invalid fd obtained\n");
		return STATUS_ERROR;
	}
	int count = dbhdr->count;
	dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->version = htons(dbhdr->version);
	dbhdr->count = htons(dbhdr->count);
	dbhdr->filesize = htonl(sizeof(struct dbheader_t) + sizeof(struct employee_t) * count);

	lseek(fd,0,SEEK_SET);
	write(fd,dbhdr,sizeof(struct dbheader_t));
	//printf("Header added!!\n");

	for(int i=0;i<count;i++){
		employees[i].hours = htonl(employees[i].hours);
		write(fd,&employees[i],sizeof(struct employee_t));
		employees[i].hours = ntohl(employees[i].hours);
	}

	dbhdr->magic = ntohl(dbhdr->magic);
	dbhdr->version = ntohs(dbhdr->version);
	dbhdr->count = ntohs(dbhdr->count);
	dbhdr->filesize = ntohl(sizeof(struct dbheader_t) + sizeof(struct employee_t) * count);

	ftruncate(fd,ntohl(dbhdr->filesize));

	return;
}

int create_db_header(int fd,struct dbheader_t** headerOut){
	struct dbheader_t* header = calloc(1,sizeof(struct dbheader_t));
	if(header==NULL){
		printf("Cannot create database header from malloc \n");
		return STATUS_ERROR;
	}
	header->magic = HEADER_MAGIC;
	header->version = 1;
	header->count = 0;
	header->filesize = sizeof(struct dbheader_t);

	*headerOut = header;
	return STATUS_SUCCESS;
}

int validate_db_header(int fd,struct dbheader_t** headerOut){
	if(fd < 0){
		printf("Wrong FD from the user\n");
		return STATUS_ERROR;
	}

	struct dbheader_t* header = calloc(1,sizeof(struct dbheader_t));
	if(header==NULL){
		printf("Cannot create database header from malloc \n");
		return STATUS_ERROR;
	}
	if(read(fd,header,sizeof(struct	dbheader_t)) != sizeof(struct dbheader_t)){
		perror(read);
		free(header);
		return STATUS_ERROR;
	}

	header->version = ntohs(header->version);
	header->count = ntohs(header->count);
	header->filesize = ntohl(header->filesize);
	header->magic = ntohl(header->magic);

	if(header->magic != HEADER_MAGIC){
		printf("Invalid header magic\n");
		free(header);
		return STATUS_ERROR;
	}

	if(header->version != 1){
		printf("Invalid version\n");
		free(header);
		return STATUS_ERROR;
	}

	struct stat dbstat = {0};
	fstat(fd,&dbstat);

	if(header->filesize != dbstat.st_size){
		printf("Corrupted database\n");
		free(header);
		return STATUS_ERROR;
	}

	*headerOut = header;
	return STATUS_SUCCESS;
}
