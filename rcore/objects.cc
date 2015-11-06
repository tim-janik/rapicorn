// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "objects.hh"
#include "thread.hh"
#include <typeinfo>
#include <cxxabi.h> // abi::__cxa_demangle

namespace Rapicorn {

/** Demangle a std::typeinfo.name() string into a proper C++ type name.
 * This function uses abi::__cxa_demangle() from <cxxabi.h> to demangle C++ type names,
 * which works for g++, libstdc++, clang++, libc++.
 */
String
cxx_demangle (const char *mangled_identifier)
{
  int status = 0;
  char *malloced_result = abi::__cxa_demangle (mangled_identifier, NULL, NULL, &status);
  String result = malloced_result && !status ? malloced_result : mangled_identifier;
  if (malloced_result)
    free (malloced_result);
  return result;
}

} // Rapicorn
