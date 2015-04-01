#include "hdri_cubemap.h"
#include "big_quokka.h"

void SImage::open_hdri(const char* filename)
{
  if (pixels) delete[] pixels;

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

  delete[] data;
}

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
  delete[] n_edge;
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
  DDS_HEADER header;
  header.dwMipMapCount = 0;
  header.dwWidth = cube_edge_i;
  header.dwHeight = cube_edge_i;

  FILE* f;
  errno_t err = fopen_s(&f, filename, "wb");

  fwrite(&DDS_MAGIC_NUMBER, sizeof(DWORD), 1, f);
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

  delete[] out_data;

  fclose(f);
}

SCube::SCube() {
  for (int i = 0; i < 6; i++)
  {
    edges[i] = nullptr;
    blurred_edges[i] = nullptr;
  }
}

void SCube::clear_edges()
{
  for (int i = 0; i < 6; i++)
  {
    if (edges[i]) delete[] edges[i];
    if (blurred_edges[i]) delete[] blurred_edges[i];
  }
}

pixel** SCube::copy_cube()
{
  pixel** edges = new pixel*[6];
  for (int i = 0; i < 6; i++)
  {
    edges[i] = new pixel[cube_edge_i*cube_edge_i];
    memcpy(edges[i], blurred_edges[i], sizeof(pixel)*cube_edge_i*cube_edge_i);
  }
  return edges;
}

void SCube::delete_edges(pixel** edges, int cube_edge_i)
{
  for (int i = 0; i < 6; i++)
  {
    delete[] edges[i];
    edges[i] = nullptr;
  }

  delete[] edges;
}

pixel* SCube::get_unreal_cubemap()
{
  pixel** l_edges = copy_cube();

  pixel* copy_edge = new pixel[cube_edge_i*cube_edge_i];
  memcpy(copy_edge, l_edges[2], sizeof(pixel)*cube_edge_i*cube_edge_i);
  memcpy(l_edges[2], l_edges[3], sizeof(pixel)*cube_edge_i*cube_edge_i);
  memcpy(l_edges[3], copy_edge, sizeof(pixel)*cube_edge_i*cube_edge_i);

  ::turn_right(l_edges[0], cube_edge_i);
  ::turn_right(l_edges[0], cube_edge_i);
  ::turn_right(l_edges[0], cube_edge_i);
  ::turn_right(l_edges[1], cube_edge_i);
  ::turn_right(l_edges[3], cube_edge_i);
  ::turn_right(l_edges[3], cube_edge_i);

  pixel* diffuse = new pixel[cube_edge_i * 6 * cube_edge_i];

  for (int j = 0; j < cube_edge_i; j++)
  {
    for (int i = 0; i < 6; i++)
    {
      memcpy(diffuse + (i + j * 6) * cube_edge_i, (pixel*)(l_edges[i]) + j*cube_edge_i, sizeof(pixel)*cube_edge_i);
    }
  }

  return diffuse;
}

void SCube::make_cube(pixel* pixels, int width, int height, int cube_edge_i, float angle_degrees_z)
{
  clear_edges();

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
    blurred_edges[i] = new pixel[cube_edge_i*cube_edge_i];

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
        if (inclination >= 2 * M_PI) inclination -= 2 * M_PI;
        if (azimuth >= 2 * M_PI) azimuth -= 2 * M_PI;

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

  for (int i = 0; i < 6; i++)
    memcpy(blurred_edges[i], edges[i], sizeof(pixel)*cube_edge_i*cube_edge_i);
}

void SCube::turn_right(Surface s)
{
  ::turn_right(edges[(int)s], cube_edge_i);
  ::turn_right(blurred_edges[(int)s], cube_edge_i);
}

void SCube::flip_x(Surface s)
{
  pixel* edge = edges[int(s)];
  for (int i = 0; i < cube_edge_i / 2; i++)
  {
    for (int j = 0; j < cube_edge_i; j++)
    {
      std::swap(edge[i + j*cube_edge_i], edge[cube_edge_i - i - 1 + j*cube_edge_i]);
    }
  }
}

