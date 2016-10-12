// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CORE_HH__
#define __RAPICORN_CORE_HH__

#include <rcore/aida.hh>
#include <rcore/aidacxx.hh>
#include <rcore/aidaprops.hh>
#include <rcore/bindable.hh>
#include <rcore/sys-config.h>
#include <rcore/resources.hh>
#include <rcore/cpuasm.hh>
#include <rcore/cxxaux.hh>
#include <rcore/formatter.hh>
#include <rcore/inout.hh>
#include <rcore/inifile.hh>
#include <rcore/platform.hh>
#include <rcore/debugtools.hh>
#include <rcore/loop.hh>
#include <rcore/main.hh>
#include <rcore/markup.hh>
#include <rcore/memory.hh>
#include <rcore/quicktimer.hh>
#include <rcore/randomhash.hh>
#include <rcore/thread.hh>
#include <rcore/visitor.hh>
#include <rcore/math.hh>
#include <rcore/unicode.hh>
#include <rcore/utilities.hh>
#include <rcore/xmlnode.hh>
#include <rcore/regex.hh>
#include <rcore/strings.hh>

/**
 * @brief The Rapicorn namespace encompasses core utilities and toolkit functionality.
 *
 * The core utilities are available via including <rapicorn-core.hh> and
 * the toolkit functionality can be included via <rapicorn.hh>.
 */
namespace Rapicorn {}

#endif // __RAPICORN_CORE_HH__
