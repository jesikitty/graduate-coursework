//CS550-01
//Merrick, Boliske, Pyka
//Client

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dirent.h>
#include <stdbool.h>

#define PORT "13001"   // port the server side of the client listens on - different than the port for the indexing server

//makes printing out the error a little easier
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

// get sockaddr, IPv4 or IPv6:
//CURRENTLY UNNECCESARY/UNUSED
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//connect to a client-server and retrieve a file
void *retrieveFile(char *hostname, char *filename){
    int sockfd, n, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    portno = atoi(PORT);//convert port constant to int for use
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(hostname); 
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return NULL;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting ");

    //construct the retf command and send it
    bzero(buffer,256);
    strcat(buffer,"retf ");
    strcat(buffer,filename);
    strcat(buffer,"\n");
    printf("buffer is: %s, %i\n",buffer, strlen(buffer));
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
        error("ERROR writing to socket");
    bzero(buffer,256);    

    //been changing this around a lot, trying to figure out what works
    //read file size from the socket
    n = read(sockfd,buffer,256);
    int sz = atoi(buffer);
    //will get size of -1 for no file
    if(sz==-1){
      printf("File does not exist!\n");
    }
    //otherwise, read more data and write to file
    else{
    FILE *file;
    file = fopen(filename,"w"); 
    int i=0;
    for(i=0;i<sz;i+=n)
    {
        bzero(buffer,256);
        int read_bytes = 0;
        while (read_bytes < 256) {
            read_bytes += recv(sockfd, buffer + read_bytes, 256-read_bytes, 0);
        }
        //n = recv(sockfd,buffer,256,0);
        
        printf("%i\n",n);
        //printf("%i\n",n);
        /*if () {
            //error("ERROR reading from socket");
            printf("Error reading from socket");
            break;
        }
        else {*/
            if((sz-i)<256){
                fwrite(buffer, 1, (sz%256), file);
            }else{
                fwrite(buffer, 1, n, file);
            }
            fflush(file);
        //}
    }
    fclose(file);
    printf("Download of %s is complete.", filename);
    } //end else

    return NULL;
}

// defining a file index for local tracking
struct findex{
    time_t file_time;
    char *filename;
    struct findex *nextFile;
};

struct findex *root=NULL;
struct findex *current=NULL;

struct findex* init_index(time_t file_time, char *filename)
{
    struct findex *ptr = (struct findex*)malloc(sizeof(struct findex));
    if(NULL == ptr)
    {
        return NULL;
    }
    ptr->file_time = file_time;
    ptr->filename = filename;
    ptr->nextFile = NULL;

    root = current = ptr;
    return ptr;
}

struct findex* add_file(time_t file_time, char *filename)
{
    if(NULL == root)
    {
        return (init_index(file_time,filename));
    }

    struct findex *ptr = (struct findex*)malloc(sizeof(struct findex));
    if(NULL == ptr)
    {
        return NULL;
    }
    ptr->file_time = file_time;
    ptr->filename = filename;
    ptr->nextFile = NULL;

    current->nextFile = ptr;
    current = ptr;
    return ptr;
}

char* search_for_file(char *filename)
{
    struct findex *ptr = root;
    struct findex *tmp = NULL;
    bool found = false;

    while(ptr != NULL)
    {
        if(ptr->filename == filename)
        {
            found = true;
            break;
        }
        else
        {
            tmp = ptr;
            ptr = ptr->nextFile;
        }
    }

    if(true == found)
    {
        printf("file found\n");
        return ptr->file_time;
    }
    else
    {
        return NULL;
    }
}

struct findex* search_index(time_t file_time, char *filename, struct findex **prev)
{
    struct findex *ptr = root;
    struct findex *temp = NULL;
    bool found = false;

    while(ptr != NULL)
    {
        if(ptr->filename == filename && ptr->file_time == file_time)
        {
            found = true;
            break;
        }
        else
        {
            temp = ptr;
            ptr = ptr->nextFile;
        }
    }

    if(true == found)
    {
        *prev = temp;
        return ptr;
    }
    else
    {
        return NULL;
    }
}

int remove_file(time_t file_time, char *filename)
{
    struct findex *prev = NULL;
    struct findex *rm = NULL;

    rm = search_index(file_time, filename, &prev);
    if(rm == NULL)
    {
        return -1;
    }
    else
    {
        if(prev != NULL)
            prev->nextFile = rm->nextFile;

        if(rm == current)
        {
            current = prev;
        }
        else if(rm == root)
        {
            root = rm->nextFile;
        }
    }

    free(rm);
    rm = NULL;

    return 0;
}


//function to check if the date modified on the file is more recent than the one stored
//(meaning it was changed)
int checkFileModified(FILE *fp, time_t oldTime){
  struct stat file_stat;
  int err = fstat(fp, &file_stat);
  if(err!=0){
    error("checkFileModified stat failed");
  }
  return file_stat.st_mtime > oldTime;
}