void SCube::flip_y(Surface s)
{
  pixel* edge = edges[int(s)];
  for (int i = 0; i < cube_edge_i; i++)
  {
    for (int j = 0; j < cube_edge_i / 2; j++)
    {
      std::swap(edge[i + j*cube_edge_i], edge[i + (cube_edge_i - j - 1)*cube_edge_i]);
    }
  }
}

void SCube::assign_borders(pixel* top, pixel* bottom, pixel* left, pixel* right, Surface k)
{
  switch (k)
  {
  case  Surface::X_P:
    for (int i = 0; i < cube_edge_i; i++)
    {
      left[i] = blurred_edges[int(Surface::Y_P)][cube_edge_i*(i + 1) - 1];
      right[i] = blurred_edges[int(Surface::Y_N)][cube_edge_i*i];
      top[i] = blurred_edges[int(Surface::Z_P)][cube_edge_i*i];
      bottom[i] = blurred_edges[int(Surface::Z_N)][cube_edge_i*(i + 1) - 1];
    }
    break;
  case  Surface::X_N:
    for (int i = 0; i < cube_edge_i; i++)
    {
      left[i] = blurred_edges[int(Surface::Y_N)][cube_edge_i*(i + 1) - 1];
      right[i] = blurred_edges[int(Surface::Y_P)][cube_edge_i*i];
      top[cube_edge_i - i - 1] = blurred_edges[int(Surface::Z_P)][cube_edge_i*(i + 1) - 1];
      bottom[cube_edge_i - i - 1] = blurred_edges[int(Surface::Z_N)][cube_edge_i*i];
    }
    break;
  case  Surface::Y_P:
    for (int i = 0; i < cube_edge_i; i++)
    {
      left[i] = blurred_edges[int(Surface::X_N)][cube_edge_i*(i + 1) - 1];
      right[i] = blurred_edges[int(Surface::X_P)][cube_edge_i*i];
      top[cube_edge_i - i - 1] = blurred_edges[int(Surface::Z_P)][i];
      bottom[i] = blurred_edges[int(Surface::Z_N)][i];
    }
    break;
  case  Surface::Y_N:
    for (int i = 0; i < cube_edge_i; i++)
    {
      left[i] = blurred_edges[int(Surface::X_P)][cube_edge_i*(i + 1) - 1];
      right[i] = blurred_edges[int(Surface::X_N)][cube_edge_i*i];
      top[i] = blurred_edges[int(Surface::Z_P)][cube_edge_i*(cube_edge_i - 1) + i];
      bottom[cube_edge_i - i - 1] = blurred_edges[int(Surface::Z_N)][cube_edge_i*(cube_edge_i - 1) + i];
    }
    break;
  case  Surface::Z_P:
    for (int i = 0; i < cube_edge_i; i++)
    {
      left[i] = blurred_edges[int(Surface::X_P)][i];
      right[cube_edge_i - i - 1] = blurred_edges[int(Surface::X_N)][i];
      top[cube_edge_i - i - 1] = blurred_edges[int(Surface::Y_P)][i];
      bottom[i] = blurred_edges[int(Surface::Y_N)][i];
    }
    break;
  case  Surface::Z_N:
    for (int i = 0; i < cube_edge_i; i++)
    {
      left[cube_edge_i - i - 1] = blurred_edges[int(Surface::X_N)][cube_edge_i*(cube_edge_i - 1) + i];
      right[i] = blurred_edges[int(Surface::X_P)][cube_edge_i*(cube_edge_i - 1) + i];
      top[i] = blurred_edges[int(Surface::Y_P)][cube_edge_i*(cube_edge_i - 1) + i];
      bottom[cube_edge_i - i - 1] = blurred_edges[int(Surface::Y_N)][cube_edge_i*(cube_edge_i - 1) + i];
    }
    break;
  }
}

