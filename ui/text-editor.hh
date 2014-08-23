// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_TEXT_EDITOR_HH__
#define __RAPICORN_TEXT_EDITOR_HH__

#include <ui/container.hh>

namespace Rapicorn {

struct ParagraphState {
  AlignType     align;
  EllipsizeType ellipsize;
  double        line_spacing;
  double        indent;
  String        font_family;
  double        font_size; /* absolute font size */
  explicit      ParagraphState();
};

struct TextAttrState {
  String   font_family;
  double   font_scale; /* relative font size */
  bool     bold;
  bool     italic;
  bool     underline;
  bool     small_caps;
  bool     strike_through;
  Color    foreground;
  Color    background;
  explicit TextAttrState();
};

class TextBlock {
protected:
  const PropertyList& text_block_property_list();
  virtual String      save_markup  () const = 0;
  virtual void        load_markup  (const String &markup) = 0;
public:
  virtual            ~TextBlock    ();
  virtual const char* peek_text    (int *byte_length) = 0;                    ///< Peek at the memory containng plain text.
  virtual ParagraphState para_state () const = 0;                             ///< Retrieve paragraph settings for the text.
  virtual void           para_state (const ParagraphState &pstate) = 0;       ///< Assign paragraph settings for the text.
  virtual TextAttrState  attr_state () const = 0;
  virtual void           attr_state (const TextAttrState &astate) = 0;
  // properties
  virtual String      markup_text  () const;                                  ///< Serialize text to markup.
  virtual void        markup_text  (const String &markup);                    ///< Clear and assign new text from markup.
  virtual String      plain_text   () const;                                  ///< Retrieve text without markup.
  virtual void        plain_text   (const String &ptext);                     ///< Clear text and assign new plain text.
  virtual TextMode    text_mode    () const = 0;                              ///< Get text layout mode.
  virtual void        text_mode    (TextMode text_mode) = 0;                  ///< Set wrapped/ellipsized/single-line mode.
  // size negotiation
  virtual double      text_requisition (uint n_chars, uint n_digits) = 0;     ///< Estimate size for a number of characters and digits.
  // mark handling
  virtual int         mark            () const = 0;                           ///< Get byte index of text editing mark.
  virtual void        mark            (int byte_index) = 0;                   ///< Set the byte index of the text editing mark.
  virtual bool        mark_at_end     () const = 0;                           ///< Test wether mark is at text buffer bound.
  virtual bool        mark_to_coord   (double x, double y) = 0;               ///< Jump mark to x/y coordinates, returns if moved.
  virtual void        step_mark       (int visual_direction) = 0;             ///< Move mark forward (+1) or backward (-1).
  virtual void        mark2cursor     () = 0;                                 ///< Jump the cursor position to the current mark.
  virtual void        cursor2mark     () = 0;                                 ///< Set the mark to the current cursor position.
  virtual void        hide_cursor     () = 0;                                 ///< Disable cursor in text area.
  virtual void        mark2selector   () = 0;                                 ///< Set selection mark, counterpart to cursor.
  virtual void        hide_selector   () = 0;                                 ///< Unset any current selection, preserves cursor.
  virtual bool        get_selection   (int *start = NULL, int *end = NULL,
                                       int *nutf8 = NULL) = 0;                ///< Get selection positions between cursor and selector.
  virtual void        mark_delete     (uint n_utf8_chars) = 0;                ///< Forward delete characters following mark.
  virtual void        mark_insert     (String utf8string,
                                       const TextAttrState *astate = NULL) = 0; ///< Insert characters before mark.
  // notifications
  Aida::Signal<void ()> sig_selection_changed;        ///< Notification signal for operations that affected the selection.
};

class TextController : public virtual ContainerImpl {
protected:
  virtual const PropertyList& __aida_properties__();
public:
  virtual void          text           (const String &text) = 0;
  virtual String        text           () const = 0;
  virtual TextMode      text_mode      () const = 0;
  virtual void          text_mode      (TextMode      text_mode) = 0;
  virtual String        markup_text    () const = 0;
  virtual void          markup_text    (const String &markup) = 0;
  virtual String        plain_text     () const = 0;
  virtual void          plain_text     (const String &ptext) = 0;
  virtual int           request_chars  () const = 0;
  virtual void          request_chars  (int nc) = 0;
  virtual int           request_digits () const = 0;
  virtual void          request_digits (int nd) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_TEXT_EDITOR_HH__ */
