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

bool
ApplicationIface::factory_window (const std::string &factory_definition)
{
  return Factory::check_ui_window (factory_definition);
}

struct XurlNode : public Deletable::DeletionHook {
  String       xurl;
  Deletable   *object;
  bool         needs_remove;
  explicit     XurlNode (const String &_xu, Deletable *_ob);
  virtual void monitoring_deletable (Deletable &deletable) {}
  virtual void dismiss_deletable ();
  virtual     ~XurlNode();
};

class XurlMap {
  typedef std::map<String,XurlNode*>           StringMap;
  typedef std::map<const Deletable*,XurlNode*> ObjectMap;
  StringMap     m_smap;
  ObjectMap     m_omap;
  XurlNode*     lookup_node (const String *xurl, const Deletable *object);
public:
  Deletable*    lookup (const String &xurl)     { XurlNode *xnode = lookup_node (&xurl, NULL); return xnode ? xnode->object : NULL; }
  String        lookup (const Deletable *obj)   { XurlNode *xnode = lookup_node (NULL, obj);   return xnode ? xnode->xurl : ""; }
  bool          add    (String xurl, Deletable *obj);
  bool          remove (String xurl);
  bool          remove (Deletable *obj);
};

bool
XurlMap::add (String xurl, Deletable *obj)
{
  XurlNode *xnode = lookup_node (&xurl, obj);
  if (xnode)
    return false;
  xnode = new XurlNode (xurl, obj);
  m_smap[xnode->xurl] = xnode;
  m_omap[xnode->object] = xnode;
  return true;
}

bool
XurlMap::remove (String xurl)
{
  auto sit = m_smap.find (xurl);
  if (sit == m_smap.end())
    return false;
  XurlNode *xnode = sit->second;
  auto oit = m_omap.find (xnode->object);
  assert (oit != m_omap.end());
  m_smap.erase (sit);
  m_omap.erase (oit);
  delete xnode;
  return true;
}

bool
XurlMap::remove (Deletable *obj)
{
  auto oit = m_omap.find (obj);
  if (oit == m_omap.end())
    return false;
  XurlNode *xnode = oit->second;
  auto sit = m_smap.find (xnode->xurl);
  assert (sit != m_smap.end());
  m_smap.erase (sit);
  m_omap.erase (oit);
  delete xnode;
  return true;
}

XurlNode*
XurlMap::lookup_node (const String *xurl, const Deletable *object)
{
  if (xurl)
    {
      auto it = m_smap.find (*xurl);
      if (it != m_smap.end())
        return it->second;
    }
  if (object)
    {
      auto it = m_omap.find (object);
      if (it != m_omap.end())
        return it->second;
    }
  return NULL;
}

static Mutex   xurl_mutex;
static XurlMap xurl_map;

XurlNode::XurlNode (const String &_xu, Deletable *_ob) :
  xurl (_xu), object (_ob), needs_remove (0)
{
  deletable_add_hook (object);
  needs_remove = true;
}

void
XurlNode::dismiss_deletable ()
{
  ScopedLock<Mutex> locker (xurl_mutex);
  needs_remove = false;
  // printerr ("DISMISS: deletable=%p hook=%p\n", object, this);
  const bool deleted = xurl_map.remove (object);
  // here, this is deleted
  assert (deleted == true);
  // this is leaked if deleted==false
}

XurlNode::~XurlNode()
{
  if (needs_remove)
    deletable_remove_hook (object);
}

bool
ApplicationIface::xurl_add (const String &model_path, ListModelIface &model)
{
  assert_return (model_path.find ("//local/data/") == 0, false); // FIXME: should be auto-added?
  if (model_path.find ("//local/data/") != 0)
    return false;
  ScopedLock<Mutex> locker (xurl_mutex);
  return xurl_map.add (model_path, &model);
}

bool
ApplicationIface::xurl_sub (ListModelIface &model)
{
  ScopedLock<Mutex> locker (xurl_mutex);
  return xurl_map.remove (&model);
}

ListModelIface*
ApplicationIface::xurl_find (const String &model_path)
{
  ScopedLock<Mutex> locker (xurl_mutex);
  Deletable *deletable = xurl_map.lookup (model_path);
  locker.unlock();
  return dynamic_cast<ListModelIface*> (deletable);
}

String
ApplicationIface::xurl_path (const ListModelIface &model)
{
  ScopedLock<Mutex> locker (xurl_mutex);
  return xurl_map.lookup (&model);
}

ApplicationImpl::ApplicationImpl() :
  m_tc (0)
{}

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
      critical ("%s: constructed widget lacks window interface: %s", window_identifier.c_str(), item.typeid_name().c_str());
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
  StringVector spl;
  if (search_vpath)
    {
      const char *vpath = getenv ("VPATH");
      spl = Path::searchpath_split (vpath ? vpath : "");
    }
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
ApplicationImpl::close_all ()
{
  vector<WindowIface*> candidates = m_windows;
  for (auto wip : candidates)
    {
      auto alive = find (m_windows.begin(), m_windows.end(), wip);
      if (alive != m_windows.end())
        wip->close();
    }
  // FIXME: use WindowImpl::close_all
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
