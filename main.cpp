
#include <algorithm>
#include <math.h>
#include <iostream>
#include <fstream>

#ifdef _DEBUG
#include "ConsoleWindow.h"
#include "MemoryCounter.h"
#endif

#include "big_quokka.h"

#include "hdri_cubemap.h"

namespace quokka
{
  Profiler* Profiler::Instance = new Profiler();
  Profiler* GProfiler() { return Profiler::GetInstance(); }
}

struct Singletone
{ 
  Singletone()
  {
    rendered_image = new pixel[1024 * 1024];
  }

  SImage image;
  SCube cube;

  model sphere;
  model sphere_inv;

  void init()
  {    
    sphere.from_file("skysphere.obj");    
    sphere_inv.from_file("skysphere_inv.obj");    
  }

  pixel* rendered_image = nullptr;  

  renderer _renderer;

  pixel* render(float z_angle)
  {
    if (rendered_image) delete[] rendered_image;

    rendered_image = new pixel[1024 * 1024];

    pixel* diffuse = cube.get_unreal_cubemap();
    int diffuse_w = cube.cube_edge_i * 6;
    int diffuse_h = cube.cube_edge_i;
    
    sphere.diffuse = diffuse;
    sphere.d_height = diffuse_h;
    sphere.d_width = diffuse_w;
    sphere.set_rotation(z_angle);
    
    sphere_inv.diffuse = diffuse;
    sphere_inv.d_height = diffuse_h;
    sphere_inv.d_width = diffuse_w;
    sphere_inv.scale = 2.f;
    
    _renderer.clear_z();
    quokka::GProfiler()->Start("sphere");
    _renderer.draw_triangular_model(sphere, rendered_image);
    quokka::GProfiler()->Stop("sphere");

    quokka::GProfiler()->Start("sphere_inv");
    _renderer.draw_triangular_model(sphere_inv, rendered_image);
    quokka::GProfiler()->Stop("sphere_inv");
    
    delete[] diffuse;

    return rendered_image;
  }

} Singletone;

extern "C" __declspec(dllexport)
void init()
{
  Singletone.init();
#ifdef _DEBUG
  CreateConsole();
#endif
}

extern "C" __declspec(dllexport)
void deinit()
{
#ifdef _DEBUG
  quokka::GProfiler()->Print();
#endif
}

extern "C" __declspec(dllexport)
void open_hdri(const char* filename)
{
  Singletone.image.open_hdri(filename);
}

extern "C" __declspec(dllexport)
void make_cube(int cube_edge_i, float degrees)
{
  Singletone.cube.make_cube(
    Singletone.image.pixels,
    Singletone.image.width,
    Singletone.image.height,
    cube_edge_i, degrees);
}

extern "C" __declspec(dllexport)
void save_cube_dds(const char* filename, int cube_edge_i)
{
  Singletone.cube.turn_right(Surface::X_P);
  Singletone.cube.turn_right(Surface::X_P);
  Singletone.cube.turn_right(Surface::X_P);
  Singletone.cube.turn_right(Surface::X_N);
  Singletone.cube.turn_right(Surface::Y_P);
  Singletone.cube.turn_right(Surface::Y_P);

  write_dds_cubemap(filename, Singletone.cube.blurred_edges, cube_edge_i);
}

extern "C" __declspec(dllexport) int get_width()  { return Singletone.image.width; }
extern "C" __declspec(dllexport) int get_height() { return Singletone.image.height; }

extern "C" __declspec(dllexport) pixel* get_pixels() { return Singletone.image.pixels; }
extern "C" __declspec(dllexport) pixel* get_edge(int i) { return Singletone.cube.edges[i]; }
extern "C" __declspec(dllexport) pixel* get_blurred_edge(int i) { return Singletone.cube.blurred_edges[i]; }
extern "C" __declspec(dllexport) pixel* get_edge_t(int i, int turns)
{
  int cube_edge_i = Singletone.cube.cube_edge_i;

  pixel* edge = new pixel[cube_edge_i*cube_edge_i];
  memcpy(edge, Singletone.cube.edges[i], sizeof(pixel)*cube_edge_i*cube_edge_i);
  for (int i = 0; i < turns; i++)
    turn_right(edge, cube_edge_i);

  return edge;
}

extern "C" __declspec(dllexport) pixel* get_blurred_edge_t(int i, int turns)
{
  int cube_edge_i = Singletone.cube.cube_edge_i;

  pixel* edge = new pixel[cube_edge_i*cube_edge_i];
  memcpy(edge, Singletone.cube.blurred_edges[i], sizeof(pixel)*cube_edge_i*cube_edge_i);
  for (int i = 0; i < turns; i++)
    turn_right(edge, cube_edge_i);

  return edge;
}

extern "C" __declspec(dllexport) int get_float_size() { return sizeof(float); }
extern "C" __declspec(dllexport) void blur(int power) { Singletone.cube.blur(power); }

