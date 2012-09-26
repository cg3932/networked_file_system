/*

nfs_server.c
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
#include <getopt.h>
#include "sfs_api.h"


struct sockaddr_in echoServAddr, echoClntAddr;
char* init_msg = "123456789";
char request_msg[4];
char instr_msg_server[80];
int servSock, clntLen, clntSock;
int echoServPort = 5000;
int req_id, exec_error;
extern char *filename;

// instruction identification char arrays
char ls_cmd[2] = "ls";
char open_cmd[4] = "open";
char close_cmd[5] = "close";
char write_cmd[5] = "write";
char read_cmd[4] = "read";
char remove_cmd[6] = "remove";


// perform required execution for command "ls"
void ls_exec(){
	// send the list names
	nfs_ls(clntSock);
	
	// send eof message and return
	instr_msg_server[0] = '0';
	send(clntSock, instr_msg_server, 80, 0);
}

// perform required execution for command "open"
void open_exec(char *name){
	char send_return_open[4];
	int open_file = 0;

	// open the file
	open_file = sfs_open(name);

	// send the return to the client
	send(clntSock, &open_file, 4, 0);
}

// perform required execution for command "close"
void close_exec(int rqst_fd){
	int close_file = 0;

	// close the file
	close_file = sfs_close(rqst_fd);

	// send the return to the client
	send(clntSock, &close_file, 4, 0);
}

int receive_write(int socket, void *buffer_write, int length){
	int rec_data = 0;
	while(rec_data < length){
		rec_data += recv(socket, (buffer_write + rec_data), (length - rec_data), 0);
	}
}

// perform required execution for command "write"
void write_exec(int write_len, int fd_write){
	int send_return_write = 0;
	char *buf = malloc(write_len);

	// receive the characters to write
	receive_write(clntSock, buf, write_len);
	send_return_write = sfs_write(fd_write, buf, write_len);

	// send the return statement
	send(clntSock, &send_return_write, 4, 0);
}

// perform required execution for command "read"
void read_exec(int read_len, int fd_read){
	int send_return_read = 0;
	char *buf = malloc(read_len);

	// send the read characters to the client
	send_return_read = sfs_read(fd_read, buf,read_len);
	send(clntSock, buf, read_len, 0);

	// send the return statement
	send(clntSock, &send_return_read, 4, 0);
}

// perform required execution for command "remove"
void remove_exec(){
	char remove_filename[12];
	int remove_file = 0;

	// receive the filename
	recv(clntSock, remove_filename, 12, 0);

	// remove the file
	remove_file = sfs_remove(remove_filename);

	// send the return to the client
	send(clntSock, &remove_file, 4, 0);
}
static int fresh_flag = 0;

int main (int argc, char **argv)
{
	int c, port = 8010;
	int rqst_ident;
     
	while (1)
	{
		static struct option long_options[] =
		{
			/* These options set a flag. */
			{"existing", no_argument,       &fresh_flag, 0},
			{"fresh",   no_argument,       &fresh_flag, 1},
			/* These options don't set a flag.
			  We distinguish them by their indices. */
			{"port",  required_argument, 0, 'p'},
			{"filename",  required_argument, 0, 'f'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "p:f:",
			    long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
		break;

		switch (c)
		{
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
				 break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
				 printf (" with arg %s", optarg);
				printf ("\n");
			break;

			case 'p':
				port = atoi(optarg);
				break;

			case 'f':
				filename = strdup(optarg);
				break; 

			case '?':
				/* getopt_long already printed an error message. */
				printf("Usage: nfs_server [OPTION]\n");
				printf("Disk options:\n");
				printf("      --existing		Use an existing file system\n");
				printf("      --fresh			Create a new file system\n\n");
				printf("Server options:\n");
				printf("  -p, --port=PORT		Listen on this port\n");
				printf("  -f, --filename=FILENAME	Listen on this port\n");
				exit(1);

			default:
				abort ();
		}
	}

	
	// create socket for connections
	if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		  printf("Socket creation unsuccesful.\n");
		  exit(1);
	}
	
	// construct the server address structure
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	echoServAddr.sin_port = htons(port);

	// bind to local address                                                                                  
	if (bind(servSock, (struct sockaddr*) &echoServAddr, sizeof(echoServAddr)) < 0) {
		   printf("Unable to bind to the socket.\n");
		   exit(1);
	}
	
	// listen for incoming connections
	if (listen(servSock, 5) < 0) {
		     printf("Failure in listening to the port.\n");
		     exit(1);
	}

	// make the file system
	// mksfs(fresh_flag);
	
	for (;;) {
                                                             
	     	// set the size of the in-out parameter
	    	clntLen = sizeof(echoClntAddr);
	     	// wait for a client to connect
		printf("waiting for connection\n");
	     	if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0) {
	              	printf("Failure in accepting the connection from a client.\n");
			exit(1);
	     	}

		mksfs(1);
 
		while(1){
			// receive instruction request from client
			rqst_ident = 0;
			int bytes = recv(clntSock, &rqst_ident, 4, 0);
	
			// identify instruction and initiate appropriate execution method
			if(rqst_ident == 1){

				// initiate ls operation
				ls_exec();

			} else if(rqst_ident == 2){
				
				// initiate open operation
				char open_filename[12];
				int bytes2 = recv(clntSock, open_filename, 12, 0);
				open_exec(open_filename);

			} else if(rqst_ident == 3){

				// initiate close operation
				int receive_fd;
				recv(clntSock, &receive_fd, 4, 0);
				close_exec(receive_fd);

			} else if(rqst_ident == 4){

				// initiate write operation
				int l_write = 0, fd_write = 0;
				recv(clntSock, &l_write, 4, 0);
				recv(clntSock, &fd_write, 4, 0);
				write_exec(l_write, fd_write);

			} else if(rqst_ident == 5){

				// receive the length from the client
				int l_read = 0, fd_rd = 0;
				recv(clntSock, &l_read, 4, 0);
				recv(clntSock, &fd_rd, 4, 0);
				read_exec(l_read, fd_rd);

			} else if(rqst_ident == 6){

				remove_exec();

			} else {
				// return of -1 and thus error
				printf("Failure in identifying client request.\n");
				exit(1);
			}
		}	
	}	
}

