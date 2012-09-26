/*

nfs_client.c
ECSE 427 PA# 3
Christian Gallai
November 24, 2009

*/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nfs_api.h"
#define max_fd 64000

int empty_fd = -1;

struct sockaddr_in echoServAddr;
char request_msg_client[4];
char instr_msg_client[80];
char* echoStr, *servIP;
int echoServPort, echoStrLen;
int server_sockets = 0, initialize = 0;
int sock_id, request_client = 0;
int sock[5];
char *sock_filesys[5]={NULL,NULL,NULL,NULL,NULL};
struct sock_fd
{	int socket;
	int fd_remote;
	int valid;
};
struct sock_fd fd_s[max_fd];

// instruction identification char arrays
char ls_cmd[2] = "ls";
char open_cmd[4] = "open";
char close_cmd[5] = "close";
char write_cmd[5] = "write";
char read_cmd[4] = "read";
char remove_cmd[6] = "remove";

int find_socket(char *filesys){
	int n_sock, found_sock_id = 0;
	// search for the socket id
	for(n_sock = 0;n_sock<5;n_sock++){
		if(sock_filesys[n_sock] != NULL){
			if(strcmp(filesys, sock_filesys[n_sock])){
				found_sock_id = n_sock;
				break; 
			}
		}
	}
	return found_sock_id -1;
}

int find_empty_fd(){
	int n_fd, found_empty_fd = 0;
	for(n_fd=0;n_fd<max_fd;n_fd++){
		if(fd_s[n_fd].valid == 0){
			found_empty_fd = n_fd;
			break;
		}
	}
	return found_empty_fd;
}

// mount the network file system
int nfs_mount(char *filesys, char *host, int port){
	// intialize fd_s structure
	if(initialize == 0){
		memset(&fd_s, 0, sizeof(fd_s));
		initialize++;
	}

	// set initial variables
	servIP = host;
	echoStr = filesys;
	echoServPort = port;

	// set the socket id and filesys
	sock_id = server_sockets;
	server_sockets++;
	sock_filesys[server_sockets] = filesys;

	if(server_sockets == 4){
		server_sockets = 0;	
	}	
	
	// create the socket
	if((sock[sock_id] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0){
	    printf("Socket creation unsuccessful.\n");
	   return -1;
	}

	// construct the server address structure
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);
	echoServAddr.sin_port = htons(echoServPort);

	// establish connection to server
	if(connect(sock[sock_id], (struct sockaddr*) &echoServAddr, sizeof(echoServAddr)) < 0){
		printf("Unable to connect to the server.\n");
		return -1;
	}
	return 0;
}

int nfs_umount(char *filesys){
	int sock_umount_id = find_socket(filesys);
	close(sock[sock_umount_id]);
	return 0;
}

void nfs_ls(char *filesys){
	// find the socket
	int sock_ls = find_socket(filesys);

	// send request message to server
	request_client = 1;
	send(sock[sock_ls], &request_client, 4, 0);

	// receive filenames until  the valid bit at the start of the message is 0, indication end of send
	for (;;) {
		recv(sock[sock_ls], instr_msg_client, 80, 0);
		if(instr_msg_client[0] == '1'){
			// filename valid, print the filename
			printf("%s \n",&instr_msg_client[2]);
		} else{
			// send complete
			break;
		}
	}
}

int nfs_open(char *filesys, char *name){
	char file_to_open[13];
	int fd_return = 0;
	char buf_open[16];

	empty_fd++;

	// find the socket
	int sock_open = find_socket(filesys);

	// organize all send information
	request_client = 2;
	memcpy(buf_open, &request_client, 4);
	memcpy(&buf_open[4], name, 12);

	// send all open information to server
	send(sock[sock_open], buf_open, 16, 0);

	// receive the return
	recv(sock[sock_open], &fd_return, 4, 0);

	// set the fd struct variables
	//empty_fd = find_empty_fd();
	fd_s[empty_fd].socket = sock_open;
	fd_s[empty_fd].fd_remote = fd_return;
	fd_s[empty_fd].valid = 1;
	
	if(fd_return < 0){
		return -1;
	} else{
		return empty_fd;
	}
}

