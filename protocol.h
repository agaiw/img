#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#define MAX_PAYLOAD (MAX_MESSAGE - MSG_HEADER_SIZE)
#define MAX_MESSAGE 20000
#define MAX_TXT_GROSS 618
#define MAX_TXT 600

#define MSG_TYPE_SIZE 3
#define MSG_LEN_SIZE 5
#define MSG_HEADER_SIZE (MSG_TYPE_SIZE + MSG_LEN_SIZE)

#endif
