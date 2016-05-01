// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "style.hh"
#include "painter.hh"
#include "factory.hh"
#include <unordered_map>
#include "../rcore/svg.hh"

#define TDEBUG(...)     RAPICORN_KEY_DEBUG ("Theme", __VA_ARGS__)
#define SDEBUG(...)     RAPICORN_KEY_DEBUG ("Style", __VA_ARGS__)
#define CDEBUG(...)     RAPICORN_KEY_DEBUG ("Color", __VA_ARGS__)

namespace Rapicorn {

// == Config ==
static StringVector
user_ini_files()
{
  /* INI file search order:
   * 1. ${XDG_CONFIG_HOME:-~/.config}/application/rapicorn.ini
   * 2. ${XDG_CONFIG_HOME:-~/.config}/rapicorn/config.ini
   * 3. ${XDG_CONFIG_DIRS} * /application/rapicorn.ini
   * 4. ${XDG_CONFIG_DIRS} * /rapicorn/config.ini
   * 5. /etc/application/rapicorn.ini
   * 6. /etc/rapicorn/config.ini
   */
  String config_searchpaths = Path::searchpath_join (Path::config_home(), Path::config_dirs());
  if (!Path::searchpath_contains (config_searchpaths, "/etc/"))
    config_searchpaths = Path::searchpath_join (config_searchpaths, "/etc");
  String config_files = Path::searchpath_multiply (Path::config_names(), "rapicorn.ini");
  config_files = Path::searchpath_join (config_files, "rapicorn/config.ini");
  config_searchpaths = Path::searchpath_multiply (config_searchpaths, config_files);
  StringVector inifiles = Path::searchpath_list (config_searchpaths, "r");
  return inifiles;
}

String
Config::get (const String &setting, const String &fallback)
{
  static const StringVector ini_file_names = user_ini_files();
  static vector<IniFile*> ini_files (ini_file_names.size());
  String value;
  for (size_t i = 0; i < ini_files.size(); i++)
    {
      if (!ini_files[i])
        ini_files[i] = new IniFile (Blob::load (ini_file_names[i]));
      if (ini_files[i]->has_value (setting, &value))
        return value;
    }
  return fallback;
}

// == Colors ==
static inline uint32
cairo_image_surface_peek_argb (cairo_surface_t *surface, uint x, uint y)
{
  assert_return (surface != NULL, 0x00000000);
  assert_return (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32, 0x00000000);
  const uint height = cairo_image_surface_get_height (surface);
  const uint width = cairo_image_surface_get_width (surface);
  const uint stride = cairo_image_surface_get_stride (surface);
  if (x < width && y < height)
    {
      const uint32 *pixels = (const uint32*) cairo_image_surface_get_data (surface);
      const uint32 argb = pixels[stride / 4 * y + x];
      return argb;
    }
  return 0x00000000;
}

static Color
adjust_color (Color color, double saturation_factor, double value_factor)
{
  double hue, saturation, value;
  color.get_hsv (&hue, &saturation, &value);
  saturation *= saturation_factor;
  value *= value_factor;
  color.set_hsv (hue, MIN (1, saturation), MIN (1, value));
  return color;
}
// static Color lighten   (Color color) { return adjust_color (color, 1.0, 1.1); }
// static Color darken    (Color color) { return adjust_color (color, 1.0, 0.9); }
// static Color alternate (Color color) { return adjust_color (color, 1.0, 0.98); } // tainting for even-odd alterations

// == StyleImpl ==
class StyleImpl : public StyleIface {
  Svg::FileP svg_file_;
  typedef std::pair<String,WidgetState> FragmentStatePair;
  std::map<FragmentStatePair,Color> color_cache_;
public:
  explicit      StyleImpl       (Svg::FileP svgf);
  virtual Color theme_color     (double hue360, double saturation100, double brightness100, const String &detail) override;
  virtual Color state_color     (WidgetState state, StyleColor color_type, const String &detail) override;
  Color         fragment_color  (const String &fragment, WidgetState state);
};

StyleImpl::StyleImpl (Svg::FileP svgf) :
  svg_file_ (svgf)
{}

Color
StyleImpl::theme_color (double hue360, double saturation100, double brightness100, const String &detail)
{
  Color c;
  c.set_hsv (hue360, saturation100 * 0.01, brightness100 * 0.01);
  return c.argb();
}

Color
StyleImpl::state_color (WidgetState state, StyleColor color_type, const String &detail)
{
  switch (color_type)
    {
    case StyleColor::FOREGROUND:    return fragment_color ("fg", state);
    case StyleColor::BACKGROUND:    return fragment_color ("bg", state);
    case StyleColor::DARK:          return fragment_color ("dark", state);
    case StyleColor::DARK_SHADOW:   return adjust_color (state_color (state, StyleColor::DARK, detail), 1, 0.9); // 0xff8f8c88
    case StyleColor::DARK_GLINT:    return adjust_color (state_color (state, StyleColor::DARK, detail), 1, 1.1); // 0xffafaca8
    case StyleColor::LIGHT:         return fragment_color ("light", state);
    case StyleColor::LIGHT_SHADOW:  return adjust_color (state_color (state, StyleColor::LIGHT, detail), 1, 0.93); // 0xffcfccc8
    case StyleColor::LIGHT_GLINT:   return adjust_color (state_color (state, StyleColor::LIGHT, detail), 1, 1.07); // 0xffefece8
    case StyleColor::FOCUS_COLOR:   return fragment_color ("fc", state);
    default:                        return 0x00000000; // silence warnings
    }
}

Color
StyleImpl::fragment_color (const String &fragment, WidgetState state)
{
  const FragmentStatePair fsp = std::make_pair (fragment, state);
  auto it = color_cache_.find (fsp);
  if (it != color_cache_.end())
    return it->second; // pair of <FragmentStatePair,Color>
  Color color = string_startswith (fragment, "fg") ? 0xff006600 : 0xffeebbee;
  const String match = svg_file_ ? StyleIface::pick_fragment (fragment, state, svg_file_->list()) : "";
  if (!match.empty())
    {
      ImagePainter painter (svg_file_, match);
      Requisition size = painter.image_size ();
      if (size.width >= 1 && size.height >= 1)
        {
          cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, iceil (size.width), iceil (size.height));
          cairo_t *cr = cairo_create (surface);
          Rect rect (0, 0, size.width, size.height);
          painter.draw_image (cr, rect, rect);
          const uint argb = cairo_image_surface_peek_argb (surface, size.width / 2, size.height / 2);
          cairo_surface_destroy (surface);
          cairo_destroy (cr);
          color = argb;
        }
    }
  color_cache_[fsp] = color;
  CDEBUG ("fragment=%-5s state=%s match='%s': color=%08x", fragment, Aida::enum_value_to_string (state), match, color.argb());
  return color;
}

