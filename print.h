#pragma once 

#include <stdarg.h>
#include <iostream>
#include <Windows.h>

#define m_print_out(str, ...) \
  {     \
  char output[256]; \
  sprintf_s(output, str, ##__VA_ARGS__); \
  std::cout << output; \
  OutputDebugString(output); \
  }

void print_out(const char* format, ...)
{
  char output[256];
  va_list args;
  va_start(args, format);
  vsprintf_s(output, format, args);
  va_end(args);
  std::cout << output;
  OutputDebugString(output);
}