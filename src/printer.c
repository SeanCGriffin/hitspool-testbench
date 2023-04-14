#include "printer.h"

#include <stdio.h>
#include <stdarg.h>

void print(const char *fmt, ...) {
  static char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  printf((char *)buffer);
}