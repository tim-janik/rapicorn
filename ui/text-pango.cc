// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "text-pango.hh"
#if     RAPICORN_WITH_PANGO
#include <pango/pangoft2.h>
#include <pango/pangocairo.h>
#include "factory.hh"
#include "painter.hh"

#define RDEBUG(...)     RAPICORN_KEY_DEBUG ("Label-Rendering", __VA_ARGS__)

#include <algorithm>

#if PANGO_SCALE != 1024
#error code needs adaption to unknown PANGO_SCALE value
#endif

// undefine integer based pango macros to avoid accidental use
#undef  PANGO_SCALE
#undef  PANGO_PIXELS
#undef  PANGO_PIXELS_FLOOR
#undef  PANGO_PIXELS_CEIL
// provide pango unit <-> pixel conversion functions
#define UNITS2PIXELS(pu)        ((pu) / 1024.0)
#define PIXELS2UNITS(pp)        ((pp) * 1024.0)

#define MONOSPACE_NAME  (String ("Monospace"))

namespace Rapicorn {

// provide threading guard
static Mutex rapicorn_pango_mutex;

/* --- Pango support code --- */
static const char*
default_text_language()
{
  static const char *language = NULL;
  if (!language)
    {
      String lc_ctype = setlocale (LC_CTYPE, NULL);
      String::size_type sep1 = lc_ctype.find ('.');
      String::size_type sep2 = lc_ctype.find ('@');
      String::size_type sep = MIN (sep1, sep2);
      if (sep != lc_ctype.npos)
        lc_ctype[sep] = 0;
      language = strdup (lc_ctype.c_str());
    }
  return language;
}

static PangoDirection
default_pango_direction()
{
  /* TRANSLATORS: Leave as language-direction:LTR for standard left-to-right (text)
   * layout direction. Translate to language-direction:RTL for right-to-left layout.
   */
  const char *ltr_dir = "language-direction:LTR", *rtl_dir = "language-direction:RTL", *default_dir = _("language-direction:LTR");
  PangoDirection pdirection = PANGO_DIRECTION_LTR;
  if (strcmp (default_dir, rtl_dir) == 0)
    pdirection = PANGO_DIRECTION_RTL;
  else if (strcmp (default_dir, ltr_dir) == 0)
    pdirection = PANGO_DIRECTION_LTR;
  else
    critical ("invalid translation of \"%s\": %s", ltr_dir, default_dir);
  return pdirection;
}

static Point
default_pango_dpi ()
{
  return Point (96, 96);
}

static const PangoFontDescription*
default_pango_font_description()
{
  static const char *default_font_name = "Sans 10";
  static PangoFontDescription *font_desc = NULL;
  if (!font_desc)
    {
      font_desc = pango_font_description_from_string (default_font_name);
      if (!pango_font_description_get_family (font_desc))
        pango_font_description_set_family (font_desc, "Sans");
      if (pango_font_description_get_size (font_desc) <= 0)
        pango_font_description_set_size (font_desc, iround (PIXELS2UNITS (10)));
    }
  return font_desc;
}

static void
default_pango_cairo_font_options (PangoContext *pcontext,
                                  cairo_t      *cairo)
{
  cairo_font_options_t *fopt = cairo_font_options_create();
  assert_return (fopt != NULL);
  cairo_font_options_set_hint_metrics (fopt, CAIRO_HINT_METRICS_ON); // ON, OFF
  cairo_font_options_set_hint_style (fopt, CAIRO_HINT_STYLE_FULL); // NONE, SLIGHT, MEDIUM, FULL
  cairo_font_options_set_antialias (fopt, CAIRO_ANTIALIAS_GRAY); // NONE, GRAY, SUBPIXEL
  cairo_font_options_set_subpixel_order (fopt, CAIRO_SUBPIXEL_ORDER_DEFAULT); // RGB, BGR, VRGB, VBGR
  assert_return (CAIRO_STATUS_SUCCESS == cairo_font_options_status (fopt));
  if (cairo)
    cairo_set_font_options (cairo, fopt);
  if (pcontext)
    {
      pango_cairo_context_set_font_options (pcontext, fopt);
      pango_cairo_context_set_resolution (pcontext, 106);
    }
  cairo_font_options_destroy (fopt);
}

static PangoAlignment
pango_alignment_from_align_type (Align at)
{
  switch (at)
    {
    default:
    case Align::LEFT:    return PANGO_ALIGN_LEFT;
    case Align::CENTER:  return PANGO_ALIGN_CENTER;
    case Align::RIGHT:   return PANGO_ALIGN_RIGHT;
    }
}

static Align
align_type_from_pango_alignment (PangoAlignment pa)
{
  switch (pa)
    {
    default:
    case PANGO_ALIGN_LEFT:    return Align::LEFT;
    case PANGO_ALIGN_CENTER:  return Align::CENTER;
    case PANGO_ALIGN_RIGHT:   return Align::RIGHT;
    }
}

static PangoEllipsizeMode
pango_ellipsize_mode_from_ellipsize_type (Ellipsize et)
{
  switch (et)
    {
      // case Ellipsize::NONE: return PANGO_ELLIPSIZE_NONE;
    case Ellipsize::START:     return PANGO_ELLIPSIZE_START;
    case Ellipsize::MIDDLE:    return PANGO_ELLIPSIZE_MIDDLE;
    default:
    case Ellipsize::END:       return PANGO_ELLIPSIZE_END;
    }
}

static Ellipsize
ellipsize_type_from_pango_ellipsize_mode (PangoEllipsizeMode em)
{
  switch (em)
    {
      // case PANGO_ELLIPSIZE_NONE:   return Ellipsize::NONE;
    case PANGO_ELLIPSIZE_START:       return Ellipsize::START;
    case PANGO_ELLIPSIZE_MIDDLE:      return Ellipsize::MIDDLE;
    default:
    case PANGO_ELLIPSIZE_END:         return Ellipsize::END;
    }
}

/* --- LayoutCache --- */
#define RETURN_LESS_IF_UNEQUAL(lhs,rhs) { if (lhs < rhs) return 1; else if (rhs < lhs) return 0; }
class LayoutCache {
  static uint const       cache_threshold   = 100; /* threshold beyond which the cache needs trimming */
  static double constexpr cache_probability = 0.5; /* probability for keeping an element alive */
  struct ContextKey {
    PangoDirection direction;
    String         language;
    Point          dpi;
    inline bool
    operator< (const ContextKey &rhs) const
    {
      const ContextKey &lhs = *this;
      RETURN_LESS_IF_UNEQUAL (lhs.direction, rhs.direction);
      RETURN_LESS_IF_UNEQUAL (lhs.language,  rhs.language);
      RETURN_LESS_IF_UNEQUAL (lhs.dpi.x,     rhs.dpi.x);
      RETURN_LESS_IF_UNEQUAL (lhs.dpi.y,     rhs.dpi.y);
      return 0;
    }
  };
  std::map<ContextKey, PangoContext*> context_cache;
  PangoContext*
  retrieve_context (ContextKey key)
  {
    PangoContext *pcontext = context_cache[key];
    if (!pcontext)
      {
        PangoFontMap *fontmap = pango_cairo_font_map_new ();
        pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (fontmap), key.dpi.x);
        pcontext = pango_font_map_create_context (fontmap);
        default_pango_cairo_font_options (pcontext, NULL);
        pango_context_set_base_dir (pcontext, key.direction);
        pango_context_set_language (pcontext, pango_language_from_string (key.language.c_str()));
        pango_context_set_font_description (pcontext, default_pango_font_description());
        /* trim cache before insertion */
        if (context_cache.size() >= cache_threshold)
          {
            std::map<ContextKey, PangoContext*>::iterator it = context_cache.begin();
            while (it != context_cache.end())
              {
                std::map<ContextKey, PangoContext*>::iterator last = it++;
                if (drand48() > cache_probability)
                  {
                    if (last->second)   // maybe NULL due to failing lookups
                      g_object_unref (last->second);
                    context_cache.erase (last);
                  }
              }
          }
        context_cache[key] = pcontext; // takes over initial ref()
      }
    return pcontext;
  }
public:
  PangoLayout*
  create_layout (String         font_description,
                 Align          align,
                 PangoWrapMode  pangowrap,
                 Ellipsize      ellipsize,
                 int            indent,
                 int            spacing,
                 bool           single_paragraph)
  {
    ContextKey key;
    key.direction = default_pango_direction();
    key.language = default_text_language();
    key.dpi = default_pango_dpi();
    PangoContext *pcontext = retrieve_context (key);
    PangoFontDescription *pfdesc;
    if (font_description.size())
      pfdesc = pango_font_description_from_string (font_description.c_str());
    else
      pfdesc = pango_font_description_copy_static (default_pango_font_description());
    gchar *gstr = pango_font_description_to_string (pfdesc);
    font_description = gstr;
    g_free (gstr);
    PangoLayout *playout = pango_layout_new (pcontext);
    pango_layout_set_alignment (playout, pango_alignment_from_align_type (align));
    pango_layout_set_wrap (playout, pangowrap);
    pango_layout_set_ellipsize (playout, pango_ellipsize_mode_from_ellipsize_type (ellipsize));
    pango_layout_set_indent (playout, iround (PIXELS2UNITS (indent)));
    pango_layout_set_spacing (playout, iround (PIXELS2UNITS (spacing)));
    pango_layout_set_attributes (playout, NULL);
    pango_layout_set_width (playout, -1);
    pango_layout_set_single_paragraph_mode (playout, single_paragraph);
    pango_layout_set_tabs (playout, NULL);
    pango_layout_set_text (playout, "", 0);
    pango_font_description_merge_static (pfdesc, default_pango_font_description(), FALSE);
    pango_layout_set_font_description (playout, pfdesc);
    pango_font_description_free (pfdesc);
    return playout;
  }
  static const PangoFontDescription*
  font_description_from_layout (PangoLayout *playout)
  {
    const PangoFontDescription *cfdesc = pango_layout_get_font_description (playout);
    if (!cfdesc)
      cfdesc = pango_context_get_font_description (pango_layout_get_context (playout));
    return cfdesc;
  }
  static String
  font_string_from_layout (PangoLayout *playout)
  {
    const PangoFontDescription *cfdesc = font_description_from_layout (playout);
    gchar *gstr = pango_font_description_to_string (cfdesc);
    String font_desc = gstr;
    g_free (gstr);
    return font_desc;
  }
  static int
  dot_size_from_layout (PangoLayout *playout)
  {
    const PangoFontDescription *cfdesc = pango_layout_get_font_description (playout);
    if (!cfdesc)
      cfdesc = pango_context_get_font_description (pango_layout_get_context (playout));
    // FIXME: hardcoded dpi, assumes relative size
    double dot_size = (pango_font_description_get_size (cfdesc) * default_pango_dpi().y) / PIXELS2UNITS (72 * 6);
    return iround (max (1, dot_size));
  }
};
static LayoutCache global_layout_cache; // protected by rapicorn_pango_mutex.lock / rapicorn_pango_mutex.unlock

