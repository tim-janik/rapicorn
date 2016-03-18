// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "heritage.hh"
#include "widget.hh"

namespace Rapicorn {

static Color
adjust_color (Color  color,
              double saturation_factor,
              double value_factor)
{
  double hue, saturation, value;
  color.get_hsv (&hue, &saturation, &value);
  saturation *= saturation_factor;
  value *= value_factor;
  color.set_hsv (hue, MIN (1, saturation), MIN (1, value));
  return color;
}

static __attribute__ ((unused)) Color lighten   (Color color) { return adjust_color (color, 1.0, 1.1); }
static __attribute__ ((unused)) Color darken    (Color color) { return adjust_color (color, 1.0, 0.9); }
static __attribute__ ((unused)) Color alternate (Color color) { return adjust_color (color, 1.0, 0.98); } // tainting for even-odd alterations

static Color
state_color (Color color, WidgetState widget_state, ColorType ctype)
{
  /* foreground colors remain stable across certain states */
  bool background_color = true;
  switch (ctype)
    {
    case ColorType::FOREGROUND: case ColorType::FOCUS:
    case ColorType::BLACK:      case ColorType::WHITE:
    case ColorType::RED:        case ColorType::YELLOW:
    case ColorType::GREEN:      case ColorType::CYAN:
    case ColorType::BLUE:       case ColorType::MAGENTA:
      background_color = false;
    default: ;
    }
  Color c = color;
  const uint64 state = uint64 (widget_state);
  if (state & uint64 (WidgetState::ACTIVE) && background_color)
    c = adjust_color (c, 1.0, 0.8);
  if (state & uint64 (WidgetState::INSENSITIVE))
    c = adjust_color (c, 0.8, 1.075);
  if (state & uint64 (WidgetState::HOVER) && background_color &&
      !(state & uint64 (WidgetState::INSENSITIVE)))     // ignore hover if insensitive
    c = adjust_color (c, 1.2, 1.0);
  return c;
}

static Color
colorset_normal (WidgetState state,
                 ColorType color_type)
{
  switch (color_type)
    {
    default:
    case ColorType::NONE:            return 0x00000000;
    case ColorType::FOREGROUND:      return 0xff000000;
    case ColorType::BACKGROUND:      return 0xffdfdcd8;
    case ColorType::BACKGROUND_EVEN: return colorset_normal (state, ColorType::BACKGROUND);
    case ColorType::BACKGROUND_ODD:  return alternate (colorset_normal (state, ColorType::BACKGROUND));
    case ColorType::DARK:            return 0xff9f9c98;
    case ColorType::DARK_SHADOW:     return adjust_color (colorset_normal (state, ColorType::DARK), 1, 0.9); // 0xff8f8c88
    case ColorType::DARK_GLINT:      return adjust_color (colorset_normal (state, ColorType::DARK), 1, 1.1); // 0xffafaca8
    case ColorType::LIGHT:           return 0xffdfdcd8;
    case ColorType::LIGHT_SHADOW:    return adjust_color (colorset_normal (state, ColorType::LIGHT), 1, 0.93); // 0xffcfccc8
    case ColorType::LIGHT_GLINT:     return adjust_color (colorset_normal (state, ColorType::LIGHT), 1, 1.07); // 0xffefece8
    case ColorType::FOCUS:           return 0xff000060;
    case ColorType::BLACK:           return 0xff000000;
    case ColorType::WHITE:           return 0xffffffff;
    case ColorType::RED:             return 0xffff0000;
    case ColorType::YELLOW:          return 0xffffff00;
    case ColorType::GREEN:           return 0xff00ff00;
    case ColorType::CYAN:            return 0xff00ffff;
    case ColorType::BLUE:            return 0xff0000ff;
    case ColorType::MAGENTA:         return 0xffff00ff;
    }
}

static Color
colorset_selected (WidgetState state,
                   ColorType color_type)
{
  switch (color_type)
    {
    case ColorType::FOREGROUND:      return 0xfffcfdfe;      // inactive: 0xff010203
    case ColorType::BACKGROUND:      return 0xff2595e5;      // inactive: 0xff33aaff
    case ColorType::BACKGROUND_EVEN: return colorset_selected (state, ColorType::BACKGROUND);
    case ColorType::BACKGROUND_ODD:  return alternate (colorset_selected (state, ColorType::BACKGROUND));
    default:                    return colorset_normal (state, color_type);
    }
}

static Color
colorset_base (WidgetState state,
               ColorType color_type)
{
  switch (color_type)
    {
    case ColorType::FOREGROUND:      return 0xff101010;
    case ColorType::BACKGROUND:      return 0xfff4f4f4;
    case ColorType::BACKGROUND_EVEN: return colorset_base (state, ColorType::BACKGROUND);
    case ColorType::BACKGROUND_ODD:  return alternate (colorset_base (state, ColorType::BACKGROUND));
    default:                    return colorset_normal (state, color_type);
    }
}

typedef Color (*ColorFunc) (WidgetState, ColorType);

class Heritage::Internals {
  WidgetImpl &widget_;
  ColorFunc ncf, scf;
public:
  HeritageP selected;
  Internals (WidgetImpl &widget, ColorFunc normal_cf, ColorFunc selected_cf) :
    widget_ (widget), ncf (normal_cf), scf (selected_cf), selected (NULL)
  {}
  bool
  match (WidgetImpl &widget, ColorFunc normal_cf, ColorFunc selected_cf)
  {
    return widget == widget_ && normal_cf == ncf && selected_cf == scf;
  }
  Color
  get_color (const Heritage *heritage, WidgetState state, ColorType ct) const
  {
    if (selected.get() == heritage)
      return scf (state, ct);
    else
      return ncf (state, ct);
  }
};

Heritage::~Heritage ()
{
  if (internals_)
    {
      if (internals_->selected.get() == this)
        internals_->selected = NULL;
      else
        {
          if (internals_->selected)
            internals_->selected->internals_ = NULL;
          delete internals_;
          internals_ = NULL;
        }
    }
}

HeritageP
Heritage::create_heritage (WindowImpl &window, WidgetImpl &widget, ColorScheme color_scheme)
{
  WindowImpl *iwindow = widget.get_window();
  assert (iwindow == &window);
  ColorFunc cnorm = colorset_normal, csel = colorset_selected;
  switch (color_scheme)
    {
    case ColorScheme::BASE:            cnorm = colorset_base; break;
    case ColorScheme::SELECTED:        cnorm = colorset_selected; break;
    case ColorScheme::NORMAL: case ColorScheme::INHERIT: ;
    }
  Internals *internals = new Internals (widget, cnorm, csel);
  return FriendAllocator<Heritage>::make_shared (window, internals);
}

HeritageP
Heritage::adapt_heritage (WidgetImpl &widget, ColorScheme color_scheme)
{
  if (internals_)
    {
      ColorFunc cnorm = colorset_normal, csel = colorset_selected;
      switch (color_scheme)
        {
        case ColorScheme::INHERIT:     return shared_from_this();
        case ColorScheme::BASE:        cnorm = colorset_base; break;
        case ColorScheme::SELECTED:    cnorm = colorset_selected; break;
        case ColorScheme::NORMAL:      ;
        }
      if (internals_->match (widget, cnorm, csel))
        return shared_from_this();
    }
  WindowImpl *window = widget.get_window();
  if (!window)
    fatal ("Heritage: create heritage without window widget for: %s", widget.name().c_str());
  return create_heritage (*window, widget, color_scheme);
}

Heritage::Heritage (WindowImpl &window,
                    Internals  *internals) :
  internals_ (internals), window_ (window)
{}

Heritage&
Heritage::selected ()
{
  if (internals_)
    {
      if (!internals_->selected)
        {
          internals_->selected = FriendAllocator<Heritage>::make_shared (window_, internals_);
        }
      return *internals_->selected;
    }
  else
    return *this;
}

Color
Heritage::get_color (WidgetState state,
                     ColorType color_type) const
{
  Color c;
  if (internals_)
    c = internals_->get_color (this, state, color_type);
  return state_color (c, state, color_type);
}

Color
Heritage::insensitive_ink (WidgetState state,
                           Color    *glintp) const
{
  /* constrcut insensitive ink from a mixture of foreground and dark_color */
  Color ink = get_color (state, ColorType::FOREGROUND);
  ink.combine (get_color (state, ColorType::DARK), 0x80);
  Color glint = get_color (state, ColorType::LIGHT);
  if (glintp)
    *glintp = glint;
  return ink;
}

Color
Heritage::resolve_color (const String  &color_name,
                         WidgetState      state,
                         ColorType      color_type)
{
  if (color_name[0] == '#')
    {
      uint32 argb = string_to_int (&color_name[1], NULL, 16);
      Color c (argb);
      /* invert alpha (transparency -> opacity) */
      c.alpha (0xff - c.alpha());
      return state_color (c, state, color_type);
    }
  Aida::EnumValue evalue = Aida::enum_info<ColorType>().find_value (color_name);
  if (evalue.ident)
    return get_color (state, ColorType (evalue.value));
  else
    {
      Color parsed_color = Color::from_name (color_name);
      if (!parsed_color) // transparent black
        return get_color (state, color_type);
      else
        return state_color (parsed_color, state, color_type);
    }
}

} // Rapicorn
