
#include <algorithm>
#include <math.h>
#include <iostream>
#include <fstream>


#include "rgbe.h"
#include "dds.h"

struct pixel
{
  float r = 0, g = 0, b = 0;
  pixel() : r(0), g(0), b(0) {}
  pixel(float ar, float ag, float ab) : r(ar), g(ag), b(ab) {}

  pixel operator/(float v)
  {
    return pixel(r/v, g/v, b/v);
  }
};

pixel operator+(pixel& a, pixel& b)
{
  return pixel(a.r + b.r, a.g + b.g, a.b + b.b);
}



struct SImage
{
  int width = 0;
  int height = 0;

  pixel* pixels = nullptr;

  void open_hdri(const char* filename)
  {
    FILE* f;

    errno_t err = fopen_s(&f, filename, "rb");
    rgbe_header_info header;

    RGBE_ReadHeader(f, &width, &height, NULL);

    // read pixels
    float* data = new float[width*height * 3];
    RGBE_ReadPixels_RLE(f, data, width, height);
    fclose(f);

    pixels = new pixel[width*height];
    memcpy(pixels, data, sizeof(float)* 3 * width*height);
  }
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

void turn_right(pixel* edge, int cube_edge_i)
{
  pixel* n_edge = new pixel[cube_edge_i*cube_edge_i];
  for (int i = 0; i < cube_edge_i; i++)
  {
    for (int j = 0; j < cube_edge_i; j++)
    {
      n_edge[i + j*cube_edge_i] = edge[j + (cube_edge_i - i - 1)*cube_edge_i];
    }
  }
  memcpy(edge, n_edge, cube_edge_i*cube_edge_i*sizeof(pixel));
}

void assign_xyz(float& x, float& y, float& z, int c1, int c2, int half_edge, Surface surf)
{
  switch (surf)
  {
  case Surface::X_P: x = half_edge; y = c1; z = c2; break;
  case Surface::X_N: x = -half_edge; y = c1; z = c2; break;
  case Surface::Y_P: x = c1; y = half_edge; z = c2; break;
  case Surface::Y_N: x = c1; y = -half_edge; z = c2; break;
  case Surface::Z_P: x = c1; y = c2; z = half_edge; break;
  case Surface::Z_N: x = c1; y = c2; z = -half_edge; break;
  }
}

struct SCube
{
  void make_cube(pixel* pixels, int width, int height, int cube_edge_i, float angle_degrees_z = 0.0f)
  {
    this->cube_edge_i = cube_edge_i;

    float r = height / 2.f;

    float cube_edge = cube_edge_i;

    int half_edge = cube_edge_i / 2;

    float c_x = -cube_edge / 2;
    float c_y = -cube_edge / 2;
    float c_z = -cube_edge / 2;

    float M_PI = 3.1415;

    float angle_z = M_PI * angle_degrees_z / 180.f;

    for (int i = 0; i < 6; i++)
    {
      edges[i] = new pixel[cube_edge_i*cube_edge_i];

      for (int c1 = -half_edge; c1 < half_edge; c1 += 1)
      {
        for (int c2 = -half_edge; c2 < half_edge; c2 += 1)
        {
          float x = 0.f;
          float y = 0.f;
          float z = 0.f;

          assign_xyz(x, y, z, c1, c2, half_edge, (Surface)i);

          float xyz_sqrt = sqrtf(x*x + y*y + z*z);
          float s_x = r*x / xyz_sqrt;
          float s_y = r*y / xyz_sqrt;
          float s_z = r*z / xyz_sqrt;

          float inclination = atan2f(sqrtf(s_x*s_x + s_y*s_y), s_z);
          float azimuth = atan2f(s_y, s_x) + angle_z;
          if (inclination < 0) inclination += 2 * M_PI;
          if (azimuth     < 0) azimuth += 2 * M_PI;
          if (inclination >= 2*M_PI) inclination -= 2 * M_PI;
          if (azimuth     >= 2*M_PI) azimuth -= 2 * M_PI;

          int r_x = round(azimuth / (2 * M_PI)*width);
          int r_y = round(inclination / M_PI*height);

          if (r_x >= width) r_x -= width;
          if (r_y >= height) r_y -= height;
          if (r_x < 0) r_x += width;
          if (r_y < 0) r_y += height;
          
          pixel p = pixels[r_x + r_y*width];
          int index = (c1 + half_edge) + cube_edge_i*(c2 + half_edge);
          if (index < cube_edge_i*cube_edge_i && index >= 0) edges[i][index] = p;          
        }
      }
    }

    flip_x(Surface::X_P);
    flip_x(Surface::Y_N);
    flip_x(Surface::Z_P);

    flip_y(Surface::X_P);
    flip_y(Surface::X_N);
    flip_y(Surface::Y_P);
    flip_y(Surface::Y_N);
    flip_y(Surface::Z_P);
    flip_y(Surface::Z_N);
  }

