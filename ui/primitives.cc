// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "primitives.hh"
#include "utilities.hh"
#include "blitfuncs.hh"

namespace Rapicorn {

// == DRect ==
static inline double
dsqr (double x)
{
  return x * x;
}

double
DRect::dist2 (const Point &p) const
{
  if (p.x <= x)
    {
      if (p.y <= y)
        return p.dist2 (lower_left());
      if (p.y >= upper_y())
        return p.dist2 (upper_left());
      return dsqr (x - p.x);
    }
  if (p.x >= upper_x())
    {
      if (p.y <= y)
        return p.dist2 (lower_right());
      if (p.y >= upper_y())
        return p.dist2 (upper_right());
      return dsqr (p.x - x);
    }
  if (p.y >= upper_y())
    return dsqr (p.y - upper_y());
  if (p.y <= y)
    return dsqr (y - p.y);
  return 0; /* inside */
}

double
DRect::dist (const Point &p) const
{
  if (p.x <= x)
    {
      if (p.y <= y)
        return p.dist (lower_left());
      if (p.y >= upper_y())
        return p.dist (upper_left());
      return x - p.x;
    }
  if (p.x >= upper_x())
    {
      if (p.y <= y)
        return p.dist (lower_right());
      if (p.y >= upper_y())
        return p.dist (upper_right());
      return p.x - upper_x();
    }
  if (p.y >= upper_y())
    return p.y - upper_y();
  if (p.y <= y)
    return y - p.y;
  return 0; /* inside */
}

static inline Point
tangent_intersection (Point l1, Point l2, Point c)
{
  // crossing point of tangent through c for line (l1, l2) is: p = l1 + t (l2 - l1)
  // solving the dot product of the line and tangent: (c - p) dot (l2 - l1) = 0
  // gives: t = ( (cx-l1x)*(l2x-l1x) + (cy-l1y)*(l2y-l1y) ) / dist (l1, l2) ^ 2
  const double td = l1.dist2 (l2);
  if (td == 0.0)
    return c;
  const double tx = (c.x - l1.x) * (l2.x - l1.x);
  const double ty = (c.y - l1.y) * (l2.y - l1.y);
  const double t = (tx + ty) / td;
  const double px = l1.x + t * (l2.x - l1.x);
  const double py = l1.y + t * (l2.y - l1.y);
  return Point (px, py);
}

Point
DRect::ul_tangent () const
{
  return tangent_intersection (ll(), ur(), ul());
}

Point
DRect::ur_tangent () const
{
  return tangent_intersection (ul(), lr(), ur());
}

Point
DRect::lr_tangent () const
{
  return tangent_intersection (ll(), ur(), lr());
}

Point
DRect::ll_tangent () const
{
  return tangent_intersection (ul(), lr(), ll());
}

// Return rectangle intersection with or mapped onto @a bounds for non-intersections.
DRect
DRect::mapped_onto (const DRect &bounds) const
{
  const double x1 = clamp (x, bounds.x, bounds.x + bounds.width);
  const double x2 = clamp (x + width, bounds.x, bounds.x + bounds.width);
  const double y1 = clamp (y, bounds.y, bounds.y + bounds.height);
  const double y2 = clamp (y + height, bounds.y, bounds.y + bounds.height);
  return DRect (Point (x1, y1), Point (x2, y2));
}

String
DRect::string() const
{
  return string_format ("((%.17g, %.17g), %.17g, %.17g)", x, y, width, height);
}

Point
DRect::anchor_point (Anchor anchor)
{
  switch (anchor)
    {
    default:
    case Anchor::CENTER:        return center();        break;
    case Anchor::NORTH:         return north();         break;
    case Anchor::NORTH_EAST:    return north_east();    break;
    case Anchor::EAST:          return east();          break;
    case Anchor::SOUTH_EAST:    return south_east();    break;
    case Anchor::SOUTH:         return south();         break;
    case Anchor::SOUTH_WEST:    return south_west();    break;
    case Anchor::WEST:          return west();          break;
    case Anchor::NORTH_WEST:    return north_west();    break;
    }
}

DRect
DRect::create_anchored (Anchor anchor, double width, double height)
{
  DRect b (Point (0, 0), abs (width), abs (height)); /* SOUTH_WEST */
  Point delta = b.anchor_point (anchor);
  b.translate (-delta.x, -delta.y);
  return b;
}

// == IRect ==
String
IRect::string() const
{
  return string_format ("((%d, %d), %d, %d)", x, y, width, height);
}

double
IRect::dist2 (const Point &p) const
{
  return DRect (*this).dist2 (p);
}

double
IRect::dist (const Point &p) const
{
  return DRect (*this).dist (p);
}

// Return rectangle intersection with or mapped onto @a bounds for non-intersections.
IRect
IRect::mapped_onto (const IRect &bounds) const
{
  const double x1 = clamp (x, bounds.x, bounds.x + bounds.width);
  const double x2 = clamp (x + width, bounds.x, bounds.x + bounds.width);
  const double y1 = clamp (y, bounds.y, bounds.y + bounds.height);
  const double y2 = clamp (y + height, bounds.y, bounds.y + bounds.height);
  return IRect (x1, y1, x2 - x1, y2 - y1);
}

// == Color ==
String
Color::string() const
{
  return string_format ("{.rgba=0x%08x}", argb());
}

void
Color::get_hsv (double *huep,           /* 0..360: 0=red, 120=green, 240=blue */
                double *saturationp,    /* 0..1 */
                double *valuep)         /* 0..1 */
{
  double r = red(), g = green(), b = blue();
  double value = MAX (MAX (r, g), b);
  double delta = value - MIN (MIN (r, g), b);
  double saturation = value == 0 ? 0 : delta / value;
  double hue = 0;
  if (saturation && huep)
    {
      if (r == value)
        {
          hue = 0 + 60 * (g - b) / delta;
          if (hue <= 0)
            hue += 360;
        }
      else if (g == value)
        hue = 120 + 60 * (b - r) / delta;
      else /* b == value */
        hue = 240 + 60 * (r - g) / delta;
    }
  if (huep)
    *huep = hue;
  if (saturationp)
    *saturationp = saturation;
  if (valuep)
    *valuep = value / 255;
}

void
Color::set_hsv (double hue,             /* 0..360: 0=red, 120=green, 240=blue */
                double saturation,      /* 0..1 */
                double value)           /* 0..1 */
{
  uint center = ifloor (hue / 60);
  double frac = hue / 60 - center;
  double v1s = value * (1 - saturation);
  double vsf = value * (1 - saturation * frac);
  double v1f = value * (1 - saturation * (1 - frac));
  switch (center)
    {
    case 6:
    case 0: /* red */
      red (iround (value * 0xff));
      green (iround (v1f * 0xff));
      blue (iround (v1s * 0xff));
      break;
    case 1: /* red + green */
      red (iround (vsf * 0xff));
      green (iround (value * 0xff));
      blue (iround (v1s * 0xff));
      break;
    case 2: /* green */
      red (iround (v1s * 0xff));
      green (iround (value * 0xff));
      blue (iround (v1f * 0xff));
      break;
    case 3: /* green + blue */
      red (iround (v1s * 0xff));
      green (iround (vsf * 0xff));
      blue (iround (value * 0xff));
      break;
    case 4: /* blue */
      red (iround (v1f * 0xff));
      green (iround (v1s * 0xff));
      blue (iround (value * 0xff));
      break;
    case 5: /* blue + red */
      red (iround (value * 0xff));
      green (iround (v1s * 0xff));
      blue (iround (vsf * 0xff));
      break;
    }
}

void
Color::set_hsv_value (double v)
{
  double frac = v / get_hsv_value();
  double r = red() * frac, g = green() * frac, b = blue() * frac;
  set (iround (r), iround (g), iround (b));
}

void
Color::tint (double hsv_shift, double saturation_factor, double value_factor)
{
  double hue, saturation, value;
  get_hsv (&hue, &saturation, &value);
  saturation *= saturation_factor;
  value *= value_factor;
  hue += hsv_shift;
  if (hue > 360)
    hue -= 360;
  if (hue < 0)
    hue += 360;
  set_hsv (hue, MIN (1, saturation), MIN (1, value));
}

Color
Color::from_name (const String &color_name)
{
  String name = color_name;
  for (uint i = 0; i < name.size(); i++)
    name[i] = tolower (name[i]);
  /* HTML 4.0 color names */
  if (name == "black")			return 0xff000000;
  if (name == "green")			return 0xff008000;
  if (name == "silver")			return 0xffc0c0c0;
  if (name == "lime")	        	return 0xff00ff00;
  if (name == "gray")	        	return 0xff808080;
  if (name == "olive")			return 0xff808000;
  if (name == "white")			return 0xffffffff;
  if (name == "yellow")			return 0xffffff00;
  if (name == "maroon")			return 0xff800000;
  if (name == "navy")	        	return 0xff000080;
  if (name == "red")	        	return 0xffff0000;
  if (name == "blue")	        	return 0xff0000ff;
  if (name == "purple")			return 0xff800080;
  if (name == "teal")			return 0xff008080;
  if (name == "fuchsia")		return 0xffff00ff;
  if (name == "aqua")			return 0xff00ffff;
  /* named colors from http://www.lightlink.com/xine/bells/namedcolors.html */
  if (name == "white")			return 0xffffffff;
  if (name == "red")	        	return 0xffff0000;
  if (name == "green")			return 0xff00ff00;
  if (name == "blue")			return 0xff0000ff;
  if (name == "magenta")		return 0xffff00ff;
  if (name == "cyan")			return 0xff00ffff;
  if (name == "yellow")			return 0xffffff00;
  if (name == "black")			return 0xff000000;
  if (name == "aquamarine")		return 0xff70db93;
  if (name == "baker's chocolate")	return 0xff5c3317;
  if (name == "blue violet")		return 0xff9f5f9f;
  if (name == "brass")			return 0xffb5a642;
  if (name == "bright gold")		return 0xffd9d919;
  if (name == "brown")			return 0xffa62a2a;
  if (name == "bronze")			return 0xff8c7853;
  if (name == "bronze ii")		return 0xffa67d3d;
  if (name == "cadet blue")		return 0xff5f9f9f;
  if (name == "cool copper")		return 0xffd98719;
  if (name == "copper")			return 0xffb87333;
  if (name == "coral")			return 0xffff7f00;
  if (name == "corn flower blue")	return 0xff42426f;
  if (name == "dark brown")		return 0xff5c4033;
  if (name == "dark green")		return 0xff2f4f2f;
  if (name == "dark green copper")	return 0xff4a766e;
  if (name == "dark olive green")	return 0xff4f4f2f;
  if (name == "dark orchid")		return 0xff9932cd;
  if (name == "dark purple")		return 0xff871f78;
  if (name == "dark slate blue")	return 0xff6b238e;
  if (name == "dark slate grey")	return 0xff2f4f4f;
  if (name == "dark tan")		return 0xff97694f;
  if (name == "dark turquoise")		return 0xff7093db;
  if (name == "dark wood")		return 0xff855e42;
  if (name == "dim grey")		return 0xff545454;
  if (name == "dusty rose")		return 0xff856363;
  if (name == "feldspar")		return 0xffd19275;
  if (name == "firebrick")		return 0xff8e2323;
  if (name == "forest green")		return 0xff238e23;
  if (name == "gold")			return 0xffcd7f32;
  if (name == "goldenrod")		return 0xffdbdb70;
  if (name == "grey")			return 0xffc0c0c0;
  if (name == "green copper")		return 0xff527f76;
  if (name == "green yellow")		return 0xff93db70;
  if (name == "hunter green")		return 0xff215e21;
  if (name == "indian red")		return 0xff4e2f2f;
  if (name == "khaki")			return 0xff9f9f5f;
  if (name == "light blue")		return 0xffc0d9d9;
  if (name == "light grey")		return 0xffa8a8a8;
  if (name == "light steel blue")	return 0xff8f8fbd;
  if (name == "light wood")		return 0xffe9c2a6;
  if (name == "lime green")		return 0xff32cd32;
  if (name == "mandarian orange")	return 0xffe47833;
  if (name == "maroon")			return 0xff8e236b;
  if (name == "medium aquamarine")	return 0xff32cd99;
  if (name == "medium blue")		return 0xff3232cd;
  if (name == "medium forest green")	return 0xff6b8e23;
  if (name == "medium goldenrod")	return 0xffeaeaae;
  if (name == "medium orchid")		return 0xff9370db;
  if (name == "medium sea green")	return 0xff426f42;
  if (name == "medium slate blue")	return 0xff7f00ff;
  if (name == "medium spring green")	return 0xff7fff00;
  if (name == "medium turquoise")	return 0xff70dbdb;
  if (name == "medium violet red")	return 0xffdb7093;
  if (name == "medium wood")		return 0xffa68064;
  if (name == "midnight blue")		return 0xff2f2f4f;
  if (name == "navy blue")		return 0xff23238e;
  if (name == "neon blue")		return 0xff4d4dff;
  if (name == "neon pink")		return 0xffff6ec7;
  if (name == "new midnight blue")	return 0xff00009c;
  if (name == "new tan")		return 0xffebc79e;
  if (name == "old gold")		return 0xffcfb53b;
  if (name == "orange")			return 0xffff7f00;
  if (name == "orange red")		return 0xffff2400;
  if (name == "orchid")			return 0xffdb70db;
  if (name == "pale green")		return 0xff8fbc8f;
  if (name == "pink")			return 0xffbc8f8f;
  if (name == "plum")			return 0xffeaadea;
  if (name == "quartz")			return 0xffd9d9f3;
  if (name == "rich blue")		return 0xff5959ab;
  if (name == "salmon")			return 0xff6f4242;
  if (name == "scarlet")		return 0xff8c1717;
  if (name == "sea green")		return 0xff238e68;
  if (name == "semi-sweet chocolate")	return 0xff6b4226;
  if (name == "sienna")			return 0xff8e6b23;
  if (name == "silver")			return 0xffe6e8fa;
  if (name == "sky blue")		return 0xff3299cc;
  if (name == "slate blue")		return 0xff007fff;
  if (name == "spicy pink")		return 0xffff1cae;
  if (name == "spring green")		return 0xff00ff7f;
  if (name == "steel blue")		return 0xff236b8e;
  if (name == "summer sky")		return 0xff38b0de;
  if (name == "tan")			return 0xffdb9370;
  if (name == "thistle")		return 0xffd8bfd8;
  if (name == "turquoise")		return 0xffadeaea;
  if (name == "very dark brown")	return 0xff5c4033;
  if (name == "very light grey")	return 0xffcdcdcd;
  if (name == "violet")			return 0xff4f2f4f;
  if (name == "violet red")		return 0xffcc3299;
  if (name == "wheat")			return 0xffd8d8bf;
  if (name == "yellow green")		return 0xff99cc32;
  /* De-facto NS & MSIE recognized HTML color names */
  if (name == "aliceblue")		return 0xfff0f8ff;
  if (name == "antiquewhite")		return 0xfffaebd7;
  if (name == "aqua")			return 0xff00ffff;
  if (name == "aquamarine")		return 0xff7fffd4;
  if (name == "azure")			return 0xfff0ffff;
  if (name == "beige")			return 0xfff5f5dc;
  if (name == "bisque")			return 0xffffe4c4;
  if (name == "black")			return 0xff000000;
  if (name == "blanchedalmond")		return 0xffffebcd;
  if (name == "blue")			return 0xff0000ff;
  if (name == "blueviolet")		return 0xff8a2be2;
  if (name == "brown")			return 0xffa52a2a;
  if (name == "burlywood")		return 0xffdeb887;
  if (name == "cadetblue")		return 0xff5f9ea0;
  if (name == "chartreuse")		return 0xff7fff00;
  if (name == "chocolate")		return 0xffd2691e;
  if (name == "coral")			return 0xffff7f50;
  if (name == "cornflowerblue")		return 0xff6495ed;
  if (name == "cornsilk")		return 0xfffff8dc;
  if (name == "crimson")		return 0xffdc143c;
  if (name == "cyan")			return 0xff00ffff;
  if (name == "darkblue")		return 0xff00008b;
  if (name == "darkcyan")		return 0xff008b8b;
  if (name == "darkgoldenrod")		return 0xffb8860b;
  if (name == "darkgray")		return 0xffa9a9a9;
  if (name == "darkgreen")		return 0xff006400;
  if (name == "darkkhaki")		return 0xffbdb76b;
  if (name == "darkmagenta")		return 0xff8b008b;
  if (name == "darkolivegreen")		return 0xff556b2f;
  if (name == "darkorange")		return 0xffff8c00;
  if (name == "darkorchid")		return 0xff9932cc;
  if (name == "darkred")		return 0xff8b0000;
  if (name == "darksalmon")		return 0xffe9967a;
  if (name == "darkseagreen")		return 0xff8fbc8f;
  if (name == "darkslateblue")		return 0xff483d8b;
  if (name == "darkslategray")		return 0xff2f4f4f;
  if (name == "darkturquoise")		return 0xff00ced1;
  if (name == "darkviolet")		return 0xff9400d3;
  if (name == "deeppink")		return 0xffff1493;
  if (name == "deepskyblue")		return 0xff00bfff;
  if (name == "dimgray")		return 0xff696969;
  if (name == "dodgerblue")		return 0xff1e90ff;
  if (name == "firebrick")		return 0xffb22222;
  if (name == "floralwhite")		return 0xfffffaf0;
  if (name == "forestgreen")		return 0xff228b22;
  if (name == "fuchsia")		return 0xffff00ff;
  if (name == "gainsboro")		return 0xffdcdcdc;
  if (name == "ghostwhite")		return 0xfff8f8ff;
  if (name == "gold")			return 0xffffd700;
  if (name == "goldenrod")		return 0xffdaa520;
  if (name == "gray")			return 0xff808080;
  if (name == "green")			return 0xff008000;
  if (name == "greenyellow")		return 0xffadff2f;
  if (name == "honeydew")		return 0xfff0fff0;
  if (name == "hotpink")		return 0xffff69b4;
  if (name == "indianred")		return 0xffcd5c5c;
  if (name == "indigo")			return 0xff4b0082;
  if (name == "ivory")			return 0xfffffff0;
  if (name == "khaki")			return 0xfff0e68c;
  if (name == "lavender")		return 0xffe6e6fa;
  if (name == "lavenderblush")		return 0xfffff0f5;
  if (name == "lawngreen")		return 0xff7cfc00;
  if (name == "lemonchiffon")		return 0xfffffacd;
  if (name == "lightblue")		return 0xffadd8e6;
  if (name == "lightcoral")		return 0xfff08080;
  if (name == "lightcyan")		return 0xffe0ffff;
  if (name == "lightgoldenrodyellow")	return 0xfffafad2;
  if (name == "lightgreen")		return 0xff90ee90;
  if (name == "lightgrey")		return 0xffd3d3d3;
  if (name == "lightpink")		return 0xffffb6c1;
  if (name == "lightsalmon")		return 0xffffa07a;
  if (name == "lightseagreen")		return 0xff20b2aa;
  if (name == "lightskyblue")		return 0xff87cefa;
  if (name == "lightslategray")		return 0xff778899;
  if (name == "lightsteelblue")		return 0xffb0c4de;
  if (name == "lightyellow")		return 0xffffffe0;
  if (name == "lime")			return 0xff00ff00;
  if (name == "limegreen")		return 0xff32cd32;
  if (name == "linen")			return 0xfffaf0e6;
  if (name == "magenta")		return 0xffff00ff;
  if (name == "maroon")			return 0xff800000;
  if (name == "mediumaquamarine")	return 0xff66cdaa;
  if (name == "mediumblue")		return 0xff0000cd;
  if (name == "mediumorchid")		return 0xffba55d3;
  if (name == "mediumpurple")		return 0xff9370db;
  if (name == "mediumseagreen")		return 0xff3cb371;
  if (name == "mediumslateblue")	return 0xff7b68ee;
  if (name == "mediumspringgreen")	return 0xff00fa9a;
  if (name == "mediumturquoise")	return 0xff48d1cc;
  if (name == "mediumvioletred")	return 0xffc71585;
  if (name == "midnightblue")		return 0xff191970;
  if (name == "mintcream")		return 0xfff5fffa;
  if (name == "mistyrose")		return 0xffffe4e1;
  if (name == "moccasin")		return 0xffffe4b5;
  if (name == "navajowhite")		return 0xffffdead;
  if (name == "navy")			return 0xff000080;
  if (name == "oldlace")		return 0xfffdf5e6;
  if (name == "olive")			return 0xff808000;
  if (name == "olivedrab")		return 0xff6b8e23;
  if (name == "orange")			return 0xffffa500;
  if (name == "orangered")		return 0xffff4500;
  if (name == "orchid")			return 0xffda70d6;
  if (name == "palegoldenrod")		return 0xffeee8aa;
  if (name == "palegreen")		return 0xff98fb98;
  if (name == "paleturquoise")		return 0xffafeeee;
  if (name == "palevioletred")		return 0xffdb7093;
  if (name == "papayawhip")		return 0xffffefd5;
  if (name == "peachpuff")		return 0xffffdab9;
  if (name == "peru")			return 0xffcd853f;
  if (name == "pink")			return 0xffffc0cb;
  if (name == "plum")			return 0xffdda0dd;
  if (name == "powderblue")		return 0xffb0e0e6;
  if (name == "purple")			return 0xff800080;
  if (name == "red")			return 0xffff0000;
  if (name == "rosybrown")		return 0xffbc8f8f;
  if (name == "royalblue")		return 0xff4169e1;
  if (name == "saddlebrown")		return 0xff8b4513;
  if (name == "salmon")			return 0xfffa8072;
  if (name == "sandybrown")		return 0xfff4a460;
  if (name == "seagreen")		return 0xff2e8b57;
  if (name == "seashell")		return 0xfffff5ee;
  if (name == "sienna")			return 0xffa0522d;
  if (name == "silver")			return 0xffc0c0c0;
  if (name == "skyblue")		return 0xff87ceeb;
  if (name == "slateblue")		return 0xff6a5acd;
  if (name == "slategray")		return 0xff708090;
  if (name == "snow")			return 0xfffffafa;
  if (name == "springgreen")		return 0xff00ff7f;
  if (name == "steelblue")		return 0xff4682b4;
  if (name == "tan")			return 0xffd2b48c;
  if (name == "teal")			return 0xff008080;
  if (name == "thistle")		return 0xffd8bfd8;
  if (name == "tomato")			return 0xffff6347;
  if (name == "turquoise")		return 0xff40e0d0;
  if (name == "violet")			return 0xffee82ee;
  if (name == "wheat")			return 0xfff5deb3;
  if (name == "white")			return 0xffffffff;
  if (name == "whitesmoke")		return 0xfff5f5f5;
  if (name == "yellow")			return 0xffffff00;
  if (name == "yellowgreen")		return 0xff9acd32;
  return 0x00000000;
}

// == Affine ==
String
Affine::string() const
{
  return string_format ("{ { %.17g, %.17g, %.17g }, { %.17g, %.17g, %.17g } }", xx, xy, xz, yx, yy, yz);
}

Affine
Affine::from_triangles (Point src_a, Point src_b, Point src_c,
                        Point dst_a, Point dst_b, Point dst_c)
{
  const double Ax = src_a.x, Ay = src_a.y;
  const double Bx = src_b.x, By = src_b.y;
  const double Cx = src_c.x, Cy = src_c.y;
  const double ax = dst_a.x, ay = dst_a.y;
  const double bx = dst_b.x, by = dst_b.y;
  const double cx = dst_c.x, cy = dst_c.y;

  /* solve the linear equation system:
   *   ax = Ax * matrix.xx + Ay * matrix.xy + matrix.xz;
   *   ay = Ax * matrix.yx + Ay * matrix.yy + matrix.yz;
   *   bx = Bx * matrix.xx + By * matrix.xy + matrix.xz;
   *   by = Bx * matrix.yx + By * matrix.yy + matrix.yz;
   *   cx = Cx * matrix.xx + Cy * matrix.xy + matrix.xz;
   *   cy = Cx * matrix.yx + Cy * matrix.yy + matrix.yz;
   * for matrix.*
   */
  double AxBy = Ax * By, AyBx = Ay * Bx;
  double AxCy = Ax * Cy, AyCx = Ay * Cx;
  double BxCy = Bx * Cy, ByCx = By * Cx;
  double divisor = AxBy - AyBx - AxCy + AyCx + BxCy - ByCx;

  if (fabs (divisor) < 1e-9)
    return false;

  Affine matrix;
  matrix.xx = By * ax - Ay * bx + Ay * cx - Cy * ax - By * cx + Cy * bx;
  matrix.yx = By * ay - Ay * by + Ay * cy - Cy * ay - By * cy + Cy * by;
  matrix.xy = Ax * bx - Bx * ax - Ax * cx + Cx * ax + Bx * cx - Cx * bx;
  matrix.yy = Ax * by - Bx * ay - Ax * cy + Cx * ay + Bx * cy - Cx * by;
  matrix.xz = AxBy * cx - AxCy * bx - AyBx * cx + AyCx * bx + BxCy * ax - ByCx * ax;
  matrix.yz = AxBy * cy - AxCy * by - AyBx * cy + AyCx * by + BxCy * ay - ByCx * ay;
  double rec_divisor = 1.0 / divisor;
  matrix.xx *= rec_divisor;
  matrix.xy *= rec_divisor;
  matrix.xz *= rec_divisor;
  matrix.yx *= rec_divisor;
  matrix.yy *= rec_divisor;
  matrix.yz *= rec_divisor;

  return true;
}

} // Rapicorn
