#ifndef _HANDLESOCKETS_H_
#define _HANDLESOCKETS_H_

/**
* Function:    makeSocket
* 
* Description: Creates new socket using provided
*              port number
*
* Parameters:  [in] port - number of port
*
* Returns:     File descriptor of created socket      
**/
int makeSocket(int port);

/**
* Function:    readFromClient
*
* Description: Reads incoming client request and puts it
*              into provided buffer
* Parameters:  [in/out] message - empty buffer to put client message in
*              [in] message_limit - request buffer size
*              [in] descriptor - file descriptor of particular client socket 
*
* Returns:     Length of read message or -1 in case of error
**/
int readFromClient(char* message, int message_limit, int descriptor);

#endif
