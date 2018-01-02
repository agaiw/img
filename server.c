#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <time.h>

#include "handlesockets.h"

#define MAX_MESSAGE 50000 // was 4096
#define MAX_CLIENTS 1024
#define MIN_PORT 1024
#define MAX_PORT 49151
#define HEADER_SIZE 9
#define FILE_SIZE 18486

int connectedClients[MAX_CLIENTS];

int main(int argc, char* argv[]) {
  memset(connectedClients, 0, sizeof(connectedClients));
  if (argc < 2) {
    printf("Please provide port number.\n");
    exit(EXIT_SUCCESS);
  }
  int port = strtol(argv[1], NULL, 10);
  if (port < MIN_PORT || port > MAX_PORT) {
    printf("Please provide valid port number. Example usage:\n ");
    printf("./server 5100\n");
    exit(EXIT_SUCCESS);
  }
  // Create socket for accepting connections
  int sock = makeSocket(port);
  if (listen(sock, 1) < 0) {
    perror("Failed to start listening on socket.");
    exit(EXIT_FAILURE);
  }

  // Initialize the set of active sockets
  fd_set active_fds, read_fds;
  FD_ZERO(&active_fds);
  FD_SET(sock, &active_fds);

  // Eternal loop for handling connections
  // and communicating with clients
  while (1) {
    read_fds = active_fds;
    if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
      perror("Failed to select client socket.");
    }
    // Handle socket which have some pending information
    for (int i = 0; i < FD_SETSIZE; ++i) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == sock) {
          // Connection request on original socket
          struct sockaddr_in clientname;
          size_t size = sizeof(clientname);
          int new = accept(sock,
                           (struct sockaddr*)&clientname,
                           (unsigned int*)&size);
          if (new < 0) {
            perror("Failed to accept connection.");
          }
          else {
            FD_SET(new, &active_fds);
            connectedClients[new] = 1; 
         }
        } 
        else {
          // Pending data from already connected client
         char* message = (char*)malloc((FILE_SIZE + 32 + 9) * sizeof(char));
          strcpy(message, "");

          int img = 0;
          int status = readFromClient(message, FILE_SIZE + 32 + 9, i);
          time_t ct;
          time(&ct);
          printf("Time: %sMessage: %s\n", ctime(&ct), message);
          if (status > 0) {
            if ((strncmp(message, "MMMM", 4) == 0) || strncmp(message, "IIII", 4) == 0) {
              // Zero-length payload in case of Ack message
              if (strncmp(message, "IIII", 4) == 0) {
                img = 1;
              }
              char* response = "AAAA00000";
              write(i, response, strlen(response));
            }
            else if (strncmp(message, "EEEE", 4) == 0) {
              // clearing due to exit message sent by client
              close(i);
              FD_CLR(i, &active_fds);
              connectedClients[i] = 0;
            }
            else {printf("strange message received\n");}
            for (int j = 0; j < FD_SETSIZE; ++j) {
              // propagation of client message to other clients
              if (connectedClients[j] == 1) {
                if (img == 0) {
                  write(j, message, strlen(message));
                }
                else {
                  write(j, message, HEADER_SIZE + 32 + FILE_SIZE);
                }
              }
            }
          }
          else {
            // clearing due to unresponsive client
            close(i);
            FD_CLR(i, &active_fds);
            connectedClients[i] = 0;
          }
          free(message);
        }
      } // end if FD_ISSET
    }   // end for fd loop
  }     // end while 
  return 0;
}