/* --- LazyColorAttr --- */
class LazyColorAttr {
  /* We need to implement our own color attribute here, because color names can
   * only be resolved for anchored widgets which happens after parsing and span
   * attribute assignments. And we need to be able to read the original color
   * string back from span attributes.
   */
  PangoAttrColor pcolor;
  String         cname;
  ColorType      ctype;
protected:
  static PangoAttribute*
  copy (const PangoAttribute *attr)
  {
    const LazyColorAttr *self = (const LazyColorAttr*) attr;
    const PangoAttrColor *cattr = &self->pcolor;
    return create (attr->klass, cattr->color.red, cattr->color.green, cattr->color.blue, self->cname, self->ctype);
  }
  static void
  destroy (PangoAttribute *attr)
  {
    LazyColorAttr *self = (LazyColorAttr*) attr;
    delete self;
  }
  static gboolean
  equal (const PangoAttribute *attr1,
         const PangoAttribute *attr2)
  {
    const LazyColorAttr *self1 = (const LazyColorAttr*) attr1, *self2 = (const LazyColorAttr*) attr2;
    return self1->ctype == self2->ctype && self1->cname == self2->cname;
  }
  static PangoAttribute*
  create (const PangoAttrClass *klass,
          guint16               red,
          guint16               green,
          guint16               blue,
          const String         &cname,
          ColorType             ctype)
  {
    LazyColorAttr *self = new LazyColorAttr;
    PangoAttrColor *cattr = (PangoAttrColor*) self;
    PangoAttribute *attr = (PangoAttribute*) self;
    attr->klass = klass;
    cattr->color.red = red;
    cattr->color.green = green;
    cattr->color.blue = blue;
    self->cname = cname;
    self->ctype = ctype;
    return (PangoAttribute*) self;
  }
public:
  static inline const PangoAttrClass*
  foreground_klass()
  {
    static const PangoAttrClass klass = { PANGO_ATTR_FOREGROUND, LazyColorAttr::copy, LazyColorAttr::destroy, LazyColorAttr::equal };
    return &klass;
  }
  static inline const PangoAttrClass*
  background_klass()
  {
    static const PangoAttrClass klass = { PANGO_ATTR_BACKGROUND, LazyColorAttr::copy, LazyColorAttr::destroy, LazyColorAttr::equal };
    return &klass;
  }
  static PangoAttribute*
  create_lazy_color (const PangoAttrClass *klass,
                     const String         &cname,
                     ColorType             ctype)
  {
    // 0xfd1bfe marks unresolved lazy colors
    return create (klass, 0xfdfd, 0x1b1b, 0xfefe, cname, ctype);
  }
  static PangoAttribute*
  create_preset_color (const PangoAttrClass *klass,
                       guint16               red,
                       guint16               green,
                       guint16               blue,
                       const String         &cname,
                       ColorType             ctype)
  {
    return create (klass, red, green, blue, cname, ctype);
  }
  static String
  get_name (const PangoAttribute *attr)
  {
    assert (attr->klass == foreground_klass() || attr->klass == background_klass());
    const LazyColorAttr *self = (const LazyColorAttr*) attr;
    return self->cname;
  }
};

