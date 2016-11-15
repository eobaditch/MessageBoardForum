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

#define BUFSIZE 4096
#define MAX_BOARDS 100

// error handling
void error(char *msg) {
  perror(msg);
  exit(1);
}

int destroyBoard(char *name, char * username); 
int dwn_check(char *); 
int append_check(char * , char *); 
int createFile(char *name, char * username); 
void readFile(char *dest, char *fname); 
int checkUser(char*username); 
int login(char * username, char * password, int newUser); 
void addBoard(char * name);
void deleteBoards();
bool has_txt_extension(char const *name);
int add_message(char * board, char * message, char * command); 
int delete_message(char * board, char * message);
int edit_message(char * name, char * old_message, char * new_message);
void update_boards(char * boards[MAX_BOARDS], int boardCount); 

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
    int n, k, i, size;                  // message size, key size, counter
    short len;
    int newUser;                        // determines if user is new 
    char name[BUFSIZE];                 // holds name of board
    char message[BUFSIZE];              // message to post 
    char file[BUFSIZE];                 // file name to append
    char username[BUFSIZE];         
    char dwn_name[BUFSIZE]; 
    char apn_name[BUFSIZE]; 
    char old_message[BUFSIZE];          // EDT- edit message
    char new_message[BUFSIZE];          // EDT - edit message
    char *len_string; 
    unsigned char * serverHash; 
    char com[BUFSIZE];                  // command
    char password[BUFSIZE]; 
    char* path, filename;
    char choice[BUFSIZE]; 
    char * boards[MAX_BOARDS]; 
    int boardCount = 0; 
    int file_size;
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
					printf("%s\n", buf);
                    n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
                    if (n < 0)
                        error("ERROR in password correct, shutting down message");
					printf("wtf is going on\n");
                    // Shut down server - delete all board files and appended files, close all sockets
					
					char command[BUFSIZE];
					printf("HERE\n");
					for (i = 0; i < boardCount; i++) {
						fprintf(stderr, "here\n");
						bzero(command, BUFSIZE);
						sprintf(command, "rm %s", boards[i]);
						printf("Command: %s\n", command);
						fflush(stdout);
						system(command);
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
            
            
            } else if (strcmp(com, "DLT") == 0){
                //delete message
                bzero(buf, BUFSIZE); 
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
                if(n<0)
                    error("Error in receiving board name\n"); 
                strcpy(name, buf);
                printf("%s\n", name); 
                bzero(buf, BUFSIZE); 
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
                if(n<0)
                    error("Error in receiving message to be deleted\n"); 
                strcpy(message, buf);
                printf("%s\n", message); 
                int delete_result = delete_message(name, message); 
                bzero(buf, BUFSIZE); 
                if(delete_result)
                    strcpy(buf, "Successfully deleted post\n"); 
                else
                    strcpy(buf, "Error in deleting post\n"); 
                n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen); 
                if(n<0)
                    error("Error in sending DLT confirmation\n");
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
                bzero(buf, BUFSIZE); 
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
                if(n<0)
                    error("Error in receiving board name\n"); 
                strcpy(name, buf); 
                bzero(buf, BUFSIZE); 
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
                if(n<0)
                    error("Error in receiving message to edit\n"); 
                strcpy(old_message, buf);
                strcat(old_message, " "); 
                strcat(old_message, username); 
                bzero(buf, BUFSIZE); 
                n = recvfrom(udpsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
                if(n<0)
                    error("Error in receiving new message \n"); 
                strcpy(new_message, buf); 
                bzero(buf, BUFSIZE); 
                int edit_result = edit_message(name, old_message, new_message); 
                if(edit_result){
                    strcpy(buf, "Successfully edited message\n"); 
                } else {
                    strcpy(buf, "Error in editing message\n"); 
                }
                n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen); 
                if(n<0)
                    error("Error in sending EDT confirmation\n");
            } else if (strcmp(com, "LIS") == 0){
                //List Boards
                bzero(buf, BUFSIZE);
                readFile(buf, "boards.txt"); 
                n = sendto(udpsockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen); 
                if(n<0)
                    error("Error in sending LIS\n");
            
            } else if (strcmp(com, "RDB") == 0){
                //Read a board
				int s;
				struct stat st;
				int rounds, j, size, i;
				int j_limit = 4095;
				char currBuf[BUFSIZE];

				printf("in RDB.\n");
				bzero(buf, BUFSIZE);
				n = read(sockfd2, buf, BUFSIZE);
				if (n < 0)
					error("Error reading filename.\n");

				char name[BUFSIZE];
				strcpy(name, buf);
				bzero(buf, BUFSIZE);
				printf("Before if.\n");
				if (access(name, F_OK) == -1) { 	// file does not exist
					printf("hey\n");
					s = -1;
					sprintf(buf, "%s", s);
					n = write(sockfd2, buf, BUFSIZE);
					if (n < 0)
						error("Error in sending failure message.\n");
					break;
				}
				printf("After if\n");

				stat(name, &st);
				size = st.st_size;
				bzero(buf, BUFSIZE);
				sprintf(buf, "%d", size);
				printf("filesize: %d\n", size);
				// Send file size to client
				n = write(sockfd2, buf, BUFSIZE);
				if (n < 0)
					error("Error sending file size.\n");
				
				char fileBuf[size + 1];
				readFile(fileBuf, name);
				printf("After reading file.\n");
				// Send file to client
				if (size < 4096) {
					printf("Sending\n");
					n = write(sockfd2, fileBuf, BUFSIZE);
					if (n < 0)
						error("Error sending file.\n");
				} else {
					rounds = (size + 4095) / 4096;
					int round_num = 0;
					for (i = 0; i < rounds; i++) {
						for (j = 0; j < 4095; j++) {
							currBuf[j] = fileBuf[round_num + j];
						}
						n = write(tcpsockfd, currBuf, BUFSIZE);
						round_num += 4096;
						bzero(currBuf, BUFSIZE);
					}
				}

            } else if (strcmp(com, "APN") == 0){
                //Append file
                bzero(buf, BUFSIZE); 
                n = read(sockfd2, buf, BUFSIZE); 
                if(n<0)
                    error("Error in recieving board name for APN\n"); 
                strcpy(name, buf); 
                bzero(buf, BUFSIZE); 
                n = read(sockfd2, buf, BUFSIZE); 
                if (n <0)
                    error("Error in receiving filename for APN\n"); 
                strcpy(file, buf); 
                sprintf(apn_name, "%s-%s", name, file); 
                bzero(buf, BUFSIZE);
                printf("before check\n"); 
                int apn_result = append_check(name, file); 
                printf("%d\n", apn_result); 
                if(apn_result)
                    strcpy(buf, "Board exists and file can be created\n"); 
                else
                    strcpy(buf, "Error.  File cannot be appended\n"); 
                n = write(sockfd2, buf, strlen(buf)); 
                if (n < 0)
                    error("Error in APN confirmation\n"); 
                bzero(buf, BUFSIZE); 
                //get filesize
                if(apn_result){
                    n = read(sockfd2, buf, BUFSIZE); 
                    if(n,0)
                        error("Error in recieving filesize"); 
                    file_size = atoi(buf); 
                    for(i = 0; i < ((BUFSIZE + file_size -1) / BUFSIZE); i++){
                        bzero(buf, BUFSIZE); 
                        n = read(sockfd2, buf, BUFSIZE); 
                        if(n<0)
                            error("Error in recieving APN data"); 
                        add_message(apn_name, buf, com); 
                    }
                    bzero(buf, BUFSIZE); 
                    sprintf(buf, "%s appended by %s", file, username); 
                    add_message(name, buf, com); 

                } // end if(apn_result)
            } else if (strcmp(com, "DWN") == 0){
                //Download file
                struct stat st; 
                bzero(buf, BUFSIZE); 
                n = read(sockfd2, buf, BUFSIZE); 
                if(n<0)
                    error("Error in recieving board name for DWN\n"); 
                strcpy(name, buf); //board name
                bzero(buf, BUFSIZE); 
                n = read(sockfd2, buf, BUFSIZE); 
                if (n <0)
                    error("Error in receiving filename for DWN\n"); 
                strcpy(file, buf); //file name
                sprintf(dwn_name, "%s-%s", name, file); //naming convention
                printf("%s\n", dwn_name); 
                bzero(buf, BUFSIZE);
                stat(dwn_name, &st);
                size = st.st_size; 
                if (!(dwn_check(dwn_name))){
                    size = -1; 
                }
                sprintf(buf, "%d", size); 
                //send filesize
                n = write(sockfd2, buf, strlen(buf)); 
                if (n < 0)
                    error("Error in sending filesize for DNW\n"); 
                if(dwn_check(dwn_name) != 0){ 
                    for(i = 0; i< ((BUFSIZE + size -1) / BUFSIZE); i++){
                        bzero(buf, BUFSIZE); 
                        readFile(buf, dwn_name); 
                        n = write(sockfd2, buf, strlen(buf)); 
                        if(n<0)
                            error("Error in sending file in DWN\n"); 
                    }
                }
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

void deleteBoards() {
	FILE *fp;
	int k;
	k = remove("rmtest.txt");
	char buf[256];
	char *rm;
	struct stat st;
	fp = fopen("boards.txt", "r");
	while (fgets(buf, sizeof(buf), fp)) {
		printf("File: %s\n", buf);
		char file[strlen(buf)];
		strcpy(file, buf);
		printf("file: %s", file);
		if (stat(file, &st) == 0 && S_ISREG(st.st_mode)) {
			if (asprintf(&rm, "rm %s", file) != -1) {
				printf("RM: %s\n", rm);
				k = system(rm);
			}
		}
	}
	fclose(fp);
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

int add_message(char * board, char * message, char * command){
    
    FILE *fp; 
    if(strcmp(command, "MSG") == 0){
        if (!(fp = fopen(board, "r"))){
            return 0; 
        }
    }
    fp = fopen(board, "a"); 
    fprintf(fp, "%s\n", message);
    fclose(fp); 
    return 1; 
    
}

int edit_message(char * name, char * old, char * new){
    FILE *fp, *fp2; 
    char fileBuf[BUFSIZE];  
    char command[BUFSIZE]; 
    bool found = false; 


    if (!(fp = fopen(name, "r"))){
        printf("here\n"); 
        return 0; 
    }
    fclose(fp); 
    fp = fopen(name, "r"); 
    printf("before while\n"); 
    fp2 = fopen("temp.txt", "w+"); 
    while(fgets(fileBuf, sizeof(fileBuf), fp) != NULL){ //!feof(fp)){
        printf("%s\n", fileBuf); 
        fileBuf[strlen(fileBuf) -1 ] = '\0'; 
        if (strcmp(fileBuf, old) == 0){
            found = true; 
            fprintf(fp2, "%s\n", new); 
        } else{
            fprintf(fp2, "%s\n", fileBuf); 
        }
        bzero(fileBuf, BUFSIZE); 
    }
    fclose(fp); 
    fclose(fp2); 
    sprintf(command, "mv temp.txt %s", name); 
    system(command);
    if (found)
        return 1; 

    return 0;    
}

int delete_message(char * board, char * message){

    FILE *fp, *fp2; 
    char fileBuf[BUFSIZE];  
    char command[BUFSIZE]; 
    bool found = false; 


    if (!(fp = fopen(board, "r"))){
        return 0; 
    }
    fclose(fp); 
    fp = fopen(board, "r"); 
    fp2 = fopen("temp.txt", "w+"); 
    while(fgets(fileBuf, sizeof(fileBuf), fp) != NULL){ 
        printf("%s\n", fileBuf); 
        fileBuf[strlen(fileBuf) -1 ] = '\0'; 
        if (strcmp(fileBuf, message) == 0){
            found = true; 
        } else{
            fprintf(fp2, "%s\n", fileBuf); 
        }
        bzero(fileBuf, BUFSIZE); 
    }
    fclose(fp); 
    fclose(fp2); 
    sprintf(command, "mv temp.txt %s", board); 
    system(command);
    if (found)
        return 1; 

    return 0;   

}

int append_check(char * board, char * file){
    
    char filename[BUFSIZE]; 
    sprintf(filename, "%s-%s", board, file); //required naming convention

    //if board does not exist 
    if (!(access(board, F_OK) != -1)){
        return 0; 
    }
    //if file with required name already exists or attachment already exists
    if( access(filename, F_OK) != -1 ){
        return 0; 
    }
    
    return 1; 

}

int dwn_check(char * name){

    if (!(access(name, F_OK) != -1)){
        return 0; 
    }
    return 1; 

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
