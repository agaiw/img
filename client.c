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

#define MSG_MAX_NICK 16
#define MSG_LENGTH_SIZE 6
#define MSG_TIME_SIZE 24
#define MSG_IMG_SIZE 18486
#define MSG_SUBHEADER_SIZE 27

typedef enum {
  TEXT_MESSAGE = 0,
  IMAGE_MESSAGE = 1,
  HELLO_MESSAGE = 2,
  EXIT_MESSAGE = 3
} msgType;

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
*               [in/out] type - type of message being constructed
*
* Returns       Void
**/
void prepareMsg(char** msg, msgType* type);

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
    perror("Failed to create socket.\n");
    exit(EXIT_FAILURE);
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if(connect(sock_fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
    perror("Failed to establish a connection with server.\n");
    exit(EXIT_FAILURE);
  }
  printf("Connection established. Make yourself comfortable and start chatting.\n");
  printf("Max message length: 600 characters.\n");
  printf("To enter encrypted message, type 'secret'. To quit, just type 'exit'.\n");

  pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * 2);

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
  char* payload = (char*)malloc(sizeof(char) * MAX_PAYLOAD);
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
      char len[MSG_LENGTH_SIZE];
      strncpy(len, header + MSG_TYPE_SIZE, MSG_LEN_SIZE);
      int msgLen = atoi(len);
      readBytes = read(sock_fd, payload, msgLen);
      printf("%s\n", payload);
    }
    else if (strncmp(header, "IMG", MSG_TYPE_SIZE) == 0) {
      char len[MSG_LENGTH_SIZE];
      strncpy(len, header + MSG_TYPE_SIZE, MSG_LEN_SIZE);
      int msgLen = atoi(len);

      readBytes = read(sock_fd, payload, msgLen);
      char* message = (char*)malloc(sizeof(char) * MAX_TXT_GROSS);
      decodeMessage(message, payload, msgLen - MSG_IMG_SIZE);
      printf("%.*s%s", msgLen - MSG_IMG_SIZE, payload, message);
      free(message);
    }
    else {
      perror("unknown message received");
      exit(EXIT_FAILURE);
    }
  }
  free(payload);
}

void* sendLoop(void* fd) {
  int sock_fd = *((int*)fd);

  msgType type = HELLO_MESSAGE;
  while (1) {
    char* msg;
    prepareMsg(&msg, &type);
    int msgLen = 0;
    switch(type) {
      case HELLO_MESSAGE:
        msgLen = strlen(msg);
        write(sock_fd, msg, MSG_HEADER_SIZE + msgLen);
        free(msg);
        type = TEXT_MESSAGE;
        break;
      case TEXT_MESSAGE:
        msgLen = strlen(msg);
        write(sock_fd, msg, MSG_HEADER_SIZE + msgLen);
        free(msg);
        break;
      case IMAGE_MESSAGE:
        msgLen = MSG_SUBHEADER_SIZE + strlen(nick) + MSG_IMG_SIZE;
        write(sock_fd, msg, MSG_HEADER_SIZE + msgLen);
        free(msg);
        break;
      case EXIT_MESSAGE:
        msgLen = strlen(msg);
        write(sock_fd, msg, MSG_HEADER_SIZE + msgLen);
        free(msg);
        close(sock_fd);
        printf("Bye.\n");
        exit(EXIT_SUCCESS);
        break;
      default:
        perror("unknown status from prepareMsg");
        exit(EXIT_FAILURE);
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

void prepareMsg(char** message, msgType* type) {

  /* Message structure: 

  +--MESSAGE------------------------------------------------+
  |  +--HEADER: 8---------------+  +--PAYLOAD-------------+ |
  |  |  TYPE: 3   |  LEGTH: 5   |  |  SUBHEADER  |  TEXT  | |
  |  +--------------------------+  +----------------------+ |
  +---------------------------------------------------------+

  */

  /* Step 1: Gathering input from the user
  /*         and generating TEXT part of PAYLOAD */

  char* text = (char*)malloc(sizeof(char) * MAX_MESSAGE);
  if (*type == HELLO_MESSAGE) {
    strcpy(text, "*** User has joined the chat ***\n");
  }
  else {
    fgets(text, MAX_TXT, stdin);

    // erase just printed message after writing it to 'text' buffer
    printf("\33[1A\33[2K");
    if (strcmp(text, "exit\n") == 0) {
      *type = EXIT_MESSAGE;
      strcpy(text, "*** User has left the chat ***\n");
    }

    else if (strcmp(text, "secret\n") == 0) {
      *type = IMAGE_MESSAGE;
      printf("Please enter a message to encrypt. \n");
      strcpy(text, "SECRET MESSAGE: ");
      char secret_text[MAX_TXT];
      fgets(secret_text, MAX_TXT, stdin);
      strncat(text, secret_text, MAX_TXT);
      char* img_buffer = encodeMessage(text);
      *text = '\0';
      memcpy(text, img_buffer, MSG_IMG_SIZE);
      printf("\33[1A\33[2K");
      printf("\33[1A\33[2K");
      free(img_buffer);
    }
    else { *type = TEXT_MESSAGE; }
  }

  /* Step 2: Generating SUBHEADER 
  /          and appending it to PAYLOAD */ 
 
  char* payload = (char*)malloc(MAX_PAYLOAD * sizeof(char));
  time_t ct;
  time(&ct);
  payload[0] = '\0';
  strncat(payload, ctime(&ct), MSG_TIME_SIZE);
  strcat(payload, " ");
  strcat(payload, nick);
  strcat(payload, ": ");

  /* Step 3: Appending TEXT to PAYLOAD */ 

  int length;
  if (*type == IMAGE_MESSAGE) {
    length = MSG_SUBHEADER_SIZE + strlen(nick) + MSG_IMG_SIZE;
    char* payload_img_p = payload + MSG_SUBHEADER_SIZE + strlen(nick);
    memcpy(payload_img_p, text, length);
  }
  else {
    (strlen(text) - 1 < MAX_TXT) ?
    strncat(payload, text, strlen(text) - 1) : 
    strncat(payload, text, MAX_TXT);
    length = (int)strlen(payload);
  }

  /* Step 4: Generating MESSAGE buffer
  /          and appending HEADER (= TYPE + LENGTH) to it */

  *message = (char*)malloc(MAX_MESSAGE * sizeof(char));
  switch (*type) {
    case HELLO_MESSAGE:  //passthrough
    case TEXT_MESSAGE:
      strcpy(*message, "MSG");
      break;
    case IMAGE_MESSAGE:
      strcpy(*message, "IMG");
      break;
    case EXIT_MESSAGE:
      strcpy(*message, "EXT");
      break;
  }

  char msgLen[MSG_LENGTH_SIZE];
  sprintf(msgLen, "%d", length);
  if (length < 10) {
    strcat(*message, "0000");
  }
  else if (length < 100) {
    strcat(*message, "000");
  }
  else if (length < 1000) {
    strcat(*message, "00");
  }
  else if (length < 10000) {
    strcat(*message, "0");
  }
  strcat(*message, msgLen);

  /* Step 5: Appending PAYLOAD to MESSAGE */

  if (*type == IMAGE_MESSAGE) {
    memcpy(*message + MSG_HEADER_SIZE, payload, MSG_SUBHEADER_SIZE + strlen(nick) + MSG_IMG_SIZE);
  } 
  else {
    strcat(*message, payload);
  }

  /* Step 6: Cleanup */

  free(payload);
  free(text);
  return;
}


