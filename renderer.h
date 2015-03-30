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
};

pixel operator+(pixel& a, pixel& b)
{
  return pixel(a.r + b.r, a.g + b.g, a.b + b.b);
}

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

void triangle(face& f, model& m, pixel* image, int w, int h, float intensity, float* z_buffer)
{
  vec3 verts[3] = { m.verts[f.v1], m.verts[f.v2], m.verts[f.v3] };
  vec3 uv_verts[3] = { m.uvs[f.t1], m.uvs[f.t2], m.uvs[f.t3] };
  vec3 normals[3] = { m.normals[f.n1], m.normals[f.n2], m.normals[f.n3] };
  if (verts[0].x > verts[1].x) { std::swap(verts[0], verts[1]), std::swap(uv_verts[0], uv_verts[1]); std::swap(normals[0], normals[1]); }
  if (verts[1].x > verts[2].x) { std::swap(verts[1], verts[2]), std::swap(uv_verts[1], uv_verts[2]); std::swap(normals[1], normals[2]); }
  if (verts[0].x > verts[1].x) { std::swap(verts[0], verts[1]), std::swap(uv_verts[0], uv_verts[1]); std::swap(normals[0], normals[1]); }

  for (int j = 0; j < 3; j++)
  {
    verts[j].x = (verts[j].x + 1.f)*w / 2;
    verts[j].y = (verts[j].y + 1.f)*h / 2;
  }

  vec3 dir1 = verts[1] - verts[0];
  vec3 dir2 = verts[2] - verts[0];
  vec3 uv_dir1 = uv_verts[1] - uv_verts[0];
  vec3 uv_dir2 = uv_verts[2] - uv_verts[0];

  float step_x = 0.4f;
  float step_y = 0.4f;
  for (float x = verts[0].x; x <= verts[2].x; x += step_x)
  {
    bool bSecondHalf = x >= verts[1].x;
    if (bSecondHalf) { dir1 = verts[2] - verts[1]; uv_dir1 = uv_verts[2] - uv_verts[1]; }

    float k1 = dir1.x != 0 ? (x - (bSecondHalf ? verts[1].x : verts[0].x)) / dir1.x : 1.f;
    float k2 = dir2.x != 0 ? (x - verts[0].x) / dir2.x : 1.f;
    vec3 p1 = (bSecondHalf ? verts[1] : verts[0]) + dir1*k1;
    vec3 p2 = verts[0] + dir2*k2;

    vec3 uv_p1 = (bSecondHalf ? uv_verts[1] : uv_verts[0]) + uv_dir1*k1;
    vec3 uv_p2 = uv_verts[0] + uv_dir2*k2;

    if (p1.y > p2.y) { std::swap(p1, p2); std::swap(uv_p1, uv_p2); }

    vec3 dir = p2 - p1;
    vec3 uv_dir = uv_p2 - uv_p1;
        
    for (float y = p1.y; y <= p2.y; y += 1)
    {
      float k = dir.y != 0 ? (y - p1.y) / (p2.y - p1.y) : 1.f;
      vec3 p = p1 + dir*k;
      vec3 uv_p = uv_p1 + uv_dir*k;

      if (p.x > w - 1 ||
        p.y > h - 1 ||
        p.x < 0 || p.y < 0)
        continue;

      int index = int(p.x) + int(p.y)*w;
      if (z_buffer[index] < p.z)
      {
        z_buffer[index] = p.z;
        int dif_index = int(m.d_width*uv_p.x) + m.d_width*int(m.d_height*uv_p.y);
        if (dif_index < 0) dif_index = 0;        
        if (dif_index >= m.d_width*m.d_height) dif_index = m.d_width*m.d_height - 1;
        image[index] = m.diffuse ? m.diffuse[dif_index]*intensity : pixel(intensity, intensity, intensity);
      }
    }
  }

  //dir1 = verts[2] - verts[1];		
  //
  //for(float x = verts[1].x; x <= verts[2].x; x++)
  //{
  //	vec3 p1 = verts[1] + dir1*(x-verts[1].x)/dir1.x;
  //	vec3 p2 = verts[0] + dir2*(x-verts[0].x)/dir2.x;;
  //	if(dir1.x == 0) p1 = verts[2];
  //	if(dir2.x == 0) p2 = verts[2];
  //	
  //	if(p1.y > p2.y) std::swap(p1, p2);
  //	
  //	vec3 dir = p2 - p1;
  //
  //	for(float y = p1.y; y <= p2.y; y++)
  //	{
  //		vec3 p = p1 + dir*(y-p1.y)/(p2.y - p1.y);
  //		if(dir.y == 0) p = p2;
  //
  //		if(p.x > image.get_width()-1 ||
  //			p.y > image.get_height()-1 ||
  //			p.x < 0 || p.y < 0)
  //			continue;
  //
  //		int index = int(p.x) + int(p.y)*image.get_width();
  //		if(z_buffer[index] < p.z)
  //		{
  //			z_buffer[index] = p.z;
  //			image.set(p.x, p.y, color);
  //		}
  //	}
  //}

}

void draw_triangular_model(model& a_model, pixel* a_image, int w, int h, float* z_buffer)
{
  for (int i = 0; i < a_model.faces.size(); i++)
  {
    face f = a_model.faces[i];
    vec3 v[3] = { a_model.verts[f.v1], a_model.verts[f.v2], a_model.verts[f.v3] };

    vec3 e1, e2;
    e1 = v[1] - v[0];
    e2 = v[2] - v[0];

    vec3 normal = cross(e2, e1);
    normal.normalize();

    vec3 light = { 0, 0, 1 };

    float intensity = dot(normal, light);

    if (intensity > 0)
      triangle(f, a_model, a_image, w, h, intensity, z_buffer);
  }
}

void render(pixel* image, int width, int height, pixel* diffuse = nullptr, int d_w = 0, int d_h = 0)
{
  model m;
  m.from_file("skysphere.obj");
  m.diffuse = diffuse;
  m.d_height = d_h;
  m.d_width = d_w;
  //m.from_file("skysphere.obj");
  //m.from_file("african_head.obj");
  //m.diffuse.read_tga_file("tinyrenderer-master\\tinyrenderer-master\\obj\\african_head_diffuse.tga");
  //m.diffuse.flip_vertically();

  float* z_buffer = new float[width*height];
  for (int i = 0; i < width*height; i++) z_buffer[i] = -FLT_MAX;

  draw_triangular_model(m, image, width, height, z_buffer);

  model m2;
  m2.scale = 2.0f;
  m2.from_file("skysphere_inv.obj");
  m2.diffuse = diffuse;
  m2.d_height = d_h;
  m2.d_width = d_w;
    
  draw_triangular_model(m2, image, width, height, z_buffer);

  delete[] z_buffer;
}



