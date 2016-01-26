// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_HERITAGE_HH__
#define __RAPICORN_HERITAGE_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

class WindowImpl;
class WidgetImpl;

class Heritage : public std::enable_shared_from_this<Heritage> {
  class Internals;
  Internals    *internals_;
  WindowImpl   &window_;
  friend           class FriendAllocator<Heritage>; // allows make_shared() access to ctor/dtor
  explicit         Heritage         (WindowImpl &window, Internals *internals);
public:
  typedef std::shared_ptr<Heritage> HeritageP;
protected:
  virtual         ~Heritage         ();
  static HeritageP create_heritage  (WindowImpl &window, WidgetImpl &widget, ColorScheme color_scheme);
public:
  HeritageP     adapt_heritage  (WidgetImpl &widget, ColorScheme color_scheme);
  WindowImpl&   window          () const                { return window_; }
  // colors
  Color         get_color       (WidgetState state,
                                 ColorType ct) const;
  Color         foreground      (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_FOREGROUND); }
  Color         background      (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_BACKGROUND); }
  Color         background_even (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_BACKGROUND_EVEN); }
  Color         background_odd  (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_BACKGROUND_ODD); }
  Color         dark_color      (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_DARK); }
  Color         dark_shadow     (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_DARK_SHADOW); }
  Color         dark_glint      (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_DARK_GLINT); }
  Color         light_color     (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_LIGHT); }
  Color         light_shadow    (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_LIGHT_SHADOW); }
  Color         light_glint     (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_LIGHT_GLINT); }
  Color         focus_color     (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_FOCUS); }
  Color         black           (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_BLACK); }
  Color         white           (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_WHITE); }
  Color         red             (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_RED); }
  Color         yellow          (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_YELLOW); }
  Color         green           (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_GREEN); }
  Color         cyan            (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_CYAN); }
  Color         blue            (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_BLUE); }
  Color         magenta         (WidgetState st = STATE_NORMAL) const { return get_color (st, COLOR_MAGENTA); }
  Color         insensitive_ink (WidgetState st = STATE_NORMAL, Color *glint = NULL) const;
  // variants
  Heritage&     selected        ();
  // parsing
  Color         resolve_color   (const String &color_name, WidgetState state, ColorType color_type = COLOR_NONE);
};
typedef Heritage::HeritageP HeritageP;

} // Rapicorn

#endif  /* __RAPICORN_HERITAGE_HH__ */
