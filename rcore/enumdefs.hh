// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_ENUMDEFS_HH__
#define __RAPICORN_ENUMDEFS_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

/* --- enum/flags type descriptions --- */
struct EnumClass {
  struct Value {
    const int64             value;
    const char       *const value_name;
    const uint              name_length;
  };
  virtual            ~EnumClass         () {}
  virtual void        list_values       (uint &n_values, const Value* &values) const = 0;
  virtual const char* enum_name         () const = 0;
  virtual bool        flag_combinable   () const = 0;
  virtual int64       constrain         (int64 value) const = 0;
  bool                match_partial     (const char *value_name1, const char *partial_value_name) const;
  bool                match             (const char *value_name1, const char *value_name2) const;
  const Value*        find_first        (int64 value) const;
  const Value*        find_first        (const String &value_name) const;
  int64               parse             (const char *value_string, String *error = NULL) const;
  String              string            (int64 value) const;
};
template<typename EnumType> EnumType inline enum_type_constrain (EnumType value) { return value; }
template<typename EType>
struct EnumType : public virtual EnumClass {
  typedef EType Type;
  virtual void        list_values       (uint &c_n_values, const Value* &c_values) const { c_n_values = n_values, c_values = values; }
  virtual const char* enum_name         () const { return ename; }
  virtual bool        flag_combinable   () const { return false; }
  virtual int64       constrain         (int64 value) const { return enum_type_constrain<EType> (EType (value)); }
  inline              EnumType          () {}
private:
  static const uint         n_values;
  static const Value *const values;
  static const char        *ename;
};
template<typename EType>
struct FlagsType : public virtual EnumClass {
  typedef EType Type;
  virtual void        list_values       (uint &c_n_values, const Value* &c_values) const { c_n_values = n_values, c_values = values; }
  virtual const char* enum_name         () const { return ename; }
  virtual bool        flag_combinable   () const { return true; }
  virtual int64       constrain         (int64 value) const { return enum_type_constrain<EType> (EType (value)); }
  inline              FlagsType         () {}
private:
  static const uint         n_values;
  static const Value *const values;
  static const char        *ename;
};

/* --- enums --- */
typedef enum {
  ADJUSTMENT_SOURCE_NONE = 0,
  ADJUSTMENT_SOURCE_ANCESTRY_HORIZONTAL,
  ADJUSTMENT_SOURCE_ANCESTRY_VERTICAL,
  ADJUSTMENT_SOURCE_ANCESTRY_VALUE
} AdjustmentSourceType;
typedef EnumType<AdjustmentSourceType> EnumTypeAdjustmentSourceType;

typedef enum {
  ALIGN_LEFT = 1,
  ALIGN_CENTER,
  ALIGN_RIGHT,
} AlignType;
typedef EnumType<AlignType> EnumTypeAlignType;

/**
 * AnchorType specifies an anchoring point for graphical elements.
 */
typedef enum {
  ANCHOR_NONE,
  ANCHOR_CENTER,
  ANCHOR_EAST,
  ANCHOR_NORTH_EAST,
  ANCHOR_NORTH,
  ANCHOR_NORTH_WEST,
  ANCHOR_WEST,
  ANCHOR_SOUTH_WEST,
  ANCHOR_SOUTH,
  ANCHOR_SOUTH_EAST,
} AnchorType;
typedef EnumType<AnchorType> EnumTypeAnchorType;

typedef enum {
  CLICK_ON_PRESS        = 1,
  CLICK_ON_RELEASE,
  CLICK_SLOW_REPEAT,
  CLICK_FAST_REPEAT,
  CLICK_KEY_REPEAT,
} ClickType;
typedef EnumType<ClickType> EnumTypeClickType;

typedef enum {
  COLOR_NONE,
  COLOR_FOREGROUND,
  COLOR_BACKGROUND,
  COLOR_BACKGROUND_EVEN,
  COLOR_BACKGROUND_ODD,
  COLOR_DARK,
  COLOR_DARK_SHADOW,
  COLOR_DARK_GLINT,
  COLOR_LIGHT,
  COLOR_LIGHT_SHADOW,
  COLOR_LIGHT_GLINT,
  COLOR_FOCUS,
  COLOR_BLACK,
  COLOR_WHITE,
  COLOR_RED,
  COLOR_YELLOW,
  COLOR_GREEN,
  COLOR_CYAN,
  COLOR_BLUE,
  COLOR_MAGENTA,
} ColorType;
typedef EnumType<ColorType> EnumTypeColorType;

typedef enum {
  COLOR_INHERIT,        ///< Inherit color from parent component
  COLOR_NORMAL,         ///< Normal color specification
  COLOR_SELECTED,       ///< Color used for selected areas
  COLOR_BASE,           ///< Color used for text or scroll fields
} ColorSchemeType;
typedef EnumType<ColorSchemeType> EnumTypeColorSchemeType;

typedef enum {
  DIR_NONE,
  DIR_RIGHT,
  DIR_UP,
  DIR_LEFT,
  DIR_DOWN,
} DirType;              ///< Enum type for direction indication
typedef EnumType<DirType> EnumTypeDirType;

