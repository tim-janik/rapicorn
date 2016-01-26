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
  Color         foreground      (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::FOREGROUND); }
  Color         background      (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::BACKGROUND); }
  Color         background_even (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::BACKGROUND_EVEN); }
  Color         background_odd  (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::BACKGROUND_ODD); }
  Color         dark_color      (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::DARK); }
  Color         dark_shadow     (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::DARK_SHADOW); }
  Color         dark_glint      (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::DARK_GLINT); }
  Color         light_color     (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::LIGHT); }
  Color         light_shadow    (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::LIGHT_SHADOW); }
  Color         light_glint     (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::LIGHT_GLINT); }
  Color         focus_color     (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::FOCUS); }
  Color         black           (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::BLACK); }
  Color         white           (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::WHITE); }
  Color         red             (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::RED); }
  Color         yellow          (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::YELLOW); }
  Color         green           (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::GREEN); }
  Color         cyan            (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::CYAN); }
  Color         blue            (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::BLUE); }
  Color         magenta         (WidgetState st = WidgetState::NORMAL) const { return get_color (st, ColorType::MAGENTA); }
  Color         insensitive_ink (WidgetState st = WidgetState::NORMAL, Color *glint = NULL) const;
  // variants
  Heritage&     selected        ();
  // parsing
  Color         resolve_color   (const String &color_name, WidgetState state, ColorType color_type = ColorType::NONE);
};
typedef Heritage::HeritageP HeritageP;

} // Rapicorn

#endif  /* __RAPICORN_HERITAGE_HH__ */
