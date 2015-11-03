// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CMDLIB_HH__
#define __RAPICORN_CMDLIB_HH__

#include <ui/window.hh>

namespace Rapicorn {

bool    command_lib_exec        (WidgetImpl           &widget,
                                 const String       &cmd_name,
                                 const StringSeq    &args);
bool    command_scan            (const String       &input,
                                 String             *cmd_name,
                                 StringSeq          *args);
String  command_string_unquote  (const String       &input);

} // Rapicorn

#endif  /* __RAPICORN_CMDLIB_HH__ */
