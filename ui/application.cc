// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "application.hh"
#include "window.hh"
#include "factory.hh"
#include "screenwindow.hh"
#include "image.hh"
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
  return Factory::check_ui_window (factory_definition);
}

typedef std::map<String,BaseObject*> StringObjectMap;
static Mutex           xurl_mutex;
static StringObjectMap xurl_map;

static bool
xurl_map_add (BaseObject   &object,
              const String &key)
{
  ScopedLock<Mutex> locker (xurl_mutex);
  StringObjectMap::iterator it = xurl_map.find (key);
  if (it != xurl_map.end())
    return false;
  xurl_map[key] = &object;
  return true;
}

static bool
xurl_map_sub (const String &key)
{
  ScopedLock<Mutex> locker (xurl_mutex);
  StringObjectMap::iterator it = xurl_map.find (key);
  if (it != xurl_map.end())
    {
      xurl_map.erase (it);
      return true;
    }
  return false;
}

static BaseObject*
xurl_map_get (const String &key)
{
  ScopedLock<Mutex> locker (xurl_mutex);
  StringObjectMap::iterator it = xurl_map.find (key);
  return it != xurl_map.end() ? it->second : NULL;
}

static class XurlDataKey : public DataKey<String> {
  virtual void destroy (String data) { xurl_map_sub (data); }
} xurl_name_key;

bool
ApplicationIface::xurl_add (const String &model_path, ListModelIface &model)
{
  assert_return (model_path.find ("//local/data/") == 0, false); // FIXME
  if (model_path.find ("//local/data/") != 0)
    return false;
  const String xurl_name = model.get_data (&xurl_name_key);
  if (!xurl_name.empty())
    return false; // model already in map
  if (!xurl_map_add (model, model_path))
    return false; // model_path already in map
  model.set_data (&xurl_name_key, model_path);
  return true;
}

bool
ApplicationIface::xurl_sub (ListModelIface &model)
{
  const String xurl_name = model.get_data (&xurl_name_key);
  if (!xurl_name.empty())
    {
      BaseObject *bo = xurl_map_get (xurl_name);
      assert_return (bo == &model, false);
      model.delete_data (&xurl_name_key);       // calls xurl_map_sub
      bo = xurl_map_get (xurl_name);
      assert_return (bo == NULL, false);
      return true;
    }
  else
    return false;
}

ListModelIface*
ApplicationIface::xurl_find (const String &model_path)
{
  BaseObject *bo = model_path.empty() ? NULL : xurl_map_get (model_path);
  return dynamic_cast<ListModelIface*> (bo);
}

String
ApplicationIface::xurl_path (const ListModelIface &model)
{
  const String xurl_name = model.get_data (&xurl_name_key);
  return xurl_name;
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
                                const StringSeq      &arguments)
{
  ItemImpl &item = Factory::create_ui_item (window_identifier, arguments);
  WindowIface *window = dynamic_cast<WindowIface*> (&item);
  if (!window)
    {
      ref_sink (item);
      critical ("%s: constructed widget lacks window interface: %s", window_identifier.c_str(), item.typeid_pretty_name().c_str());
      unref (item);
    }
  return window;
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

StringSeq
ApplicationImpl::auto_load (const String  &defs_domain,
                            const String  &file_name,
                            const String  &binary_path,
                            const String  &i18n_domain)
{
  String fullname = auto_path (file_name, binary_path, true);
  StringSeq definitions;
  String errs = Factory::parse_ui_file (defs_domain, fullname, i18n_domain, &definitions);
  if (!errs.empty())
    fatal ("%s: %s", fullname.c_str(), errs.c_str());
  return definitions;
}

void
ApplicationImpl::load_string (const std::string &defs_domain,
                              const std::string &xml_string,
                              const std::string &i18n_domain)
{
  String errs = Factory::parse_ui_data (defs_domain, "<ApplicationImpl::load_string>", xml_string.size(), xml_string.data(), i18n_domain);
  if (!errs.empty())
    fatal ("failed to parse string: %s\n%s", errs.c_str(), xml_string.c_str());
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

WindowIface*
ApplicationImpl::query_window (const String &selector)
{
  vector<ItemImpl*> input;
  for (vector<WindowIface*>::const_iterator it = m_windows.begin(); it != m_windows.end(); it++)
    {
      ItemImpl *item = dynamic_cast<ItemImpl*> (*it);
      if (item)
        input.push_back (item);
    }
  vector<ItemImpl*> result = Selector::Matcher::match_selector (selector, input.begin(), input.end());
  if (result.size() == 1) // unique
    return dynamic_cast<WindowIface*> (result[0]);
  return NULL;
}

WindowList
ApplicationImpl::query_windows (const String &selector)
{
  vector<ItemImpl*> input;
  for (vector<WindowIface*>::const_iterator it = m_windows.begin(); it != m_windows.end(); it++)
    {
      ItemImpl *item = dynamic_cast<ItemImpl*> (*it);
      if (item)
        input.push_back (item);
    }
  vector<ItemImpl*> result = Selector::Matcher::match_selector (selector, input.begin(), input.end());
  WindowList wlist;
  for (vector<ItemImpl*>::const_iterator it = result.begin(); it != result.end(); it++)
    wlist.push_back (dynamic_cast<WindowIface*> (*it));
  return wlist;
}

WindowList
ApplicationImpl::list_windows ()
{
  WindowList wl;
  for (uint i = 0; i < m_windows.size(); i++)
    wl.push_back (m_windows[i]);
  return wl;
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
