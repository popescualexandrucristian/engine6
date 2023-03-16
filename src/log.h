#include <stdio.h>

#define LOG_INFO(msg, ...) \
do \
{ \
  printf("[%s:%d] ", __FILE__, __LINE__); \
  printf(msg, __VA_ARGS__); \
  putc('\n', stdout); \
} while(0)