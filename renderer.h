#pragma once

#include <fstream>
#include <sstream>
#include <iostream>

#include <vector>
#include "geometry.h"

struct pixel
{
  float r = 0, g = 0, b = 0;
  pixel() : r(0), g(0), b(0) {}
  pixel(float ar, float ag, float ab) : r(ar), g(ag), b(ab) {}

  pixel operator/(float v)
  {
    return pixel(r / v, g / v, b / v);
  }

  pixel operator*(float v)
  {
    return pixel(r * v, g * v, b * v);
  }

  pixel operator+(pixel& v)
  {
    return pixel(this->r + v.r, this->g + v.g, this->b + v.b);
  }
};



struct model
{
  std::vector<vec3> verts;
  std::vector<vec3> normals;
  std::vector<vec3> uvs;
  std::vector<face> faces;

  float scale = 1.0f;

  pixel* diffuse = nullptr;
  int d_width = 0;
  int d_height = 0;

  void from_file(std::string filename)
  {
    verts.clear();
    faces.clear();
    normals.clear();
    uvs.clear();

    std::ifstream file(filename);

    float c_max = 0;

    for (std::string line; std::getline(file, line);)
    {
      std::istringstream s_line = std::istringstream(line);

      std::string id;
      s_line >> id;
      if (id == "v")
      {
        vec3 vert;
        s_line >> vert.x >> vert.y >> vert.z;
        if (abs(vert.x) > c_max) c_max = abs(vert.x);
        if (abs(vert.y) > c_max) c_max = abs(vert.y);
        if (abs(vert.z) > c_max) c_max = abs(vert.z);
        verts.push_back(vert);
      }
      else
      if (id == "vt")
      {
        vec3 uv;
        s_line >> uv.x >> uv.y >> uv.z;
        uvs.push_back(uv);
      }
      else
      if (id == "vn")
      {
        vec3 normal;
        s_line >> normal.x >> normal.y >> normal.z;
        normals.push_back(normal);
      }
      else
      if (id == "f")
      {
        face f;
        char trash;
        s_line >> f.v1 >> trash >> f.t1 >> trash >> f.n1;
        s_line >> f.v2 >> trash >> f.t2 >> trash >> f.n2;
        s_line >> f.v3 >> trash >> f.t3 >> trash >> f.n3;
        f.v1--; f.t1--; f.n1--;
        f.v2--; f.t2--; f.n2--;
        f.v3--; f.t3--; f.n3--;
        faces.push_back(f);
      }
    }

    float angle_x = 45.f / 180 * 3.14;
    float angle_z = 45.f / 180 * 3.14;

    for (vec3& v : verts)
    {
      v = v / c_max * scale;
      
      float x = v.x*cosf(angle_x) - v.y*sinf(angle_x);
      float y = v.x*sinf(angle_x) + v.y*cosf(angle_x);
      float z = v.z;

      //v.x = x;
      //v.y = y;
      //v.z = z;

      //x = v.x;
      //y = v.y*cosf(angle_z) - v.z*sinf(angle_z);
      //z = v.y*sinf(angle_z) + v.z*cosf(angle_z);
      
      //v.x = x;
      //v.y = y;
      //v.z = z;
    }
      

    file.close();
  }

  //TGAImage diffuse;
};

struct renderer
{
  int width = 1024;
  int height = 1024;

  float* z_buffer = nullptr;

  void clear_z();
  void render(pixel* image, model &m);
  void draw_triangular_model(model& a_model, pixel* a_image);
  void triangle(face& f, model& m, pixel* image, float intensity);  
};






