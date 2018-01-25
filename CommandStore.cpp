#include "CommandStore.h"
#include "TopLedUtilityFunctions.h"
ChangeInstruction::ChangeInstruction()
{
  type = synctest;
}

ChangeInstruction::ChangeInstruction (char instruct[50]) {
  switch ((programType)(int)instruct[0])
  {
    case gradient:
      type = gradient;
      gradientStart = Vector3(float(instruct[1]), float(instruct[2]), float(instruct[3]));
      gradientEnd =  Vector3(float(instruct[4]), float(instruct[5]), float(instruct[6]));
      char converty[4];
      converty[0] = instruct[7];
      converty[1] = instruct[8];
      converty[2] = instruct[9];
      converty[3] = instruct[10];

      startTime = convertCharsToUL(converty);
      //Serial.printf("startTime: %u" , startTime);

      converty[0] = instruct[11];
      converty[1] = instruct[12];
      converty[2] = instruct[13];
      converty[3] = instruct[14];
      duration = convertCharsToUL(converty);
      //Serial.printf("duration: %u \n", duration);
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
  if (serverTime >= startTime && serverTime <= startTime + duration)
  {
    switch (type)
    {
      case gradient:
        {
          Vector3 addVector = Vector3::scalarMult(gradientChange, float((float(serverTime) - float(startTime)) / float(duration)));
          Vector3 resultant = Vector3::vectorAdd(gradientStart, addVector);
          writeVector(0, ledArray, resultant);
          break;
        }

      default:
        {
          bool is1000 = false;
          //Square wave function that takes the server time and outputs true or false (hopefully)
          if (pow(-1, floor(float(serverTime / 1000))) < 0)
          {
            is1000 = false;
          } else {
            is1000 = true;
          }
          break;
        }
    }
  }
}

unsigned long ChangeInstruction::convertCharsToUL(char chars[4])
{
  union CharLong converter;
  //Might need to change endianness
  converter.str[0] = chars[0];
  converter.str[1] = chars[1];
  converter.str[2] = chars[2];
  converter.str[3] = chars[3];
  //Serial.printf(" converted %u\n", converter.l);
  return converter.l;
}
void ChangeInstruction::writeVector(int led, int ledArray[][3], Vector3 vec)
{
  //The analogwrite on this micro is 10-bit instead of 8-bit
  //and we're controlling common cathode LEDs, so we need to invert the input 
  analogWrite(ledArray[led][0], 1024 - int(vec.x));
  analogWrite(ledArray[led][1], 1024 - int(vec.y));
  analogWrite(ledArray[led][2], 1024 - int(vec.z));
}

