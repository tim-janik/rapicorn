/* Rapicorn
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "text-editor.hh"
#include "factory.hh"
#include "container.hh"

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
Editor::Client::client_list_properties()
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
Editor::list_properties()
{
  static Property *properties[] = {
    MakeProperty (Editor, text_mode,   _("Text Mode"),   _("The basic text layout mechanism to use."), "rw"),
    MakeProperty (Editor, markup_text, _("Markup Text"), _("The text to display, containing font and style markup."), "rw"),
    MakeProperty (Editor, plain_text,  _("Plain Text"),  _("The text to display, without markup information."), "rw"),
    MakeProperty (Editor, request_chars,  _("Request Chars"),  _("Number of characters to request space for."), 0, INT_MAX, 2, "rw"),
    MakeProperty (Editor, request_digits, _("Request Digits"), _("Number of digits to request space for."), 0, INT_MAX, 2, "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::list_properties());
  return property_list;
}

class EditorImpl : public virtual SingleContainerImpl, public virtual Editor, public virtual EventHandler {
  uint     m_request_chars, m_request_digits;
  int      m_cursor;
  TextMode m_text_mode;
  Client*       get_client () const { return interface<Client*>(); }
public:
  EditorImpl() :
    m_request_chars (0), m_request_digits (0),
    m_cursor (0), m_text_mode (TEXT_MODE_SINGLE_LINE)
  {}
private:
  virtual void
  size_request (Requisition &requisition)
  {
    update_client();
    SingleContainerImpl::size_request (requisition);
    uint fallback_chars = 0, fallback_digits = 0;
    if (m_text_mode == TEXT_MODE_SINGLE_LINE)
      {
        requisition.width = 0;
        if (m_request_chars <= 0 && m_request_digits <= 0)
          {
            fallback_chars = 26;
            fallback_digits = 10;
          }
      }
    Client *client = get_client();
    if (client)
      requisition.width += client->text_requisition (fallback_chars + m_request_chars, fallback_digits + m_request_digits);
  }
  virtual bool
  can_focus () const
  {
    Client *client = get_client();
    return client != NULL;
  }
  virtual void
  reset (ResetMode mode = RESET_ALL)
  {}
  virtual bool
  handle_event (const Event &event)
  {
    bool handled = false;
    switch (event.type)
      {
        const EventKey *kevent;
      case KEY_PRESS:
        kevent = dynamic_cast<const EventKey*> (&event);
        switch (kevent->key)
          {
          case KEY_Home:    case KEY_KP_Home:           handled = move_cursor (WARP_HOME);      break;
          case KEY_End:     case KEY_KP_End:            handled = move_cursor (WARP_END);       break;
          case KEY_Right:   case KEY_KP_Right:          handled = move_cursor (NEXT_CHAR);      break;
          case KEY_Left:    case KEY_KP_Left:           handled = move_cursor (PREV_CHAR);      break;
          case KEY_BackSpace:                           handled = delete_backward();            break;
          case KEY_Delete:  case KEY_KP_Delete:         handled = delete_foreward();            break;
          default:
            if (!key_value_is_modifier (kevent->key))
              handled = insert_literally (key_value_to_unichar (kevent->key));
            break;
          }
        if (!handled && kevent->key_state & MOD_CONTROL && key_value_is_focus_dir (kevent->key) &&
            kevent->key != KEY_Left && kevent->key != KEY_Right) // keep Ctrl+Left & Ctrl+Right reserved
          handled = false; /* pass on Ctrl+FocusKey for focus handling */
        else if (!handled && !key_value_is_modifier (kevent->key))
          {
            notify_key_error();
            handled = true;
          }
        break;
      case KEY_CANCELED:
      case KEY_RELEASE:
        break;
      case BUTTON_PRESS:
        grab_focus();
        {
          Client *client = get_client();
          if (client)
            {
              const EventButton *bevent = dynamic_cast<const EventButton*> (&event);
              int o = client->mark();
              bool moved = client->mark_to_coord (bevent->x, bevent->y);
              int m = client->mark();
              if (o != m)
                {
                  m_cursor = m;
                  client->mark2cursor();
                  changed();
                }
              (void) moved;
            }
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
    return m_cursor;
  }
  bool
  move_cursor (CursorMovement cm)
  {
    Client *client = get_client();
    if (client)
      {
        client->mark (m_cursor);
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
        m_cursor = m;
        client->mark2cursor();
        changed();
        return true;
      }
    return false;
  }
  bool
  insert_literally (unichar uc)
  {
    Client *client = get_client();
    if (client && uc)
      {
        client->mark (m_cursor);
        char str[8];
        utf8_from_unichar (uc, str);
        client->mark_insert (str);
        move_cursor (NEXT_CHAR);
        changed();
        return true;
      }
    return false;
  }
  bool
  delete_backward ()
  {
    Client *client = get_client();
    if (client)
      {
        client->mark (m_cursor);
        int o = client->mark();
        client->step_mark (-1);
        int m = client->mark();
        if (o == m)
          return false;
        m_cursor = m;
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
    if (client)
      {
        client->mark (m_cursor);
        if (client->mark_at_end())
          return false;
        client->mark_delete (1);
        changed();
        return true;
      }
    return false;
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
    Client *client = get_client();
    return client ? client->markup_text() : "";
  }
  void
  update_client ()
  {
    // FIXME: this funciton and its callers may be optimized when we create our own text layouts
    Client *client = get_client();
    if (client)
      {
        client->text_mode (m_text_mode);
        // client->markup_text (markup);
      }
  }
  virtual TextMode text_mode      () const                      { return m_text_mode; }
  virtual void     text_mode      (TextMode      text_mode)
  {
    m_text_mode = text_mode;
    Client *client = get_client();
    if (client)
      client->text_mode (m_text_mode);
    invalidate_size();
  }
  virtual String   markup_text    () const                      { Client *client = get_client(); return client ? client->markup_text() : ""; }
  virtual void     markup_text    (const String &markup)        { Client *client = get_client(); if (client) client->markup_text (markup); }
  virtual String   plain_text     () const                      { Client *client = get_client(); return client ? client->plain_text() : ""; }
  virtual void     plain_text     (const String &ptext)         { Client *client = get_client(); if (client) client->plain_text (ptext); }
  virtual uint     request_chars  () const                      { return m_request_chars; }
  virtual void     request_chars  (uint nc)                     { m_request_chars = nc; invalidate_size(); }
  virtual uint     request_digits () const                      { return m_request_digits; }
  virtual void     request_digits (uint nd)                     { m_request_digits = nd; invalidate_size(); }
};
static const ItemFactory<EditorImpl> editor_factory ("Rapicorn::Factory::Text::Editor");


} // Text
} // Rapicorn
