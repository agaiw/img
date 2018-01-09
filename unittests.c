#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "image.h"
#include "protocol.h"

void messageEncodeTest();
void loadFileTest();
void bitmapReadTest();

int main() {
  messageEncodeTest();
  loadFileTest();
  bitmapReadTest();
  printf("All tests successfully passed.\n");
}

void messageEncodeTest() {

  // Test Case 1: Encoding and decoding a valid message
  char* in_msg = "This is a secret message for my enemy";
  char* out_msg = malloc(sizeof(char) * MAX_TXT);
  decodeMessage(out_msg, encodeMessage(in_msg), 0);
  assert(strcmp(in_msg, out_msg) == 0);

  // Test Case 2: Encoding and decoding a valid message
  in_msg = "Hello/nKitty/n";
  decodeMessage(out_msg, encodeMessage(in_msg), 0);
  assert(strcmp(in_msg, out_msg) == 0);

  // Test Case 3: Encoding and decoding an empty message
  in_msg = "";
  decodeMessage(out_msg, encodeMessage(in_msg), 0);
  assert(strcmp(in_msg, out_msg) == 0);
}

void loadFileTest() {
  char* buffer = NULL;
  // Test Case 4: Loading existing bitmap into memory
  buffer = loadFile("bmps/muppets.bmp");
  assert(buffer != NULL);

  // Test Case 5: Loading unexisting bitmap into memory
  buffer = loadFile("bmps/unexistingFile.bmp");
  assert(buffer == NULL);
}

void bitmapReadTest() {
  // Test Case 6: Reading bitmap properties
  char* buffer = loadFile("bmps/butterfly.bmp");
  struct bitmapS b;
  readBitmapProperties(buffer, &b);
  assert(b.bmpWidth == 96);
  assert(b.bmpHeight == 64);
  assert(b.bitsPerPixel == 24);
  assert(b.bmpRowSize == 288);

}