typedef enum {
  ELLIPSIZE_START = 1,
  ELLIPSIZE_MIDDLE,
  ELLIPSIZE_END
} EllipsizeType;
typedef EnumType<EllipsizeType> EnumTypeEllipsizeType;

typedef enum {
  FOCUS_NEXT    = 1,
  FOCUS_PREV,
  FOCUS_RIGHT,
  FOCUS_UP,
  FOCUS_LEFT,
  FOCUS_DOWN
} FocusDirType;         ///< Enum type for focus movements
typedef EnumType<FocusDirType> EnumTypeFocusDirType;

typedef enum {
  FRAME_NONE,
  FRAME_BACKGROUND,
  FRAME_IN,
  FRAME_OUT,
  FRAME_ETCHED_IN,
  FRAME_ETCHED_OUT,
  FRAME_FOCUS,
  FRAME_ALERT_FOCUS,
} FrameType;
typedef EnumType<FrameType> EnumTypeFrameType;

typedef enum {
  LIGHTING_NONE,
  LIGHTING_UPPER_LEFT,
  LIGHTING_UPPER_RIGHT,
  LIGHTING_LOWER_LEFT,
  LIGHTING_LOWER_RIGHT,
  LIGHTING_CENTER,
  LIGHTING_DIFFUSE,
  LIGHTING_DARK_UPPER_LEFT   = 0x80 | LIGHTING_UPPER_LEFT,
  LIGHTING_DARK_UPPER_RIGHT  = 0x80 | LIGHTING_UPPER_RIGHT,
  LIGHTING_DARK_LOWER_LEFT   = 0x80 | LIGHTING_LOWER_LEFT,
  LIGHTING_DARK_LOWER_RIGHT  = 0x80 | LIGHTING_LOWER_RIGHT,
  LIGHTING_DARK_CENTER       = 0x80 | LIGHTING_CENTER,
  LIGHTING_DARK_DIFFUSE      = 0x80 | LIGHTING_DIFFUSE,
} LightingType;
static const LightingType LIGHTING_DARK_FLAG = LightingType (0x80);
typedef EnumType<LightingType> EnumTypeLightingType;
inline LightingType  operator&  (LightingType  s1, LightingType s2) { return LightingType (s1 & (uint64) s2); }
inline LightingType& operator&= (LightingType &s1, LightingType s2) { s1 = s1 & s2; return s1; }
inline LightingType  operator|  (LightingType  s1, LightingType s2) { return LightingType (s1 | (uint64) s2); }
inline LightingType& operator|= (LightingType &s1, LightingType s2) { s1 = s1 | s2; return s1; }

typedef enum {
  SELECTION_NONE,       ///< No selection possible
  SELECTION_BROWSE,     ///< Browse by always forcing a single selected item
  SELECTION_SINGLE,     ///< Allow selection toggling of a single item
  SELECTION_INTERVAL,   ///< Allow selection of multiple consecutive items
  SELECTION_MULTIPLE,   ///< Allow arbitrary combinations of selected items
} SelectionMode;
typedef EnumType<SelectionMode> EnumTypeSelectionMode;

typedef enum {
  SIZE_POLICY_NORMAL            = 0,
  SIZE_POLICY_WIDTH_FROM_HEIGHT,
  SIZE_POLICY_HEIGHT_FROM_WIDTH,
} SizePolicyType;
typedef EnumType<SizePolicyType> EnumTypeSizePolicyType;

typedef enum {
  TEXT_MODE_WRAPPED = 1,
  TEXT_MODE_ELLIPSIZED,
  TEXT_MODE_SINGLE_LINE,
} TextMode;
typedef EnumType<TextMode> EnumTypeTextMode;

typedef enum {
  /* main window types */
  WINDOW_TYPE_NORMAL  = 0,      ///< Normal window
  WINDOW_TYPE_DESKTOP,          ///< Desktop background
  WINDOW_TYPE_DOCK,             ///< Dock or panel
  WINDOW_TYPE_TOOLBAR,          ///< Torn-off toolbar
  WINDOW_TYPE_MENU,             ///< Torn-off menu
  WINDOW_TYPE_UTILITY,          ///< Palette or toolbox
  WINDOW_TYPE_SPLASH,           ///< Startup/splash screen
  WINDOW_TYPE_DIALOG,           ///< Dialog window, usually transient
  WINDOW_TYPE_DROPDOWN_MENU,    ///< Menu, opened from menubar
  WINDOW_TYPE_POPUP_MENU,       ///< Menu, opened as context menu
  WINDOW_TYPE_TOOLTIP,          ///< Transient context info window
  WINDOW_TYPE_NOTIFICATION,     ///< Transient info window (e.g. info bubble)
  WINDOW_TYPE_COMBO,            ///< Combo box menu or list window
  WINDOW_TYPE_DND,              ///< Window for dragged during DND operations
} WindowType;
typedef EnumType<WindowType> EnumTypeWindowType;

} // Rapicorn

#endif  /* __RAPICORN_ENUMDEFS_HH__ */
