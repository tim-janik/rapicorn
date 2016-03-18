// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_TEXT_EDITOR_HH__
#define __RAPICORN_TEXT_EDITOR_HH__

#include <ui/container.hh>

namespace Rapicorn {

///< Enum type to discriminate various Text widget types.
enum TextMode {
  TEXT_MODE_WRAPPED = 1,
  TEXT_MODE_ELLIPSIZED,
  TEXT_MODE_SINGLE_LINE,
};

/// Configurable aspects about text paragraphs.
struct ParagraphState {
  Align         align;
  Ellipsize     ellipsize;
  double        line_spacing;
  double        indent;
  String        font_family;
  double        font_size; // absolute font size
  explicit      ParagraphState();
};

/// Configurable aspects about text characters.
struct TextAttrState {
  String   font_family;
  double   font_scale; // relative font size
  bool     bold;
  bool     italic;
  bool     underline;
  bool     small_caps;
  bool     strike_through;
  Color    foreground;
  Color    background;
  explicit TextAttrState();
};

/// Interface for editable text widgets.
class TextBlock : public virtual Aida::ImplicitBase {
protected:
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
typedef std::shared_ptr<TextBlock> TextBlockP;

/// Text layout controller supporting edits, selection and pasting.
class TextControllerImpl : public virtual SingleContainerImpl, public virtual EventHandler {
  int        cursor_;
  TextMode   text_mode_;
  bool       allow_edits_;
  TextBlockP cached_tblock_;
  size_t     tblock_sig_;
  String     next_markup_;
  uint       next_handler_;
  String     clipboard_;
  uint64     clipboard_nonce_, selection_nonce_, paste_nonce_;
  enum CursorMovement { NEXT_CHAR, PREV_CHAR, WARP_HOME, WARP_END, };
  TextBlock*            get_text_block          ();
  void                  update_text_block       ();
  bool                  move_cursor             (CursorMovement cm, const bool reset_selection);
  bool                  insert_literally        (const String &utf8text);
  bool                  select_all              ();
  bool                  delete_selection        ();
  bool                  delete_backward         ();
  bool                  delete_foreward         ();
  void                  selection_changed       ();
protected:
  explicit              TextControllerImpl      ();
  virtual              ~TextControllerImpl      () override;
  virtual void          construct               () override;
  virtual bool          can_focus               () const override;
  virtual void          reset                   (ResetMode mode = RESET_ALL) override;
  virtual bool          handle_event            (const Event &event) override;
  bool                  allow_edits             () const                { return allow_edits_; }
  void                  allow_edits             (bool allow_edits);
  int                   cursor                  () const                { return cursor_; }
  String                get_text                () const;
  void                  set_text                (const String &text);
  TextMode              get_mode                () const                { return text_mode_; }
  void                  set_mode                (TextMode text_mode);
  String                get_markup              () const;
  void                  set_markup              (const String &markup);
  String                get_plain               () const;
  void                  set_plain               (const String &ptext);
  double                text_requisition        (uint n_chars, uint n_digits);
  enum ChangesType { CURSOR = 1, TEXT = 2 };
  friend ChangesType    operator|               (ChangesType a, ChangesType b)  { return ChangesType (uint64 (a) | b); }
  virtual void          changes                 (ChangesType changes_flags)     {}
};

class LabelImpl : public virtual TextControllerImpl, public virtual LabelIface {
protected:
  virtual void     changes        (ChangesType changes_flags) override;
public:
  explicit         LabelImpl      ();
  virtual String   plain_text     () const override;
  virtual void     plain_text     (const String &ptext) override;
  virtual String   markup_text    () const override;
  virtual void     markup_text    (const String &markup) override;
};

class TextEditorImpl : public virtual TextControllerImpl, public virtual TextEditorIface {
  uint16           request_chars_, request_digits_;
protected:
  virtual void     size_request   (Requisition &requisition) override;
  virtual void     changes        (ChangesType changes_flags) override;
public:
  explicit         TextEditorImpl ();
  virtual String   plain_text     () const override;
  virtual void     plain_text     (const String &ptext) override;
  virtual String   markup_text    () const override;
  virtual void     markup_text    (const String &markup) override;
  virtual int      request_chars  () const override;
  virtual void     request_chars  (int nc) override;
  virtual int      request_digits () const override;
  virtual void     request_digits (int nd) override;
};

} // Rapicorn

#endif  /* __RAPICORN_TEXT_EDITOR_HH__ */
