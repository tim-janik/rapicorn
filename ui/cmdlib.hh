// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CMDLIB_HH__
#define __RAPICORN_CMDLIB_HH__

#include <ui/window.hh>

namespace Rapicorn {

bool    command_lib_exec        (ItemImpl           &item,
                                 const String       &cmd_name,
                                 const StringSeq    &args);
bool    command_scan            (const String       &input,
                                 String             *cmd_name,
                                 StringSeq          *args);
String  command_string_unquote  (const String       &input);

} // Rapicorn

#endif  /* __RAPICORN_CMDLIB_HH__ */
