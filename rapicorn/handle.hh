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
#ifndef __RAPICORN_HANDLE_HH__
#define __RAPICORN_HANDLE_HH__

#include <rapicorn/primitives.hh>

namespace Rapicorn {

/* --- Handle<> --- */
template<class Client> class Handle;
/* Handle<&> */
template<class Client>
class Handle<Client&> {
  Client     &m_client;
  OwnedMutex &m_omutex;
  Handle&     operator= (const Handle &handle);
public:
  explicit    Handle    (Client       &client,
                         OwnedMutex   &omutex);
  /*Copy*/    Handle    (const Handle &handle);
  void        lock      ()                      { m_omutex.lock(); }
  bool        trylock   ()                      { return m_omutex.trylock(); }
  Client&     get       ()                      { return *get (dothrow); }
  Client*     get       (const nothrow_t &nt);
  void        unlock    ()                      { m_omutex.unlock(); }
  /*Des*/     ~Handle   ();
};
/* Handle<class> - alias to Handle<&> */
template<class Client> struct Handle : Handle<Client&> {
  explicit Handle (Client       &client,
                   OwnedMutex   &omutex) : Handle<Client&> (client, omutex) {}
  /*Copy*/ Handle (const Handle &handle) : Handle<Client&> (handle)         {}
};
/* Handle<*> */
template<class Client> class Handle<Client*> : TEMPLATE_ERROR::InvalidType<Client*> {}; // not supported

/* --- implementation --- */
template<class Client>
Handle<Client&>::Handle (Client       &client,
                         OwnedMutex   &omutex) :
  m_client (ref_sink (client)),
  m_omutex (omutex)
{}

template<class Client>
Handle<Client&>::Handle (const Handle &handle) :
  m_client (ref (handle.m_client)),
  m_omutex (handle.m_omutex)
{}

template<class Client> Client*
Handle<Client&>::get (const nothrow_t &nt)
{
  if (m_omutex.mine())
    return &m_client;
  if (&nt == &dothrow)
    throw Exception ("Unsyncronized handle access in thread: ", Thread::Self::name());
  else
    return NULL;
}

template<class Client>
Handle<Client&>::~Handle ()
{
  unref (m_client);
}

} // Rapicorn

#endif  /* __RAPICORN_HANDLE_HH__ */
