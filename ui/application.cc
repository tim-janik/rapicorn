// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "application.hh"
#include "window.hh"
#include "factory.hh"
#include "displaywindow.hh"
#include "image.hh"
#include "uithread.hh"
#include "internal.hh"
#include "selob.hh"
#include <algorithm>
#include <stdlib.h>

namespace Rapicorn {

ApplicationImpl::ApplicationImpl() :
  tc_ (0)
{}

ApplicationImpl&
ApplicationImpl::the ()
{
  // call new and never delete to keep singelton across static dtors
  static DurableInstance<ApplicationImplP*> singleton;
  if (!*singleton)
    *singleton = std::make_shared<ApplicationImpl>();
  return *singleton->get();
}

WindowIfaceP
ApplicationImpl::create_window (const String &window_identifier, const StringSeq &arguments)
{
  WidgetImplP widget = Factory::create_ui_widget (window_identifier, arguments);
  WindowIfaceP window = shared_ptr_cast<WindowIface> (widget);
  if (!window)
    critical ("%s: constructed widget lacks window interface: %s", window_identifier.c_str(), widget->typeid_name());
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
ApplicationImpl::auto_load (const String &file_name, const String &binary_path, const String &i18n_domain)
{
  String fullname = auto_path (file_name, binary_path, true);
  StringSeq definitions;
  size_t flen = 0;
  String errors;
  char *fdata = Path::memread (fullname, &flen);
  if (!fdata)
    errors = strerror (errno ? errno : ENOENT);
  else
    {
      errors = Factory::parse_ui_data (fullname, flen, fdata, i18n_domain, NULL, &definitions);
      Path::memfree (fdata);
    }
  if (!errors.empty())
    fatal ("%s: %s", fullname.c_str(), errors.c_str());
  return definitions;
}

void
ApplicationImpl::load_string (const std::string &xml_string,
                              const std::string &i18n_domain)
{
  String errs = Factory::parse_ui_data ("<ApplicationImpl::load_string>", xml_string.size(), xml_string.data(), i18n_domain, NULL, NULL);
  if (!errs.empty())
    fatal ("failed to parse string: %s\n%s", errs.c_str(), xml_string.c_str());
}

bool
ApplicationImpl::factory_window (const std::string &factory_definition)
{
  return Factory::check_ui_window (factory_definition);
}

bool
ApplicationImpl::finishable ()
{
  return uithread_main_loop()->finishable();
}

void
ApplicationImpl::close_all ()
{
  vector<WindowIfaceP> candidates = windows_;
  for (auto wip : candidates)
    {
      auto alive = find (windows_.begin(), windows_.end(), wip);
      if (alive != windows_.end())
        wip->close();
    }
  // FIXME: use WindowImpl::close_all
}

WindowIfaceP
ApplicationImpl::query_window (const String &selector)
{
  Selector::SelobAllocator sallocator;
  vector<Selector::Selob*> input;
  for (vector<WindowIfaceP>::const_iterator it = windows_.begin(); it != windows_.end(); it++)
    {
      WidgetImplP widget = shared_ptr_cast<WidgetImpl> (*it);
      if (widget)
        input.push_back (sallocator.widget_selob (*widget));
    }
  vector<Selector::Selob*> result = Selector::Matcher::query_selector_objects (selector, input.begin(), input.end());
  if (result.size() == 1) // unique
    return shared_ptr_cast<WindowIface> (sallocator.selob_widget (*result[0]));
  return NULL;
}

WindowList
ApplicationImpl::query_windows (const String &selector)
{
  Selector::SelobAllocator sallocator;
  vector<Selector::Selob*> input;
  for (vector<WindowIfaceP>::const_iterator it = windows_.begin(); it != windows_.end(); it++)
    {
      WidgetImplP widget = shared_ptr_cast<WidgetImpl> (*it);
      if (widget)
        input.push_back (sallocator.widget_selob (*widget));
    }
  vector<Selector::Selob*> result = Selector::Matcher::query_selector_objects (selector, input.begin(), input.end());
  WindowList wlist;
  for (vector<Selector::Selob*>::const_iterator it = result.begin(); it != result.end(); it++)
    wlist.push_back (shared_ptr_cast<WindowIface> (sallocator.selob_widget (**it)));
  return wlist;
}

WindowList
ApplicationImpl::list_windows ()
{
  WindowList wl;
  for (size_t i = 0; i < windows_.size(); i++)
    wl.push_back (windows_[i]);
  return wl;
}

void
ApplicationImpl::add_window (WindowIface &window)
{
  windows_.push_back (shared_ptr_cast<WindowIface> (&window));
}

void
ApplicationImpl::lost_primaries()
{
  sig_missing_primary.emit();
}

bool
ApplicationImpl::remove_window (WindowIface &window)
{
  for (auto it = windows_.begin(); it != windows_.end(); ++it)
    if (&window == &**it)
      {
        windows_.erase (it);
        return true;
      }
  return false;
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
