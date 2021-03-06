#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "image.h"

char* bitmps[] = {"bmps/bike.bmp",
                  "bmps/bird.bmp",
                  "bmps/butterfly.bmp",
                  "bmps/car.bmp",
                  "bmps/cat.bmp",
                  "bmps/cows.bmp",
                  "bmps/dog.bmp",
                  "bmps/fish.bmp",
                  "bmps/flowers.bmp",
                  "bmps/food.bmp",
                  "bmps/friends.bmp",
                  "bmps/fundog.bmp",
                  "bmps/guineapig.bmp",
                  "bmps/jellyfish.bmp",
                  "bmps/monkey.bmp",
                  "bmps/muppets.bmp",
                  "bmps/rhino.bmp",
                  "bmps/turtle.bmp"};

char* loadFile(char* filename) {
  FILE* pFile;
  long lSize;
  char* buffer;
  size_t result;

  pFile = fopen(filename, "rb");
  if (pFile != NULL) {
    fseek(pFile, 0, SEEK_END);
    lSize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    buffer = (char*)malloc(sizeof(char) * lSize);
    result = fread(buffer, 1, lSize, pFile);
    if (result != lSize) { buffer = NULL; }
    fclose(pFile);
  }
  else { buffer = NULL; }
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
  time_t t;
  srand((unsigned) time(&t));
  int bmp_index = rand() % NO_OF_FILES;
  char* img_buffer = loadFile(bitmps[bmp_index]);
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

void decodeMessage(char* out_msg, char* payload, int offset) {
  struct bitmapS b;
  payload += offset;
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
    if (character == 0) { break; }
  ++counter;
  }
}
