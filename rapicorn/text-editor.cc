/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
  virtual int
  cursor () const
  {
    return m_cursor;
  }
  virtual void
  cursor (int pos)
  {
    m_cursor = MAX (0, pos);
    Client *client = get_client();
    if (client)
      {
        client->cursor (m_cursor);
        m_cursor = client->cursor();
      }
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
            cursor (cursor() + 1);
            handled = true;
            break;
          case KEY_Left:
            cursor (cursor() - 1);
            handled = true;
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