//thread to constantly look at files for changes
void *checkFiles(){
  while(1){ //loop forever
    struct findex *ptr = root;

    while(ptr != NULL)
    {
      FILE *curfile;
      curfile = fopen(ptr->filename,"r");
        
      if(checkFileModified(curfile,ptr->file_time)){
        printf("modified");
      }

      ptr = ptr->nextFile;

      fclose(curfile);
    }
  }
}

//similar to indexing server
void *clientAsServer(){
        fd_set master;    // master file descriptor list
        fd_set read_fds;  // temp file descriptor list for select()
        int fdmax;        // maximum file descriptor number

        int listener;     // listening socket descriptor
        int newfd;        // newly accept()ed socket descriptor
        struct sockaddr_storage remoteaddr; // client address
        socklen_t addrlen;

        char buf[256];    // buffer for client data
        int nbytes;

        char remoteIP[INET6_ADDRSTRLEN];

        int yes=1;        // for setsockopt() SO_REUSEADDR, below
        int i, j, rv;

        struct addrinfo hints, *ai, *p;

        FD_ZERO(&master);    // clear the master and temp sets
        FD_ZERO(&read_fds);

        // get us a socket and bind it
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
            fprintf(stderr, "%s\n", gai_strerror(rv));
            exit(1);
        }
        
        for(p = ai; p != NULL; p = p->ai_next) {
            listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (listener < 0) { 
                continue;
            }
            
            // lose the pesky "address already in use" error message
            setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

            if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
                close(listener);
                continue;
            }

            break;
        }

        // if we got here, it means we didn't get bound
        if (p == NULL) {
            fprintf(stderr, "failed to bind\n");
            exit(2);
        }

        freeaddrinfo(ai); // all done with this

        // listen
        if (listen(listener, 10) == -1) {
            perror("listen");
            exit(3);
        }

        // add the listener to the master set
        FD_SET(listener, &master);

        // keep track of the biggest file descriptor
        fdmax = listener; // so far, it's this one

        // main loop
        for(;;) {
            //NOTE: should be done better - make sure client server cannot be cancelled while sending data
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //can be cancelled
            //pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); //can't be cancelled while handling data

            read_fds = master; // copy it
            if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
                perror("select");
                exit(4);
            }

            // run through the existing connections looking for data to read
            for(i = 0; i <= fdmax; i++) {
                if (FD_ISSET(i, &read_fds)) { // we got one!!
                    if (i == listener) {
                        // handle new connections
                        addrlen = sizeof remoteaddr;
                        newfd = accept(listener,
                            (struct sockaddr *)&remoteaddr,
                            &addrlen);

                        if (newfd == -1) {
                            perror("accept");
                        } else {
                            FD_SET(newfd, &master); // add to master set
                            if (newfd > fdmax) {    // keep track of the max
                                fdmax = newfd;
                            }
                            printf(" new connection from %s on "
                                "socket %d\n",
                                inet_ntop(remoteaddr.ss_family,
                                    get_in_addr((struct sockaddr*)&remoteaddr),
                                    remoteIP, INET6_ADDRSTRLEN),
                                newfd);
                        }
                    } else { //NEEDS TESTING
                        // handle data from a client
                        if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                            // got error or connection closed by client
                            if (nbytes == 0) {
                                // connection closed
                                printf("socket %d hung up\n", i);
                            } else {
                                perror("recv");
                            }
                            close(i); // bye!
                            FD_CLR(i, &master); // remove from master set
                        } else { //Send file to peer
                            char *userData = strtok(buf, " ");
                            if(strncmp(userData, "retf", 4) == 0){ //check
                                userData = strtok(NULL, "\n"); //filename
                                if(userData == NULL) { //if null, send back error
                                    send(i, "Please include a file name\n", 27, 0);
                                }
                                else {
                                    //check if file requested exists
                                    if(access(userData, F_OK) == -1) {
                                        send(i, "-1", 2, 0);
                                    }
                                    else { //else start sending data
                                        //open file for reading
                                        char buffer[256]; FILE *file;
                                        bzero(buffer,256);
                                        file = fopen(userData,"r");   
                                        fseek(file, 0L, SEEK_END);
                                        int sz = ftell(file);
                                        fseek(file, 0L, SEEK_SET);
                                        sprintf(buffer, "%i", sz);
                                        send(i, buffer, sizeof(buffer), 0);
                                        bzero(buffer, 256);
                                        while ((fread(buffer,1,256,file)) != NULL)
                                        {
                                            send(i, buffer, 256, 0);
                                            bzero(buffer,256);
                                        } 
                                        //strcat(buffer,"end of file");
                                        //send(i,buffer,11,0);
                                        bzero(buffer,256);
                                    }                                     
                                }    
                            }
                            
                        }
                    } //end handle data from other client
                } //end got new incoming connection
            } //end looping through file descriptors
        } //end for(;;)
        //printf("Server ending\n"); //Check
        return NULL;
}

