// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
#include <rapicorn-core.hh>
#include <rapicorn-test.hh>
#include <rapicorn.hh>
using namespace Rapicorn;

extern "C" int
main (int argc, char *argv[])
{
  ApplicationH app = init_app ("t190-api-use", &argc, argv);
  Blob blob = Blob::load (".");
  return app.run_and_exit();
}
