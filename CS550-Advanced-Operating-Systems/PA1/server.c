//CS550-01
//Merrick, Boliske, Pyka
//Indexing server

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

#define PORT "13000"   // port we're listening on

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// defining a file index
struct findex{
    char *ipaddy;
    char *filename;
    struct findex *nextFile;
};

struct findex *root=NULL;//root of linked list
struct findex *current=NULL;//temp linked list node

struct findex* init_index(char *ipaddy, char *filename)
{
    struct findex *ptr = (struct findex*)malloc(sizeof(struct findex));
    if(NULL == ptr)
    {
        return NULL;
    }
    ptr->ipaddy = (char*)malloc(strlen(ipaddy));
    ptr->filename = (char*)malloc(strlen(filename));
    strcpy(ptr->ipaddy, ipaddy);
    strcpy(ptr->filename, filename);
    ptr->nextFile = NULL;

    root = current = ptr;

    printf("file %s from host %s added.\n",filename,ipaddy);

    return ptr;
}

struct findex* add_file(char *ipaddy, char *filename)
{
    if(NULL == root)
    {
        return (init_index(ipaddy,filename));
    }

    struct findex *ptr = (struct findex*)malloc(sizeof(struct findex));
    if(NULL == ptr)
    {
        return NULL;
    }
    ptr->ipaddy = (char*)malloc(strlen(ipaddy));
    ptr->filename = (char*)malloc(strlen(filename));
    strcpy(ptr->ipaddy, ipaddy);
    strcpy(ptr->filename, filename);
    ptr->nextFile = NULL;

    current->nextFile = ptr;
    current = ptr;

    printf("file %s from host %s added.\n",filename,ipaddy);

    return ptr;
}

struct findex* search_for_file(char *filename){
    struct findex *ptr = root;
    struct findex *nroot = NULL;
    struct findex *temp = NULL;
    struct findex *c = NULL;
    //bool found = false;

    while(ptr != NULL){
        //printf("%s\n",ptr->filename);
        if(strcmp(ptr->filename, filename) == 0){
            /*found = true;
            break;*/
            //return ptr->ipaddy;
            if(nroot==NULL){
                nroot = (struct findex*)malloc(sizeof(struct findex));
                c = (struct findex*)malloc(sizeof(struct findex));
                nroot->ipaddy = (char*)malloc(strlen(ptr->ipaddy));
                nroot->filename = (char*)malloc(strlen(ptr->filename));
                strcpy(nroot->ipaddy, ptr->ipaddy);
                strcpy(nroot->filename, ptr->filename);
                nroot->nextFile = NULL;
                c = nroot;
                temp = (struct findex*)malloc(sizeof(struct findex));
            }else{
                temp->ipaddy = (char*)malloc(strlen(ptr->ipaddy));
                temp->filename = (char*)malloc(strlen(ptr->filename));
                strcpy(temp->ipaddy, ptr->ipaddy);
                strcpy(temp->filename, ptr->filename);
                temp->nextFile = NULL;
                c->nextFile = temp;
                c = temp;
            }
        }/*else{
            //tmp = ptr;
            ptr = ptr->nextFile;
        }*/
        ptr = ptr->nextFile;
    }

    return nroot;//NULL;
}

int search_index(char *ipaddy, char *filename, struct findex **prev)
{
    struct findex *ptr = root;

    while(ptr != NULL)
    {
        if((strcmp(ptr->filename, filename)==0 && strcmp(ptr->ipaddy, ipaddy)==0))
        {
            //found = true;
            //break;
            *prev = ptr;
            return 1;
        }
        else
        {
            //temp = ptr;
            ptr = ptr->nextFile;
        }
    }

    return 0;
}

int remove_file(char *ipaddy, char *filename)
{
    struct findex *prev = NULL;
    struct findex *rm = NULL;

    rm = search_index(ipaddy, filename, &prev);
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

    printf("file %s from host %s removed.\n",filename,ipaddy);

    free(rm);
    rm = NULL;

    return 0;
}

int main(void)
{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accepted socket descriptor
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
    //create all the listeners
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

                    //error with connection
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        //tell the user about the connection
                        printf("new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed - tell user
                            printf("socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // closing socket 
                        FD_CLR(i, &master); // remove from master set to be reused
                    } else {
                        //printf("%s\n", buf);
                        //get the user's command
                        char *userData = strtok(buf, " ");
                        //user wants to add file
                        if(strncmp(userData, "addf", 4) == 0){
                            userData = strtok(NULL, "\n"); //filename
                            char *filename;
                            filename = userData;
                            if(userData == NULL) {
                                send(i, "Please include a file name\n", 27, 0);
                            }
                            else {
                                //check if the user already had the file registered
                                //userData = strtok(NULL,"\n"); //IP Address
                                struct findex* prev = (struct findex*)malloc(sizeof(struct findex));
                                char *ip = (char*)malloc(256);
                                strcat(ip,inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*)&remoteaddr),remoteIP,INET6_ADDRSTRLEN));
                                if(search_index(ip,filename,&prev)==0){
                                    add_file(ip,filename);
                                    send(i, strcat(filename, " has been added\n"), (sizeof(filename)+16), 0);
                                }else{
                                    send(i, strcat(filename, " was already here\n"), (sizeof(filename)+18), 0);
                                }
                                bzero(filename,256);
                            }    
                        }
                        //user trying to remove file
                        else if(strncmp(userData, "remf", 4) == 0){
                            userData = strtok(NULL, "\n"); //filename
                            char *filename;
                            filename = userData;
                            userData = strtok(NULL,"\n");   //IP Address
                            remove_file(inet_ntop(remoteaddr.ss_family,
                                          get_in_addr((struct sockaddr*)&remoteaddr),
                                          remoteIP,INET6_ADDRSTRLEN),filename);
                            send(i, strcat(filename, " has been removed\n"), (sizeof(filename)+20), 0); 
                        }
                        //user wants list of clients with file
                        else if(strncmp(userData, "getf", 4) == 0){
                            userData = strtok(NULL, "\n"); //filename
                            //char *ipaddy = search_for_file(userData);
                            struct findex* results = (struct findex*)malloc(sizeof(struct findex));
                            results = search_for_file(userData);
                            if(results!=NULL){
                                char *message = (char *)malloc(200);
                                strcat(message, userData);
                                strcat(message, " and its clients\n");
                                while(results!=NULL){
                                    strcat(message,results->ipaddy);
                                    strcat(message,"\n");
                                    printf("%s\n",results->ipaddy);
                                    results = results->nextFile;
                                }
                                send(i, message, 200, 0); //edit this so size is appropriate 
                            }else{
                                char *message = (char *)malloc(200);
                                strcat(message,"File does not exist.");
                                send(i,message,200,0);
                            }
                        }
                        else 
                            send(i, "Invalid command\n", 16, 0);
                    }
                } //end handle data from client
            } //end got new incoming connection
        } //end looping through file descriptors
    } //end for(;;)
    
    return 0;
}
