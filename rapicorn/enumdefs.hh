/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __RAPICORN_ENUMDEFS_HH__
#define __RAPICORN_ENUMDEFS_HH__

#include <rapicorn/utilities.hh>

namespace Rapicorn {

/* --- enum/flags type descriptions --- */
struct EnumClass {
  struct Value {
    const int64             value;
    const char       *const value_name;
    const uint              name_length;
  };
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
private:
  static const uint         n_values;
  static const Value *const values;
  static const char        *ename;
};

/* --- enums --- */
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
  ALIGN_LEFT = 1,
  ALIGN_CENTER,
  ALIGN_RIGHT,
} AlignType;
typedef EnumType<AlignType> EnumTypeAlignType;

typedef enum {
  WRAP_NONE,
  WRAP_CHAR,
  WRAP_WORD,
} WrapType;
typedef EnumType<WrapType> EnumTypeWrapType;

typedef enum {
  ELLIPSIZE_NONE,
  ELLIPSIZE_START,
  ELLIPSIZE_MIDDLE,
  ELLIPSIZE_END
} EllipsizeType;
typedef EnumType<EllipsizeType> EnumTypeEllipsizeType;

typedef enum {
  STATE_INSENSITIVE     = 1 << 0,
  STATE_PRELIGHT        = 1 << 1,
  STATE_IMPRESSED       = 1 << 2,
  STATE_FOCUS           = 1 << 3,
  STATE_DEFAULT         = 1 << 4,
} StateType;
static const StateType STATE_NORMAL = StateType (0);
static const StateType STATE_MASK = StateType (0x1f);
typedef FlagsType<StateType> FlagsTypeStateType;
inline StateType  operator&  (StateType  s1, StateType s2) { return StateType (s1 & (uint64) s2); }
inline StateType& operator&= (StateType &s1, StateType s2) { s1 = s1 & s2; return s1; }
inline StateType  operator|  (StateType  s1, StateType s2) { return StateType (s1 | (uint64) s2); }
inline StateType& operator|= (StateType &s1, StateType s2) { s1 = s1 | s2; return s1; }

typedef enum {
  COLOR_NONE,
  COLOR_FOREGROUND,
  COLOR_BACKGROUND,
  COLOR_SELECTED_FOREGROUND,
  COLOR_SELECTED_BACKGROUND,
  COLOR_FOCUS,
  COLOR_DEFAULT,
  COLOR_LIGHT_GLINT,
  COLOR_LIGHT_SHADOW,
  COLOR_DARK_GLINT,
  COLOR_DARK_SHADOW,
  COLOR_WHITE,
  COLOR_BLACK,
  COLOR_RED,
  COLOR_YELLOW,
  COLOR_GREEN,
  COLOR_CYAN,
  COLOR_BLUE,
  COLOR_MAGENTA
} ColorType;
typedef EnumType<ColorType> EnumTypeColorType;

} // Rapicorn

#endif  /* __RAPICORN_ENUMDEFS_HH__ */
