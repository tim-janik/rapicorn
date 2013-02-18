// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
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

static Color lighten (Color color) { return adjust_color (color, 1.0, 1.1); }
static Color darken  (Color color) { return adjust_color (color, 1.0, 0.9); }

static Color
state_color (Color     color,
             StateType state,
             ColorType ctype)
{
  /* foreground colors remain stable across certain states */
  bool background_color = true;
  switch (ctype)
    {
    case COLOR_FOREGROUND: case COLOR_FOCUS:
    case COLOR_BLACK:      case COLOR_WHITE:
    case COLOR_RED:        case COLOR_YELLOW:
    case COLOR_GREEN:      case COLOR_CYAN:
    case COLOR_BLUE:       case COLOR_MAGENTA:
      background_color = false;
    default: ;
    }
  Color c = color;
  if (state & STATE_IMPRESSED && background_color)
    c = adjust_color (c, 1.0, 0.8);
  if (state & STATE_INSENSITIVE)
    c = adjust_color (c, 0.8, 1.075);
  if (state & STATE_PRELIGHT && background_color &&
      !(state & STATE_INSENSITIVE))     /* ignore prelight if insensitive */
    c = adjust_color (c, 1.2, 1.0);
  return c;
}

static Color
colorset_normal (StateType state,
                 ColorType color_type)
{
  switch (color_type)
    {
    default:
    case COLOR_NONE:            return 0x00000000;
    case COLOR_FOREGROUND:      return 0xff000000;
    case COLOR_BACKGROUND:      return 0xffdfdcd8;
    case COLOR_BACKGROUND_EVEN: return lighten (colorset_normal (state, COLOR_BACKGROUND));
    case COLOR_BACKGROUND_ODD:  return darken  (colorset_normal (state, COLOR_BACKGROUND));
    case COLOR_DARK:            return 0xff9f9c98;
    case COLOR_DARK_SHADOW:     return adjust_color (colorset_normal (state, COLOR_DARK), 1, 0.9); // 0xff8f8c88
    case COLOR_DARK_GLINT:      return adjust_color (colorset_normal (state, COLOR_DARK), 1, 1.1); // 0xffafaca8
    case COLOR_LIGHT:           return 0xffdfdcd8;
    case COLOR_LIGHT_SHADOW:    return adjust_color (colorset_normal (state, COLOR_LIGHT), 1, 0.93); // 0xffcfccc8
    case COLOR_LIGHT_GLINT:     return adjust_color (colorset_normal (state, COLOR_LIGHT), 1, 1.07); // 0xffefece8
    case COLOR_FOCUS:           return 0xff000060;
    case COLOR_BLACK:           return 0xff000000;
    case COLOR_WHITE:           return 0xffffffff;
    case COLOR_RED:             return 0xffff0000;
    case COLOR_YELLOW:          return 0xffffff00;
    case COLOR_GREEN:           return 0xff00ff00;
    case COLOR_CYAN:            return 0xff00ffff;
    case COLOR_BLUE:            return 0xff0000ff;
    case COLOR_MAGENTA:         return 0xffff00ff;
    }
}

static Color
colorset_selected (StateType state,
                   ColorType color_type)
{
  switch (color_type)
    {
    case COLOR_FOREGROUND:      return 0xffeeeeee;
    case COLOR_BACKGROUND:      return 0xff111188;
    case COLOR_BACKGROUND_EVEN: return lighten (colorset_selected (state, COLOR_BACKGROUND));
    case COLOR_BACKGROUND_ODD:  return darken  (colorset_selected (state, COLOR_BACKGROUND));
    default:                    return colorset_normal (state, color_type);
    }
}

