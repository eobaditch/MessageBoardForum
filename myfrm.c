/* TCP/UDP MessageBoardForum Client
 * Emily Obaditch - eobaditc
 * Teddy Brombach - tbrombac
 * Matthew Reilly - mreill11
 *
 * This is our implementation of a MessageBoardForum Client, written in C.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mhash.h>
#include <limits.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/time.h>


#define BUFSIZE 4096

void error(char *msg);

int main(int argc, char *argv[]) {
    int XIT = 0;
    MHASH td; 
    char filecontent[BUFSIZE]; 
    struct sockaddr_in serveraddr;
    struct hostent *server;
    int udpsockfd, tcpsockfd, filesize, portno, n, k, serverlen;
    char *hostname;
    char *command;
    unsigned char * serverHash; 
    char name[BUFSIZE]; 
    char len [BUFSIZE]; 
    char buf[BUFSIZE];
    int len_s; 
    char key[BUFSIZE];
    int i = 0;

    /* check command line arguments */
    // CHANGE <text or file name>
    if (argc != 3) {
        fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
        exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);


    // Load buffer, 

    bzero(buf, BUFSIZE);
    // Create the socket 
    tcpsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpsockfd < 0){
        error("ERROR opening tcp socket");
    }

    udpsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (udpsockfd < 0){
        error("ERROR opening udp socket");
    }

    // Load DNS Entry
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    // Obtain server address
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect */
    if (connect(tcpsockfd,(struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0){
        error ("ERROR connecting"); 
    }

    while(1){
        printf("Enter Command: "); 
        scanf("%s", buf);  
        n = write(tcpsockfd, buf, strlen(buf)); 
        if(n<0)
            error("ERROR writing to socket");

		if (strcmp(buf, "XIT") == 0) {
			printf("The connection has been closed.\n");
			exit(0);
		}
        bzero(buf, BUFSIZE); 
    }

    return 0; 
}

// error handling
void error(char *msg) {
    perror(msg);
    exit(0);
}
