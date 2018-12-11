#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BACKLOG 25
#define COMMAND_LENGTH 24
#define MAX_CONNECTIONS 20
#define MESSAGE_LENGTH_LIMIT 255
#define PORT "1337"

#define CMD_NONE 0
#define CMD_QUIT 1

void caesar(char *str)
{
   int ndx;
   char chr, offset;

   for (ndx = 0; ndx < strlen(str); ndx++) {
      chr = str[ndx];

      if (isalpha(chr)) {
         offset = isupper(chr) ? 'A': 'a';

         chr = chr - offset + 3;
         chr %= 26;
         chr = chr + offset;

         str[ndx] = chr;
      }
   }
}

int handle_message(int connfd)
{
   char buffer[MESSAGE_LENGTH_LIMIT];
   int rv;

   memset(buffer, 0, MESSAGE_LENGTH_LIMIT);
   rv = recv(connfd, buffer, MESSAGE_LENGTH_LIMIT - 1, 0);

   if (rv == 0) {
      fprintf(stderr, "Remote host hung up\n");
      return -1;
   } else if (rv == -1) {
      perror("server: recv");
      return -1;
   } else {
      printf("server: recv %3d: %s", rv, buffer);

      caesar(buffer);

      if ((rv = send(connfd, buffer, strlen(buffer), 0)) == -1) {
         perror("server: send");
         return -1;
      } else {
         printf("server: send %3d: %s", rv, buffer);
      }
   }

   return 0;
}

int handle_connection(sockfd)
{
   int connfd;
   struct sockaddr_storage remote_addr;
   unsigned int addr_size = sizeof(remote_addr);

   if ((connfd = accept(sockfd, (struct sockaddr *) &remote_addr, &addr_size))
         == -1) {
      fprintf(stderr, "Could not accpet: %s\n", strerror(errno));
      return -1;
   }

   return connfd;
}

int handle_command(int fd)
{
   char buffer[COMMAND_LENGTH + 1];

   memset(buffer, 0, COMMAND_LENGTH + 1);
   fgets(buffer, COMMAND_LENGTH, stdin);

   if (strcmp(buffer, "quit\n") == 0)
      return CMD_QUIT;

   return CMD_NONE;
}

void run_server(int sockfd)
{
   fd_set readfds;
   int conn_fds[MAX_CONNECTIONS], readyfds, ndx, done = 0, newfd;
   int max_fd = sockfd;

   // Set every fd to -1.
   for (ndx = 0; ndx < MAX_CONNECTIONS; ndx++)
      conn_fds[ndx] = -1;

   // Add stdin and our listening socket to file descriptor list.
   conn_fds[0] = STDIN_FILENO;
   conn_fds[1] = sockfd;

   while (!done) {
      FD_ZERO(&readfds);

      for (ndx = 0; ndx < MAX_CONNECTIONS; ndx++) {
         if (conn_fds[ndx] != -1)
            FD_SET(conn_fds[ndx], &readfds);
      }

      readyfds = select(max_fd + 1, &readfds, NULL, NULL, NULL);

      if (readyfds == -1)
         perror("server: select");

      for (ndx = 0; ndx < MAX_CONNECTIONS; ndx++) {
         if (conn_fds[ndx] == -1)
            continue;

         if (FD_ISSET(conn_fds[ndx], &readfds)) {
            // We recieved a command message on stdin.
            if (conn_fds[ndx] == STDIN_FILENO) {
               switch (handle_command(STDIN_FILENO)) {
               case CMD_QUIT:
                  printf("quitting...\n");
                  for (ndx = 2; ndx < MAX_CONNECTIONS; ndx++) {
                     if (conn_fds[ndx] != -1)
                        close(conn_fds[ndx]);
                  }
                  done = 1;
                  break;
               }
               break;
            // A new connection is ready on the listening socket.
            } else if (conn_fds[ndx] == sockfd) {
               if ((newfd = handle_connection(sockfd)) != -1) {
                  for (ndx = 0; ndx < MAX_CONNECTIONS; ndx++) {
                     if (conn_fds[ndx] == -1) {
                        conn_fds[ndx] = newfd;
                        break;
                     }
                  }

                  if (newfd > max_fd)
                     max_fd = newfd;

                  fprintf(stderr, "fromed new connection\n");
               }
               break;
            // A connection is ready with data.
            } else {
               if (handle_message(conn_fds[ndx]) == -1) {
                  close(conn_fds[ndx]);
                  conn_fds[ndx] = -1;
               }
               break;
            }
         }
      }
   }
}

int main(void)
{
   int sockfd, rv;
   struct addrinfo hints, *servinfo, *p;

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
   }

   for (p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
         perror("server: socket");
         continue;
      }

      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(sockfd);
         perror("server: bind");
         continue;
      }

      break;
   }

   if (p == NULL) {
      fprintf(stderr, "server: failed to bind\n");
      return 2;
   }

   freeaddrinfo(servinfo);

   if (listen(sockfd, BACKLOG) == -1) {
      perror("listen");
      exit(1);
   }

   run_server(sockfd);

   close(sockfd);

   return 0;
}
