// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "text-editor.hh"
#include "factory.hh"
#include "container.hh"
#include "window.hh" // FIXME: unneeded

namespace Rapicorn {
namespace Text {
using namespace Rapicorn;

typedef enum {
  NEXT_CHAR,
  PREV_CHAR,
  WARP_HOME,
  WARP_END,
} CursorMovement;

ParaState::ParaState() :
  align (ALIGN_LEFT), ellipsize (ELLIPSIZE_END),
  line_spacing (1), indent (0),
  font_family ("Sans"), font_size (12)
{}

AttrState::AttrState() :
  font_family (""), font_scale (1.0),
  bold (false), italic (false), underline (false),
  small_caps (false), strike_through (false),
  foreground (0), background (0)
{}

Editor::Client::~Client ()
{}

String
Editor::Client::markup_text () const
{
  return save_markup();
}

void
Editor::Client::markup_text (const String &markup)
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
Editor::Client::plain_text (const String &markup)
{
  load_markup (escape_xml (markup));
}

String
Editor::Client::plain_text () const
{
  int byte_length = 0;
  const char *t = const_cast<Client*> (this)->peek_text (&byte_length);
  return String (t, byte_length);
}

const PropertyList&
Editor::Client::client_property_list()
{
  static Property *properties[] = {
    MakeProperty (Client, markup_text, _("Markup Text"), _("The text to display, containing font and style markup."), "rw"),
    MakeProperty (Client, plain_text,  _("Plain Text"),  _("The text to display, without markup information."), "rw"),
    MakeProperty (Client, text_mode,   _("Text Mode"),   _("The basic text layout mechanism to use."), "rw"),
  };
  static const PropertyList property_list (properties);
  return property_list;
}


const PropertyList&
Editor::__aida_properties__()
{
  static Property *properties[] = {
    MakeProperty (Editor, text_mode,   _("Text Mode"),   _("The basic text layout mechanism to use."), "rw"),
    MakeProperty (Editor, markup_text, _("Markup Text"), _("The text to display, containing font and style markup."), "rw"),
    MakeProperty (Editor, plain_text,  _("Plain Text"),  _("The text to display, without markup information."), "rw"),
    MakeProperty (Editor, request_chars,  _("Request Chars"),  _("Number of characters to request space for."), 0, INT_MAX, 2, "rw"),
    MakeProperty (Editor, request_digits, _("Request Digits"), _("Number of digits to request space for."), 0, INT_MAX, 2, "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::__aida_properties__());
  return property_list;
}

class EditorImpl : public virtual SingleContainerImpl, public virtual Editor, public virtual EventHandler {
  uint     request_chars_, request_digits_;
  int      cursor_;
  TextMode text_mode_;
  Client  *cached_client_;
  size_t   client_sig_;
public:
  EditorImpl() :
    request_chars_ (0), request_digits_ (0),
    cursor_ (0), text_mode_ (TEXT_MODE_SINGLE_LINE), cached_client_ (NULL), client_sig_ (0)
  {}
private:
  ~EditorImpl()
  {
    if (cached_client_)
      {
        cached_client_->sig_selection_changed() -= client_sig_;
        client_sig_ = 0;
        ReferenceCountable *trash = dynamic_cast<WidgetImpl*> (cached_client_);
        cached_client_ = NULL;
        if (trash)
          unref (trash);
      }
  }
  Client*
  get_client()
  {
    // check if text client changed
    Client *candidate = interface<Client*>();
    if (cached_client_ == candidate)
      return cached_client_;
    // adjust to new client
    Client *const old = cached_client_;
    const size_t old_sig = client_sig_;
    ReferenceCountable *base = dynamic_cast<WidgetImpl*> (candidate);
    cached_client_ = base ? candidate : NULL;
    if (base)
      {
        ref (base);
        client_sig_ = cached_client_->sig_selection_changed() += Aida::slot (this, &EditorImpl::selection_changed);
      }
    else
      client_sig_ = 0;
    // cleanup old
    base = dynamic_cast<WidgetImpl*> (old);
    if (base)
      {
        old->sig_selection_changed() -= old_sig;
        unref (base);
      }
    // update new client
    if (cached_client_)
      update_client();
    return cached_client_;
  }
  void
  update_client ()
  {
    Client *client = get_client();
    return_unless (client);
    client->text_mode (text_mode_);
    client->cursor2mark();
    cursor_ = client->mark();
    selection_changed();
  }
  virtual void
  size_request (Requisition &requisition)
  {
    update_client();
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
    Client *client = get_client();
    if (client)
      requisition.width += client->text_requisition (fallback_chars + request_chars_, fallback_digits + request_digits_);
  }
  virtual bool
  can_focus () const
  {
    Client *client = cached_client_;
    return client != NULL;
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
        Client *client;
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
          case 'v':
            if (kevent->key_state & MOD_CONTROL)
              {
                get_window()->request_clipboard (77, "text/plain"); // FIXME: should operate on viewport
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
        if (devent->nonce == 77 && devent->data_type == "text/plain")
          {
            handled = insert_literally (devent->data);
          }
        break;
      case CONTENT_REQUEST:
        devent = dynamic_cast<const EventData*> (&event);
        client = get_client();
        if (client && devent->data_type == "text/plain")
          {
            int start, end;
            const bool has_selection = client->get_selection (&start, &end);
            if (has_selection && start >= 0 && end > start)
              {
                String text = client->plain_text();
                if (size_t (end) <= text.size())
                  {
                    text = text.substr (start, end - start);
                    if (utf8_validate (text))
                      get_window()->provide_selection (devent->nonce, "text/plain", text);
                  }
              }
          }
        break;
      case BUTTON_PRESS:
        bevent = dynamic_cast<const EventButton*> (&event);
        client = get_client();
        grab_focus();
        if (client && bevent->button == 1)
          {
            int o = client->mark();
            bool moved = client->mark_to_coord (bevent->x, bevent->y);
            int m = client->mark();
            if (o != m)
              {
                cursor_ = m;
                client->mark2cursor();
                changed();
              }
            (void) moved;
          }
        else if (bevent->button == 2)
          {
            get_window()->request_selection (77, "text/plain"); // FIXME: should operate on viewport
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
    Client *client = get_client();
    return_unless (client, false);
    int start, end;
    const bool has_selection = client->get_selection (&start, &end);
    // special case, cursor left/right deselects
    if (has_selection && reset_selection && (cm == PREV_CHAR || cm == NEXT_CHAR))
      {
        client->mark (cm == PREV_CHAR ? start : end);
        client->hide_selector();
        client->mark2cursor();
        cursor_ = client->mark();
        changed();
        return true;
      }
    client->mark (cursor_);
    if (!has_selection && adjust_selection)
      client->mark2selector();                      // old cursor starts selection
    int o = client->mark();
    switch (cm)
      {
      case NEXT_CHAR:       client->step_mark (+1); break;
      case PREV_CHAR:       client->step_mark (-1); break;
      case WARP_HOME:       client->mark (0);       break;
      case WARP_END:        client->mark (-1);      break;
      }
    int m = client->mark();
    if (o == m)
      return false;
    if (reset_selection && has_selection)
      client->hide_selector();
    cursor_ = m;
    client->mark2cursor();
    changed();
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
    Client *client = get_client();
    if (client && !utf8text.empty())
      {
        delete_selection();
        client->mark (cursor_);
        client->mark_insert (utf8text);
        cursor_ = client->mark();
        client->mark2cursor();
        changed();
        return true;
      }
    return false;
  }
  bool
  select_all()
  {
    Client *client = get_client();
    return_unless (client, false);
    client->mark (0);
    client->mark2selector();
    client->mark (-1);
    cursor_ = client->mark();
    client->mark2cursor();
    changed();
    return client->get_selection();
  }
  bool
  delete_selection()
  {
    Client *client = get_client();
    return_unless (client, false);
    int start, end, nutf8;
    const bool has_selection = client->get_selection (&start, &end, &nutf8);
    if (!has_selection)
      return false;
    client->mark (start);
    client->mark_delete (nutf8);
    client->hide_selector();
    cursor_ = client->mark();
    client->mark2cursor();
    changed();
    return true;
  }
  bool
  delete_backward ()
  {
    if (delete_selection())
      return true;
    Client *client = get_client();
    if (client)
      {
        client->mark (cursor_);
        int o = client->mark();
        client->step_mark (-1);
        int m = client->mark();
        if (o == m)
          return false;
        cursor_ = m;
        client->mark2cursor();
        client->mark_delete (1);
        changed();
        return true;
      }
    return false;
  }
  bool
  delete_foreward ()
  {
    Client *client = get_client();
    return_unless (client, false);
    if (delete_selection())
      return true;
    client->mark (cursor_);
    if (client->mark_at_end())
      return false;
    client->mark_delete (1);
    changed();
    return true;
  }
  void
  selection_changed()
  {
    Client *client = get_client();
    int start, end, nutf8;
    const bool has_selection = client->get_selection (&start, &end, &nutf8);
    if (!has_selection || nutf8 < 1)
      get_window()->claim_selection (StringVector()); // give up ownership
    else
      get_window()->claim_selection (cstrings_to_vector ("text/plain", NULL)); // claim new selection
  }
  virtual void
  text (const String &text)
  {
    Client *client = get_client();
    if (client)
      client->markup_text (text);
  }
  virtual String
  text () const
  {
    return cached_client_ ? cached_client_->markup_text() : "";
  }
  virtual TextMode text_mode      () const                      { return text_mode_; }
  virtual void     text_mode      (TextMode      text_mode)
  {
    text_mode_ = text_mode;
    Client *client = get_client();
    if (client)
      client->text_mode (text_mode_);
    invalidate_size();
  }
  virtual String   markup_text    () const                      { return cached_client_ ? cached_client_->markup_text() : ""; }
  virtual void     markup_text    (const String &markup)        { Client *client = get_client(); if (client) client->markup_text (markup); }
  virtual String   plain_text     () const                      { return cached_client_ ? cached_client_->plain_text() : ""; }
  virtual void     plain_text     (const String &ptext)         { Client *client = get_client(); if (client) client->plain_text (ptext); }
  virtual uint     request_chars  () const                      { return request_chars_; }
  virtual void     request_chars  (uint nc)                     { request_chars_ = nc; invalidate_size(); }
  virtual uint     request_digits () const                      { return request_digits_; }
  virtual void     request_digits (uint nd)                     { request_digits_ = nd; invalidate_size(); }
};
static const WidgetFactory<EditorImpl> editor_factory ("Rapicorn::Factory::Text::Editor");


} // Text
} // Rapicorn
