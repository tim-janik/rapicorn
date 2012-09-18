// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "serverapi.hh"
#include "uithread.hh"
#include "internal.hh"

namespace { // Anon
static Aida::ServerConnection _serverglue_connection;
};

namespace Rapicorn {

void
uithread_serverglue (Aida::ServerConnection connection)
{
 _serverglue_connection = connection;
}

} // Rapicorn

#define PLIC_CONNECTION()       (_serverglue_connection)

#include "serverapi.cc"