static Color
colorset_base (StateType state,
               ColorType color_type)
{
  switch (color_type)
    {
    case COLOR_FOREGROUND:      return 0xff101010;
    case COLOR_BACKGROUND:      return 0xfff4f4f4;
    case COLOR_BACKGROUND_EVEN: return lighten (colorset_base (state, COLOR_BACKGROUND));
    case COLOR_BACKGROUND_ODD:  return darken  (colorset_base (state, COLOR_BACKGROUND));
    default:                    return colorset_normal (state, color_type);
    }
}

typedef Color (*ColorFunc) (StateType, ColorType);

class Heritage::Internals {
  WidgetImpl &widget_;
  ColorFunc ncf, scf;
public:
  Heritage *selected;
  Internals (WidgetImpl &widget,
             ColorFunc normal_cf,
             ColorFunc selected_cf) :
    widget_ (widget), ncf (normal_cf), scf (selected_cf), selected (NULL)
  {}
  bool
  match (WidgetImpl &widget,
         ColorFunc normal_cf,
         ColorFunc selected_cf)
  {
    return widget == widget_ && normal_cf == ncf && selected_cf == scf;
  }
  Color
  get_color (const Heritage *heritage,
             StateType       state,
             ColorType       ct) const
  {
    if (heritage == selected)
      return scf (state, ct);
    else
      return ncf (state, ct);
  }
};

Heritage::~Heritage ()
{
  if (internals_)
    {
      if (internals_->selected == this)
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

Heritage*
Heritage::create_heritage (WindowImpl     &window,
                           WidgetImpl       &widget,
                           ColorSchemeType color_scheme)
{
  WindowImpl *iwindow = widget.get_window();
  assert (iwindow == &window);
  ColorFunc cnorm = colorset_normal, csel = colorset_selected;
  switch (color_scheme)
    {
    case COLOR_BASE:            cnorm = colorset_base; break;
    case COLOR_SELECTED:        cnorm = colorset_selected; break;
    case COLOR_NORMAL: case COLOR_INHERIT: ;
    }
  Internals *internals = new Internals (widget, cnorm, csel);
  Heritage *self = new Heritage (window, internals);
  return self;
}

Heritage*
Heritage::adapt_heritage (WidgetImpl       &widget,
                          ColorSchemeType color_scheme)
{
  if (internals_)
    {
      ColorFunc cnorm = colorset_normal, csel = colorset_selected;
      switch (color_scheme)
        {
        case COLOR_INHERIT:     return this;
        case COLOR_BASE:        cnorm = colorset_base; break;
        case COLOR_SELECTED:    cnorm = colorset_selected; break;
        case COLOR_NORMAL:      ;
        }
      if (internals_->match (widget, cnorm, csel))
        return this;
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
          internals_->selected = new Heritage (window_, internals_);
          ref_sink (internals_->selected);
        }
      return *internals_->selected;
    }
  else
    return *this;
}

Color
Heritage::get_color (StateType state,
                     ColorType color_type) const
{
  Color c;
  if (internals_)
    c = internals_->get_color (this, state, color_type);
  return state_color (c, state, color_type);
}

Color
Heritage::insensitive_ink (StateType state,
                           Color    *glintp) const
{
  /* constrcut insensitive ink from a mixture of foreground and dark_color */
  Color ink = get_color (state, COLOR_FOREGROUND);
  ink.combine (get_color (state, COLOR_DARK), 0x80);
  Color glint = get_color (state, COLOR_LIGHT);
  if (glintp)
    *glintp = glint;
  return ink;
}

Color
Heritage::resolve_color (const String  &color_name,
                         StateType      state,
                         ColorType      color_type)
{
  Aida::EnumInfo ect = Aida::enum_info<ColorType>();
  if (color_name[0] == '#')
    {
      uint32 argb = string_to_int (&color_name[1], 16);
      Color c (argb);
      /* invert alpha (transparency -> opacity) */
      c.alpha (0xff - c.alpha());
      return state_color (c, state, color_type);
    }
  const Aida::EnumInfo::Value *value = ect.find_first (color_name);
  if (value)
    return get_color (state, ColorType (value->value));
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
