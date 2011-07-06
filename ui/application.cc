// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "application.hh"
#include "window.hh"
#include "factory.hh"
#include "viewp0rt.hh"
#include "image.hh"
#include "compath.hh"
#include <algorithm>
#include <stdlib.h>

namespace Rapicorn {

ApplicationIface::ApplicationMutex ApplicationIface::mutex;

void
ApplicationIface::pixstream (const String  &pix_name,
                             const uint8   *static_const_pixstream)
{
  Pixmap::add_stock (pix_name, static_const_pixstream);
}

static struct __StaticCTorTest { int v; __StaticCTorTest() : v (0x123caca0) {} } __staticctortest;
static ApplicationImpl *the_app = NULL;

ApplicationImpl&
ApplicationImpl::the ()
{
  if (!the_app)
    fatal ("librapicornui: library uninitialized, call Rapicorn::init_app() first");
  return *the_app;
}

Application_SmartHandle
init_app (const String       &app_ident,
          int                *argcp,
          char              **argv,
          const StringVector &args)
{
  // assert global_ctors work
  if (__staticctortest.v != 0x123caca0)
    fatal ("librapicornui: link error: C++ constructors have not been executed");
  // ensure single initialization
  if (the_app)
    fatal ("librapicornui: multiple calls to Rapicorn::init_app()");
  // initialize core
  if (program_ident().empty())
    init_core (app_ident, argcp, argv, args);
  else if (app_ident != program_ident())
    fatal ("librapicornui: application identifier changed during ui initialization");
  // initialize sub systems
  struct InitHookCaller : public InitHook {
    static void  invoke (const String &kind, int *argcp, char **argv, const StringVector &args)
    { invoke_hooks (kind, argcp, argv, args); }
  };
  InitHookCaller::invoke ("ui/", argcp, argv, args);
  the_app = new ApplicationImpl();
  InitHookCaller::invoke ("ui-thread/", argcp, argv, args);
  assert (rapicorn_thread_entered() == false);
  rapicorn_thread_enter();
  return ApplicationImpl::the();
}

Wind0wIface*
ApplicationImpl::create_wind0w (const std::string    &wind0w_identifier,
                                const StringList     &arguments,
                                const StringList     &env_variables)
{
  return &Factory::create_wind0w (wind0w_identifier,
                                  arguments,
                                  env_variables);
}

String
ApplicationImpl::auto_path (const String  &file_name,
                            const String  &binary_path,
                            bool           search_vpath)
{
  assert (rapicorn_thread_entered());
  /* test absolute file_name */
  if (Path::isabs (file_name))
    return file_name;
  /* assign bpath the absolute binary path */
  String bpath = binary_path;
  if (!Path::isabs (bpath))
    bpath = Path::join (Path::cwd(), bpath);
  /* extract binary dirname */
  if (!Path::check (bpath, "d") && !Path::isdirname (bpath))
    {
      bpath = Path::dirname (bpath);
      /* strip .libs or _libs directories */
      String tbase = Path::basename (bpath);
      if (tbase == ".libs" or tbase == "_libs")
        bpath = Path::dirname (bpath);
    }
  /* construct search path list */
  const char *gvp = getenv ("RAPICORN_VPATH");
  StringVector spl;
  if (search_vpath)
    spl = Path::searchpath_split (gvp ? gvp : "");
  spl.insert (spl.begin(), bpath);
  /* pick first existing file_name */
  for (uint i = 0; i < spl.size(); i++)
    {
      String fullname = Path::join (spl[i], file_name);
      if (Path::check (fullname, "e"))
        return fullname;
    }
  // fallback to cwd/binary_path relative file name (non-existing)
  return Path::join (bpath, file_name);
}

void
ApplicationImpl::auto_load (const String  &defs_domain,
                            const String  &file_name,
                            const String  &binary_path,
                            const String  &i18n_domain)
{
  String fullname = auto_path (file_name, binary_path, true);
  int err = Factory::parse_file (i18n_domain, fullname, defs_domain);
  if (err)
    fatal ("failed to load \"%s\": %s", fullname.c_str(), string_from_errno (err).c_str());
}

void
ApplicationImpl::load_string (const std::string &xml_string,
                              const std::string &i18n_domain)
{
  int err = Factory::parse_string (xml_string, i18n_domain);
  if (err)
    fatal ("failed to parse string: %s\n%s", string_from_errno (err).c_str(), xml_string.c_str());
}

int
ApplicationIface::execute_loops ()
{
  assert (rapicorn_thread_entered());           // guards exit_code
  while (!EventLoop::loops_exitable())
    EventLoop::iterate_loops (true, true);      // prepare/check/dispatch and may_block
  return 0;
}

bool
ApplicationIface::has_primary ()
{
  return !EventLoop::loops_exitable();
}

void
ApplicationIface::close ()
{
}

Wind0wList
ApplicationImpl::list_wind0ws ()
{
  Wind0wList wl;
  for (uint i = 0; i < m_wind0ws.size(); i++)
    wl.push_back (m_wind0ws[i]);
  return wl;
}

ItemIface*
ApplicationImpl::unique_component (const String &path)
{
  ItemSeq items = collect_components (path);
  if (items.size() == 1)
    return &*items[0];
  return NULL;
}

ItemSeq
ApplicationImpl::collect_components (const String &path)
{
  ComponentMatcher *cmatcher = ComponentMatcher::parse_path (path);
  ItemSeq result;
  if (cmatcher) // valid path
    {
      for (uint i = 0; i < m_wind0ws.size(); i++)
        {
          ItemImpl *witem = dynamic_cast<ItemImpl*> (m_wind0ws[i]);
          if (witem)
            {
              vector<ItemImpl*> more = collect_items (*witem, *cmatcher);
              result.insert (result.end(), more.begin(), more.end());
            }
        }
      delete cmatcher;
    }
  return result;
}

void
ApplicationImpl::add_wind0w (Wind0wIface &wind0w)
{
  ref_sink (wind0w);
  m_wind0ws.push_back (&wind0w);
}

void
ApplicationImpl::check_primaries()
{
  if (m_wind0ws.size() == 0) // FIXME: check temporary wind0w types
    sig_missing_primary.emit();
}

bool
ApplicationImpl::remove_wind0w (Wind0wIface &wind0w)
{
  vector<Wind0wIface*>::iterator it = std::find (m_wind0ws.begin(), m_wind0ws.end(), &wind0w);
  if (it == m_wind0ws.end())
    return false;
  m_wind0ws.erase (it);
  unref (wind0w);
  check_primaries(); // FIXME: should exec_update
  return true;
}

void
ApplicationImpl::test_counter_set (int val)
{
  m_tc = val;
}

void
ApplicationImpl::test_counter_add (int val)
{
  m_tc += val;
}

int
ApplicationImpl::test_counter_get ()
{
  return m_tc;
}

int
ApplicationImpl::test_counter_inc_fetch ()
{
  return ++m_tc;
}

} // Rapicorn
