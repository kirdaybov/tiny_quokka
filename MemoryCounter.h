#pragma once

#include "print.h"

const int MEMORY_COUNTER_SIZE = 512;

struct s_memory
{
  ~s_memory() { print_out("Memory left: %d\n", in_use); }

  int counter = 0;
  int in_use = 0;
  struct
  {
    void* ptr;
    int size;
  } arr[MEMORY_COUNTER_SIZE];
} memory;

inline void* operator new(std::size_t sz)
{
  void* ptr = malloc(sz);
  //print_out("\nNew called: %10d byte allocated at line %d, ", sz, __LINE__);
  memory.in_use += sz;
  if (memory.counter < MEMORY_COUNTER_SIZE)
  {
    memory.arr[memory.counter] = { ptr, sz };
    memory.counter++;
  }
  return ptr;
}

inline void operator delete(void* ptr)
{
  int i = 0;
  for (i = 0; i < MEMORY_COUNTER_SIZE; i++)
  if (memory.arr[i].ptr == ptr) break;
  //if (i < MEMORY_COUNTER_SIZE-1)
  //  print_out("\nDel called: %10d byte released at line %d", memory.arr[i].size, __LINE__);
  memory.in_use -= memory.arr[i].size;
  free(ptr);
}