// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "application.hh"
#include "window.hh"
#include "factory.hh"
#include "screenwindow.hh"
#include "image.hh"
#include "uithread.hh"
#include "internal.hh"
#include "selob.hh"
#include <algorithm>
#include <stdlib.h>

namespace Rapicorn {

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
  StringMap     smap_;
  ObjectMap     omap_;
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
  smap_[xnode->xurl] = xnode;
  omap_[xnode->object] = xnode;
  return true;
}

bool
XurlMap::remove (String xurl)
{
  auto sit = smap_.find (xurl);
  if (sit == smap_.end())
    return false;
  XurlNode *xnode = sit->second;
  auto oit = omap_.find (xnode->object);
  assert (oit != omap_.end());
  smap_.erase (sit);
  omap_.erase (oit);
  delete xnode;
  return true;
}

bool
XurlMap::remove (Deletable *obj)
{
  auto oit = omap_.find (obj);
  if (oit == omap_.end())
    return false;
  XurlNode *xnode = oit->second;
  auto sit = smap_.find (xnode->xurl);
  assert (sit != smap_.end());
  smap_.erase (sit);
  omap_.erase (oit);
  delete xnode;
  return true;
}

XurlNode*
XurlMap::lookup_node (const String *xurl, const Deletable *object)
{
  if (xurl)
    {
      auto it = smap_.find (*xurl);
      if (it != smap_.end())
        return it->second;
    }
  if (object)
    {
      auto it = omap_.find (object);
      if (it != omap_.end())
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
  tc_ (0)
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
  WidgetImpl &widget = Factory::create_ui_widget (window_identifier, arguments);
  WindowIface *window = dynamic_cast<WindowIface*> (&widget);
  if (!window)
    {
      ref_sink (widget);
      critical ("%s: constructed widget lacks window interface: %s", window_identifier.c_str(), widget.typeid_name().c_str());
      unref (widget);
    }
  return window;
}

String
ApplicationImpl::auto_path (const String  &file_name,
                            const String  &binary_path,
                            bool           search_vpath)
{
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
  vector<WindowIface*> candidates = windows_;
  for (auto wip : candidates)
    {
      auto alive = find (windows_.begin(), windows_.end(), wip);
      if (alive != windows_.end())
        wip->close();
    }
  // FIXME: use WindowImpl::close_all
}

WindowIface*
ApplicationImpl::query_window (const String &selector)
{
  Selector::SelobAllocator sallocator;
  vector<Selector::Selob*> input;
  for (vector<WindowIface*>::const_iterator it = windows_.begin(); it != windows_.end(); it++)
    {
      WidgetImpl *widget = dynamic_cast<WidgetImpl*> (*it);
      if (widget)
        input.push_back (sallocator.widget_selob (*widget));
    }
  vector<Selector::Selob*> result = Selector::Matcher::query_selector_objects (selector, input.begin(), input.end());
  if (result.size() == 1) // unique
    return dynamic_cast<WindowIface*> (sallocator.selob_widget (*result[0]));
  return NULL;
}

WindowList
ApplicationImpl::query_windows (const String &selector)
{
  Selector::SelobAllocator sallocator;
  vector<Selector::Selob*> input;
  for (vector<WindowIface*>::const_iterator it = windows_.begin(); it != windows_.end(); it++)
    {
      WidgetImpl *widget = dynamic_cast<WidgetImpl*> (*it);
      if (widget)
        input.push_back (sallocator.widget_selob (*widget));
    }
  vector<Selector::Selob*> result = Selector::Matcher::query_selector_objects (selector, input.begin(), input.end());
  WindowList wlist;
  for (vector<Selector::Selob*>::const_iterator it = result.begin(); it != result.end(); it++)
    wlist.push_back (dynamic_cast<WindowIface*> (sallocator.selob_widget (**it))->*Aida::_handle);
  return wlist;
}

WindowList
ApplicationImpl::list_windows ()
{
  WindowList wl;
  for (uint i = 0; i < windows_.size(); i++)
    {
      WindowHandle wh = windows_[i]->*Aida::_handle;
      wl.push_back (wh);
    }
  return wl;
}

void
ApplicationImpl::add_window (WindowIface &window)
{
  ref_sink (window);
  windows_.push_back (&window);
}

void
ApplicationImpl::lost_primaries()
{
  sig_missing_primary.emit();
}

bool
ApplicationImpl::remove_window (WindowIface &window)
{
  vector<WindowIface*>::iterator it = std::find (windows_.begin(), windows_.end(), &window);
  if (it == windows_.end())
    return false;
  windows_.erase (it);
  unref (window);
  return true;
}

void
ApplicationImpl::test_counter_set (int val)
{
  tc_ = val;
}

void
ApplicationImpl::test_counter_add (int val)
{
  tc_ += val;
}

int
ApplicationImpl::test_counter_get ()
{
  return tc_;
}

int
ApplicationImpl::test_counter_inc_fetch ()
{
  return ++tc_;
}

int64
ApplicationImpl::test_hook ()
{
  return server_app_test_hook();
}

} // Rapicorn