/* --- dump Markup --- */
class MarkupDumper {
  String output;
  void
  list_elements (const PangoAttribute *attr,
                 std::list<String>    &elements,
                 bool                  opening_tag = false)
  {
    if (attr->klass->type == PANGO_ATTR_WEIGHT &&
        ((PangoAttrInt*) attr)->value == PANGO_WEIGHT_BOLD)
      elements.push_back ("B");
    else if (attr->klass->type == PANGO_ATTR_STYLE &&
             ((PangoAttrInt*) attr)->value == PANGO_STYLE_ITALIC)
      elements.push_back ("I");
    else if (attr->klass->type == PANGO_ATTR_UNDERLINE &&
             ((PangoAttrInt*) attr)->value == PANGO_UNDERLINE_SINGLE)
      elements.push_back ("U");
    else if (attr->klass->type == PANGO_ATTR_STRIKETHROUGH &&
             ((PangoAttrInt*) attr)->value != false)
      elements.push_back ("S");
    else if (attr->klass->type == PANGO_ATTR_SCALE &&
             ((PangoAttrFloat*) attr)->value)
      {
        int n = iround (log (((PangoAttrFloat*) attr)->value) / log (1.2));
        for (uint i = 0; i < ABS (n); i++)
          elements.push_back (n > 0 ? "Larger" : "Smaller");
      }
    else if (attr->klass->type == PANGO_ATTR_FAMILY &&
             ((PangoAttrString*) attr)->value == MONOSPACE_NAME)
      elements.push_back ("TT");
    else if (attr->klass->type == PANGO_ATTR_FAMILY &&
             ((PangoAttrString*) attr)->value)
      elements.push_back (!opening_tag ? "Font" : "Font family='" + String (((PangoAttrString*) attr)->value) + "'");
    else if (attr->klass->type == PANGO_ATTR_FOREGROUND &&
             attr->klass == LazyColorAttr::foreground_klass())
      elements.push_back (!opening_tag ? "FG" : "FG color='" + LazyColorAttr::get_name (attr) + "'");
    else if (attr->klass->type == PANGO_ATTR_BACKGROUND &&
             attr->klass == LazyColorAttr::background_klass())
      elements.push_back (!opening_tag ? "BG" : "BG color='" + LazyColorAttr::get_name (attr) + "'");
    if (!opening_tag)
      reverse (elements.begin(), elements.end());
  }
  void
  element_start (const PangoAttribute *attr)
  {
    std::list<String> elements;
    list_elements (attr, elements, true);
    for (std::list<String>::iterator it = elements.begin(); it != elements.end(); it++)
      output += "<" + *it + ">";
  }
  void
  element_end (const PangoAttribute *attr)
  {
    std::list<String> elements;
    list_elements (attr, elements, false);
    for (std::list<String>::iterator it = elements.begin(); it != elements.end(); it++)
      output += "</" + *it + ">";
  }
  static gboolean
  pattribute_filter (PangoAttribute *attribute,
                     gpointer        data)
  {
    std::list<PangoAttribute*> *alist = (std::list<PangoAttribute*>*) data;
    alist->push_back (attribute);
    return false; // preserve pango's attribute list
  }
  void
  append_markup (PangoLayout *playout)
  {
    String text = pango_layout_get_text (playout);
    const uint length = text.size();
    output += "<TEXT>";
    std::list<PangoAttribute*> alist;
    PangoAttrList *palist = pango_layout_get_attributes (playout);
    if (palist)
      pango_attr_list_filter (palist, pattribute_filter, &alist);
    std::list<PangoAttribute*>::iterator ait = alist.begin();
    std::list<PangoAttribute*> astack;
    for (uint i = 0; i < length; i++)
      {
        /* close attributes */
        while (!astack.empty() && (*astack.begin())->end_index == i)
          {
            const PangoAttribute *attr = astack.front();
            astack.pop_front();
            element_end (attr);
          }
        /* open attributes */
        while (ait != alist.end() && (*ait)->start_index == i)
          {
            PangoAttribute *attr = *ait++;
            astack.push_front (attr);
            element_start (attr);
          }
        /* dump text */
        output += text[i];
      }
    /* close attributes */
    while (!astack.empty())
      {
        const PangoAttribute *attr = astack.front();
        astack.pop_front();
        element_end (attr);
      }
    output += "</TEXT>";
  }
public:
  static String
  dump_markup (PangoLayout *playout)
  {
    MarkupDumper dumper;
    dumper.append_markup (playout);
    return dumper.output;
  }
};

