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
#define MAX_BOARDS 100

// error handling
void error(char *msg) {
  perror(msg);
  exit(1);
}

int destroyBoard(char *name, char * username); 
int createFile(char *name, char * username); 
void readFile(char *dest, char *fname); 
int checkUser(char*username); 
int login(char * username, char * password, int newUser); 
void addBoard(char * name);
void deleteFiles();
bool has_txt_extension(char const *name);
void update_boards(char * boards[MAX_BOARDS], int boardCount); 
int add_message(char * board, char * message); 

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
    int newUser; 
    char name[BUFSIZE];
    char message[BUFSIZE]; 
    char username[BUFSIZE]; 
    char *len_string; 
    unsigned char * serverHash; 
    char com[BUFSIZE];
    char password[BUFSIZE]; 
    char* path, filename;
    char choice[BUFSIZE]; 
    char * boards[MAX_BOARDS]; 
    int boardCount = 0; 
	char adminPassword[BUFSIZE];

    // parse command line arguments
    if (argc != 3) {
        fprintf(stderr, "usage: %s <port> <password>\n", argv[0]);
        exit(1);
    }

    // Store command line arguments
    port = atoi(argv[1]);
	strcpy(adminPassword, argv[2]);
	printf("%s\n", adminPassword);

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
        n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr*)&clientaddr, &clientlen);  
        if (strcmp(buf, "Start") == 0){
            bzero(buf, BUFSIZE); 
            strcpy(buf, "Enter username: "); 
            n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen); 
            if (n<0)
            error("Error in sending username request\n"); 
            bzero(buf, BUFSIZE); 
            n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr*)&clientaddr, &clientlen);      
            sprintf(username, "%s", buf);  
            printf("%s\n", username); 
            //check if username exists
            
            if(!checkUser(username)){
                bzero(buf, BUFSIZE); 
                strcpy(buf, "Create password: "); 
                n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
                if(n<0)
                    error("Error in requesting password\n"); 
                bzero(buf, BUFSIZE); 
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr*)&clientaddr, &clientlen);    
                if(n<0)
                    error("Error in recieving password\n"); 
                strcpy(password, buf); 
                printf("%s pw: %s\n", username, password); 
                newUser = 1;  
            }
            else{
                bzero(buf, BUFSIZE); 
                strcpy(buf, "Enter password: "); 
                n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
                if(n<0)
                    error("Error in requesting password\n"); 
                bzero(buf, BUFSIZE); 
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr*)&clientaddr, &clientlen);    
                if (n<0)
                    error("Error in recieving password\n"); 
                strcpy(password, buf); 
                printf("%s pw: %s\n", username, password); 
                newUser = 0; 
            }
            printf("newUser (yes =1): %d\n", newUser); 
            int valid_login = login(username, password, newUser); 
            printf("%d\n", valid_login); 
            if(valid_login == 0){
                printf("login invalid\n");
                exit(1); 
            }
        }//end Start blcok
        
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
		    } else if (strcmp(com, "SHT") == 0) {
                //Shut down
                // receive admin password from client
                bzero(buf, BUFSIZE);
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
                printf("%s\n", buf);
				if (n < 0)
                    error("ERROR in receiving password");

               // bzero(buf, BUFSIZE);
                if (strcmp(adminPassword, buf) == 0) {       // Password correct
                    bzero(buf, BUFSIZE);
					strcpy(buf, "Password correct. Shutting down.");
                    n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
                    if (n < 0)
                        error("ERROR in password correct, shutting down message");

                    // Shut down server - delete all board files and appended files, close all sockets
                    deleteFiles();
					int i;
					for (i = 0; i < boardCount; i++) {
						// delete files	
					}
                    bzero(boards, MAX_BOARDS);
                    boardCount = 0;
                    close(udpsockfd);
                    close(tcpsockfd);
                    break;
                } else {
					bzero(buf, BUFSIZE);
                    error("PASSWORD INCORRECT");
                    strcpy(buf, "Password incorrect.");
                    n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
                    if (n < 0)
                        error("ERROR in password incorrect message");
                }

            } else if(strcmp(com, "CRT") == 0){
                //Create Board
                bzero(buf, BUFSIZE); 
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
                if (n<0)
                    error("Error in creating board\n"); 
                if(createFile(buf, username) == 0){
                    //successfully created board
                    strcpy(name, buf); 
                    bzero(buf, BUFSIZE); 
                    strcpy(buf, "successfully created board"); 
                    n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen); 
                    if (n < 0)
                        error("Error in sending board creation confirmation\n");
                    //keep track of boards created in current program
                    boards[boardCount] = name; 
                    boardCount++;
                    //add board to full list
                    addBoard(name); 

                } else{
                    //error in creating board
                    bzero(buf, BUFSIZE); 
                    strcpy(buf, "error in creating board"); 
                    n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen); 
                    if(n<0)
                        error("Error in sending board creation error\n"); 
                }
            } else if (strcmp(com, "MSG") == 0){
                //Leave Message
                bzero(buf, BUFSIZE); 
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
                if(n<0)
                    error("Error in receiving board name\n"); 
                strcpy(name, buf);
                printf("%s\n", name); 
                bzero(buf, BUFSIZE); 
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
                if(n<0)
                    error("Error in receiving message name\n"); 
                strcpy(message, buf);
                printf("%s\n", message); 
                bzero(buf, BUFSIZE); 
                int message_result = add_message(name, message); 
                if(message_result){
                    strcpy(buf, "Succes in posting message\n"); 
                }else{
                    strcpy(buf, "Error in posting message\n"); 
                }
                n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen); 
                if(n<0)
                    error("Error in sending MSG confirmation\n");
            
            } else if (strcmp(com, "DLT") == 0){
                //delete message
            } else if (strcmp(com, "DST") == 0){
                //Delete board
                bzero(buf, BUFSIZE); 
                //get name of board to be destroyed
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
                if(n<0)
                    error("Error in receiving board name\n"); 
                int dst = destroyBoard(buf, username);  
                for(i = 0; i<boardCount; i++){
                    printf("%s\n", boards[i]); 
                    if (strcmp(boards[i],buf)==0){
                            boards[i] = '\0'; 
                            boardCount --; 
                    }
                }
                bzero(buf, BUFSIZE); 
                if (dst){
                    update_boards(boards, boardCount); 
                    strcpy(buf, "Successfully destroyed\n"); 
                }else{
                    strcpy(buf, "Error in destruction\n"); 
                }
                n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen); 
                if(n<0)
                    error("Error in sending DST confirmation\n"); 

            } else if (strcmp(com, "EDT") == 0){
                //Edit Message Board
            } else if (strcmp(com, "LIS") == 0){
                //List Boards
                bzero(buf, BUFSIZE);
                readFile(buf, "boards.txt"); 
                n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen); 
                if(n<0)
                    error("Error in sending LIS\n");
            
            } else if (strcmp(com, "RDB") == 0){
                //Read a board
            } else if (strcmp(com, "APN") == 0){
                //Append file
            } else if (strcmp(com, "DWN") == 0){
                //Download file
            }
            n = write(sockfd2, buf, strlen(buf));

            if(n < 0)
                error("Error reading from socket");

        }
    }
}

