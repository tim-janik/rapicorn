/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
#ifndef __RAPICORN_UTILITIES_HH__
#define __RAPICORN_UTILITIES_HH__

#include <rapicorn/birnetutils.hh>
#include <rapicorn/userconfig.hh>
#include <typeinfo>
#ifdef  RAPICORN_PRIVATE
#include <rapicorn/private.hh>
#endif

namespace Rapicorn {

/* --- import Birnet namespace --- */
using namespace Birnet;

/* --- i18n macros --- */
void            rapicorn_init           (int        *argcp,
                                         char     ***argvp,
                                         const char *app_name);
const char*     rapicorn_gettext        (const char *text);
RecMutex*       rapicorn_mutex          ();

#ifdef  RAPICORN_PRIVATE
#define _(str)  rapicorn_gettext (str)
#define N_(str) (str)
#endif

/* --- standard utlities --- */
using Birnet::abs;
using Birnet::clamp;
//template<typename T> inline const T& min   (const T &a, const T &b) { return ::std::min<T> (a, b); }
//template<typename T> inline const T& max   (const T &a, const T &b) { return ::std::min<T> (a, b); }
using ::std::min;
using ::std::max;
inline double min     (double a, int64  b) { return min<double> (a, b); }
inline double min     (int64  a, double b) { return min<double> (a, b); }
inline double max     (double a, int64  b) { return max<double> (a, b); }
inline double max     (int64  a, double b) { return max<double> (a, b); }
inline void   memset4 (uint32 *mem, uint32 filler, uint length) { Birnet::memset4 (mem, filler, length); }

/* --- Convertible --- */
class Convertible : public virtual ReferenceCountImpl {
public:
  /* interface matching base class */
  class InterfaceMatch {
    BIRNET_PRIVATE_CLASS_COPY (InterfaceMatch);
  public:
    explicit            InterfaceMatch  (const String &ident) : m_ident (ident), m_match_found (false) {}
    bool                done            () { return m_match_found; }
    virtual  bool       match           (Convertible  *object,
                                         const String &ident = String()) = 0;
    const String&       identifier      () { return m_ident; }
  protected:
    const String       &m_ident;
    bool                m_match_found;
  };
  typedef Signal<Convertible, Convertible* (const InterfaceMatch&, const String&), CollectorWhile0<Convertible*> > SignalFindInterface;
  /* SignalFindInterface sig_find_interface; */
private:
  /* interface matching base template class */
  template<typename Type>
  struct InterfaceCast : InterfaceMatch {
    typedef Type        Interface;
    explicit            InterfaceCast   (const String &ident) : InterfaceMatch (ident), m_instance (NULL) {}
    virtual bool        match           (Convertible *obj,
                                         const String &ident);
  protected:
    Interface          *m_instance;
  };
  /* interface matching template class implementation for Type, Type& and Type* */
  template<typename Type>
  struct InterfaceType : InterfaceCast<Type> {
    typedef Type&       Result;
    explicit            InterfaceType   (const String &ident) : InterfaceCast<Type> (ident) {}
    Type&               result          (bool may_throw);
  };
  template<typename Type>
  struct InterfaceType<Type&> : InterfaceType<Type> {
    explicit            InterfaceType   (const String &ident) : InterfaceType<Type> (ident) {}
  };
  template<typename Type>
  struct InterfaceType<Type*> : InterfaceCast<Type> {
    typedef Type*       Result;
    explicit            InterfaceType   (const String &ident) : InterfaceCast<Type> (ident) {}
    Type*               result          (bool may_throw);
  };
public: /* user API */
  explicit              Convertible     ();
  virtual bool          match_interface (InterfaceMatch &imatch) const;
  template<typename Type>
  typename
  InterfaceType<Type>::
  Result                interface       (const String         &ident = String(),
                                         const std::nothrow_t &nt = dothrow) const;
  template<typename Type>
  typename
  InterfaceType<Type>::
  Result                interface       (const std::nothrow_t &nt) const;
};

/* --- template implementations --- */
template<typename Type> bool
Convertible::InterfaceCast<Type>::match (Convertible  *obj,
                                         const String &ident)
{
  if (!m_instance)
    {
      const String &id = identifier();
      if (id.empty() || id == ident)
        {
          m_instance = dynamic_cast<Interface*> (obj);
          m_match_found = m_instance != NULL;
        }
    }
  return m_match_found;
}

template<typename Type> Type&
Convertible::InterfaceType<Type>::result (bool may_throw)
{
  if (!this->m_instance && may_throw)
    throw NullInterface();
  return *this->m_instance;
}

template<typename Type> Type*
Convertible::InterfaceType<Type*>::result (bool may_throw)
{
  return InterfaceCast<Type>::m_instance;
}

template<typename Type> typename Convertible::InterfaceType<Type>::Result
Convertible::interface (const String         &ident,
                        const std::nothrow_t &nt) const
{
  InterfaceType<Type> interface_type (ident);
  match_interface (interface_type);
  return interface_type.result (&nt == &dothrow);
}

template<typename Type> typename Convertible::InterfaceType<Type>::Result
Convertible::interface (const std::nothrow_t &nt) const
{
  return interface<Type> (String(), nt);
}

} // Rapicorn

#endif  /* __RAPICORN_UTILITIES_HH__ */
