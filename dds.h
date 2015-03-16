#ifndef DDS_H
#define DDS_H

#include <Windows.h>
#include <stdio.h>

const DWORD DDS_MAGIC_NUMBER = 542327876;

struct DDS_PIXELFORMAT {
  DWORD dwSize = 32;
  DWORD dwFlags = 65;
  DWORD dwFourCC = 0;
  DWORD dwRGBBitCount = 32;
  DWORD dwRBitMask = 16711680;
  DWORD dwGBitMask = 65280;
  DWORD dwBBitMask = 255;
  DWORD dwABitMask = 4278190080;
};


struct DDS_HEADER{
  DWORD           dwSize = 124;                       
  DWORD           dwFlags = 135175;
  DWORD           dwHeight = 0;
  DWORD           dwWidth = 0;
  DWORD           dwPitchOrLinearSize = 0;
  DWORD           dwDepth = 0;
  DWORD           dwMipMapCount = 0;
  DWORD           dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  DWORD           dwCaps = 4198410;
  DWORD           dwCaps2 = 65024;
  DWORD           dwCaps3 = 0;
  DWORD           dwCaps4 = 0;
  DWORD           dwReserved2 = 0;
};

void read_dds_header(FILE* f, DDS_HEADER* header)
{  
  size_t bytes = fread(header, sizeof(DDS_HEADER), 1, f);
};

#endif