void SCube::blur(int power)
{
  int cube_edge_i_2 = cube_edge_i + 2;
  int cube_edge_i_1 = cube_edge_i + 1;

  pixel* top = new pixel[cube_edge_i];
  pixel* bottom = new pixel[cube_edge_i];
  pixel* left = new pixel[cube_edge_i];
  pixel* right = new pixel[cube_edge_i];

  for (int i = 0; i < 6; i++)
    memcpy(blurred_edges[i], edges[i], sizeof(pixel)*cube_edge_i*cube_edge_i);

  pixel* new_edges[6];
  for (int i = 0; i < 6; i++) new_edges[i] = new pixel[cube_edge_i*cube_edge_i];

  for (int p = 0; p < power; p++)
  {
    for (int k = 0; k < 6; k++)
    {
      quokka::GProfiler()->Start("assign_borders");
      assign_borders(top, bottom, left, right, (Surface)k);
      quokka::GProfiler()->Stop("assign_borders");

      pixel* ext_edge = new pixel[(cube_edge_i + 2)*(cube_edge_i + 2)];

      quokka::GProfiler()->Start("init_ext_edge");
      for (int j = 1; j < cube_edge_i_1; j++)
        memcpy(&ext_edge[j*cube_edge_i_2 + 1], &blurred_edges[k][(j - 1)*cube_edge_i], sizeof(pixel)*cube_edge_i);

      for (int i = 1; i < cube_edge_i + 1; i++)
      {
        ext_edge[i] = top[i - 1];
        ext_edge[i*cube_edge_i_2] = left[i - 1];
        ext_edge[(i + 1)*cube_edge_i_2 - 1] = right[i - 1];
        ext_edge[cube_edge_i_2*cube_edge_i_1 + i] = bottom[i - 1];
      }

      ext_edge[0] = (ext_edge[1] + ext_edge[cube_edge_i_2]) / 2;
      ext_edge[cube_edge_i_1] = (ext_edge[cube_edge_i - 1] + ext_edge[2 * (cube_edge_i_2)-1]) / 2;
      ext_edge[cube_edge_i_2*cube_edge_i_1] = (ext_edge[cube_edge_i_2*cube_edge_i_1 + 1] + ext_edge[cube_edge_i * cube_edge_i_2]) / 2;
      ext_edge[cube_edge_i_2*cube_edge_i_2 - 1] = (ext_edge[cube_edge_i_2*cube_edge_i_2 - 2] + ext_edge[cube_edge_i_2*cube_edge_i_1]) / 2;
      quokka::GProfiler()->Stop("init_ext_edge");

      quokka::GProfiler()->Start("sum");
      for (int i = 1; i < cube_edge_i_1; i++)
      for (int j = 1; j < cube_edge_i_1; j++)
      {
        //pixel sum = pixel();
        //for (int jj = -1; jj < 2; jj++)
        //for (int ii = -1; ii < 2; ii++)              
        //    sum = sum + ext_edge[(i + ii) + (j + jj)*(cube_edge_i + 2)];
        //new_edges[k][i - 1 + (j - 1)*cube_edge_i] = sum / 9;

        new_edges[k][i - 1 + (j - 1)*cube_edge_i] =
          (
          ext_edge[(i - 1) + (j - 1)*cube_edge_i_2] +
          ext_edge[(i + 1) + (j - 1)*cube_edge_i_2] +
          ext_edge[(i - 1) + (j + 1)*cube_edge_i_2] +
          ext_edge[(i + 1) + (j + 1)*cube_edge_i_2] +
          ext_edge[(i - 1) + j*cube_edge_i_2] +
          ext_edge[(i + 1) + j*cube_edge_i_2] +
          ext_edge[i + (j - 1)*cube_edge_i_2] +
          ext_edge[i + (j + 1)*cube_edge_i_2] +
          ext_edge[i + j*cube_edge_i_2]
          ) / 9;
      }
      quokka::GProfiler()->Stop("sum");
    }
    quokka::GProfiler()->Start("back_to_edge");
    for (int i = 0; i < 6; i++)
      memcpy(blurred_edges[i], new_edges[i], sizeof(pixel)*cube_edge_i*cube_edge_i);
    quokka::GProfiler()->Stop("back_to_edge");
  }

  for (int i = 0; i < 6; i++) delete[] new_edges[i];

}