#include <stdio.h>
#include <stdlib.h>

namespace Rapicorn1002 {

void
rapicorn_init_with_gtk_thread (int        *argcp,
                               char     ***argvp,
                               const char *app_name)
{
  printf ("PRELOAD and EXIT!\n");
  exit (255);
}

};

// g++ -shared -fPIC -Wall -O2 preloadexit.cc -o libpreloadexit.so
// LD_PRELOAD=./libpreloadexit.so .../rapicorn-exec
