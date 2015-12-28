// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_INTERNAL_HH__
#define __RAPICORN_INTERNAL_HH__

#include <rapicorn-core.hh>

namespace RapicornInternal {

bool  uithread_bootup      (int *argcp, char **argv, const StringVector &args);
void  uithread_shutdown    (); // uithread.cc
int64 client_app_test_hook (); // clientglue.cc
int64 server_app_test_hook ();

} // RapicornInternal

#endif  /* __RAPICORN_INTERNAL_HH__ */
