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

void encodeMessage(char* in_msg) {
  char* in_img = loadFile(INPUT_FILE);
  struct bitmapS b;
  readBitmapProperties(in_img, &b);
  char* bmp_p = b.pixelArray_p;
  for (int i = 0; i < strlen(in_msg); ++i) {
        for (int j = 7; j >= 0; --j) {
          char bit = (in_msg[i] & (1 << j) ? 1 : 0);
          *bmp_p = *bmp_p & 0xFE | bit;
          bmp_p = bmp_p + 3;
        }
    FILE* pFile = fopen(OUTPUT_FILE, "wb+");
    fwrite(b.file_p, sizeof(char), 49206, pFile);
    fclose(pFile);
  }
  return;
}

void decodeMessage(char* out_msg) {
  char* out_img = loadFile(OUTPUT_FILE);
  struct bitmapS b;
  readBitmapProperties(out_img, &b);
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
      out_msg[counter] = '\0';
      break;
    }
  ++counter;
  };

}


int main(int argc, char* argv[]) {

  char in_msg[MSG_LIMIT] = "message for my enemy";

  printf("before encoding: %s\n", in_msg);

  char out_msg[MSG_LIMIT] = "";

  encodeMessage(in_msg);
  decodeMessage(out_msg);

  printf("after decoding: %s\n", out_msg);
}


