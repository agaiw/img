#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

char* loadFile(char* filename) {

  FILE* pFile;
  long lSize;
  char* buffer;
  size_t result;

  pFile = fopen(filename, "rb");
 
  fseek(pFile, 0, SEEK_END);
  lSize = ftell(pFile);
  fseek(pFile, 0, SEEK_SET);

  buffer = (char*)malloc(sizeof(char) * lSize);
  result = fread(buffer, 1, lSize, pFile);
  fclose(pFile);

  return buffer;

}

void readBitmapProperties(char* buffer, struct bitmapS* bitmap) {
  bitmap->file_p = buffer;
  bitmap->pixelArray_p = buffer + 54;
  bitmap->bmpWidth = *((int*)(buffer + 18));
  bitmap->bmpHeight = *((int*)(buffer + 22));
  short bitsPerPixel = *((short*)(buffer + 28));
  bitmap->bitsPerPixel = bitsPerPixel;
  bitmap->bmpRowSize = ((int)((bitsPerPixel * bitmap->bmpWidth + 31) / 32)) * 4;

}

char* encodeMessage(char* in_msg) {
  char* img_buffer = loadFile(INPUT_FILE);
  struct bitmapS b;
  readBitmapProperties(img_buffer, &b);
  char* bmp_p = b.pixelArray_p;
  for (int i = 0; i <= strlen(in_msg); ++i) {
        for (int j = 7; j >= 0; --j) {
          char bit = (in_msg[i] & (1 << j) ? 1 : 0);
          *bmp_p = *bmp_p & 0xFE | bit;
          bmp_p = bmp_p + 3;
        }
  }
  return img_buffer;
}

void decodeMessage(char* out_msg, char* payload) {
  struct bitmapS b;
  payload += 32;
  for (int i = 0; i < 100; i++) {
  }
  readBitmapProperties(payload, &b);
  char* bmp_p = b.pixelArray_p;
  char character;
  int counter = 0;

  while(1) { 
    character = 0;
    for (int j = 7; j >= 0; --j) {
        char bit = *bmp_p & 0x1;
        character |= bit << j;
        bmp_p += 3;
      }
    out_msg[counter] = character;
    if (character == 0) {
      //out_msg[counter] = '\0';
      break;
    }
  ++counter;
  };

}
