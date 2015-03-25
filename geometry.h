#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <math.h>

struct vec3
{
  float x, y, z;

  float length()
  {
    return sqrtf(x*x + y*y + z*z);
  }

  vec3 normalize()
  {
    float l = length();
    x /= l;
    y /= l;
    z /= l;
    return *this;
  }

  vec3 operator *(float f)
  {
    vec3 res = *this;
    res.x *= f;
    res.y *= f;
    res.z *= f;
    return res;
  }

  vec3 operator /(float f)
  {
    vec3 res = *this;
    res.x /= f;
    res.y /= f;
    res.z /= f;
    return res;
  }
};

inline float dot(vec3& a, vec3& b)
{
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline vec3 cross(vec3& a, vec3& b)
{
  vec3 res = { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.y*b.x - a.x*b.y };
  return res;
}

inline vec3 operator-(vec3 a, vec3 b)
{
  vec3 res = { a.x - b.x, a.y - b.y, a.z - b.z };
  return res;
}

inline vec3 operator+(vec3 a, vec3 b)
{
  vec3 res = { a.x + b.x, a.y + b.y, a.z + b.z };
  return res;
}

struct face
{
  int v1, t1, n1;
  int v2, t2, n2;
  int v3, t3, n3;
};

#endif