#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include "image.h"
#include "protocol.h"

#define MSG_MAX_NICK 16 // nie protokol
#define MSG_TIME_SIZE 24 // nie protokol
#define MSG_IMG_SIZE 18486 //nie protokol
#define MSG_SUBHEADER_SIZE 27 // nie protokol


/**
* Function:     receiveLoop
*
* Description:  Works in eternal loop. Reads stream of data
*               sent by server, one message at a time. 
*               If message is of Acknowledgement type, receiveLoop
*               informs sendLoop that it may proceed with creating
*               next message to the server. If message is of Message
*               type, it displays it to the user.
*               Function can be run in a separate thread, as it conforms
*               to definition of callback required by pthread_create.
*
* Parameters:    [in] fd - file descriptor of a socket
*
* Returns       void
**/
void* receiveLoop(void* fd);

/**
* Function:     sendLoop
*
* Description:  Works in eternal loop. Sends prepared message, one at a time,
*               and awaits message of type Acknowledgement from the server.
*               When Acknowledgement comes, sendLoop is allowed to proceed
*               to next message.
*               Function can be run in a separate thread, as it conforms
*               to definition of callback required by pthread_create.
*
* Parameters:    [in] fd - file descriptor of a socket
*
* Returns       void
**/
void* sendLoop(void* fd);

/**
* Function:     prepareMsg
*
* Description:  Prepares message conforming to application-level chat protocol
*               build upon TCP/IP stack. Function constructs messages provided
*               by the user as well as messages pertaining to user's actions
*               (chat join/chat exit).
*
* Parameters:   [in/out] msg - address of a pointer to empty message
*               [in] hello - boolean-type indication whether hello message shall
*               be constructed
*
* Returns       int - exit status (-1 means that user has requested to exit the chat)
**/
int prepareMsg(char** msg, int* hello);

pthread_mutex_t ackLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ackCond = PTHREAD_COND_INITIALIZER;

char nick[MSG_MAX_NICK];

