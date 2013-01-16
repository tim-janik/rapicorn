// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "serverapi.hh"
#include "uithread.hh"
#include "internal.hh"

namespace { // Anon
static Rapicorn::Aida::ServerConnection *_serverglue_connection;
};

namespace Rapicorn {

void
uithread_serverglue (Rapicorn::Aida::ServerConnection &connection)
{
 _serverglue_connection = &connection;
}

} // Rapicorn

#define AIDA_CONNECTION()       (*_serverglue_connection)

// compile server-side API
#include "serverapi.cc"

// compile server-side Pixmap<Pixbuf> template
#include "pixmap.cc"