static const WidgetState INVALID_STATE = WidgetState (0x80000000);

WidgetState
StyleIface::state_from_name (const String &lower_case_state_name)
{
  switch (fnv1a_consthash64 (lower_case_state_name.c_str()))
    {
    case fnv1a_consthash64 ("normal"):          return WidgetState::NORMAL;
    case fnv1a_consthash64 ("hover"):           return WidgetState::HOVER;
    case fnv1a_consthash64 ("panel"):           return WidgetState::PANEL;
    case fnv1a_consthash64 ("acceleratable"):   return WidgetState::ACCELERATABLE;
    case fnv1a_consthash64 ("default"):         return WidgetState::DEFAULT;
    case fnv1a_consthash64 ("selected"):        return WidgetState::SELECTED;
    case fnv1a_consthash64 ("focused"):         return WidgetState::FOCUSED;
    case fnv1a_consthash64 ("insensitive"):     return WidgetState::INSENSITIVE;
    case fnv1a_consthash64 ("active"):          return WidgetState::ACTIVE;
    case fnv1a_consthash64 ("toggled"):         return WidgetState::TOGGLED;
    case fnv1a_consthash64 ("retained"):        return WidgetState::RETAINED;
    default:                                    return INVALID_STATE;
    }
}

uint64
StyleIface::state_from_list (const String &state_list)
{
  const StringVector sv = string_split_any (state_list, "+:"); // support '+' and CSS-like ':' as state list delimiters
  /* Construct the WidgetState bits from a list of widget state names that can be used as a acroe
   * to determine best matches for state list combinations.
   * The WidgetState bit values are ordered by visual importance, so for competing matches, the
   * one with the greatest visual importance (least loss of information) is picked. E.g. match:
   * A: HOVER+SELECTED (score: 1+16=17) against
   * B: HOVER+INSENSITIVE (score: 1+64*0=1) or
   * C: SELECTED (score: 16=16).
   * Note, B's "INSENSITIVE" flag should be ignored as it's not present in A, so C scores as
   * highest match for A. C doesn't reflect the HOVER state in A (of little importance), but
   * it does show that A is "SELECTED" (mildly important) and does not misleadingly indicate
   * that A is "INSENSITIVE" (highest importance) as B would.
   */
  uint64 bits = 0;
  for (const String &s : sv)
    bits |= uint64 (state_from_name (s));           // the WidgetState flag values represent an importance score
  return bits >= uint64 (INVALID_STATE) ? 0 : bits; // unmatched names cancel the entire score
}

