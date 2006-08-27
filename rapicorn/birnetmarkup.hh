/* BirnetMarkup - simple XML parser
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
#ifndef __BIRNET_MARKUP_HH__
#define __BIRNET_MARKUP_HH__

#include <rapicorn/birnetutils.hh>

namespace Birnet {

class MarkupParser {
public:
  typedef enum {
    NONE        = 0,
    READ_FAILED,
    BAD_UTF8,
    DOCUMENT_EMPTY,
    PARSE_ERROR,
    /* client errors */
    INVALID_CONTENT
  } ErrorType;
  struct Error {
    ErrorType   code;
    String      message;
    uint        line_number;
    uint        char_number;
    explicit    Error() : code (NONE), line_number (0), char_number (0) {}
    void        set  (ErrorType c, String msg)  { code = c; message = msg; }
    bool        set  ()                         { return code != NONE; }
  };
  typedef const vector<String> ConstStrings;
  static MarkupParser*  create_parser   (const String   &input_name);
  virtual               ~MarkupParser   ();
  bool                  parse           (const char     *text,
                                         ssize_t         text_len,  
                                         Error          *error);
  bool                  end_parse       (Error          *error);
  String                get_element     ();
  void                  get_position    (int            *line_number,
                                         int            *char_number,
                                         const char    **input_name_p = NULL);
  virtual void          error           (const Error    &error);
  /* useful when saving */
  static String         escape_text     (const char     *text,
                                         ssize_t         length);
  static String         printf_escaped  (const char     *format,
                                         ...) BIRNET_PRINTF (1, 2);
  static String         vprintf_escaped (const char     *format,
                                         va_list         args);
  struct Context;
protected:
  explicit              MarkupParser    (const String   &input_name);
  virtual void          start_element   (const String   &element_name,
                                         ConstStrings   &attribute_names,
                                         ConstStrings   &attribute_values,
                                         Error          &error);
  virtual void          end_element     (const String   &element_name,
                                         Error          &error);
  virtual void          text            (const String   &text,
                                         Error          &error);
  virtual void          pass_through    (const String   &pass_through_text,
                                         Error          &error);
private:
  Context *context;
  String   m_input_name;
};

} // Birnet

#endif  /* __BIRNET_MARKUP_HH__ */
