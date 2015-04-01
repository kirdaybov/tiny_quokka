#pragma once

#include "rgbe.h"
#include "renderer.h"
#include "dds.h"

struct SImage
{
  int width = 0;
  int height = 0;

  ~SImage() {}

  pixel* pixels = nullptr;

  void open_hdri(const char* filename);
};

enum class Surface
{
  X_P = 0,
  X_N,
  Y_P,
  Y_N,
  Z_P,
  Z_N
};

void turn_right(pixel* edge, int cube_edge_i);
void assign_xyz(float& x, float& y, float& z, int c1, int c2, int half_edge, Surface surf);
void write_hdri_cross(const char* filename, const pixel** edges, int cube_edge);
void write_dds_cubemap(const char* filename, pixel** edges, int cube_edge_i);

struct SCube
{
  SCube();
  ~SCube() { clear_edges(); }

  void clear_edges();

  pixel** copy_cube();

  static void delete_edges(pixel** edges, int cube_edge_i);

  pixel* get_unreal_cubemap();

  void make_cube(pixel* pixels, int width, int height, int cube_edge_i, float angle_degrees_z = 0.0f);
  void turn_right(Surface s);

  void flip_x(Surface s);
  void flip_y(Surface s);
  void assign_borders(pixel* top, pixel* bottom, pixel* left, pixel* right, Surface k);
  void blur(int power);

  pixel* edges[6];
  pixel* blurred_edges[6];
  int cube_edge_i;
};

