// 602gcli9.cpp
#include "glibc23replac.c"

extern "C" void start_wx(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  //fputs("starting...", stderr);
  start_wx(argc, argv);
  return 0;
}

     
