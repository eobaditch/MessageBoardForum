/* TCP/UDP MessageBoardForum Server
 * Emily Obaditch - eobaditc
 * Teddy Brombach - tbrombac
 * Matthew Reilly - mreill11
 *
 * This is our implementation of a MessageBoardForum Server, written in C.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <mhash.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include "uthash.h"

#define BUFSIZE 4096

// error handling
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) {
    int tcpsockfd;                      // tcp socket
    int udpsockfd;                      // udp socket
    int sockfd2;
    MHASH td; 
    int port;                           // port number
    int clientlen;                      // byte size of client's address
    struct sockaddr_in serveraddr;      // server's addr
    struct sockaddr_in clientaddr;      // client addr
    struct timeval start_t, end_t; 
    struct hostent *hostp;              // client host info
    char buf[BUFSIZE];                  // message buffer
    char *hostaddrp;                    // formatted host addr string
    int optval;                         // flag value for setsockopt
    int n, k, i;                        // message size, key size, counter
    short len;
    char *name;
    char *len_string; 
    unsigned char * serverHash; 
    char com[BUFSIZE];
    char* path, filename, password;
    char choice[BUFSIZE]; 

    // parse command line arguments
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port> <password>\n", argv[0]);
        exit(1);
    }

    // Store command line arguments
    port = atoi(argv[1]);
    password = argv[2];

    // create the TCP socket
    tcpsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpsockfd < 0) 
        error("ERROR opening tcp socket");

    // create the UDP socket
    udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpsockfd < 0) 
        error("ERROR opening udp socket");

    // setsockopt: rerun server faster
    optval = 1;
    setsockopt(tcpsockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    setsockopt(udpsockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    // build server internet address
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);

    // bind the parent TCP socket and port
    if (bind(tcpsockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
        error("ERROR on tcp binding");

    if (bind(udpsockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        error("ERROR on udp binding");

    // Listen 
    while(1){

        if (listen(tcpsockfd, 5) < 0)
            error("Error on binding");

        // Wait for message, send response
        clientlen = sizeof(clientaddr);
    
        // Accept tcp socket
        sockfd2 = accept( tcpsockfd, (struct sockaddr *) &clientaddr, &clientlen);
        if(sockfd2 < 0)
            error("Error on tcp accept");
        
        hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);

        if (hostp == NULL)
            error("ERROR on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if (hostaddrp == NULL)
            error("ERROR on inet_ntoa\n");

        printf("Server connected with %s (%s)\n", hostp->h_name, hostaddrp);

        bzero(buf, BUFSIZE);
        strcpy(buf, "Enter admin password: ");
        n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0)
            error("ERROR in pass request");

        while(1) {
            // receive a datagram from a client
            bzero(buf, BUFSIZE);
            n = read(sockfd2, buf, BUFSIZE);
            strcpy(com, buf);
			
            if(n < 0)
               error("Error reading from socket");
        
			if (strcmp(com, "XIT") == 0) {
				close(sockfd2);
				break;
			}

            n = write(sockfd2, buf, strlen(buf));

            if(n < 0)
                error("Error reading from socket");

        }
    }
}
