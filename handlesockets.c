#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include "handlesockets.h"

int makeSocket(int port) {

  struct sockaddr_in server;
  memset(&server, 0, sizeof(server));
  int sock_fd = 0;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(port);

  // Create server socket
  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Failed to create socket.");
  };

 // Bind socket to port
  if (bind(sock_fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
    perror("Failed to bind socket.");
  }

  return sock_fd;
}

int readFromClient(char* message, int message_limit, int descriptor) {

  int len = read(descriptor, message, message_limit);

  if (len > 0) {
    message[len] = '\0';
  }

  return len;
}