  void turn_right(Surface s)
  {
    ::turn_right(edges[(int)s], cube_edge_i);
  }

  void flip_x(Surface s)
  {
    pixel* edge = edges[int(s)];
    for (int i = 0; i < cube_edge_i/2; i++)
    {
      for (int j = 0; j < cube_edge_i; j++)
      {
        std::swap(edge[i + j*cube_edge_i],edge[cube_edge_i - i - 1 + j*cube_edge_i]);
      }
    }    
  }

  void flip_y(Surface s)
  {
    pixel* edge = edges[int(s)];
    for (int i = 0; i < cube_edge_i; i++)
    {
      for (int j = 0; j < cube_edge_i/2; j++)
      {
        std::swap(edge[i + j*cube_edge_i], edge[i + (cube_edge_i - j - 1)*cube_edge_i]);
      }
    }
  }

  void assign_borders(pixel* top, pixel* bottom, pixel* left, pixel* right, Surface k)
  {
    switch (k)
    {
    case  Surface::X_P: 
      for (int i = 0; i < cube_edge_i; i++)
      {
        left[i] = edges[int(Surface::Y_P)][cube_edge_i*(i + 1) - 1];
        right[i] = edges[int(Surface::Y_N)][cube_edge_i*i];
        top[i] = edges[int(Surface::Z_P)][cube_edge_i*i];
        bottom[i] = edges[int(Surface::Z_N)][cube_edge_i*(i + 1) - 1];        
      } 
      break;
    case  Surface::X_N:
      for (int i = 0; i < cube_edge_i; i++)
      {
        left[i] = edges[int(Surface::Y_N)][cube_edge_i*(i + 1) - 1];
        right[i] = edges[int(Surface::Y_P)][cube_edge_i*i];
        top[cube_edge_i - i - 1] = edges[int(Surface::Z_P)][cube_edge_i*(i + 1) - 1];
        bottom[cube_edge_i - i - 1] = edges[int(Surface::Z_N)][cube_edge_i*i];        
      }      
      break;
    case  Surface::Y_P: 
      for (int i = 0; i < cube_edge_i; i++)
      {
        left[i] = edges[int(Surface::X_N)][cube_edge_i*(i + 1) - 1];
        right[i] = edges[int(Surface::X_P)][cube_edge_i*i];
        top[cube_edge_i - i - 1] = edges[int(Surface::Z_P)][i];
        bottom[i] = edges[int(Surface::Z_N)][i];        
      }
      break;
    case  Surface::Y_N: 
      for (int i = 0; i < cube_edge_i; i++)
      {
        left[i] = edges[int(Surface::X_P)][cube_edge_i*(i + 1) - 1];
        right[i] = edges[int(Surface::X_N)][cube_edge_i*i];
        top[i] = edges[int(Surface::Z_P)][cube_edge_i*(cube_edge_i - 1) + i];
        bottom[cube_edge_i - i - 1] = edges[int(Surface::Z_N)][cube_edge_i*(cube_edge_i - 1) + i];        
      }
      break;
    case  Surface::Z_P:
      for (int i = 0; i < cube_edge_i; i++)
      {
        left[i] = edges[int(Surface::X_P)][i];
        right[cube_edge_i - i - 1] = edges[int(Surface::X_N)][i];
        top[cube_edge_i - i - 1] = edges[int(Surface::Y_P)][i];
        bottom[i] = edges[int(Surface::Y_N)][i];        
      }
      break;
    case  Surface::Z_N: 
      for (int i = 0; i < cube_edge_i; i++)
      {
        left[cube_edge_i - i - 1] = edges[int(Surface::X_N)][cube_edge_i*(cube_edge_i - 1) + i];
        right[i] = edges[int(Surface::X_P)][cube_edge_i*(cube_edge_i - 1) + i];
        top[i] = edges[int(Surface::Y_P)][cube_edge_i*(cube_edge_i - 1) + i];
        bottom[cube_edge_i - i - 1] = edges[int(Surface::Y_N)][cube_edge_i*(cube_edge_i - 1) + i];        
      }
      break;
    }    
  }

