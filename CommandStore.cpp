#include "CommandStore.h"

ChangeInstruction::ChangeInstruction()
{
  type = synctest;
}

ChangeInstruction::ChangeInstruction (byte instruct[50]) {
  switch ((programType)(int)instruct[0])
  {
    case gradient:
      type = gradient;
      gradientStart = Vector3(float(instruct[1]), float(instruct[2]), float(instruct[3]));
      gradientEnd =  Vector3(float(instruct[4]), float(instruct[5]), float(instruct[6]));
      byte converty[4];
      converty[0] = instruct[7];
      converty[1] = instruct[8];
      converty[2] = instruct[9];
      converty[3] = instruct[10];

      startTime = convertCharsToUL(converty);

      converty[0] = instruct[11];
      converty[1] = instruct[12];
      converty[2] = instruct[13];
      converty[3] = instruct[14];
      duration = convertCharsToUL(converty);
      gradientChange = Vector3::vectorSub(gradientEnd, gradientStart);
      break;

    case synctest:
      type = synctest;
      break;
    case twinkle:
      type = twinkle;
      break;

    case timeSet:
      type = timeSet;
      break;
    default:
      type = synctest;
      break;
  }
}

void ChangeInstruction::doThing(unsigned long serverTime, int ledArray[][3])
{
  switch (type)
  {
    case gradient:
      if (serverTime >= startTime && serverTime <= startTime + duration)
      {
        writeVector(0, ledArray, Vector3::vectorAdd(gradientStart, Vector3::scalarMult(gradientChange, float((serverTime - startTime) / duration))));
      }
      break;
    default:
      bool is1000 = false;
      //Square wave function that takes the server time and outputs true or false (hopefully)
      if (pow(-1, floor(float(serverTime / 1000))) < 0)
      {
        is1000 = false;
      } else {
        is1000 = true;
      }
      digitalWrite(2, is1000);
      break;
  }
}

unsigned long ChangeInstruction::convertCharsToUL(byte chars[4])
{
  union CharLong converter;
  //Might need to change endianness
  converter.str[0] = chars[0];
  converter.str[1] = chars[1];
  converter.str[2] = chars[2];
  converter.str[3] = chars[3];
  return converter.l;
}
void ChangeInstruction::writeVector(int led, int ledArray[][3], Vector3 vec)
{
  analogWrite(ledArray[led][0], int(vec.x));
  analogWrite(ledArray[led][1], int(vec.y));
  analogWrite(ledArray[led][2], int(vec.z));
}
