// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "text-editor.hh"
#include "factory.hh"
#include "container.hh"

namespace Rapicorn {

// == ParagraphState ==
ParagraphState::ParagraphState() :
  align (Align::LEFT), ellipsize (Ellipsize::END),
  line_spacing (1), indent (0),
  font_family ("Sans"), font_size (12)
{}

// == TextAttrState ==
TextAttrState::TextAttrState() :
  font_family (""), font_scale (1.0),
  bold (false), italic (false), underline (false),
  small_caps (false), strike_through (false),
  foreground (0), background (0)
{}

// == TextBlock ==
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

// == TextControllerImpl ==
TextControllerImpl::TextControllerImpl() :
  cursor_ (0), text_mode_ (TEXT_MODE_SINGLE_LINE), allow_edits_ (false),
  internal_tblock_ (NULL), tblock_clink_ (0), tblock_sig_ (0), clipboard_nonce_ (0), selection_nonce_ (0), paste_nonce_ (0)
{}

TextControllerImpl::~TextControllerImpl()
{
  assert_return (internal_tblock_ == NULL); // only used while anchored
}

void
TextControllerImpl::construct()
{
  set_flag (NEEDS_FOCUS_INDICATOR, true); // prerequisite for focusable
  set_flag (ALLOW_FOCUS, true);
  SingleContainerImpl::construct();
  update_text_block();
}

void
TextControllerImpl::hierarchy_changed (WidgetImpl *old_toplevel)
{
  this->SingleContainerImpl::hierarchy_changed (old_toplevel);
  if (anchored() && !internal_tblock_)
    get_text_block(); // setup internal_tblock_
  if (!anchored() && internal_tblock_)
    reset_text_block (*dynamic_cast<WidgetImpl*> (internal_tblock_));
}

void
TextControllerImpl::reset_text_block (WidgetImpl &tblock)
{
  assert_return (dynamic_cast<WidgetImpl*> (internal_tblock_) == &tblock);
  if (tblock_sig_)
    internal_tblock_->sig_selection_changed() -= tblock_sig_;
  tblock_sig_ = 0;
  if (tblock_clink_)
    {
      widget_cross_unlink (*this, tblock, tblock_clink_);
      tblock_clink_ = 0;
    }
  internal_tblock_ = NULL;
}

TextBlock*
TextControllerImpl::get_text_block()
{
  return_unless (anchored(), NULL);
  if (internal_tblock_)
    return internal_tblock_;
  // find or create new text block
  const bool old_focusable = focusable(); // keep track for later change notification
  if (internal_tblock_)
    reset_text_block (*dynamic_cast<WidgetImpl*> (internal_tblock_));
  // find/create a text block candidate
  TextBlock *candidate = interface<TextBlock*>();
  WidgetImpl *tbwidget = dynamic_cast<WidgetImpl*> (candidate);
  if (!candidate || !tbwidget)
    {
      WidgetImplP widgetp = Factory::create_ui_child (*this, "Rapicorn_TextBlock", StringVector());
      tbwidget = widgetp.get();
      candidate = tbwidget->interface<TextBlock*>();
      assert_return (candidate != NULL, NULL);
    }
  // setup the new text block
  internal_tblock_ = candidate;
  tblock_clink_ = widget_cross_link (*this, *tbwidget, slot (*this, &TextControllerImpl::reset_text_block));
  tblock_sig_ = internal_tblock_->sig_selection_changed() += slot (this, &TextControllerImpl::selection_changed);
  update_text_block(); // configure text block properties
  internal_tblock_->markup_text (next_markup_);
  next_markup_.clear();
  // ensure we notify about can_focus/focusable changes
  if (old_focusable != focusable())
    changed ("flags");
  return &*internal_tblock_;
}

void
TextControllerImpl::update_text_block ()
{
  TextBlock *tblock = get_text_block();
  return_unless (tblock);
  tblock->text_mode (text_mode_);
  tblock->cursor2mark();
  cursor_ = tblock->mark();
  selection_changed();
}

bool
TextControllerImpl::can_focus () const
{
  return allow_edits_ && internal_tblock_ != NULL;
}

void
TextControllerImpl::allow_edits (bool allow_edits)
{
  if (allow_edits_ && !allow_edits)
    reset();
  allow_edits_ = allow_edits;
}

void
TextControllerImpl::reset (ResetMode mode)
{}

bool
TextControllerImpl::handle_event (const Event &event)
{
  return_unless (allow_edits_, false);
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
          const Point point = point_from_event (event);
          bool moved = tblock->mark_to_coord (point.x, point.y);
          int m = tblock->mark();
          if (o != m)
            {
              cursor_ = m;
              tblock->mark2cursor();
              changes (CURSOR);
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

bool
TextControllerImpl::move_cursor (CursorMovement cm, const bool reset_selection)
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
      changes (CURSOR);
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
  changes (CURSOR);
  return true;
}

bool
TextControllerImpl::insert_literally (const String &utf8text)
{
  return_unless (allow_edits_, false);
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
      changes (CURSOR | TEXT);
      return true;
    }
  return false;
}

bool
TextControllerImpl::select_all()
{
  TextBlock *tblock = get_text_block();
  return_unless (tblock, false);
  tblock->hide_selector();    // enforces selection_changed later on
  tblock->mark (-1);
  cursor_ = tblock->mark();
  tblock->mark2cursor();      // cursor might have been at end already
  tblock->mark (0);
  tblock->mark2selector();    // selects and forces selection_changed
  changes (CURSOR);
  return tblock->get_selection();
}

bool
TextControllerImpl::delete_selection()
{
  return_unless (allow_edits_, false);
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
  changes (CURSOR);
  return true;
}

bool
TextControllerImpl::delete_backward ()
{
  return_unless (allow_edits_, false);
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
      changes (CURSOR | TEXT);
      return true;
    }
  return false;
}