int nfs_close(int fd){
	char send_remote_fd[4];
	char instr_return_close[4];
	int close_return = 0;
	int fd_send_close = fd_s[fd].fd_remote;
	char buf_close[8];

	// check if fd is valid
	if ((fd<0) || (fd>max_fd)){
		printf("invalid fd requested for close.\n");
		return -1;
	} else if(fd_s[fd].valid == 0){
		printf("invalid fd requested for close.\n");
		return -1;
	}

	// organize send information
	request_client = 3;
	memcpy(buf_close, &request_client, 4);
	memcpy(&buf_close[4], &fd_send_close, 4);

	// send all close information to the server
	send(sock[fd_s[fd].socket], buf_close, 8, 0);

	// receive return statement
	recv(sock[fd_s[fd].socket], &close_return, 4, 0);

	if(close_return < 0){
		return -1;
	} else {
		return close_return;
	}
}

int nfs_write(int fd, char *buf, int length){
	char instr_return_write[4];
	int write_return = 0;
	int fd_send = fd_s[fd].fd_remote;
	char buf_write[12];

	// check if fd is valid
	if ((fd<0) || (fd>max_fd)){
		printf("invalid fd requested for close.\n");
		return -1;
	} else if(fd_s[fd].valid == 0){
		printf("invalid fd requested for close.\n");
		return -1;
	}

	// organize send information
	request_client = 4;
	memcpy(buf_write, &request_client, 4);
	memcpy(&buf_write[4], &length, 4);
	memcpy(&buf_write[8], &fd_send, 4);

	// send all write information
	send(sock[fd_s[fd].socket], buf_write, 12, 0);

	// send the character to be written to the file to the server
	send(sock[fd_s[fd].socket], buf, length, 0);

	// receive return statement	recv(sock[fd_s[fd].socket], &write_return , 4, 0);

	if(write_return < 0){
		return -1;
	} else {
		return write_return;
	}
}

int receive_read(int socket, void *buffer_read, int length){
	int rec_data = 0;
	while(rec_data < length){
		rec_data += recv(socket, (buffer_read + rec_data), (length - rec_data), 0);
	}
}

int nfs_read(int fd, char *buf, int length){
	char instr_return_read[4];
	char send_read_len[4];
	int read_return = 0, fd_read_send;
	char buf_read[12];
	fd_read_send = fd_s[fd].fd_remote;

	// check if fd is valid
	if ((fd<0) || (fd>max_fd)){
		printf("invalid fd requested for close.\n");
		return -1;
	} else if(fd_s[fd].valid == 0){
		printf("invalid fd requested for close.\n");
		return -1;
	}

	// organize send information
	request_client = 5;
	memcpy(buf_read, &request_client, 4);
	memcpy(&buf_read[4], &length, 4);
	memcpy(&buf_read[8], &fd_read_send, 4);

	// send all read informatio to the server
	send(sock[fd_s[fd].socket], buf_read, 12, 0);

	// receive the read characters from the server and store them in the buffer
	receive_read(sock[fd_s[fd].socket], buf, length);
	
	// receive return statement	recv(sock[fd_s[fd].socket], &read_return, 4, 0);

	if(read_return < 0){
		return -1;
	} else {
		return read_return;
	}
}

int nfs_remove(char *filesys, char *name){
	int remove_return = 0;
	char instr_return_remove[4];
	char name_send[13];
	char buf_remove[16];

	// find the socket
	int sock_remove = find_socket(filesys);

	// organize send information
	request_client = 6;
	memcpy(buf_remove, &request_client, 4);
	memcpy(&buf_remove[4], name, 12);

	// send all remove information to the server
	send(sock[sock_remove], buf_remove, 16, 0);

	// receive return statement
	recv(sock[sock_remove], &remove_return, 4, 0);
	if(remove_return < 0){
		return -1;
	} else {
		return remove_return;
	}
}

