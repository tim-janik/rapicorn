/* Rapicorn
 * Copyright (C) 2007 Tim Janik
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
#ifndef __RAPICORN_WINDOW_HH__
#define __RAPICORN_WINDOW_HH__

#include <rapicorn/primitives.hh>

namespace Rapicorn {

class Root;
class Window : virtual public Deletable {
  Root         &m_root;
  OwnedMutex   &m_omutex;
  Window&       operator=       (const Window   &window);
protected:
  explicit      Window          (Root           &root,
                                 OwnedMutex     &omutex);
public:
  /* reference ops */
  void          lock            ()      { m_omutex.lock(); }
  bool          trylock         ()      { return m_omutex.trylock(); }
  void          unlock          ()      { m_omutex.unlock(); }
  Root&         root            ();     /* must be locked */
  Root*         peek_root       ();
  /* window ops */
  bool          visible         ();
  void          show            ();
  void          hide            ();
  bool          closed          ();
  void          close           ();
  /* class */
  /*Con*/       Window          (const Window   &window);
  virtual      ~Window          ();
}; 

} // Rapicorn

#endif  /* __RAPICORN_WINDOW_HH__ */