String
StyleIface::pick_fragment (const String &fragment, WidgetState state, const StringVector &fragment_list)
{
  // match an SVG element to state, xml ID syntax: <element id="elementname:active+insensitive"/>
  const uint64 mismatch_threshold = uint64 (WidgetState::ACTIVE);
  const String element = fragment[0] == '#' ? fragment.substr (1) : fragment; // strip initial hash
  const size_t colon = element.size();                  // find element / state delimiter
  String fallback, best_match;
  uint64 best_score = 0, penalty_score = 0;
  for (auto id : fragment_list)
    {
      if (id.size() < element.size() ||
          (id.size() > element.size() && id[colon] != ':') ||
          strncmp (id.data(), element.data(), element.size()) != 0)
        continue;                                       // "elementname" must match
      const bool has_states = id.size() > colon + 1 && id[colon] == ':' &&
                              strcmp (&id[colon], ":normal") != 0;
      if (!has_states)
        fallback = id;                                  // save fallback in case no matches are found
      const uint64 id_state = has_states ? state_from_list (id.substr (colon + 1)) : 0;
      const uint64 id_score = id_state & state;         // score the *matching* WidgetState bits
      const uint64 id_penalty = id_score ^ id_state;    // penalize mismatches
      if (id_penalty >= mismatch_threshold)             // reject misleading states
        continue;
      if (id_score > best_score ||                      // better display of important bits
          (id_score == best_score &&
           id_penalty < penalty_score))                 // or improve by less fudging
        {
          best_match = id;
          best_score = id_score;
          penalty_score = id_penalty;
        }
    }
  return best_match.empty() ? fallback : best_match;
}

Color
StyleIface::resolve_color (const String &color_name, WidgetState state, StyleColor style_color)
{
  // Find StyleColor enum value
  Aida::EnumValue evalue = Aida::enum_info<StyleColor>().find_value (color_name); // foreground, background, dark, light, etc
  if (evalue.ident)
    return state_color (state, StyleColor (evalue.value));
  // Eval #112233 style colors (dark blue)
  if (color_name[0] == '#' && color_name.size() == 7)
    {
      const uint32 rgb = string_to_int (&color_name[1], NULL, 16);
      Color c (0xff000000 | rgb);
      return c;
    }
  // Eval #112233ff style colors (intransparent blue)
  if (color_name[0] == '#' && color_name.size() == 9)
    {
      const uint32 rgba = string_to_int (&color_name[1], NULL, 16);
      const uint32 rgb = (rgba >> 8), alpha = rgba & 0xff, argb = (alpha << 24) | rgb;
      Color c (argb);
      return c;
    }
  // Parse color names
  Color parsed_color = Color::from_name (color_name);
  if (parsed_color) // color != transparent black
    return parsed_color;
  return state_color (state, style_color);
}

Color
StyleIface::insensitive_ink (WidgetState state, Color *glintp)
{
  // construct insensitive ink by mixing foreground and dark_color
  Color ink = state_color (state, StyleColor::FOREGROUND);
  ink.combine (state_color (state, StyleColor::DARK), 0x80);
  if (glintp)
    {
      Color glint = state_color (state, StyleColor::LIGHT);
      *glintp = glint;
    }
  return ink;
}


// == StyleIface ==
static std::map<const String, StyleIfaceP> style_file_cache;

StyleIfaceP
StyleIface::fallback()
{
  const String theme_file = "@res Rapicorn::FallbackTheme";
  StyleIfaceP style = style_file_cache[theme_file];
  if (!style)
    {
      Svg::FileP svgf;
      style = FriendAllocator<StyleImpl>::make_shared (svgf);
      style_file_cache[theme_file] = style;
    }
  return style;
}

StyleIfaceP
StyleIface::load (const String &theme_file)
{
  StyleIfaceP style = style_file_cache[theme_file];
  if (!style)
    {
      Svg::FileP svgf;
      bool do_svg_file_io = false;
      if (!string_startswith (theme_file, "@res"))
        do_svg_file_io = RAPICORN_FLIPPER ("svg-file-io", "Rapicorn::Style: allow loading of SVG files from local file system.");
      Blob blob;
      if (do_svg_file_io)
        blob = Blob::load (theme_file);
      else
        blob = Res (theme_file); // call this even if !startswith("@res") to preserve debug message about missing resource
      if (string_endswith (blob.name(), ".svg"))
        {
          svgf = Svg::File::load (blob);
          SDEBUG ("loading: %s: %s", theme_file, strerror (errno));
        }
      if (!svgf)
        {
          user_warning (UserSource (theme_file), "failed to load theme file \"%s\": %s", theme_file, strerror (ENOENT));
          style = fallback();
        }
      else
        style = FriendAllocator<StyleImpl>::make_shared (svgf);
      style_file_cache[theme_file] = style;
    }
  return style;
}

} // Rapicorn
