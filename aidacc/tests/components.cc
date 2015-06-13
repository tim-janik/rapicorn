// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include <stdio.h>
#include "components-a1-server.hh"
#include "components-a1-client.hh"

using namespace Rapicorn;

int
main (int   argc,
      char *argv[])
{
  printout ("  %-8s %-60s  %s\n", "TEST", argv[0], "OK");

  return 0;
}

#include "components-a1-client.cc"
#include "components-a1-server.cc"
