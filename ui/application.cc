// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "application.hh"
#include "window.hh"
#include "factory.hh"
#include "screenwindow.hh"
#include "image.hh"
#include "compath.hh"
#include "uithread.hh"
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

bool
ApplicationIface::factory_window (const std::string &factory_definition)
{
  return Factory::item_definition_is_window (factory_definition);
}

static ApplicationImpl *the_app = NULL;
static void
create_application (const StringVector &args)
{
  if (the_app)
    fatal ("librapicornui: multiple calls to Rapicorn::init_app()");
  the_app = new ApplicationImpl();
}
static InitHook _create_application ("ui-thread/00 Creating Application Singleton", create_application);

ApplicationImpl&
ApplicationImpl::the ()
{
  if (!the_app)
    fatal ("librapicornui: library uninitialized, call Rapicorn::init_app() first");
  return *the_app;
}

WindowIface*
ApplicationImpl::create_window (const std::string    &window_identifier,
                                const StringList     &arguments,
                                const StringList     &env_variables)
{
  return &Factory::create_window (window_identifier,
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

StringList
ApplicationImpl::auto_load (const String  &defs_domain,
                            const String  &file_name,
                            const String  &binary_path,
                            const String  &i18n_domain)
{
  String fullname = auto_path (file_name, binary_path, true);
  vector<String> definitions;
  int err = Factory::parse_file (i18n_domain, fullname, defs_domain, &definitions);
  if (err)
    fatal ("failed to load \"%s\": %s", fullname.c_str(), string_from_errno (err).c_str());
  return definitions;
}

void
ApplicationImpl::load_string (const std::string &xml_string,
                              const std::string &i18n_domain)
{
  int err = Factory::parse_string (xml_string, i18n_domain);
  if (err)
    fatal ("failed to parse string: %s\n%s", string_from_errno (err).c_str(), xml_string.c_str());
}

bool
ApplicationIface::finishable ()
{
  return uithread_main_loop()->finishable();
}

void
ApplicationIface::close ()
{
}

WindowList
ApplicationImpl::list_windows ()
{
  WindowList wl;
  for (uint i = 0; i < m_windows.size(); i++)
    wl.push_back (m_windows[i]);
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
      for (uint i = 0; i < m_windows.size(); i++)
        {
          ItemImpl *witem = dynamic_cast<ItemImpl*> (m_windows[i]);
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
ApplicationImpl::add_window (WindowIface &window)
{
  ref_sink (window);
  m_windows.push_back (&window);
}

void
ApplicationImpl::lost_primaries()
{
  sig_missing_primary.emit();
}

bool
ApplicationImpl::remove_window (WindowIface &window)
{
  vector<WindowIface*>::iterator it = std::find (m_windows.begin(), m_windows.end(), &window);
  if (it == m_windows.end())
    return false;
  m_windows.erase (it);
  unref (window);
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
