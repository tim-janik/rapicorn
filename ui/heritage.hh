// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_HERITAGE_HH__
#define __RAPICORN_HERITAGE_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

class WindowImpl;
class WidgetImpl;

class Heritage : public virtual ReferenceCountable {
  friend        class ClassDoctor;
  class Internals;
  Internals    *internals_;
  WindowImpl   &window_;
  explicit      Heritage        (WindowImpl &window,
                                 Internals  *internals);
  /*Des*/      ~Heritage        ();
  static
  Heritage*     create_heritage (WindowImpl     &window,
                                 WidgetImpl       &widget,
                                 ColorSchemeType color_scheme);
public:
  Heritage*     adapt_heritage  (WidgetImpl           &widget,
                                 ColorSchemeType color_scheme);
  WindowImpl&   window          () const { return window_; }
  /* colors */
  Color         get_color       (StateType state,
                                 ColorType ct) const;
  Color         foreground      (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_FOREGROUND); }
  Color         background      (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_BACKGROUND); }
  Color         background_even (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_BACKGROUND_EVEN); }
  Color         background_odd  (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_BACKGROUND_ODD); }
  Color         dark_color      (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_DARK); }
  Color         dark_shadow     (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_DARK_SHADOW); }
  Color         dark_glint      (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_DARK_GLINT); }
  Color         light_color     (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_LIGHT); }
  Color         light_shadow    (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_LIGHT_SHADOW); }
  Color         light_glint     (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_LIGHT_GLINT); }
  Color         focus_color     (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_FOCUS); }
  Color         black           (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_BLACK); }
  Color         white           (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_WHITE); }
  Color         red             (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_RED); }
  Color         yellow          (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_YELLOW); }
  Color         green           (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_GREEN); }
  Color         cyan            (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_CYAN); }
  Color         blue            (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_BLUE); }
  Color         magenta         (StateType st = STATE_NORMAL) const { return get_color (st, COLOR_MAGENTA); }
  Color         insensitive_ink (StateType st = STATE_NORMAL, Color *glint = NULL) const;
  /* variants */
  Heritage&     selected        ();
  /* parsing */
  Color         resolve_color   (const String  &color_name,
                                 StateType      state,
                                 ColorType      color_type = COLOR_NONE);
};

} // Rapicorn

#endif  /* __RAPICORN_HERITAGE_HH__ */
