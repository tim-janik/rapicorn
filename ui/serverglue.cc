// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "serverapi.hh"

namespace { // Anon
static Plic::Connection *_serverglue_connection = NULL;
};

namespace Rapicorn {

void
serverglue_setup (Plic::Connection *connection)
{
  assert (_serverglue_connection == NULL);
  _serverglue_connection = connection;
}

} // Rapicorn

#define PLIC_CONNECTION()       (*_serverglue_connection)

#include "serverapi.cc"