extern "C" __declspec(dllexport) pixel* render(float z_angle) { return Singletone.render(z_angle); }


//{
//  FILE* f;
//
//  errno_t err = fopen_s(&f, filename, "rb");
//
//  DDS_HEADER header;
//  read_dds_header(f, &header);
//
//  pixel* pixels = new pixel[header.dwWidth*header.dwHeight];
//  unsigned char* data = new unsigned char[header.dwWidth*header.dwHeight*4];
//  unsigned char* data_ = new unsigned char[header.dwWidth*header.dwHeight * 4];
//
//  fread(data, 1, header.dwWidth*header.dwHeight*4, f);
//  fread(data_, 1, header.dwWidth*header.dwHeight, f);
//
//  float* out_data = new float[header.dwWidth*header.dwHeight * 3];
//  float* out_data_ = new float[header.dwWidth*header.dwHeight * 3/4];
//  for (int i = 0; i < header.dwWidth*header.dwHeight; i++)
//  {
//    out_data[i * 3] =     data[i * 4 + 2]/255.f;
//    out_data[i * 3 + 1] = data[i * 4 + 1]/255.f;
//    out_data[i * 3 + 2] = data[i * 4 + 0]/255.f;
//  }
//
//  for (int i = 0; i < header.dwWidth*header.dwHeight/4; i++)
//  {
//    out_data_[i * 3] =     data_[i * 4 + 2] / 255.f;
//    out_data_[i * 3 + 1] = data_[i * 4 + 1] / 255.f;
//    out_data_[i * 3 + 2] = data_[i * 4 + 0] / 255.f;
//  }
//
//  {
//    const char* filename_o = "E:\\Work\\hdr_cubemap\\images\\output.hdr";
//    err = fopen_s(&f, filename_o, "wb");
//
//    rgbe_header_info rgbe_header;
//    rgbe_header.exposure = 1.0f;
//    strcpy_s<16>(rgbe_header.programtype, "RADIANCE");
//    rgbe_header.valid = RGBE_VALID_PROGRAMTYPE | RGBE_VALID_EXPOSURE;
//    RGBE_WriteHeader(f, header.dwWidth, header.dwHeight, &rgbe_header);
//
//    RGBE_WritePixels_RLE(f, out_data, header.dwWidth, header.dwHeight);
//    fclose(f);
//  }
//
//  const char* filename_o = "E:\\Work\\hdr_cubemap\\images\\output_.hdr";
//  err = fopen_s(&f, filename_o, "wb");
//
//  rgbe_header_info rgbe_header;
//  rgbe_header.exposure = 1.0f;
//  strcpy_s<16>(rgbe_header.programtype, "RADIANCE");
//  rgbe_header.valid = RGBE_VALID_PROGRAMTYPE | RGBE_VALID_EXPOSURE;
//  RGBE_WriteHeader(f, header.dwWidth/2, header.dwHeight/2, &rgbe_header);
//
//  RGBE_WritePixels_RLE(f, out_data_, header.dwWidth/2, header.dwHeight/2);  
//}

int main()
{   
  print_out("\n %10s %2d", "long long", sizeof(long long));
  print_out("\n %10s %2d", "long", sizeof(long));
  print_out("\n %10s %2d", "double", sizeof(double));
  print_out("\n %10s %2d", "float", sizeof(float));
  print_out("\n %10s %2d", "int", sizeof(int));

  //open_dds("E:\\Work\\hdr_cubemap\\images\\un_Papermill_Ruins_E.dds");

  //Singletone.image.open_hdri("D:\\Stuff\\hdri_cubemap_converter\\glacier.hdr");
  //image.open_hdri("E:\\Work\\hdr_cubemap\\images\\glacier.hdr");

  //Singletone.cube.make_cube(Singletone.image.pixels, Singletone.image.width, Singletone.image.height, 256, 0);

  //cube.blur(30);

  //Singletone.render();
    
  //cube.turn_right(Surface::X_P);
  //cube.turn_right(Surface::X_P);
  //cube.turn_right(Surface::X_P);
  //cube.turn_right(Surface::X_N);
  //cube.turn_right(Surface::Y_P);
  //cube.turn_right(Surface::Y_P);
  //
  //
  //write_dds_cubemap("E:\\Work\\hdr_cubemap\\images\\output.dds", Singletone.cube.edges, 256);
  //write_dds_cubemap("D:\\Stuff\\hdri_cubemap_converter\\output.dds", cube.edges, 256);

  //open_hdri("E:\\Work\\hdr_cubemap\\images\\grace-new.hdr");
  //open_hdri("E:\\Work\\hdr_cubemap\\images\\glacier.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\HDR_110_Tunnel_Ref.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\uffizi-large.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\glacier.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\output.hdr");

  quokka::GProfiler()->Print();

  system("PAUSE");
  //int exit;
  //std::cin >> exit;
  return 0;
}