int main(int argc, char* argv[]) {

  int port = strtol(argv[1], NULL, 10);
  strncpy(nick, argv[2], MSG_MAX_NICK - 1);
  printf("Run as: %s\n", nick);
  struct sockaddr_in server;
  memset(&server, 0, sizeof(server));
  int sock_fd = 0;
  size_t server_len;

  if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Failed to create socket.\n");
    return 1;
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if(connect(sock_fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
    printf("Failed to establish a connection with server.\n");
    return 1;
  }
  printf("Connection established. Make yourself comfortable and start chatting.\n");
  printf("To enter encrypted message, type 'secret'. To quit, just type 'exit'.\n");

  pthread_t* threads = malloc(sizeof(pthread_t) * 2);

  if (pthread_create(threads, NULL, receiveLoop, &sock_fd) != 0) {
    perror("pthread_create receiveLoop");
    exit(EXIT_FAILURE);
  }
  if (pthread_create((threads + 1), NULL, sendLoop, &sock_fd) != 0) {
    perror("pthread_create sendLoop");
    exit(EXIT_FAILURE);
  }
    if (pthread_join(threads[0], NULL) != 0) {
    perror("pthread_join receiveLoop");
    exit(EXIT_FAILURE);
  }
   if (pthread_join(threads[1], NULL) != 0) {
    perror("pthread_join sendLoop");
    exit(EXIT_FAILURE);
  }
}

void* receiveLoop(void* fd) {
  int sock_fd = *((int*)fd);
  char header[MSG_HEADER_SIZE];
  char* payload = malloc(sizeof(char) * MAX_PAYLOAD);
  while (1) {
    int readBytes;
    memset(header, 0, MSG_HEADER_SIZE);
    memset(payload, 0, MAX_PAYLOAD);
    readBytes = read(sock_fd, header, MSG_HEADER_SIZE);

    if (readBytes <= 0) {
      close(sock_fd);
      perror("Lost connection to server. Exiting...\n");
      exit(EXIT_FAILURE);
    }

    if (strncmp(header, "ACK", MSG_TYPE_SIZE) == 0) {
      // inform sendLoop that ack has been received
      if (pthread_mutex_lock(&ackLock) != 0) {
        perror("receiveLoop ackLock lock");
        exit(EXIT_FAILURE);
      }
      if (pthread_cond_signal(&ackCond) != 0) {
        perror("receiveLoop ackCond signal");
        exit(EXIT_FAILURE);
      }
      if (pthread_mutex_unlock(&ackLock) != 0) {
        perror("receiveLoop ackLock unlock");
        exit(EXIT_FAILURE);
      }
    }
    else if (strncmp(header, "MSG", MSG_TYPE_SIZE) == 0 || strncmp(header, "EXT", MSG_TYPE_SIZE) == 0) {
      char len[6];
      strncpy(len, header + MSG_TYPE_SIZE, MSG_LEN_SIZE);
      int msgLen = atoi(len);
      readBytes = read(sock_fd, payload, msgLen);
      printf("%s\n", payload);
    }
    else if (strncmp(header, "IMG", MSG_TYPE_SIZE) == 0) {
      char len[6];
      strncpy(len, header + MSG_TYPE_SIZE, MSG_LEN_SIZE);
      int msgLen = atoi(len);

      readBytes = read(sock_fd, payload, msgLen);
      char* message = malloc(sizeof(char) * MAX_TXT);
      decodeMessage(message, payload, msgLen - MSG_IMG_SIZE);
      printf("%.*s%s", msgLen - MSG_IMG_SIZE, payload, message);
    }
    else {
      perror("unknown message received");
      exit(EXIT_FAILURE);
    }
  }
}

void* sendLoop(void* fd) {
  int sock_fd = *((int*)fd);
  int hello = 1;
  while (1) {
    char* msg;
    int status = prepareMsg(&msg, &hello);
    int msgLen = 0;
    if (status == 1) { msgLen = MSG_SUBHEADER_SIZE + strlen(nick) + MSG_IMG_SIZE; }
    else {msgLen = strlen(msg);}
    write(sock_fd, msg, MSG_HEADER_SIZE + msgLen);
    free(msg);
    if (status == -1) {
      close(sock_fd);
      printf("Bye.\n");
      exit(EXIT_SUCCESS);
    }
    if (pthread_mutex_lock(&ackLock) != 0) {
        perror("sendLoop ackLock lock");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_wait(&ackCond, &ackLock) != 0) {
      perror("receiveLoop ackCond signal");
      exit(EXIT_FAILURE);
    }
    if (pthread_mutex_unlock(&ackLock) != 0) {
      perror("receiveLoop ackLock unlock");
      exit(EXIT_FAILURE);
    }
  }
}

int prepareMsg(char** msg, int* hello) {

  char* text = malloc(sizeof(char) * MAX_MESSAGE);
  int ex = 0;
  if (*hello != 1) {
    fgets(text, MAX_TXT - 1, stdin);
    // erase just printed message after writing it to 'text' buffer
    printf("\33[1A\33[2K");
    if (strcmp(text, "exit\n") == 0) {
      ex = -1;
      strcpy(text, "*** User has left the chat ***\n");
    }

    if (strcmp(text, "secret\n") == 0) {
      ex = 1;
      printf("Please enter a message to encrypt.\n");
      strcpy(text, "SECRET MESSAGE: ");
      char secret_text[MAX_TXT];
      fgets(secret_text, MAX_TXT - 17, stdin);
      strncat(text, secret_text, MAX_TXT - 17);
      char* img_buffer = encodeMessage(text);
      *text = '\0';
      memcpy(text, img_buffer, MSG_IMG_SIZE);
      printf("\33[1A\33[2K");
      printf("\33[1A\33[2K");
    }
  }
  else {
    strcpy(text, "*** User has joined the chat ***\n");
    *hello = 0;
  }
  *msg = malloc(MAX_MESSAGE * sizeof(char));
  char* payload = malloc(MAX_PAYLOAD * sizeof(char));
  if (ex == 0) {
    strcpy(*msg, "MSG");
  }
  else if (ex == 1) {
    strcpy(*msg, "IMG");
  }
  else {
    strcpy(*msg, "EXT");
  }
  time_t ct;
  time(&ct);
  payload[0] = '\0';
  strncat(payload, ctime(&ct), MSG_TIME_SIZE);
  strcat(payload, " ");
  strcat(payload, nick);
  strcat(payload, ": ");

  int len;
  if (ex == 1) {
    len = MSG_SUBHEADER_SIZE + strlen(nick) + MSG_IMG_SIZE;
    char* payload_img_p = payload + MSG_SUBHEADER_SIZE + strlen(nick);
    memcpy(payload_img_p, text, len);
  }
  else {
    (strlen(text) - 1 < MAX_TXT) ?
    strncat(payload, text, strlen(text) - 1) : 
    strncat(payload, text, MAX_TXT);
    len = (int)strlen(payload);
  }

  char msgLen[6];
  sprintf(msgLen, "%d", len);
  if (len < 10) {
    strcat(*msg, "0000");
  }
  else if (len < 100) {
    strcat(*msg, "000");
  }
  else if (len < 1000) {
    strcat(*msg, "00");
  }
  else if (len < 10000) {
    strcat(*msg, "0");
  }
  strcat(*msg, msgLen);
  if (ex == 1) {
    memcpy(*msg + MSG_HEADER_SIZE, payload, MSG_SUBHEADER_SIZE + strlen(nick) + MSG_IMG_SIZE);
  } 
  else {
    strcat(*msg, payload);
  }
  return ex;
}