  void blur(int power)
  {
    pixel* top = new pixel[cube_edge_i];
    pixel* bottom = new pixel[cube_edge_i];
    pixel* left = new pixel[cube_edge_i];
    pixel* right = new pixel[cube_edge_i];

    pixel* new_edges[6];
    for (int i = 0; i < 6; i++) new_edges[i] = new pixel[cube_edge_i*cube_edge_i];
    
    for (int p = 0; p < power; p++)
    {      
      for (int k = 0; k < 6; k++)
      {
        assign_borders(top, bottom, left, right, (Surface)k);

        pixel* ext_edge = new pixel[(cube_edge_i + 2)*(cube_edge_i + 2)];

        //TODO: �������� �������
        //for (int i = 1; i < cube_edge_i + 1; i++)
        //  for (int j = 1; j < cube_edge_i + 1; j++)
        //    ext_edge[i + j*(cube_edge_i + 2)] = edges[k][i - 1 + (j - 1)*cube_edge_i];
  
        for (int j = 1; j < cube_edge_i + 1; j++)
          memcpy(&ext_edge[j*(cube_edge_i + 2) + 1], &edges[k][(j - 1)*cube_edge_i], sizeof(pixel)*cube_edge_i);

        for (int i = 1; i < cube_edge_i + 1; i++)
        {
          ext_edge[i] = top[i - 1];
          ext_edge[i*(cube_edge_i + 2)] = left[i - 1];
          ext_edge[(i + 1)*(cube_edge_i + 2) - 1] = right[i - 1];
          ext_edge[(cube_edge_i + 2)*(cube_edge_i + 1) + i] = bottom[i - 1];
        }

        ext_edge[0] = (ext_edge[1] + ext_edge[cube_edge_i + 2]) / 2;
        ext_edge[cube_edge_i + 1] = (ext_edge[cube_edge_i - 1] + ext_edge[2 * (cube_edge_i + 2) - 1]) / 2;
        ext_edge[(cube_edge_i + 2)*(cube_edge_i + 1)] = (ext_edge[(cube_edge_i + 2)*(cube_edge_i + 1) + 1] + ext_edge[cube_edge_i * (cube_edge_i + 2)]) / 2;
        ext_edge[(cube_edge_i + 2)*(cube_edge_i + 2) - 1] = (ext_edge[(cube_edge_i + 2)*(cube_edge_i + 2) - 2] + ext_edge[(cube_edge_i + 2)*(cube_edge_i + 1)]) / 2;

        for (int i = 1; i < cube_edge_i + 1; i++)
          for (int j = 1; j < cube_edge_i + 1; j++)
          {
            pixel sum = pixel();
            for (int ii = -1; ii < 2; ii++)
              for (int jj = -1; jj < 2; jj++)
                sum = sum + ext_edge[(i + ii) + (j + jj)*(cube_edge_i + 2)];
            new_edges[k][i - 1 + (j - 1)*cube_edge_i] = sum / 9;
          }
      }
      for (int i = 0; i < 6; i++)
        memcpy(edges[i], new_edges[i], sizeof(pixel)*cube_edge_i*cube_edge_i);
    }
  }

