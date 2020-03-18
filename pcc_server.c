#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#define SUCCESS 0
#define FAIL 1
#define HASHSIZE 94 /*126-32*/
#define BUFFSIZE 1024

/*****global var*****/
unsigned long pcc_total[HASHSIZE];
int sigint_sent = 0;
int handeling_client = 0;
/********************/

int isPrintable(char b){
	if(b >= 32 && b <= 126){
		return 1;
	}
	return 0;
}

void print_and_exit(){
	int i;

	for(i=0; i<HASHSIZE; i++){
		printf("char ’%c’ : %u times\n",(char) i+32 ,(unsigned int) pcc_total[i]);
	}
	exit(SUCCESS);
}

void signal_handler(int sig){
	sigint_sent = 1;
	if(!handeling_client){
		print_and_exit();
	}
}

int handel_error(){
	if(errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
		printf("accepting new client connections...\n");
		handeling_client = 0;
		return 1;
	}
	else{
		exit(FAIL);
		return 0;
	}
}

int main(int argc, char *argv[]){
	unsigned short serverPort;
	unsigned long pcc_temp[HASHSIZE];
	unsigned long fileSize, printableChar;
	char recv_buff[BUFFSIZE];
	int i, bytes_read, totalread, notwritten;
	int totalsent = -1;
	int nsent     = -1;
	int listenfd  = -1;
	int connfd    = -1;
	int on = 1;
	
	struct sigaction sa;
	struct sockaddr_in serv_addr;
	struct sockaddr_in peer_addr;
	socklen_t addrsize = sizeof(struct sockaddr_in );
	
	/*define handeling of SIGINT*/
	sa.sa_handler = &signal_handler;
	sigaction(SIGINT, &sa, NULL);
	
	if(argc != 2){
		fprintf(stderr, "Server Error : Wrong number of arguments\n");
		return FAIL;
	}
	
	serverPort = atoi(argv[1]);
	if(serverPort < 0){
		fprintf(stderr, "Server Error : atoi() failed on %s\n", argv[1]);
		return FAIL;
	}
	/*initialize pcc_total*/
	for(i=0; i<HASHSIZE; i++){
		pcc_total[i] = 0;
	}
	/*fill recv_buff with 0*/
	memset(recv_buff, 0, sizeof(recv_buff));

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, addrsize);

	serv_addr.sin_family = AF_INET;
	/*INADDR_ANY = any local machine address*/
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(serverPort);
	
	/*set SO_REUSEADDR, on = 1*/
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		perror("Server Error : set SO_REUSEADDR Failed.\n");
		return FAIL;
	}

	if(0 != bind(listenfd, (struct sockaddr*) &serv_addr, addrsize)){
		perror("Server Error : Bind Failed.\n");
		return FAIL;
	}

	if(0 != listen( listenfd, 10)){
		perror("Server Error : Listen Failed.\n");
		return FAIL;
	}
	
	while(1){
		/*goto label*/
		ACCEPTNEWCLIENT:
		
		if(sigint_sent){
			print_and_exit();
		}
		connfd = accept(listenfd, (struct sockaddr*) &peer_addr, &addrsize);
		/*define handeling of SIGINT*/
		handeling_client = 1;
		
		if(connfd < 0){
			perror("Server Error : Accept Failed.\n");
			return FAIL;
		}
		
		totalread = 0;
		while(totalread < sizeof(fileSize)){
			bytes_read = read(connfd, recv_buff + totalread, sizeof(fileSize) - totalread);
			if(bytes_read <= 0){
				perror("Server Error : Read Failed reciving N.\n");
				close(connfd);
				if(handel_error()){
					goto ACCEPTNEWCLIENT;
				}
			}
			totalread += bytes_read;
		}
		memcpy(&fileSize, recv_buff, sizeof(fileSize));
		fileSize = ntohl(fileSize);
		
		/*fill recv_buff & with 0*/
		memset(recv_buff, 0, sizeof(recv_buff));
		totalread = 0;
		/*init pcc_temp & printableChar*/
		printableChar = 0;
		for(i=0; i<HASHSIZE; i++){
			pcc_temp[i] = 0;
		}
		while(totalread < fileSize){
			bytes_read = read(connfd, recv_buff , sizeof(recv_buff));
			if(bytes_read <= 0){
				perror("Server Error : Read Failed while reading File.\n");
				close(connfd);
				if(handel_error()){
					goto ACCEPTNEWCLIENT;
				}
			}
			for(i = 0; i < bytes_read; i++){
				if(isPrintable(recv_buff[i])){
					printableChar++;
					pcc_temp[recv_buff[i]-32]++;
				}
			}
			totalread += bytes_read;
		}
		/*write C = printableChar to client*/
		printableChar = htonl(printableChar);
		notwritten = sizeof(printableChar);
		totalsent = 0;
		while(notwritten > 0){
			nsent = write(connfd, &printableChar + totalsent, notwritten);
			if(nsent < 0){
				perror("Server Error : Write Failed sending C.\n");
				close(connfd);
				if(handel_error()){
					goto ACCEPTNEWCLIENT;
				}
			}
			totalsent  += nsent;
			notwritten -= nsent;
		}
		
		/*update pcc_total*/
		for(i=0; i<HASHSIZE; i++){
			pcc_total[i] += pcc_temp[i];
		}

		close(connfd);
		handeling_client = 0;
	}
}

