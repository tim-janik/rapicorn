// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "text-editor.hh"
#include "factory.hh"
#include "container.hh"

namespace Rapicorn {

typedef enum {
  NEXT_CHAR,
  PREV_CHAR,
  WARP_HOME,
  WARP_END,
} CursorMovement;

ParagraphState::ParagraphState() :
  align (ALIGN_LEFT), ellipsize (ELLIPSIZE_END),
  line_spacing (1), indent (0),
  font_family ("Sans"), font_size (12)
{}

TextAttrState::TextAttrState() :
  font_family (""), font_scale (1.0),
  bold (false), italic (false), underline (false),
  small_caps (false), strike_through (false),
  foreground (0), background (0)
{}

TextBlock::~TextBlock ()
{}

String
TextBlock::markup_text () const
{
  return save_markup();
}

void
TextBlock::markup_text (const String &markup)
{
  load_markup (markup);
}

static String
escape_xml (const String &input)
{
  String d;
  for (String::const_iterator it = input.begin(); it != input.end(); it++)
    switch (*it)
      {
      case '"':         d += "&quot;";  break;
      case '&':         d += "&amp;";   break;
      case '\'':        d += "&apos;";  break;
      case '<':         d += "&lt;";    break;
      case '>':         d += "&gt;";    break;
      default:          d += *it;       break;
      }
  return d;
}


void
TextBlock::plain_text (const String &markup)
{
  load_markup (escape_xml (markup));
}

String
TextBlock::plain_text () const
{
  int byte_length = 0;
  const char *t = const_cast<TextBlock*> (this)->peek_text (&byte_length);
  return String (t, byte_length);
}

const PropertyList&
TextBlock::text_block_property_list()
{
  static Property *properties[] = {
    MakeProperty (TextBlock, markup_text, _("Markup Text"), _("The text to display, containing font and style markup."), "rw"),
    MakeProperty (TextBlock, plain_text,  _("Plain Text"),  _("The text to display, without markup information."), "rw"),
    MakeProperty (TextBlock, text_mode,   _("Text Mode"),   _("The basic text layout mechanism to use."), "rw"),
  };
  static const PropertyList property_list (properties);
  return property_list;
}


const PropertyList&
TextController::__aida_properties__()
{
  static Property *properties[] = {
    MakeProperty (TextController, text_mode,   _("Text Mode"),   _("The basic text layout mechanism to use."), "rw"),
    MakeProperty (TextController, markup_text, _("Markup Text"), _("The text to display, containing font and style markup."), "rw"),
    MakeProperty (TextController, plain_text,  _("Plain Text"),  _("The text to display, without markup information."), "rw"),
    MakeProperty (TextController, request_chars,  _("Request Chars"),  _("Number of characters to request space for."), 0, INT_MAX, 2, "rw"),
    MakeProperty (TextController, request_digits, _("Request Digits"), _("Number of digits to request space for."), 0, INT_MAX, 2, "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::__aida_properties__());
  return property_list;
}

class TextControllerImpl : public virtual SingleContainerImpl, public virtual TextController, public virtual EventHandler {
  uint16   request_chars_, request_digits_;
  int      cursor_;
  TextMode text_mode_;
  TextBlock *cached_tblock_;
  size_t   tblock_sig_;
  String   clipboard_;
  uint64   clipboard_nonce_, selection_nonce_, paste_nonce_;
public:
  TextControllerImpl() :
    request_chars_ (0), request_digits_ (0), cursor_ (0), text_mode_ (TEXT_MODE_SINGLE_LINE),
    cached_tblock_ (NULL), tblock_sig_ (0), clipboard_nonce_ (0), selection_nonce_ (0), paste_nonce_ (0)
  {}
protected:
  virtual void
  do_changed (const String &name) override
  {
    SingleContainerImpl::do_changed (name);
    if (name == "text")
      {
        changed ("markup_text");  // notify aliasing properties
        ObjectImpl *tblock_object = dynamic_cast<ObjectImpl*> (get_text_block());
        if (tblock_object)
          {
            tblock_object->changed ("markup_text");  // bad hack, need tighter coupling between TextBlock and TextController
          }
      }
  }
private:
  ~TextControllerImpl()
  {
    if (cached_tblock_)
      {
        cached_tblock_->sig_selection_changed() -= tblock_sig_;
        tblock_sig_ = 0;
        ReferenceCountable *trash = dynamic_cast<WidgetImpl*> (cached_tblock_);
        cached_tblock_ = NULL;
        if (trash)
          unref (trash);
      }
  }
  TextBlock*
  get_text_block()
  {
    // check if text block changed
    TextBlock *candidate = interface<TextBlock*>();
    if (cached_tblock_ == candidate)
      return cached_tblock_;
    // adjust to new text block
    TextBlock *const old = cached_tblock_;
    const size_t old_sig = tblock_sig_;
    ReferenceCountable *base = dynamic_cast<WidgetImpl*> (candidate);
    cached_tblock_ = base ? candidate : NULL;
    if (base)
      {
        ref (base);
        tblock_sig_ = cached_tblock_->sig_selection_changed() += Aida::slot (this, &TextControllerImpl::selection_changed);
      }
    else
      tblock_sig_ = 0;
    // cleanup old
    base = dynamic_cast<WidgetImpl*> (old);
    if (base)
      {
        old->sig_selection_changed() -= old_sig;
        unref (base);
      }
    // update new text block
    if (cached_tblock_)
      update_text_block();
    return cached_tblock_;
  }
  void
  update_text_block ()
  {
    TextBlock *tblock = get_text_block();
    return_unless (tblock);
    tblock->text_mode (text_mode_);
    tblock->cursor2mark();
    cursor_ = tblock->mark();
    selection_changed();
  }
  virtual void
  size_request (Requisition &requisition)
  {
    update_text_block();
    SingleContainerImpl::size_request (requisition);
    uint fallback_chars = 0, fallback_digits = 0;
    if (text_mode_ == TEXT_MODE_SINGLE_LINE)
      {
        requisition.width = 0;
        if (request_chars_ <= 0 && request_digits_ <= 0)
          {
            fallback_chars = 26;
            fallback_digits = 10;
          }
      }
    TextBlock *tblock = get_text_block();
    if (tblock)
      requisition.width += tblock->text_requisition (fallback_chars + request_chars_, fallback_digits + request_digits_);
  }
  virtual bool
  can_focus () const
  {
    TextBlock *tblock = cached_tblock_;
    return tblock != NULL;
  }
  virtual void
  reset (ResetMode mode = RESET_ALL)
  {}
  virtual bool
  handle_event (const Event &event)
  {
    bool handled = false, ignore = false;
    switch (event.type)
      {
        TextBlock *tblock;
        bool rs;
        const EventKey *kevent;
        const EventData *devent;
        const EventButton *bevent;
      case KEY_PRESS:
        kevent = dynamic_cast<const EventKey*> (&event);
        rs = (kevent->key_state & MOD_SHIFT) == 0; // reset selection
        switch (kevent->key)
          {
          case 'a':
            if (kevent->key_state & MOD_CONTROL)
              {
                select_all();
                handled = true;
                break;
              }
            goto _default;
          case 'c':
            if (kevent->key_state & MOD_CONTROL)
              {
                clipboard_ = "";
                TextBlock *tblock = get_text_block();
                if (tblock)
                  {
                    int start, end, nutf8;
                    const bool has_selection = tblock->get_selection (&start, &end, &nutf8);
                    String text = tblock->plain_text();
                    if (has_selection && nutf8 > 0 && size_t (end) <= text.size())
                      {
                        text = text.substr (start, end - start);
                        if (!text.empty() && utf8_validate (text))
                          clipboard_ = text;
                      }
                  }
                if (clipboard_.empty())
                  {
                    if (clipboard_nonce_)
                      disown_content (CONTENT_SOURCE_CLIPBOARD, clipboard_nonce_);
                    clipboard_nonce_ = 0;
                  }
                else
                  {
                    clipboard_nonce_ = random_nonce();
                    own_content (CONTENT_SOURCE_CLIPBOARD, clipboard_nonce_, cstrings_to_vector ("text/plain", NULL));
                  }
                handled = true;
                break;
              }
            goto _default;
          case 'v':
            if (kevent->key_state & MOD_CONTROL)
              {
                paste_nonce_ = random_nonce();
                request_content (CONTENT_SOURCE_CLIPBOARD, paste_nonce_, "text/plain");
                handled = true;
                break;
              }
            goto _default;
          case KEY_Home:    case KEY_KP_Home:           handled = move_cursor (WARP_HOME, rs);  break;
          case KEY_End:     case KEY_KP_End:            handled = move_cursor (WARP_END, rs);   break;
          case KEY_Right:   case KEY_KP_Right:          handled = move_cursor (NEXT_CHAR, rs);  break; // FIXME: CTRL moves words
          case KEY_Left:    case KEY_KP_Left:           handled = move_cursor (PREV_CHAR, rs);  break; // FIXME: CTRL moves words
          case KEY_BackSpace:                           handled = delete_backward();            break;
          case KEY_Delete:  case KEY_KP_Delete:         handled = delete_foreward();            break;
          default: _default:
            if (kevent->key_state & MOD_CONTROL &&      // preserve Ctrl + FocusKey for navigation
                key_value_is_focus_dir (kevent->key))
              {
                handled = false;
                ignore = true;
              }
            else
              handled = insert_literally (kevent->utf8input);
            break;
          }
        if (!handled && !ignore && !key_value_is_modifier (kevent->key))
          {
            notify_key_error();
            handled = true;
          }
        break;
      case KEY_CANCELED:
      case KEY_RELEASE:
        break;
      case CONTENT_DATA:
        devent = dynamic_cast<const EventData*> (&event);
        if (devent->nonce == paste_nonce_)
          {
            if (devent->data_type == "text/plain")
              handled = insert_literally (devent->data);
            paste_nonce_ = 0;
          }
        break;
      case CONTENT_CLEAR:
        devent = dynamic_cast<const EventData*> (&event);
        tblock = get_text_block();
        if (selection_nonce_ && devent->nonce == selection_nonce_)
          {
            if (tblock && tblock->get_selection())
              tblock->hide_selector();
            selection_nonce_ = 0;
            handled = true;
          }
        else if (clipboard_nonce_ && devent->nonce == clipboard_nonce_)
          {
            if (!clipboard_.empty())
              clipboard_ = "";
            clipboard_nonce_ = 0;
            handled = true;
          }
        break;
      case CONTENT_REQUEST:
        devent = dynamic_cast<const EventData*> (&event);
        tblock = get_text_block();
        if (selection_nonce_ && devent->nonce == selection_nonce_ && tblock && devent->data_type == "text/plain")
          {
            int start, end;
            const bool has_selection = tblock->get_selection (&start, &end);
            if (has_selection && start >= 0 && end > start)
              {
                String text = tblock->plain_text();
                if (size_t (end) <= text.size())
                  {
                    text = text.substr (start, end - start);
                    if (utf8_validate (text))
                      {
                        provide_content ("text/plain", text, devent->request_id);
                        handled = true;
                      }
                  }
              }
          }
        else if (clipboard_nonce_ && devent->nonce == clipboard_nonce_)
          {
            if (devent->data_type == "text/plain")
              {
                provide_content (clipboard_.empty() ? "" : "text/plain", clipboard_, devent->request_id);
                handled = true;
              }
          }
        break;
      case BUTTON_PRESS:
        bevent = dynamic_cast<const EventButton*> (&event);
        tblock = get_text_block();
        grab_focus();
        if (tblock && bevent->button == 1)
          {
            int o = tblock->mark();
            bool moved = tblock->mark_to_coord (bevent->x, bevent->y);
            int m = tblock->mark();
            if (o != m)
              {
                cursor_ = m;
                tblock->mark2cursor();
                changed ("cursor");
              }
            (void) moved;
          }
        else if (bevent->button == 2)
          {
            paste_nonce_ = random_nonce();
            request_content (CONTENT_SOURCE_SELECTION, paste_nonce_, "text/plain");
          }
        handled = true;
        break;
      default: ;
      }
    return handled;
  }
  int
  cursor () const
  {
    return cursor_;
  }
  bool
  move_cursor (CursorMovement cm, const bool reset_selection)
  {
    const bool adjust_selection = !reset_selection;
    TextBlock *tblock = get_text_block();
    return_unless (tblock, false);
    int start, end;
    const bool has_selection = tblock->get_selection (&start, &end);
    // special case, cursor left/right deselects
    if (has_selection && reset_selection && (cm == PREV_CHAR || cm == NEXT_CHAR))
      {
        tblock->mark (cm == PREV_CHAR ? start : end);
        tblock->hide_selector();
        tblock->mark2cursor();
        cursor_ = tblock->mark();
        changed ("cursor");
        return true;
      }
    tblock->mark (cursor_);
    if (!has_selection && adjust_selection)
      tblock->mark2selector();                      // old cursor starts selection
    int o = tblock->mark();
    switch (cm)
      {
      case NEXT_CHAR:       tblock->step_mark (+1); break;
      case PREV_CHAR:       tblock->step_mark (-1); break;
      case WARP_HOME:       tblock->mark (0);       break;
      case WARP_END:        tblock->mark (-1);      break;
      }
    int m = tblock->mark();
    if (o == m)
      return false;
    if (reset_selection && has_selection)
      tblock->hide_selector();
    cursor_ = m;
    tblock->mark2cursor();
    changed ("cursor");
    return true;
  }
  bool
  insert_literally (const String &utf8text)
  {
    if (utf8text.size() == 1 &&
        (utf8text[0] == '\b' || // Backspace
         utf8text[0] == '\n' || // Newline
         utf8text[0] == '\r' || // Carriage Return
         utf8text[0] == 0x7f || // Delete
         0))
      return false;     // ignore literal inputs from "control" keys
    TextBlock *tblock = get_text_block();
    if (tblock && !utf8text.empty())
      {
        delete_selection();
        tblock->mark (cursor_);
        tblock->mark_insert (utf8text);
        cursor_ = tblock->mark();
        tblock->mark2cursor();
        changed ("text");
        changed ("cursor");
        return true;
      }
    return false;
  }
  bool
  select_all()
  {
    TextBlock *tblock = get_text_block();
    return_unless (tblock, false);
    tblock->hide_selector();    // enforces selection_changed later on
    tblock->mark (-1);
    cursor_ = tblock->mark();
    tblock->mark2cursor();      // cursor might have been at end already
    tblock->mark (0);
    tblock->mark2selector();    // selects and forces selection_changed
    changed ("cursor");
    return tblock->get_selection();
  }
  bool
  delete_selection()
  {
    TextBlock *tblock = get_text_block();
    return_unless (tblock, false);
    int start, end, nutf8;
    const bool has_selection = tblock->get_selection (&start, &end, &nutf8);
    if (!has_selection)
      return false;
    tblock->mark (start);
    tblock->mark_delete (nutf8);
    tblock->hide_selector();
    cursor_ = tblock->mark();
    tblock->mark2cursor();
    changed ("cursor");
    return true;
  }
  bool
  delete_backward ()
  {
    if (delete_selection())
      return true;
    TextBlock *tblock = get_text_block();
    if (tblock)
      {
        tblock->mark (cursor_);
        int o = tblock->mark();
        tblock->step_mark (-1);
        int m = tblock->mark();
        if (o == m)
          return false;
        cursor_ = m;
        tblock->mark2cursor();
        tblock->mark_delete (1);
        changed ("text");
        changed ("cursor");
        return true;
      }
    return false;
  }
  bool
  delete_foreward ()
  {
    TextBlock *tblock = get_text_block();
    return_unless (tblock, false);
    if (delete_selection())
      return true;
    tblock->mark (cursor_);
    if (tblock->mark_at_end())
      return false;
    tblock->mark_delete (1);
    changed ("text");
    changed ("cursor");
    return true;
  }
  void
  selection_changed()
  {
    TextBlock *tblock = get_text_block();
    int start, end, nutf8;
    const bool has_selection = tblock->get_selection (&start, &end, &nutf8);
    if (!has_selection || nutf8 < 1)
      {
        if (selection_nonce_)
          disown_content (CONTENT_SOURCE_SELECTION, selection_nonce_);
        selection_nonce_ = 0;
      }
    else
      {
        selection_nonce_ = random_nonce();
        own_content (CONTENT_SOURCE_SELECTION, selection_nonce_, cstrings_to_vector ("text/plain", NULL)); // claim new selection
      }
  }
  virtual void
  text (const String &text)
  {
    TextBlock *tblock = get_text_block();
    if (tblock)
      tblock->markup_text (text);
  }
  virtual String
  text () const
  {
    return cached_tblock_ ? cached_tblock_->markup_text() : "";
  }
  virtual TextMode text_mode      () const                      { return text_mode_; }
  virtual void     text_mode      (TextMode      text_mode)
  {
    text_mode_ = text_mode;
    TextBlock *tblock = get_text_block();
    if (tblock)
      tblock->text_mode (text_mode_);
    invalidate_size();
  }
  virtual String   markup_text    () const                      { return cached_tblock_ ? cached_tblock_->markup_text() : ""; }
  virtual void     markup_text    (const String &markup)        { TextBlock *tblock = get_text_block(); if (tblock) tblock->markup_text (markup); }
  virtual String   plain_text     () const                      { return cached_tblock_ ? cached_tblock_->plain_text() : ""; }
  virtual void     plain_text     (const String &ptext)         { TextBlock *tblock = get_text_block(); if (tblock) tblock->plain_text (ptext); }
  virtual int      request_chars  () const                      { return request_chars_; }
  virtual void     request_chars  (int nc)                      { request_chars_ = CLAMP (nc, 0, 65535); invalidate_size(); }
  virtual int      request_digits () const                      { return request_digits_; }
  virtual void     request_digits (int nd)                      { request_digits_ = CLAMP (nd, 0, 65535); invalidate_size(); }
};

static const WidgetFactory<TextControllerImpl> editor_factory ("Rapicorn::Factory::TextEditor");

} // Rapicorn
