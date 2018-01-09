#ifndef _IMAGE_H_
#define _IMAGE_H_

#define MSG_LIMIT 1000
#define NO_OF_FILES 18

/**
* Structure:   bitmapS
* 
* Description: Holds basic bitmap properties
*
**/
struct bitmapS {
  int bmpHeight;
  int bmpWidth;
  int bmpRowSize;
  int bitsPerPixel;
  char* pixelArray_p;
  char* file_p;
} bitmapS;

/**
* Function:    loadFile
* 
* Description: Reads bitmap from a file and stores its content
               in memory buffer
*
* Parameters:  [in] filename - name of file to read
*
* Returns:     Pointer to buffer allocated on heap.
*              Caller is responsible to free the memory      
**/
char* loadFile(char* filename);

/**
* Function:    readBitmapProperties
* 
* Description: Reads basic bitmap properties from buffer and stores
*              them in bitmapS structure
*
* Parameters:  [in] buffer - pointer to memory bitmap content in memory
*              [in/out] bitmap - structure containing bitmap's properties 
*
* Returns:     Void      
**/
void readBitmapProperties(char* buffer, struct bitmapS* bitmap);

/**
* Function:    encodeMessage
* 
* Description: Encodes given text message on least significant bit
*              of red color byte in a bitmap image
*
* Parameters:  [in] in_msg - message to be encoded in bitmap
*
* Returns:     Pointer to encoded image
**/
char* encodeMessage(char* in_msg);

/**
* Function:    decodeMessage
* 
* Description: Decodes message encrypted in least significant bit
*              of red color byte in a bitmap image
*
* Parameters:  [in/out] out_msg - decoded message
*              [in] payload - received data stream
*              [in] offset - bitmap offset in data stream
*
* Returns:     Void      
**/
void decodeMessage(char* out_msg, char* payload, int offset);

#endif
