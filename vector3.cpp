#include <Arduino.h>
#include <math.h>
class Vector3 {
  public:
    float x, y, z;
        Vector3()
    {
      x = 0;
      y = 0;
      z = 0;
    }
    Vector3(float X, float Y, float Z)
    {
      x = X;
      y = Y;
      z = Z;
    }
    
   Vector3& operator=(Vector3 vec) {
      x = vec.x;
      y = vec.y;
      z = vec.z;
      return *this;
    }
    
     Vector3(const Vector3& vec) {
      x = vec.x;
      y = vec.y;
      z = vec.z;
    }
    float getLength()
    {
      return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
    }

    static Vector3 scalarMult(Vector3 vec, float s)
    {
      vec.x *= s;
      vec.y *= s;
      vec.z *= s;
      return Vector3(vec.x * s,vec.y * s,vec.z * s);
    }

    static Vector3 vectorAdd(Vector3 vec1, Vector3 vec2)
    {
      vec1.x += vec2.x;
      vec1.y += vec2.y;
      vec1.z += vec2.z;
      return Vector3(vec1.x + vec2.x, vec1.y +vec2.y,vec1.z +vec2.z);
    }


    static Vector3 vectorSub(Vector3 vec1, Vector3 vec2)
    {
      vec1.x -= vec2.x;
      vec1.y -= vec2.y;
      vec1.z -= vec2.z;
      return Vector3(vec1.x - vec2.x, vec1.y -vec2.y,vec1.z -vec2.z);
    }

    Vector3 normalize(Vector3 vec)
    {
      return scalarMult(vec, 1 / vec.getLength());
    }
};

