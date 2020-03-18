#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>

#define SUCCESS 0
#define FAIL 1
#define BUFFSIZE 1024

bool isValidIpAddress(char *ipAddress){
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

int getFileSize(const char* filename){
	struct stat st;
	if(stat(filename, &st) < 0){
		perror("Client Error : getFileSize: fstat() Failed.\n");
		exit(FAIL);
	}
	return st.st_size;
}

int main(int argc, char *argv[]){
	unsigned long fileSize, printableChar;
	unsigned short serverPort;
	char data_buff[BUFFSIZE];
	char recv_buff[BUFFSIZE];
	char* ipAddress;
	FILE * file;
	int  totalsent, totalread, nsent, nread, notwritten;
	int  sockfd     = -1;
	int  bytes_read =  0;	

	struct sockaddr_in serv_addr; 
	
	if(argc != 4){
		fprintf(stderr, "Client Error : Wrong number of arguments\n");
		return FAIL;
	}
	
	
	ipAddress = argv[1];
	if(!isValidIpAddress(ipAddress)){
		fprintf(stderr, "Client Error : %s Is not a valid ip\n", argv[1]);
		return FAIL;
	}
	serverPort = atoi(argv[2]);
	if(serverPort < 0){
		fprintf(stderr, "Client Error : atoi() failed on %s\n", argv[2]);
		return FAIL;
	}
	file = fopen(argv[3], "r");
	if(!file){
		fprintf(stderr, "Client Error : fopen() on %s failed : %s\n", argv[3], strerror(errno));
		return FAIL;
	}
	/*fill recv_buff & data_buff with 0*/
	memset(recv_buff, 0, sizeof(recv_buff));
	memset(data_buff, 0, sizeof(data_buff));
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("Client Error : Could not create socket.\n");
		return FAIL;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverPort);
	serv_addr.sin_addr.s_addr = inet_addr(ipAddress);

	/*connect socket to the target address*/
	if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
		perror("Client Error : Connect Failed.\n");
		return FAIL;
	}

	/*write N = fileSize to server*/
	fileSize = htonl(getFileSize(argv[3]));
	notwritten = sizeof(fileSize);
	totalsent = 0;
	while(notwritten > 0){
		nsent = write(sockfd, &fileSize + totalsent, notwritten);
		if(nsent < 0){
			perror("Client Error : Write Failed sending N.\n");
			return FAIL;
		}
		totalsent  += nsent;
		notwritten -= nsent;
	}
	
	/*write file to server*/
	notwritten = getFileSize(argv[3]);
	while(notwritten > 0){
		/*read BUFFSIZE bytes from file*/
		nread = fread(data_buff, 1, BUFFSIZE, file);
		totalsent = 0;
		while(totalsent < nread){
			nsent = write(sockfd, data_buff + totalsent, nread - totalsent);
			if(nsent < 0){
				perror("Client Error : Write Failed sending File.\n");
				return FAIL;
			}
			totalsent  += nsent;
			notwritten -= nsent;
		}
	}
	/*read data from server into recv_buff*/
	totalread = 0;
	while(totalread < sizeof(printableChar)){
		bytes_read = read(sockfd, recv_buff + totalread, sizeof(printableChar) - totalread);
		if(bytes_read <= 0){
			perror("Client Error : Read Failed reciving C.\n");
			return FAIL;
		}
		totalread += bytes_read;
	}
	memcpy(&printableChar, recv_buff, sizeof(printableChar));
	printableChar = ntohl(printableChar);
	printf("# of printable characters: %u\n", (unsigned int) printableChar);

	close(sockfd);
	fclose(file);
	return SUCCESS;
}