bool
TextControllerImpl::delete_foreward ()
{
  return_unless (allow_edits_, false);
  TextBlock *tblock = get_text_block();
  return_unless (tblock, false);
  if (delete_selection())
    return true;
  tblock->mark (cursor_);
  if (tblock->mark_at_end())
    return false;
  tblock->mark_delete (1);
  changes (CURSOR | TEXT);
  return true;
}

void
TextControllerImpl::selection_changed()
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

void
TextControllerImpl::set_markup (const String &text)
{
  next_markup_ = text;
  const bool adjust_existing_tblock = internal_tblock_ != NULL;
  TextBlock *tblock = get_text_block();
  if (tblock && adjust_existing_tblock)
    {
      tblock->markup_text (next_markup_);
      next_markup_.clear();
    }
}

String
TextControllerImpl::get_markup () const
{
  String result;
  if (internal_tblock_)
    result = internal_tblock_->markup_text();
  else
    result = next_markup_;
  return result;
}

String
TextControllerImpl::get_plain () const
{
  return XmlNode::strip_xml_tags (get_markup());
}

void
TextControllerImpl::set_plain (const String &ptext)
{
  String text = XmlNode::xml_escape (ptext);
  set_markup (text);
}

void
TextControllerImpl::set_mode (TextMode text_mode)
{
  text_mode_ = text_mode;
  invalidate_size();
  if (internal_tblock_)
    {
      /* this method maybe called from (derived) ctors, when the factory context
       * isn't yet setup. so guard text_block access by cached value to prevent
       * get_text_block/match_interface/Widget::id/factory_context_id: fc!=NULL
       * assertions.
       */
      update_text_block();
    }
}

double
TextControllerImpl::text_requisition (uint n_chars, uint n_digits)
{
  update_text_block();
  TextBlock *tblock = get_text_block();
  double req = 0;
  if (tblock)
    req += tblock->text_requisition (n_chars, n_digits);
  return req;
}

// == LabelImpl ==
LabelImpl::LabelImpl()
{
  allow_edits (false);
  set_mode (TEXT_MODE_ELLIPSIZED);
}

void
LabelImpl::changes (ChangesType changes_flags)
{
  TextControllerImpl::changes (changes_flags);
  if (changes_flags & CURSOR)
    ;
  if (changes_flags & TEXT)
    {
      changed ("plain_text");
      changed ("markup_text");
    }
}

String
LabelImpl::text () const
{
  assert_unreached();
}

void
LabelImpl::text (const String &string)
{
  const char end_tags_pattern[] = "</[buis]|br|tt|font|larger|smaller|fg|bg>";
  if (Regex::match_simple (end_tags_pattern, string, Regex::CASELESS, Regex::MATCH_NORMAL))
    markup_text (string);
  else
    plain_text (string);
}

String
LabelImpl::markup_text () const
{
  return get_markup();
}

void
LabelImpl::markup_text (const String &markup)
{
  set_markup (markup);
  changes (TEXT);
}

String
LabelImpl::plain_text () const
{
  return get_plain();
}

void
LabelImpl::plain_text (const String &ptext)
{
  set_plain (ptext);
  changes (TEXT);
}

static const WidgetFactory<LabelImpl> label_factory ("Rapicorn::Label");

// == TextEditorImpl ==
TextEditorImpl::TextEditorImpl() :
  request_chars_ (0), request_digits_ (0)
{
  set_mode (TEXT_MODE_WRAPPED);
  allow_edits (true);
}

void
TextEditorImpl::size_request (Requisition &requisition)
{
  TextControllerImpl::size_request (requisition);
  uint fallback_chars = 0, fallback_digits = 0;
  if (get_mode() == TEXT_MODE_SINGLE_LINE)
    {
      requisition.width = 0;
      if (request_chars_ <= 0 && request_digits_ <= 0)
        {
          fallback_chars = 26;
          fallback_digits = 10;
        }
    }
  requisition.width += text_requisition (fallback_chars + request_chars_, fallback_digits + request_digits_);
}

void
TextEditorImpl::changes (ChangesType changes_flags)
{
  TextControllerImpl::changes (changes_flags);
  if (changes_flags & CURSOR)
    ;
  if (changes_flags & TEXT)
    {
      changed ("plain_text");
      changed ("markup_text");
    }
}

int
TextEditorImpl::request_chars () const
{
  return request_chars_;
}

void
TextEditorImpl::request_chars (int nc)
{
  request_chars_ = CLAMP (nc, 0, 65535);
  invalidate_size();
  changed ("request_chars");
}

int
TextEditorImpl::request_digits () const
{
  return request_digits_;
}

void
TextEditorImpl::request_digits (int nd)
{
  request_digits_ = CLAMP (nd, 0, 65535);
  invalidate_size();
  changed ("request_digits");
}

String
TextEditorImpl::markup_text () const
{
  return get_markup();
}

void
TextEditorImpl::markup_text (const String &markup)
{
  set_markup (markup);
  changes (TEXT);
}

String
TextEditorImpl::plain_text () const
{
  return get_plain();
}

void
TextEditorImpl::plain_text (const String &ptext)
{
  set_plain (ptext);
  changes (TEXT);
}

static const WidgetFactory<TextEditorImpl> editor_factory ("Rapicorn::TextEditor");

} // Rapicorn
