// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_MARKUP_HH__
#define __RAPICORN_MARKUP_HH__

#include <rcore/utilities.hh>
#include <rcore/strings.hh>

namespace Rapicorn {

/** Simple XML markup parser, based on glib/gmarkup.c.
 * @DEPRECATED: Nonpublic API, this XML parser is planned to be replaced soon.
 */
class MarkupParser {
public:
  typedef enum {
    NONE        = 0,
    READ_FAILED,
    BAD_UTF8,
    DOCUMENT_EMPTY,
    PARSE_ERROR,
    /* client errors */
    INVALID_ELEMENT,
    INVALID_ATTRIBUTE,
    INVALID_CONTENT,
    MISSING_ELEMENT,
    MISSING_ATTRIBUTE,
    MISSING_CONTENT
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
  String                input_name      ();
  void                  get_position    (int            *line_number,
                                         int            *char_number,
                                         const char    **input_name_p = NULL);
  virtual void          error           (const Error    &error);
  // parsing
  static size_t         seek_to_element (const char     *data, size_t length);
  /* useful when saving */
  static String         escape_text     (const String   &text);
  static String         escape_text     (const char     *text,
                                         ssize_t         length);
  template<class... Args> RAPICORN_PRINTF (1, 0) static String
  escape_format_args (const char *format, const Args &...args)
  {
    auto arg_transform = [] (const String &s) { return escape_text (s); };
    return Lib::StringFormatter::format (arg_transform, format, args...);
  }
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
  void                  recap_element   (const String   &element_name,
                                         ConstStrings   &attribute_names,
                                         ConstStrings   &attribute_values,
                                         Error          &error,
                                         bool            include_outer = true);
  const String&         recap_string    () const;
private:
  Context *context;
  String   input_name_;
  String   recap_;
  uint     recap_depth_;
  bool     recap_outer_;
  void     recap_start_element   (const String   &element_name,
                                  ConstStrings   &attribute_names,
                                  ConstStrings   &attribute_values,
                                  Error          &error);
  void     recap_end_element     (const String   &element_name,
                                  Error          &error);
  void     recap_text            (const String   &text,
                                  Error          &error);
  void     recap_pass_through    (const String   &pass_through_text,
                                  Error          &error);
};

} // Rapicorn

#endif  /* __RAPICORN_MARKUP_HH__ */
