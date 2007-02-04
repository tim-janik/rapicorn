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
#include "containerimpl.hh"

namespace Rapicorn {
namespace Text {
using namespace Birnet;

ParaState::ParaState() :
  align (ALIGN_LEFT), wrap (WRAP_WORD), ellipsize (ELLIPSIZE_END),
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

class EditorImpl : public virtual EventHandler, public virtual SingleContainerImpl, public virtual Editor {
  int m_cursor;
public:
  EditorImpl() :
    m_cursor (0)
  {}
private:
  Client*       get_client () const { return interface<Client*>(); }
  int
  cursor () const
  {
    return m_cursor;
  }
  bool
  step_cursor (int dir)
  {
    Client *client = get_client();
    if (client)
      {
        client->mark (m_cursor);
        int o = client->mark();
        client->step_mark (dir);
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
        step_cursor (+1);
        changed();
      }
    return true;
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
      client->load_markup (text);
  }
  virtual String
  text () const
  {
    Client *client = get_client();
    return client ? client->save_markup() : "";
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
          case KEY_Right:
            handled = step_cursor (+1);
            break;
          case KEY_Left:
            handled = step_cursor (-1);
            break;
          case KEY_BackSpace:
            handled = delete_backward();
            break;
          case KEY_Delete: case KEY_KP_Delete:
            handled = delete_foreward();
            break;
          default:
            if (!key_value_is_modifier (kevent->key))
              handled = insert_literally (key_value_to_unichar (kevent->key));
            break;
          }
        break;
      case KEY_CANCELED:
      case KEY_RELEASE:
        break;
      case BUTTON_PRESS:
        grab_focus();
        break;
      default: ;
      }
    return handled;
  }
};
static const ItemFactory<EditorImpl> editor_factory ("Rapicorn::Factory::Text::Editor");


} // Text
} // Rapicorn
