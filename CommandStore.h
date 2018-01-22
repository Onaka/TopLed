#ifndef COMMANDSTORE_H
#define COMMANDSTORE_H
#include "Enums.h"
#include "Vector3.h"
class ChangeInstruction
{
  public:
    programType type;
    Vector3 gradientStart;
    Vector3 gradientEnd;
    Vector3 gradientChange;
    unsigned long startTime, duration;

    ChangeInstruction();

    ChangeInstruction (byte instruct[50]);

    void doThing(unsigned long serverTime, int ledArray[][3]);

    unsigned long convertCharsToUL(byte chars[4]);
    //Writes a vector to the specified LED
    static void writeVector(int led, int ledArray[][3], Vector3 vec);

};

#endif
