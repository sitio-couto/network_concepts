#include "server.h"

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){

  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void){

int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
  }
  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
              p->ai_protocol)) == -1) {
          perror("server: socket");
          continue;
      }
      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
              sizeof(int)) == -1) {
          perror("setsockopt");
          exit(1);
      }
      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
          close(sockfd);
          perror("server: bind");
          continue;
      }
    break;
  }

  freeaddrinfo(servinfo); // all done with this structure
  if (p == NULL)  {
      fprintf(stderr, "server: failed to bind\n");
      exit(1);
  }
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
  }

  printf("server: waiting for connections...\n");
  while(1) {  // main accept() loop
      sin_size = sizeof their_addr;
      new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
      if (new_fd == -1) {
          perror("accept");
          continue;
      }
      inet_ntop(their_addr.ss_family,
          get_in_addr((struct sockaddr *)&their_addr),
          s, sizeof s);
      printf("server: got connection from %s\n", s);
      if (!fork()) { // this is the child process
          close(sockfd); // child doesn't need the listener
          if (send(new_fd, "Hello, world!", 13, 0) == -1)
              perror("send");
          close(new_fd);
          exit(0);
      }
      close(new_fd);  // parent doesn't need this
  }

  return 0;
}



// int main(int argc, char *argv){
//   struct addrinfo *socket_address;
//   struct sockaddr_in cliaddr, servaddr;
//   int sock_fd, new_fd;  // listening socket and new connection Socket
//   pid_t childpid;
//   socklen_t clilen;
//
//   bzero(&servaddr, sizeof(servaddr));           // Initialize servaddr with bits 0
//   servaddr.sin_family = AF_INET;                // Set to IPV4
//   servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Attribute localhost
//   servaddr.sin_port = htons(SERV_PORT);         // Attribute port
//
//   // Create and check listening socket
//   sock_fd = socket(PF_INET, SOCK_STREAM, 0); //(IPV4, TCP, IP)
//   if(sock_fd == -1) {
//     fprintf(stderr, "Socker creatiion failed.\n");
//     exit(0);
//   }
//
//   // Associate socket with the machine IP address
//   if(bind(sock_fd, socket_address->ai_addr, socket_address->ai_addrlen)) {
//     fprintf(stderr, "Socket binding failed.\n");
//     exit(0);
//   }
//
//   // Set up listening socket
//   if(listen(sock_fd, BACKLOG)) {
//     fprintf(stderr, "Listening failed.\n");
//     exit(0);
//   }
//
//   while(1){
//     clilen = sizeof(cliaddr);
//     new_fd = accept(sock_fd, (SA*)&cliaddr, &clilen);
//     if((childpid = fork()) == 0) { // If is child process
//       close(sock_fd);              // Close listening socket
//       // str_echo(new_fd);            // Process request
//       exit(0);
//     }
//     close(new_fd);
//   }
//
//   return 0;
// }