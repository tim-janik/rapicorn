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

WindowIfaceP
ApplicationImpl::create_window (const String &window_identifier, const StringSeq &arguments)
{
  WidgetImpl &widget = Factory::create_ui_widget (window_identifier, arguments);
  WindowIface *window = dynamic_cast<WindowIface*> (&widget);
  if (!window)
    {
      ref_sink (widget);
      critical ("%s: constructed widget lacks window interface: %s", window_identifier.c_str(), widget.typeid_name().c_str());
      unref (widget);
    }
  return shared_ptr_cast<WindowIface> (window);
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
      errors = Factory::parse_ui_data ("", fullname, flen, fdata, i18n_domain, &definitions);
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
  String errs = Factory::parse_ui_data ("", "<ApplicationImpl::load_string>", xml_string.size(), xml_string.data(), i18n_domain);
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

WindowIfaceP
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
    return shared_ptr_cast<WindowIface> (sallocator.selob_widget (*result[0]));
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