//client as client code
int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    //start client server
    //pthread_t client_server;
    //pthread_create(&client_server, NULL, clientAsServer, NULL);

    //start checking files for changes
    //pthread_t file_checker;
    //pthread_create(&file_checker, NULL, checkFiles, NULL);
    
    //Getting the IP Address to Send with commands
    int fd;
    struct ifreq ifr;
    fd = sockfd;
    //IPv4 Address
    ifr.ifr_addr.sa_family = AF_INET;
    ioctl(fd, SIOCGIFADDR, &ifr);
    char *ipstr;
    ipstr = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

    //adding loop here - loop forever for input
    int flag = 1;
    while(1){
        printf("Please enter a command (or \"exit\" to leave): \n");
        printf("\t Enter \"regf\" to register all files in current directory with the server\n");
        printf("\t Enter \"addf file.txt\" to add a file to the server\n");
        printf("\t Enter \"remf file.txt\" to remove a file from the server\n");
        printf("\t Enter \"getf file.txt\" to retrieve a list of clients with the file\n");
        printf("\t Enter \"retf hostname file.txt\" to retrieve a file\n");
        printf("Enter command: ");
        bzero(buffer,256);
        fgets(buffer,255,stdin);
        if(strncmp(buffer, "exit", 4) == 0)
            break;
        char tempBuf[256]; strncpy(tempBuf, buffer, 255);
        char *dataCheck = strtok(tempBuf, " ");
        if(strncmp(dataCheck, "addf", 4) == 0){ //check if file being added exists
            dataCheck = strtok(NULL, "\n"); //filename
            if(access(dataCheck, F_OK) == -1) {//file does not exist
                flag = 0; //do not send to server
                printf("That file does not exist within your directory.\n");
            }
            else{
              FILE *newfile;
              newfile = fopen(dataCheck,"r");
              struct stat file_stat;
              fstat(newfile, &file_stat);
              add_file(file_stat.st_mtime,dataCheck);
            }
        }
        if(strncmp(dataCheck, "remf", 4) == 0){ //check if file being added exists
            dataCheck = strtok(NULL, "\n"); //filename
              FILE *newfile;
              newfile = fopen(dataCheck,"r");
              struct stat file_stat;
              fstat(newfile, &file_stat);
              remove_file(file_stat.st_mtime,dataCheck);
        }
        if(strncmp(dataCheck, "retf", 4) == 0){ 
            flag = 0; //do not send to indexing server
            char *host= strtok(NULL, " "); //host
            char *requestedFile = strtok(NULL, "\n"); //file name
            //printf("Test: %s and %s \n", requestedFile, host);
            //retrieve file here
            //retrieveFile("192.168.1.101", "test2.txt");
            retrieveFile(host,requestedFile);
        }
        if(strncmp(dataCheck, "regf", 4) == 0){ //register all files with the indexing server 
              DIR *d; struct dirent *dir;
              d = opendir(".");
              if (d)
              { 
                while ((dir = readdir(d)) != NULL)
                {
                    char *tempF = dir->d_name;
                    //don't bother with the application files, but add everything else
                    if(strncmp(tempF, ".", 1) != 0 && strncmp(tempF, "client",6) != 0 && strncmp(tempF, "server",6) != 0 && strncmp(tempF, "makefile",8) != 0 ){
                        //send multiple addf commands
                        bzero(buffer,256);    
                        strcat(buffer,"addf ");
                        strcat(buffer, tempF);
                        //strcat(buffer, " ");
                        //strcat(buffer,ipstr);
                        //printf("%s", buffer);
                        n = write(sockfd,buffer,strlen(buffer));
                        if (n < 0) 
                            error("ERROR writing to socket");
                        bzero(buffer,256);
                        n = read(sockfd,buffer,256);
                        if (n < 0) 
                            error("ERROR reading from socket");
                        printf("%s\n",buffer);
                        bzero(buffer,256);
                    }
                }
                closedir(d);
              }
              flag = 0; //do not use regular statement for using indexing server
        }
        if(flag) { //send command to server
            //strcat(buffer," ");
            //strcat(buffer,ipstr);
            //printf("%s", buffer);
            n = write(sockfd,buffer,strlen(buffer));
            if (n < 0) 
                error("ERROR writing to socket");
            bzero(buffer,256);
            n = read(sockfd,buffer,255);
            if (n < 0) 
                error("ERROR reading from socket");
            printf("%s\n",buffer);
            bzero(buffer,256);
        }
        flag = 1; //reset flag
    }
    close(sockfd);

    //pthread_cancel(client_server);
    //pthread_join(client_server, NULL); //wait for server to be done

    return 0;
}
