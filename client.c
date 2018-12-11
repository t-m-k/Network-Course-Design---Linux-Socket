#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MESSAGE_LENGTH_LIMIT 255
#define PORT "1337"

int send_message(int sockfd)
{
   char buffer[MESSAGE_LENGTH_LIMIT];
   int rv;

   memset(buffer, 0, MESSAGE_LENGTH_LIMIT);
   fgets(buffer, MESSAGE_LENGTH_LIMIT, stdin);

   if ((rv = send(sockfd, buffer, strlen(buffer), 0)) == -1) {
      perror("client: send");
      return -1;
   } else {
      printf("client: send %3d: %s", rv, buffer);

      if ((rv = recv(sockfd, buffer, MESSAGE_LENGTH_LIMIT - 1, 0)) <= 0) {
         if (rv == 0)
            fprintf(stderr, "connection closed by peer\n");
         else
            perror("client: recv");
         return -1;
      } else {
         printf("client: recv %3d: %s", rv, buffer);
      }
   }

   return 0;
}

int main(int argc, char *argv[])
{
   int sockfd, rv;
   struct addrinfo hints, *servinfo, *p;

   if (argc != 2) {
      fprintf(stderr, "usage: client hostname\n");
      exit(1);
   }

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
   }

   for (p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == 1) {
         perror("client: socket");
         continue;
      }

      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(sockfd);
         perror("client: connect");
         continue;
      }

      break;
   }

   if (p == NULL) {
      fprintf(stderr, "client: failed to connect\n");
      return 2;
   }

   while (send_message(sockfd) != -1)
      ;

   close(sockfd);

   return 0;
}