/* --- XmlToPango --- */
class XmlToPango : Rapicorn::MarkupParser {
  String                     plain_text_;
  std::list<PangoAttribute*> alist_;
  PangoLayout               *layout_;
  int                        scaling_;
  uint                       pre_count_;
  bool                       preserve_last_whitespace_;
  const char          *const whitspaces_;
  String                     error_;
  const XmlNode             *error_node_;
  XmlToPango (PangoLayout   *playout,
              const String  &input_name) :
    MarkupParser (input_name), layout_ (playout), scaling_ (0),
    pre_count_ (0), preserve_last_whitespace_ (false), whitspaces_ (" \t\n\r\v\f"),
    error_ (""), error_node_ (NULL)
  {}
  ~XmlToPango()
  {
    for (std::list<PangoAttribute*>::iterator it = alist_.begin(); it != alist_.end(); it++)
      if (*it)
        pango_attribute_destroy (*it);
  }
  void
  set_error (const String  &estring,
             const XmlNode *enode)
  {
    if (error_.size())
      critical ("%s: skipping second error: %s (%p)", __func__, estring.c_str(), enode);
    else
      {
        error_ = estring;
        error_node_ = enode;
      }
  }
  static char
  lookup_text_attribute (const String &name,
                         String       *lname = NULL)
  {
    static const struct { const char token, *short_name, *long_name; } talist[] = {
      { 'B', "B",       "Bold" },
      { 'I', "I",       "Italic" },
      /*'Q', "Q",       "Oblique" */
      { 'U', "U",       "Underline" },
      { 'S', "S",       "Strike" },
      /*'A', "A",       "Small-Caps" */
      /*'C', "C",       "Condensed" */
      /*'E', "E",       "Expanded" */
      /*'H', "H",       "Heavy" */
      /*'L', "L",       "Light" */
      { '0', "BG",      "Background" },
      { '1', "FG",      "Foreground" },
      { '+', "",        "Larger" },
      { '-', "",        "Smaller" },
      { '=', "TT",      "TeleType" },
      { '#', "",        "Font" },
      { ':', "",        "TEXT" },
      { '$', "",        "BR" },
      { 'P', "",        "pre" },
    };
    for (uint i = 0; i < sizeof (talist) / sizeof (talist[0]); i++)
      if (strcasecmp (name.c_str(), talist[i].short_name) == 0 ||
          strcasecmp (name.c_str(), talist[i].long_name) == 0)
        {
          if (lname)
            *lname = talist[i].long_name;
          return talist[i].token ? talist[i].token : talist[i].short_name[0];
        }
    return 0;
  }
  void
  handle_tag (const XmlNode &xnode)
  {
    char token = lookup_text_attribute (xnode.name());
    // tag preamble
    PangoAttribute *pa = NULL;
    switch (token)
      {
      case ':':                                                                 break;  /* TEXT */
      case '$': plain_text_ += "\n"; preserve_last_whitespace_ = true;        break;  /* BR */
      case 'P': pre_count_++;                                                  break;  /* PRE */
      case 'B': pa = pango_attr_weight_new (PANGO_WEIGHT_BOLD);                 break;
      case 'I': pa = pango_attr_style_new (PANGO_STYLE_ITALIC);                 break;
      case 'U': pa = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);         break;
      case 'S': pa = pango_attr_strikethrough_new (true);                       break;
      case '+': scaling_++; pa = pango_attr_scale_new (pow (1.2, scaling_));  break;
      case '-': scaling_--; pa = pango_attr_scale_new (pow (1.2, scaling_));  break;
      case '=': pa = pango_attr_family_new (MONOSPACE_NAME.c_str());            break;
      case '#':
        {
          String val = xnode.get_attribute ("family", true);
          if (val.size())
            pa = pango_attr_family_new (val.c_str());
        }
        break;
      case '0': case '1':
        {
          String col = xnode.get_attribute ("color", true);
          if (col.size() && token == '0')
            {
              PangoColor pcolor;
              if (pango_color_parse (&pcolor, col.c_str()))
                pa = LazyColorAttr::create_preset_color (LazyColorAttr::background_klass(),
                                                         pcolor.red, pcolor.green, pcolor.blue,
                                                         col, ColorType::BACKGROUND);
              else
                pa = LazyColorAttr::create_lazy_color (LazyColorAttr::background_klass(), col, ColorType::BACKGROUND);
            }
          else if (col.size() && token == '1')
            {
              PangoColor pcolor;
              if (pango_color_parse (&pcolor, col.c_str()))
                pa = LazyColorAttr::create_preset_color (LazyColorAttr::foreground_klass(),
                                                         pcolor.red, pcolor.green, pcolor.blue,
                                                         col, ColorType::FOREGROUND);
              else
                pa = LazyColorAttr::create_lazy_color (LazyColorAttr::foreground_klass(), col, ColorType::FOREGROUND);
            }
        }
        break;
      default:
        set_error ("invalid element: " + xnode.name(), &xnode);
      }
    // open tag
    if (pa)
      pa->start_index = plain_text_.size();
    // apply tag contents
    if (!error_.size())
      apply_tags (xnode.children());
    // close tag
    if (pa)
      {
        pa->end_index = plain_text_.size();
        alist_.push_front (pa);
      }
    // tag postamble
    switch (token)
      {
      case 'P': pre_count_--;  break;  /* PRE */
      case '+': scaling_--;    break;
      case '-': scaling_++;    break;
      }
  }
  void
  apply_tags (const vector<XmlNodeP> &nodes)
  {
    /* apply nodes */
    for (vector<XmlNodeP>::const_iterator it = nodes.begin(); it != nodes.end(); it++)
      {
        const XmlNode &xnode = **it;
        if (!xnode.istext())
          handle_tag (xnode);
        else /* text */
          {
            if (pre_count_)
              {
                plain_text_ += xnode.text();
                preserve_last_whitespace_ = true;
              }
            else
              {
                const String &text = xnode.text();
                for (String::const_iterator it = text.begin(); it != text.end(); it++)
                  if (!strchr (whitspaces_, *it))
                    plain_text_ += *it;
                  else if (plain_text_.size() && !strchr (whitspaces_, plain_text_[plain_text_.size() - 1]))
                    plain_text_ += ' ';
                if (text.size())
                  preserve_last_whitespace_ = false; /* whether to preserve last whitespace */
              }
          }
        if (error_.size())
          break;
      }
  }
  String
  apply_node (XmlNode &xnode)
  {
    /* apply node */
    vector<XmlNodeP> nodes;
    nodes.push_back (shared_ptr_cast<XmlNode> (&xnode));
    apply_tags (nodes);
    /* handle errors */
    if (error_.size())
      {
        if (error_node_)
          return string_format ("%s:%d:%d: %s", input_name().c_str(), error_node_->parsed_line(), error_node_->parsed_char(), error_.c_str());
        else
          return string_format ("%s: %s", input_name().c_str(), error_.c_str());
      }
    /* strip trailing whitespaces */
    if (!preserve_last_whitespace_ && plain_text_.size() &&
        strchr (whitspaces_, plain_text_[plain_text_.size() - 1]))
      plain_text_.resize (plain_text_.size() - 1);
    pango_layout_set_text (layout_, &plain_text_[0], plain_text_.size());
    PangoAttrList *pal = pango_attr_list_new();
    while (!alist_.empty())
      {
        PangoAttribute *pa = alist_.front();
        alist_.pop_front();
        pango_attr_list_change (pal, pa); /* assumes pa ownership */
      }
    pango_layout_set_attributes (layout_, pal);
    pango_attr_list_unref (pal);
    return "";
  }