  pixel* edges[6];
  int cube_edge_i;
};

void write_hdri_cross(const char* filename, const pixel** edges, int cube_edge)
{
  int width = cube_edge * 4 + 1;
  int height = cube_edge * 3;

  pixel* out_pixels = new pixel[width*height];

  for (int i = 0; i < 6; i++)
  {
    int dx, dy;
    switch (i)
    {
    case 0: dx = 0; dy = cube_edge; break;
    case 1: dx = cube_edge; dy = cube_edge; break;
    case 2: dx = cube_edge * 2; dy = cube_edge; break;
    case 3: dx = cube_edge * 3; dy = cube_edge; break;
    case 4: dx = cube_edge; dy = cube_edge * 2; break;
    case 5: dx = cube_edge; dy = 0; break;
    }
    for (int x = 0; x < cube_edge; x++)
    {
      for (int y = 0; y < cube_edge; y++)
      {
        int index1 = x + dx + width*(height - y - dy - 1);
        int index2 = x + cube_edge*y;
        if (index1 >= 0) out_pixels[index1] = edges[i][index2];
      }
    }
  }

  {
    FILE* f;

    //std::string filename = "D:\\Stuff\\hdri_cubemap_converter\\output.hdr";
    //const char* filename = "E:\\Work\\hdr_cubemap\\images\\output.hdr";
    errno_t err = fopen_s(&f, filename, "wb");

    rgbe_header_info header;
    header.exposure = 1.0f;
    strcpy_s<16>(header.programtype, "RADIANCE");
    header.valid = RGBE_VALID_PROGRAMTYPE | RGBE_VALID_EXPOSURE;
    RGBE_WriteHeader(f, width, height, &header);

    float* data = new float[width*height * 3];
    memcpy(data, out_pixels, sizeof(float)* 3 * width*height);
    RGBE_WritePixels_RLE(f, data, width, height);

    fclose(f);
  }
}

void write_dds_cubemap(const char* filename, pixel** edges, int cube_edge_i)
{
  FILE* f;

  //errno_t err = fopen_s(&f, "E:\\Work\\hdr_cubemap\\images\\un_Papermill_Ruins_E.dds", "rb");
  errno_t err = fopen_s(&f, "D:\\Stuff\\hdri_cubemap_converter\\HDR_110_Tunnel_Bg.dds", "rb");

  DWORD magic_number;
  DDS_HEADER header;
  size_t bytes = fread(&magic_number, sizeof(DWORD), 1, f);
  read_dds_header(f, &header);
  header.dwMipMapCount = 0;
  header.dwWidth = cube_edge_i;
  header.dwHeight = cube_edge_i;

  fclose(f);

  std::ofstream headerfile;
  headerfile.open("D:\\Stuff\\hdri_cubemap_converter\\dds_header.txt");
  headerfile << magic_number << std::endl;
  for (int i = 0; i < sizeof(header) / sizeof(DWORD); i++)
  {
    DWORD line; memcpy(&line, (DWORD*)&header + i, sizeof(DWORD));
    headerfile << line << std::endl;
  }

  headerfile.close();



  //std::string filename = "D:\\Stuff\\hdri_cubemap_converter\\output.hdr";
  //const char* filename = "E:\\Work\\hdr_cubemap\\images\\output.dds";
  err = fopen_s(&f, filename, "wb");

  fwrite(&magic_number, sizeof(DWORD), 1, f);
  fwrite(&header, sizeof(DDS_HEADER), 1, f);

  unsigned char* out_data = new unsigned char[cube_edge_i*cube_edge_i * 4];
  for (int i = 0; i < 6; i++)
  {
    for (int j = 0; j < cube_edge_i*cube_edge_i; j++)
    {
      out_data[j * 4 + 0] = edges[i][j].b >= 1.f ? 255 : edges[i][j].b * 255;
      out_data[j * 4 + 1] = edges[i][j].g >= 1.f ? 255 : edges[i][j].g * 255;
      out_data[j * 4 + 2] = edges[i][j].r >= 1.f ? 255 : edges[i][j].r * 255;
      out_data[j * 4 + 3] = 255;
    }
    fwrite(out_data, sizeof(unsigned char), cube_edge_i*cube_edge_i * 4, f);
  }

  fclose(f);  
}

struct Singletone
{
  SImage image;
  SCube cube;
} Singletone;

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

  write_dds_cubemap(filename, Singletone.cube.edges, cube_edge_i);
}

extern "C" __declspec(dllexport) int get_width()  { return Singletone.image.width; }
extern "C" __declspec(dllexport) int get_height() { return Singletone.image.height; }

extern "C" __declspec(dllexport) pixel* get_pixels() { return Singletone.image.pixels; }
extern "C" __declspec(dllexport) pixel* get_edge(int i) { return Singletone.cube.edges[i]; }
extern "C" __declspec(dllexport) pixel* get_edge_t(int i, int turns)
{
  int cube_edge_i = Singletone.cube.cube_edge_i;

  pixel* edge = new pixel[cube_edge_i*cube_edge_i];
  memcpy(edge, Singletone.cube.edges[i], sizeof(pixel)*cube_edge_i*cube_edge_i);
  for (int i = 0; i < turns; i++)
    turn_right(edge, cube_edge_i);

  return edge;
}

extern "C" __declspec(dllexport) int get_float_size() { return sizeof(float); }
extern "C" __declspec(dllexport) void blur(int power) { Singletone.cube.blur(power); }


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
  SImage image;
  //open_dds("E:\\Work\\hdr_cubemap\\images\\un_Papermill_Ruins_E.dds");

  image.open_hdri("D:\\Stuff\\hdri_cubemap_converter\\glacier.hdr");

  SCube cube;
  cube.make_cube(image.pixels, image.width, image.height, 256, 0);

  //cube.blur(30);
    
  //cube.turn_right(Surface::X_P);
  //cube.turn_right(Surface::X_P);
  //cube.turn_right(Surface::X_P);
  //cube.turn_right(Surface::X_N);
  //cube.turn_right(Surface::Y_P);
  //cube.turn_right(Surface::Y_P);
  //
  //
  //write_dds_cubemap("E:\\Work\\hdr_cubemap\\images\\output.dds", cube.edges, 1024);
  write_dds_cubemap("D:\\Stuff\\hdri_cubemap_converter\\output.dds", cube.edges, 256);

  //open_hdri("E:\\Work\\hdr_cubemap\\images\\grace-new.hdr");
  //open_hdri("E:\\Work\\hdr_cubemap\\images\\glacier.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\HDR_110_Tunnel_Ref.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\uffizi-large.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\glacier.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\output.hdr");

  system("PAUSE");
  //int exit;
  //std::cin >> exit;
  return 0;
}