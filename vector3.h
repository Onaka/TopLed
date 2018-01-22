#ifndef VECTOR3_H
#define VECTOR3_H
#include <math.h>
class Vector3 {
  public:
    float x, y, z;
    //Constructors
    Vector3();
    
    Vector3(float X, float Y, float Z);

    //Assignment operator or something, was needed
    Vector3& operator=(Vector3 vec);

    Vector3(const Vector3& vec);
    
    //Get the length of the vector
    float getLength();
    
    //Multiply a vector by a scalar
    static Vector3 scalarMult(Vector3 vec, float s);
    
    //Add two vectors together
    static Vector3 vectorAdd(Vector3 vec1, Vector3 vec2);
    
    //Subtract vec2 from vec1
    static Vector3 vectorSub(Vector3 vec1, Vector3 vec2);
    
    //Returns a vector with a length of 1
    Vector3 normalize(Vector3 vec);
};



#endif