public:
  static String
  apply_markup_tree (PangoLayout  *playout,
                     XmlNode      &xnode,
                     const String &input_name)
  {
    XmlToPango xtp (playout, input_name);
    return xtp.apply_node (xnode);
  }
};

// == TextPangoImpl (TextBlock) ==
class TextPangoImpl : public virtual WidgetImpl, public virtual TextBlock {
  PangoLayout    *layout_;
  TextMode        text_mode_;
  int             mark_, cursor_, selector_;
  double          scoffset_;
  void           *last_selector_attr_;
protected:
  virtual TextMode text_mode   () const               { return text_mode_; }
  virtual void
  text_mode (TextMode text_mode)
  {
    if (text_mode != text_mode_)
      {
        text_mode_ = text_mode;
        invalidate_size();
      }
  }
public:
  TextPangoImpl() :
    layout_ (NULL),
    text_mode_ (TEXT_MODE_ELLIPSIZED), mark_ (-1), cursor_ (-1), selector_ (-1),
    scoffset_ (0), last_selector_attr_ (NULL)
  {
    ParagraphState pstate; // retrieve defaults
    rapicorn_pango_mutex.lock();
    // FIXME: using pstate.font_family as font_desc string here bypasses our default font settings
    layout_ = global_layout_cache.create_layout (pstate.font_family, pstate.align,
                                                  PANGO_WRAP_WORD_CHAR, pstate.ellipsize,
                                                  iround (pstate.indent), iround (pstate.line_spacing),
                                                  text_mode_ == TEXT_MODE_SINGLE_LINE);
    rapicorn_pango_mutex.unlock();
  }
  ~TextPangoImpl()
  {
    rapicorn_pango_mutex.lock();
    g_object_unref (layout_);
    layout_ = NULL;
    rapicorn_pango_mutex.unlock();
  }
  virtual void
  size_request (Requisition &requisition)
  {
    ParagraphState pstate; // retrieve defaults
    PangoRectangle rect = { 0, 0 };
    rapicorn_pango_mutex.lock();
    pango_layout_set_width (layout_, -1);
    pango_layout_set_ellipsize (layout_,
                                text_mode_ != TEXT_MODE_ELLIPSIZED ?
                                PANGO_ELLIPSIZE_NONE :
                                pango_ellipsize_mode_from_ellipsize_type (pstate.ellipsize));
    pango_layout_get_extents (layout_, NULL, &rect);
    rapicorn_pango_mutex.unlock();
    /* pad requisition by 1 emboss pixel */
    requisition.width = ceil (1 + UNITS2PIXELS (rect.width));
    requisition.height = ceil (1 + UNITS2PIXELS (rect.height));
  }
  virtual void
  size_allocate (Allocation area, bool changed)
  {
    ParagraphState pstate; // retrieve defaults
    PangoRectangle rect = { 0, 0 };
    rapicorn_pango_mutex.lock();
    if (text_mode_ == TEXT_MODE_SINGLE_LINE)
      pango_layout_set_width (layout_, -1);
    else
      pango_layout_set_width (layout_, ifloor (PIXELS2UNITS (area.width)));
    pango_layout_set_ellipsize (layout_,
                                text_mode_ != TEXT_MODE_ELLIPSIZED ?
                                PANGO_ELLIPSIZE_NONE :
                                pango_ellipsize_mode_from_ellipsize_type (pstate.ellipsize));
    pango_layout_get_extents (layout_, NULL, &rect);
    rapicorn_pango_mutex.unlock();
    tune_requisition (-1, ceil (1 + UNITS2PIXELS (rect.height)));
    scroll_to_cursor();
  }
protected:
  virtual double
  text_requisition (uint          n_chars,
                    uint          n_digits)
  {
    // FIXME: we need to setup a dummy cairo context here for pango_cairo_update_layout
    rapicorn_pango_mutex.lock();
    PangoContext *pcontext = pango_layout_get_context (layout_);
    const PangoFontDescription *cfdesc = LayoutCache::font_description_from_layout (layout_);
    PangoFontMetrics *metrics = pango_context_get_metrics (pcontext, cfdesc, pango_context_get_language (pcontext));
    double char_width = pango_font_metrics_get_approximate_char_width (metrics);
    double digit_width = pango_font_metrics_get_approximate_digit_width (metrics);
    pango_font_metrics_unref (metrics);
    double extra_width = n_chars * char_width + n_digits * digit_width;
#if 0
    if (sample.size())
      {
        ParagraphState pstate; // retrieve defaults
        PangoLayout *playout = pango_layout_copy (layout_);
        pango_layout_set_attributes (playout, NULL);
        pango_layout_set_tabs (playout, NULL);
        pango_layout_set_text (playout, sample.c_str(), -1);
        pango_layout_set_width (layout_, -1);
        PangoRectangle rect = { 0, 0 };
        pango_layout_set_ellipsize (playout,
                                    text_mode_ != TEXT_MODE_ELLIPSIZED ?
                                    PANGO_ELLIPSIZE_NONE :
                                    pango_ellipsize_mode_from_ellipsize_type (pstate.ellipsize));
        pango_layout_get_extents (layout_, NULL, &rect);
        g_object_unref (playout);
        extra_width += rect.width;
      }
#endif
    rapicorn_pango_mutex.unlock();
    return UNITS2PIXELS (extra_width);
  }
  virtual const char*
  peek_text (int *byte_length)
  {
    rapicorn_pango_mutex.lock();
    const char *str = pango_layout_get_text (layout_);
    rapicorn_pango_mutex.unlock();
    if (byte_length)
      *byte_length = strlen (str);
    return str;
  }
  virtual ParagraphState
  para_state () const
  {
    ParagraphState pstate;
    rapicorn_pango_mutex.lock();
    pstate.align = align_type_from_pango_alignment (pango_layout_get_alignment (layout_));
    pstate.ellipsize = ellipsize_type_from_pango_ellipsize_mode (pango_layout_get_ellipsize (layout_));
    pstate.line_spacing = UNITS2PIXELS (pango_layout_get_spacing (layout_));
    pstate.indent = UNITS2PIXELS (pango_layout_get_indent (layout_));
    PangoFontDescription *fdesc = pango_font_description_copy_static (pango_context_get_font_description (pango_layout_get_context (layout_)));
    if (pango_layout_get_font_description (layout_))
      pango_font_description_merge_static (fdesc, pango_layout_get_font_description (layout_), TRUE);
    pstate.font_family = pango_font_description_get_family (fdesc);
    pstate.font_size = UNITS2PIXELS (pango_font_description_get_size (fdesc));
    assert (pango_font_description_get_size_is_absolute (fdesc) == false);
    pango_font_description_free (fdesc);
    rapicorn_pango_mutex.unlock();
    return ParagraphState();
  }
  virtual void
  para_state (const ParagraphState &pstate)
  {
    rapicorn_pango_mutex.lock();
    pango_layout_set_alignment (layout_, pango_alignment_from_align_type (pstate.align));
    pango_layout_set_ellipsize (layout_, pango_ellipsize_mode_from_ellipsize_type (pstate.ellipsize));
    pango_layout_set_spacing (layout_, iround (PIXELS2UNITS (pstate.line_spacing)));
    pango_layout_set_indent (layout_, iround (PIXELS2UNITS (pstate.indent)));
    if (pstate.font_family.size() || pstate.font_size)
      {
        PangoFontDescription *fdesc = pango_font_description_new();
        if (pstate.font_family.size())
          pango_font_description_set_family (fdesc, pstate.font_family.c_str());
        if (pstate.font_size)
          pango_font_description_set_size (fdesc, iround (PIXELS2UNITS (pstate.font_size)));
        pango_layout_set_font_description (layout_, fdesc);
        pango_font_description_free (fdesc);
      }
    rapicorn_pango_mutex.unlock();
    invalidate();
    changed ("para_state");
  }
  virtual TextAttrState
  attr_state () const
  {
    // FIXME: implement this
    return TextAttrState();
  }
  virtual void
  attr_state (const TextAttrState &astate)
  {
    // FIXME: implement this
    changed ("attr_state");
  }
  virtual String
  save_markup () const
  {
    rapicorn_pango_mutex.lock();
    String output = MarkupDumper::dump_markup (layout_);
    rapicorn_pango_mutex.unlock();
    return output;
  }
  virtual void
  load_markup (const String &markup)
  {
    String err;
    rapicorn_pango_mutex.lock();
    MarkupParser::Error perror;
    const char *input_file = "TextPango::markup_text";
    XmlNodeP xnode = XmlNode::parse_xml (input_file, markup.c_str(), markup.size(), &perror, "text");
    if (perror.code)
      err = string_format ("%s:%d:%d: %s", input_file, perror.line_number, perror.char_number, perror.message.c_str());
    if (xnode)
      {
        if (!err.size())
          err = XmlToPango::apply_markup_tree (layout_, *xnode, input_file);
      }
    rapicorn_pango_mutex.unlock();
    if (err.size())
      critical ("%s", err.c_str());
    invalidate();
  }
  virtual int
  mark () const /* byte_index */
  {
    return mark_;
  }
  virtual void
  mark (int byte_index)
  {
    rapicorn_pango_mutex.lock();
    const char *c = pango_layout_get_text (layout_);
    int l = strlen (c);
    if (byte_index < 0)
      mark_ = l;
    else if (byte_index >= l)
      mark_ = l;
    else
      mark_ = utf8_align (c, c + byte_index) - c;
    rapicorn_pango_mutex.unlock();
    changed ("cursor");
  }
  virtual void
  cursor2mark ()
  {
    if (cursor_ != mark_)
      {
        mark_ = cursor_;
        changed ("cursor");
      }
  }
  virtual bool
  mark_at_end () const
  {
    rapicorn_pango_mutex.lock();
    const char *c = pango_layout_get_text (layout_);
    rapicorn_pango_mutex.unlock();
    int l = strlen (c);
    return mark_ >= l;
  }
  virtual bool
  mark_to_coord (double x, double y)
  {
    Rect area = layout_area (NULL);
    if (area.width >= 1 && area.height >= 1)
      {
        int xmark = -1, trailing = -1;
        /* offset into layout area */
        x -= area.x;
        y -= area.y;
        /* offset by scroll position */
        x += scoffset_;
        /* scale to pango */
        x = PIXELS2UNITS (x);
        y = PIXELS2UNITS (y);
        /* query pos */
        rapicorn_pango_mutex.lock();
        bool texthit = pango_layout_xy_to_index (layout_, iround (x), iround (y), &xmark, &trailing);
        const char *c = pango_layout_get_text (layout_);
        int l = strlen (c);
        rapicorn_pango_mutex.unlock();
        if (xmark >= 0 && xmark < l)    /* texthit is constrained to real text area */
          {
            while (xmark < l && trailing--)
              xmark = utf8_next (c + xmark) - c;
            mark (xmark);
            return true;
            (void) texthit;
          }
      }
    return false;
  }
  virtual void
  step_mark (int visual_direction)
  {
    rapicorn_pango_mutex.lock();
    const char *c = pango_layout_get_text (layout_);
    int l = strlen (c);
    int xmark = mark_, trailing;
    pango_layout_move_cursor_visually (layout_, TRUE, mark_, 0, visual_direction, &xmark, &trailing);
    while (xmark < l && trailing--)
      xmark = utf8_next (c + xmark) - c;
    rapicorn_pango_mutex.unlock();
    if (xmark >= l)
      mark_ = l;
    else
      mark_ = MAX (0, xmark);
    changed ("cursor");
  }
  virtual void
  mark2cursor ()
  {
    if (cursor_ != mark_)
      {
        cursor_ = mark_;
        expose();
        scroll_to_cursor();
        if (selector_ >= 0)
          selection_changed();
        changed ("cursor");
      }
  }
  virtual void
  hide_cursor ()
  {
    if (cursor_ >= 0)
      {
        cursor_ = -1;
        expose();
        if (selector_ >= 0)
          selection_changed();
        changed ("cursor");
      }
  }
  virtual void
  mark2selector ()
  {
    if (selector_ != mark_)
      {
        selector_ = mark_;
        expose();
        selection_changed();
        changed ("cursor");
      }
  }
  virtual void
  hide_selector ()
  {
    if (selector_ >= 0)
      {
        selector_ = -1;
        expose();
        selection_changed();
        changed ("cursor");
      }
  }
  virtual bool
  get_selection (int *start, int *end, int *nutf8)
  {
    if (cursor_ >= 0 && selector_ >= 0)
      {
        const int s = MIN (cursor_, selector_);
        const int e = MAX (cursor_, selector_);
        if (start)
          *start = s;
        if (end)
          *end = e;
        if (nutf8)
          {
            const char *c = pango_layout_get_text (layout_);
            int m, n = 0;
            for (m = s; m < e; n++)
              m = utf8_next (c + m) - c;
            *nutf8 = n;
          }
        return true;
      }
    return false;
  }
  virtual void
  mark_delete (uint n_utf8_chars)
  {
    rapicorn_pango_mutex.lock();
    const char *c = pango_layout_get_text (layout_);
    int l = strlen (c);
    int m = mark_;
    while (m < l && n_utf8_chars--)
      m = utf8_next (c + m) - c;
    String s = c;
    s.erase (mark_, m - mark_);
    pango_layout_set_text (layout_, s.c_str(), -1);
    // FIXME: adjust attributes, cursor_, selector_
    rapicorn_pango_mutex.unlock();
    invalidate();
    changed ("text");
    changed ("cursor");
  }
  virtual void
  mark_insert (String utf8string, const TextAttrState *astate = NULL)
  {
    rapicorn_pango_mutex.lock();
    String s = pango_layout_get_text (layout_);
    int s1 = s.size();
    s.insert (mark_, utf8string);
    int s2 = s.size();
    mark_ += s2 - s1;
    pango_layout_set_text (layout_, s.c_str(), -1);
    // FIXME: adjust attributes
    rapicorn_pango_mutex.unlock();
    invalidate();
    changed ("text");
    changed ("cursor");
  }
protected:
  void
  render_dots_gL (cairo_t      *cr,
                  int           x,
                  int           y,
                  int           width,
                  int           height,
                  int           dot_size,
                  Color         col)
  {
    double w3 = width / 3., r = dot_size / 2.;
    cairo_save (cr);
    cairo_set_source_rgba (cr, col.red1(), col.green1(), col.blue1(), col.alpha1());
    cairo_arc (cr, x + .5 + w3,       y + .5 + height / 2, r, 0, 2 * M_PI);
    cairo_arc (cr, x + .5 + w3 * 1.5, y + .5 + height / 2, r, 0, 2 * M_PI);
    cairo_arc (cr, x + .5 + w3 * 2,   y + .5 + height / 2, r, 0, 2 * M_PI);
    cairo_fill (cr);
    cairo_restore (cr);
  }
  void
  render_cursor_gL (cairo_t      *cairo,
                    Color         col,
                    Rect          layout_rect,
                    double        layout_x,
                    double        layout_y)
  {
    // const char *ptext = pango_layout_get_text (layout_);
    PangoRectangle crect1, crect2, irect, lrect;
    pango_layout_get_extents (layout_, &irect, &lrect);
    if (cursor_ < 0)
      return;
    pango_layout_get_cursor_pos (layout_, cursor_, &crect1, &crect2);
    double x = layout_rect.x + layout_x + UNITS2PIXELS (crect1.x);
    // double width = MIN (layout_rect.width, MAX (1, UNITS2PIXELS (crect1.width))); // FIXME: cursor width
    // double y = layout_rect.y + layout_y + UNITS2PIXELS (lrect.height - irect.y - crect1.y - crect1.height);
    double y = layout_rect.y + layout_y + UNITS2PIXELS (crect1.y);
    double height = UNITS2PIXELS (crect1.height);
    Color fg (col.premultiplied());
    int xpos = iround (x);
    int ymin = iround (y), ymax = iround (y + height - 1);
    const double cw = 2, ch = 2;
    cairo_set_source_rgba (cairo, col.red1(), col.green1(), col.blue1(), col.alpha1());
    // lower triangle
    cairo_move_to (cairo, xpos,          ymax - ch);
    cairo_line_to (cairo, xpos - cw,     ymax);
    cairo_line_to (cairo, xpos + 1 + cw, ymax);
    cairo_line_to (cairo, xpos + 1,      ymax - ch);
    cairo_close_path (cairo);
    // upper triangle
    cairo_move_to (cairo, xpos,          ymin + ch);
    cairo_line_to (cairo, xpos - cw,     ymin);
    cairo_line_to (cairo, xpos + 1 + cw, ymin);
    cairo_line_to (cairo, xpos + 1,      ymin + ch);
    cairo_close_path (cairo);
    // show triangles
    cairo_fill (cairo);
    // draw bar
    cairo_move_to (cairo, xpos + .5, ymin + ch);
    cairo_line_to (cairo, xpos + .5, ymax - ch);
    cairo_set_line_width (cairo, 1);
    cairo_stroke (cairo);
  }
  void
  scroll_to_cursor ()
  {
    if (cursor_ < 0)
      return;
    uint vdot_size;
    Rect layout_rect = layout_area (&vdot_size);
    rapicorn_pango_mutex.lock();
    PangoRectangle irect, lrect, crect1, crect2;
    pango_layout_get_extents (layout_, &irect, &lrect);
    pango_layout_get_cursor_pos (layout_, cursor_, &crect1, &crect2);
    rapicorn_pango_mutex.unlock();
    double cw = 3; // symmetric cursor width left and right from center
    double cl = UNITS2PIXELS (crect1.x - lrect.x) - cw, cr = UNITS2PIXELS (crect1.x - lrect.x) + cw;
    if (cr - scoffset_ > layout_rect.width)
      scoffset_ = MAX (scoffset_, cr - layout_rect.width);
    if (cl - scoffset_ < 0)
      scoffset_ = MIN (scoffset_, cl); /* for position 0, this "indents" the text by the cursor width */
    scoffset_ = MAX (0, scoffset_); /* un-"indent" */
  }
  void
  render_text_gL (cairo_t      *cairo,
                  Rect          layout_rect,
                  int           dot_size,
                  Color         fg)
  {
    if (dot_size)
      {
        Rect area = allocation();
        area.height -= MIN (area.height, layout_rect.height); // draw vdots beneth layout
        if (area.height > 0) // some partially visible lines have been clipped
          {
            // display vellipsis
            render_dots_gL (cairo, area.x, area.y, area.width, area.height, dot_size, fg);
          }
      }
    /* render text */
    PangoRectangle lrect; /* logical (x,y) can be != 0, e.g. for RTL-layouts and fixed width set */
    pango_layout_get_extents (layout_, NULL, &lrect);
    cairo_save (cairo);
    cairo_set_source_rgba (cairo, fg.red1(), fg.green1(), fg.blue1(), fg.alpha1());
    // translate cairo surface so current_point=(0,0) becomes layout origin
    cairo_translate (cairo, layout_rect.x - scoffset_, layout_rect.y);
    // clip to layout_rect, which has been shrunken to clip partial lines
    cairo_rectangle (cairo, scoffset_, 0, layout_rect.width, layout_rect.height);
    cairo_clip (cairo);
    pango_cairo_show_layout (cairo, layout_);
    cairo_restore (cairo);
    /* and cursor */
    double lx = UNITS2PIXELS (lrect.x) - scoffset_;
    double ly = 0;
    render_cursor_gL (cairo, fg, layout_rect, lx, ly);
  }
  Rect
  layout_area (uint *vdot_size)
  {
    /* FIXME: once PANGO_VERSION_CHECK (1, 19, 3) succeeds, this
     * should use pango_layout_set_height() instead.
     */
    Rect area = allocation();
    rapicorn_pango_mutex.lock();
    /* measure layout size */
    PangoRectangle lrect = { 0, 0 };
    pango_layout_get_extents (layout_, NULL, &lrect);
    double vpixels = UNITS2PIXELS (lrect.height);
    /* decide vertical ellipsis */
    bool vellipsize = floor (vpixels) > area.height;
    if (vellipsize)
      {
        gint last_height = 0, dotsize = LayoutCache::dot_size_from_layout (layout_);
        PangoLayoutIter *pli = pango_layout_get_iter (layout_);
        do
          {
            PangoRectangle nrect;
            pango_layout_iter_get_line_extents (pli, NULL, &nrect);
            if (UNITS2PIXELS (nrect.y + nrect.height) + 3 * dotsize > area.height)
              break;
            last_height = floor (UNITS2PIXELS (nrect.y + nrect.height));
          }
        while (pango_layout_iter_next_line (pli));
        pango_layout_iter_free (pli);
        if (vdot_size)
          *vdot_size = dotsize;
        area.y += area.height - last_height;
        area.height = last_height;
      }
    else if (vdot_size)
      *vdot_size = 0;
    /* preserve emboss space */
    area.x += 1;
    area.y += 1;
    area.width -= 1;
    area.height -= 1;
    area.height = MAX (0, area.height);
    /* center vertically */
    if (vpixels < area.height)
      {
        double extra = area.height - vpixels;
        area.y += extra / 2;
        area.height -= extra;
      }
    rapicorn_pango_mutex.unlock();
    /* check area */
    if ((area.width < 1) ||             /* area needs to be larger than emboss padding */
        (vellipsize && !vdot_size))     /* too tall without vellipsization */
      area.width = area.height = 0;
    return area;
  }
  static gboolean attribute_filter (PangoAttribute *attr, void *ptr) { return attr == ptr; }
  void
  selection_changed()
  {
    rapicorn_pango_mutex.lock();
    bool real_change = false;
    if (last_selector_attr_)
      {
        PangoAttrList *palist = pango_layout_get_attributes (layout_);
        PangoAttrList *padel = pango_attr_list_filter (palist, attribute_filter, last_selector_attr_);
        pango_attr_list_unref (padel);
        last_selector_attr_ = NULL;
        real_change = true;
      }
    if (cursor_ >= 0 && selector_ >= 0)
      {
        PangoAttribute *sbg = pango_attr_background_new (0xa8a8, 0xd1d1, 0xffff);
        sbg->start_index = MIN (cursor_, selector_);
        sbg->end_index = MAX (cursor_, selector_);
        PangoAttrList *palist = pango_layout_get_attributes (layout_);
        pango_attr_list_insert (palist, sbg);
        last_selector_attr_ = sbg;
        real_change = true;
      }
    if (real_change)
      pango_layout_set_attributes (layout_, pango_layout_get_attributes (layout_)); // refresh layout caches for new attributes
    rapicorn_pango_mutex.unlock();
    if (real_change)
      sig_selection_changed.emit();
  }
  virtual void
  render (RenderContext &rcontext, const Rect &rect)
  {
    uint vdot_size = 0;
    Rect larea = layout_area (&vdot_size);
    RDEBUG ("rendering label 0x%016x at %3.f%% coverage: %s", size_t (this),
            larea.area() > 0 ? Rect (rect).intersect (larea).area() / larea.area() * 100 : 0, peek_text (NULL));
    if (larea.width < 1) // allowed: larea.height < 1
      return;
    /* render text */
    cairo_t *cr = cairo_context (rcontext, rect);
    default_pango_cairo_font_options (NULL, cr);
    rapicorn_pango_mutex.lock();
    if (insensitive())
      {
        const double ax = larea.x, ay = larea.y;
        Color insensitive_glint, insensitive_ink = heritage()->insensitive_ink (state(), &insensitive_glint);
        /* render embossed text */
        larea.x = ax, larea.y = ay;
        render_text_gL (cr, larea, vdot_size, insensitive_glint);
        larea.x = ax - 1, larea.y = ay - 1;
        render_text_gL (cr, larea, vdot_size, insensitive_ink);
      }
    else
      {
        /* render normal text */
        render_text_gL (cr, larea, vdot_size, foreground());
      }
    rapicorn_pango_mutex.unlock();
  }
};

static const WidgetFactory<TextPangoImpl> text_pango_factory ("Rapicorn::TextPango");

} // Rapicorn

#endif  /* RAPICORN_WITH_PANGO */
