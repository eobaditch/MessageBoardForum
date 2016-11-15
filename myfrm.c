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
void readFile(char * dest, char * fname); 
void writeFile(char *, char *);

int main(int argc, char *argv[]) {
    int XIT = 0;
    MHASH td; 
    char filecontent[BUFSIZE]; 
    char filename[BUFSIZE]; 
    struct sockaddr_in serveraddr;
    struct hostent *server;
    int udpsockfd, tcpsockfd, filesize, portno, n, k, serverlen;
    char *hostname;
    char *command;
    unsigned char * serverHash; 
    int size; 
    char name[BUFSIZE]; 
    char dwn_name[BUFSIZE]; 
    char username[BUFSIZE]; 
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

    udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
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
    //START AND GET USERNAME
        printf("Type 'Start' to begin: ");  
        bzero(buf, BUFSIZE); 
        scanf("%s", buf); 
        n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)); //write(tcpsockfd, buf, strlen(buf)); 
        n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen); 
        printf("%s", buf); 
        scanf("%s", username); 
        bzero(buf, BUFSIZE); 
        strcpy(buf, username); 
        n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
        if (n<0)
            error("Error in sending username\n"); 
        
        n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
        if(n<0)
            error("Error in receving password request\n"); 
        printf("%s\n", buf); 
        bzero(buf, BUFSIZE); 
        //read and send password
        scanf("%s", buf); 
        n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
        if (n<0)
            error("Error in sending password\n"); 


    while(1){
        //GETUSERNAME
        bzero(buf, BUFSIZE); 

        printf("Enter Command: "); 
        scanf("%s", buf);  
        n = write(tcpsockfd, buf, strlen(buf)); 
        if(n<0)
            error("ERROR writing to socket");

		if (strcmp(buf, "XIT") == 0) {
			printf("The connection has been closed.\n");
			exit(0);
		} else if (strcmp(buf, "SHT") == 0) {
            bzero(buf, BUFSIZE);
            printf("Please enter the admin password: ");
            scanf("%s", buf);
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
            if (n < 0)
                error("Error in password udp sendto");

            // receive message from server
            bzero(buf, BUFSIZE);
            n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen); 
            if (n < 0)
                error("Error in recieving shutdown confirmation\n");

            if (strcmp(buf, "Password incorrect.") == 0) {  // Password incorrect
                printf("%s\n", buf);
            } else {                                        // Password correct
                printf("Shutting down.\n");
                close(udpsockfd);
                close(tcpsockfd);
                exit(0);
            }
        } else if(strcmp(buf, "CRT") == 0){
            //Create Board
            bzero(buf, BUFSIZE); 
            printf("Please enter the name of the board to be created: "); 
            scanf("%s", buf); 
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
            if (n<0)
                error("Error in creating board\n"); 
            //receive confirmation or error
            bzero(buf, BUFSIZE); 
            n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen); 
            if (n<0)
                error("Error in recieving board creation confirmation\n");
            printf("%s\n", buf); 

        } else if (strcmp(buf, "MSG") == 0){
            //Leave Message
            bzero(buf, BUFSIZE); 
            printf("Enter the name of the board to leave a message on: "); 
            scanf("%s", buf); 
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
            if(n<0)
                error("Error in sending board to leave message\n"); 
            bzero(buf, BUFSIZE); 
            printf("Enter the message you would like to leave: \n"); 
            scanf(" %[^\n]s ", buf); 
            strcat(buf, username); 
            printf("%s\n", buf); 
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
            if(n<0)
                error("Error in sending message\n"); 
            bzero(buf, BUFSIZE); 
             n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen); 
            if (n<0)
                error("Error in recieving MSG confirmation\n");
            printf("%s\n", buf);
        
        } else if (strcmp(buf, "DLT") == 0){
            //delete message
            bzero(buf, BUFSIZE); 
            printf("Enter the name of the board to delete a message off: "); 
            scanf("%s", buf); 
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
            if(n<0)
                error("Error in sending board to delete message\n"); 
            bzero(buf, BUFSIZE); 
            printf("Enter the message you would like to delete: \n"); 
            scanf(" %[^\n]s ", buf); 
            sprintf(buf, "%s %s", buf, username); 
            printf("%s\n", buf); 
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
            if(n<0)
                error("Error in sending message to delete\n"); 
            bzero(buf, BUFSIZE); 
             n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen); 
            if (n<0)
                error("Error in recieving DLT confirmation\n");
            printf("%s\n", buf);
        } else if (strcmp(buf, "DST") == 0){
            //Delete board
            bzero(buf, BUFSIZE); 
            printf("Enter the name of the board to be destroyed: "); 
            scanf("%s", &buf);         
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
            if(n<0)
                error("Error in sending board to destroy\n"); 
            bzero(buf, BUFSIZE); 
            n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen); 
            if (n<0)
                error("Error in recieving board DST confirmation\n"); 
            printf("%s\n", buf); 
        
        } else if (strcmp(buf, "EDT") == 0){
            //Edit Message Board
            bzero(buf, BUFSIZE); 
            printf("Enter the name of the board to be edited: "); 
            scanf("%s", &buf);         
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
            if(n<0)
                error("Error in sending board to edit\n"); 
            bzero(buf, BUFSIZE); 
            printf("Enter the message you would like to edit: \n"); 
            scanf(" %[^\n]s ", buf); 
           // strcat(buf, username); 
            printf("%s\n", buf); 
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
            if(n<0)
                error("Error in sending message to edit\n"); 
            bzero(buf, BUFSIZE); 
            printf("Enter the message you would like to leave: \n"); 
            scanf(" %[^\n]s ", buf);
            sprintf(buf, "%s ", buf); 
            strcat(buf, username); 
            printf("%s\n", buf); 
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)); 
            if(n<0)
                error("Error in sending message\n"); 
            bzero(buf, BUFSIZE); 
            n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
            if(n<0)
                error("Error in receving EDT confirmation\n"); 
            printf("%s\n", buf); 
        } else if (strcmp(buf, "LIS") == 0){
            //List Boards
            bzero(buf, BUFSIZE); 
            n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
            if(n<0)
                error("Error in receving password request\n"); 
            printf("%s\n", buf); 
        } else if (strcmp(buf, "RDB") == 0){
            //Read a board
			bzero(buf, BUFSIZE);
			printf("Enter the name of the board to read: ");
			scanf("%s", buf);
			n = write(tcpsockfd, buf, sizeof(buf));
			if (n < 0)
				error("ERROR sending name of board\n");

			bzero(buf, BUFSIZE);
			n = read(tcpsockfd, buf, sizeof(buf));
			if (n < 0)
				error("Error receiving confirmation.\n");
			printf("Filesize: %s\n", buf);
			if (strcmp(buf, "-1") == 0) { 				// Board does not exist
				printf("Board does not exist.\n");
			} else { 									// Read board
				int fileSize = atoi(buf);
				bzero(buf, BUFSIZE);
				int rounds;
				if (fileSize < 4096) {
					n = read(tcpsockfd, buf, BUFSIZE);
					if (n < 0)
						error("Error reading file.\n");
					printf("%s", buf);
				} else {
					rounds = (fileSize + 4095) / 4096;
					int round_num = 0, i, j;
					for (i = 0; i < rounds; i++) {
						n = read(tcpsockfd, buf, BUFSIZE);
						if (n < 0)
							error("Error reading big file.\n");
						printf("%s", buf);
						bzero(buf, BUFSIZE);
					}
					printf("\n");
				}
			}
        } else if (strcmp(buf, "APN") == 0){
            //Append file
            struct stat st;
            bzero(buf, BUFSIZE); 
            printf("Name of board to append to: "); 
            scanf("%s", buf); 
            n = write(tcpsockfd, buf, strlen(buf)); 
            if(n<0)
                error("Error in sending name of board to append to\n"); 
            bzero(buf, BUFSIZE); 

            printf("Name of file to be appended: "); 
            scanf("%s", buf);
            strcpy(filename, buf); 
            stat(filename, &st); 
            n = write(tcpsockfd, buf, strlen(buf)); 
            if(n<0)
                error("Error in sending filename for APN\n"); 
            bzero(buf, BUFSIZE); 
            n = read(tcpsockfd, buf, BUFSIZE); 
            if (n < 0)
                error("Error in receiving APN confirmation\n"); 
            printf("%s\n", buf);
            bzero(buf, BUFSIZE);
            size = st.st_size; 
            sprintf(buf, "%d", size); 
            n = write(tcpsockfd, buf, BUFSIZE); 
            if(n<0)
                error("Error in sending filesize\n"); 
            //send file
            for(i = 0; i< ((BUFSIZE + size -1 )/BUFSIZE); i++){
                bzero(buf, BUFSIZE); 
                readFile(buf, filename); 
                n = write(tcpsockfd, buf, strlen(buf)); 
                if(n < 0)
                    error("Error in sending APN file\n"); 
            }
        } else if (strcmp(buf, "DWN") == 0){
            //Download file
            bzero(buf, BUFSIZE); 
            printf("Name of board to download from: "); 
            scanf("%s", buf); 
            strcpy(name, buf); //board name 
            n = write(tcpsockfd, buf, strlen(buf)); 
            if(n<0)
                error("Error in sending name of board to download from\n"); 
            bzero(buf, BUFSIZE); 
            printf("Name of file to be downloaded: "); 
            scanf("%s", buf);
            strcpy(filename, buf); //filename 
            sprintf(dwn_name, "%s-%s", name, filename); //naming convention
            n = write(tcpsockfd, buf, strlen(buf)); 
            if(n<0)
                error("Error in sending filename for DWN\n"); 
            bzero(buf, BUFSIZE);
            n = read(tcpsockfd, buf, BUFSIZE); 
            if(n<0)
                error("Error in receiving filesize for DWN\n"); 
            size = atoi(buf); 
            if(size < 0)
                printf("Error in file existence\n"); 
            else{
                for(i = 0; i< ((BUFSIZE + size -1) / BUFSIZE); i++){
                    bzero(buf, BUFSIZE); 
                    n = read(tcpsockfd, buf, BUFSIZE); 
                    if (n<0)
                        error("Error in downloading file\n"); 
                    writeFile(dwn_name, buf); 

                }
            }
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

void readFile(char *dest, char *fname){

    FILE *fp = fopen(fname, "r"); 
    if(fp!=NULL){
        size_t new_len = fread(dest, sizeof(char), BUFSIZE, fp); 
        if(ferror(fp) != 0){
            fputs("Error reading file", stderr); 
        } else {
            dest[new_len++] = '\0'; 
        }
        fclose(fp); 
    }

}

void writeFile(char * fname, char * content){

    FILE *fp; 
    fp = fopen(fname, "a"); 
    fprintf(fp, "%s\n", content); 
    fclose(fp);

}
