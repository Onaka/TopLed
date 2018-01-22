#ifndef ENUMS_H
#define ENUMS_H
#include <Arduino.h>
//Enum for checking what the current action is
enum programType {
  synctest = 'S',
  gradient = 'G',
  twinkle = 't',
  timeSet = 'T'
};

union byteInt {
  byte b;
  uint8_t i;
};

union CharLong {
  unsigned long l;
  char str[4];
};

#endif
