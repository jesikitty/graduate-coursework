/* Ron Pyka
 * CS 553
 * Assignment 1
 * Network Benchmark */

#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
using namespace std;

typedef std::chrono::duration<long, std::ratio<1,1000>> millisecs;
template <typename T>
long time_since(std::chrono::time_point<T> time)
{
  auto diff = std::chrono::system_clock::now() - time;
  return std::chrono::duration_cast<millisecs>( diff ).count();
}

void error(const char *msg)
{
  perror(msg);
  exit(1);
}

int TCPsender(int buffsize, int iter, int portnum)
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  char buffer[buffsize];
  portno = portnum;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  server = gethostbyname("127.0.0.1");
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

  n = write(sockfd,"test",4);//send first message to server to initialize socket on that end
  cout << "socket initialized..." << endl;

  cout << "testing " << buffsize <<"B buffer..." << endl;

  bzero(buffer,buffsize);
  for(int i=0;i<buffsize;i++){
    buffer[i]='A' + random()%26;
  }

  for(int i=0; i<iter; i++){
    n = write(sockfd,buffer,buffsize);
    if (n < 0) 
      error("ERROR writing to socket");
  }

  bzero(buffer,buffsize);

  close(sockfd);
  return 0;
}

int TCPreceiver(int buffsize, int iter, int portnum)
{
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer[buffsize];
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = portnum;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
        sizeof(serv_addr)) < 0) 
    error("ERROR on binding");
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, 
      (struct sockaddr *) &cli_addr, 
      &clilen);
  if (newsockfd < 0) 
    error("ERROR on accept");
  bzero(buffer,buffsize);
  for(int i=0; i<iter; i++){
    n = read(newsockfd,buffer,buffsize);
    if (n < 0) error("ERROR reading from socket");
  }

  close(newsockfd);
  close(sockfd);
  return 0; 
}

//UDP functions are direct clones of TCP changed to datagram type to make use of UDP instead
int UDPsender(int buffsize, int iter, int portnum)
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  char buffer[buffsize];
  portno = portnum;
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  server = gethostbyname("127.0.0.1");
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

  //n = write(sockfd,"test",4); //send first message to server to initialize socket on that end
  cout << "socket initialized..." << endl;

  cout << "testing " << buffsize << "B buffer..." << endl;

  bzero(buffer,buffsize);
  for(int i=0;i<buffsize;i++){
    buffer[i]='A' + random()%26;
  }

  for(int i=0; i<iter; i++){
    n = send(sockfd,buffer,buffsize,MSG_CONFIRM);
    if (n < 0) 
      error("ERROR writing to socket");
  }

  bzero(buffer,buffsize);

  close(sockfd);
  return sockfd;
}

int UDPreceiver(int buffsize, int iter, int portnum)
{
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer[buffsize];
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = portnum;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
        sizeof(serv_addr)) < 0) 
    error("ERROR on binding");
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  /*newsockfd = accept(sockfd, 
      (struct sockaddr *) &cli_addr, 
      &clilen);
  if (newsockfd < 0) 
    error("ERROR on accept");*/
  bzero(buffer,buffsize);
  for(int i=0; i<iter; i++){
    n = recv(sockfd,buffer,buffsize,MSG_WAITALL);
    if (n < 0) error("ERROR reading from socket");
  }

  close(newsockfd);
  close(sockfd);
  return 0; 
}

int main(int argc, char* argv[])
{
  if (argc<5){
    cout << "format is ./net [buffer size (B)] [iterations] [port] [num threads]" << endl;
    return 0;
  }

  int buffsize = atoi(argv[1]);
  int iter = atoi(argv[2]);
  int portnum = atoi(argv[3]);
  int numthreads = atoi(argv[4]);

  thread threads[numthreads*2];

  if (numthreads==1){
    cout << "TCP single thread" << endl;

    threads[0] = thread(TCPreceiver,buffsize,(2*iter),portnum);
    
    auto beforeTCP = std::chrono::system_clock::now();
    
    threads[1] = thread(TCPsender,buffsize,iter,portnum);

    threads[1].join();

    auto elapsedTCP = time_since(beforeTCP);
    cout << elapsedTCP << "ms" <<endl;
    ulong speedTCP = (iter/elapsedTCP) * 1000 / 1000;
    cout << speedTCP*buffsize << "KB/s" << endl;

    threads[0].join();
  
    cout << "Waiting 5 sec before UDP..." << endl;
    sleep(5);
    cout << "UDP single thread" << endl;

    threads[0] = thread(UDPreceiver,buffsize,iter,portnum);
    
    auto beforeUDP = std::chrono::system_clock::now();
    
    threads[1] = thread(UDPsender,buffsize,iter,portnum);

    threads[1].join();
    
    auto elapsedUDP = time_since(beforeUDP);
    cout << elapsedUDP << "ms" <<endl;
    ulong speedUDP = (iter/elapsedUDP) * 1000 / 1000;
    cout << speedUDP*buffsize << "KB/s" << endl;

    threads[0].join();
  }
  else{
    cout << "TCP two threads" <<endl;

    threads[0] = thread(TCPreceiver,buffsize,iter,portnum);
    threads[1] = thread(TCPreceiver,buffsize,iter,portnum+1);
    
    auto beforeTCP = std::chrono::system_clock::now();
    
    threads[2] = thread(TCPsender,buffsize,(iter/2),portnum);
    threads[3] = thread(TCPsender,buffsize,(iter/2),portnum+1);

    threads[3].join();
    threads[2].join();

    auto elapsedTCP = time_since(beforeTCP);
    cout << elapsedTCP << "ms" <<endl;
    ulong speedTCP = (iter/elapsedTCP) * 1000 /1000;
    cout << speedTCP*buffsize << "KB/s" << endl;

    threads[1].join();
    threads[0].join();

    cout << "Waiting 15 sec before UDP in attempt to prevent system from refusing connection" << endl;
    sleep(15);
    cout << "UDP two threads" << endl;

    threads[0] = thread(UDPreceiver,buffsize,iter,portnum);
    threads[1] = thread(UDPreceiver,buffsize,iter,portnum+1);
    
    auto beforeUDP = std::chrono::system_clock::now();
    
    threads[2] = thread(UDPsender,buffsize,(iter/2),portnum);
    threads[3] = thread(UDPsender,buffsize,(iter/2),portnum+1);

    threads[3].join();
    threads[2].join();
    
    auto elapsedUDP = time_since(beforeUDP);
    cout << elapsedUDP << "ms" <<endl;
    ulong speedUDP = (iter/elapsedUDP) * 1000 / 1000;
    cout << speedUDP*buffsize << "KB/s" << endl;

    cout << "UDP server side hangs often. If program doesn't terminate, kill it now" << endl;

    threads[1].join();
    threads[0].join();
  }
  return 0;
}