int createFile(char * name, char * username){
   FILE *fp;  
   char s[BUFSIZE]; 
   
   fp = fopen(name, "w+"); 
   sprintf(s, "%s\n", username); 
   fprintf(fp, "%s", s);  
   int result = fclose(fp); 
    return result; 
}

int checkUser(char * username){

    FILE *fp; 
    char  buf[BUFSIZE];
    fp = fopen("users.txt", "r");
    while(fgets(buf, sizeof(buf), fp)){
        buf[strlen(buf) -1]='\0';  
        if(strcmp(buf, username) == 0){
            fclose(fp); 
            return 1; 
        }
        fgets(buf, sizeof(buf), fp); //skip password
    }
    fclose(fp); 
	return 0;
}

int login(char * username, char * password, int newUser){
    char  fileBuf[BUFSIZE]; 
    char fileBuf2[BUFSIZE]; 
    FILE *fp; 
    if(newUser){
        //add username and password to users.txt file
        fp = fopen("users.txt", "a"); 
        sprintf(fileBuf, "%s\n%s\n", username, password); 
        fprintf(fp, fileBuf); 
        fclose(fp);
        return 1; 
    } else{
        //compare passwords
        fp = fopen("users.txt", "r"); 
        while(fgets(fileBuf2, sizeof(fileBuf2), fp)){
            if(fgets(fileBuf2, sizeof(fileBuf2), fp) != NULL){
                fileBuf2[strlen(fileBuf2)-1]='\0'; 
                if(strcmp(fileBuf2, password) == 0){
                    printf("%s vs %s\n", fileBuf2, password); 
                    fclose(fp); 
                    return 1; 
                }
            }
        }
    }
    fclose(fp); 
    return 0; 
}

void addBoard(char * name){

    FILE *fp; 
    fp = fopen("boards.txt", "a");
    fprintf(fp, name);
    fprintf(fp, "\n"); 
    fclose(fp); 

}

void readFile(char *dest, char *fname) {
    FILE *fp = fopen(fname, "r");
    if (fp != NULL) {
        size_t new_len = fread(dest, sizeof(char), BUFSIZE, fp);
        if (ferror(fp) != 0) {
            fputs("Error reading file", stderr);
        } else {
            dest[new_len++] = '\0';
        }
        fclose(fp);
    }
}

void deleteFiles() {
    // These are data types defined in the "dirent" header
    printf("Deleting files...\n");
    DIR *theFolder = opendir(".");
    struct dirent *next_file;
    char filepath[256];

    while ( (next_file = readdir(theFolder)) != NULL )
    {
        // build the path for each file in the folder
        sprintf(filepath, "%s/%s", ".", next_file->d_name);
        if (has_txt_extension(filepath))
            remove(filepath);
    }
    closedir(theFolder);
}

bool has_txt_extension(char const *name) {
    size_t len = strlen(name);
    return len > 4 && strcmp(name + len - 4, ".txt") == 0;
}

int destroyBoard(char * name, char * username){
    FILE *fp; 
    FILE *fp2; 
    char fileBuf[BUFSIZE]; 
    char command[BUFSIZE]; 

    fp = fopen(name, "r"); 
    fgets(fileBuf, sizeof(fileBuf), fp); 
    fileBuf[strlen(fileBuf) -1] = '\0'; 
    if (strcmp(fileBuf, username) != 0){
        fclose(fp);
        return 0; 
    } else{
        sprintf(command, "rm %s", name); 
        system(command); 
        fclose(fp); 
        return 1;         
    }

}

void update_boards(char * boards[MAX_BOARDS], int boardCount){

    FILE *fp; 
    fp = fopen("boards.txt", "w+"); 
    int i; 
    for(i = 0; i<boardCount; i++){
        fprintf(fp, "%s\n", boards[i]); 
    }
    fclose(fp); 

}

int add_message(char * board, char * message){
    
    FILE *fp; 
    if (!(fp = fopen(board, "r"))){
        return 0; 
    }
    fclose(fp); 
    fp = fopen(board, "a"); 
    fprintf(fp, "%s\n", message); 
    fclose(fp); 
    return 1; 
    
}
