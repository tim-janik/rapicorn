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
#ifdef  RAPICORN_PRIVATE
#include <rapicorn/private.hh>
#endif

namespace Rapicorn {

/* --- import Birnet namespace --- */
using namespace Birnet;
using Birnet::uint;     // resolve conflict with global uint

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
using Birnet::min;
using Birnet::max;
//template<typename T> inline const T& min   (const T &a, const T &b) { return ::std::min<T> (a, b); }
//template<typename T> inline const T& max   (const T &a, const T &b) { return ::std::min<T> (a, b); }
inline double min (double a, long   b) { return min<double> (a, b); }
inline double min (long   a, double b) { return min<double> (a, b); }
inline double max (double a, long   b) { return max<double> (a, b); }
inline double max (long   a, double b) { return max<double> (a, b); }

/* --- Convertible --- */
class Convertible : public virtual ReferenceCountImpl {
public: /* typedefs */
  struct InterfaceMatch {
    /*Con*/                     InterfaceMatch  () : match_found (false) {}
    bool                        done            () { return match_found; }
    virtual  bool               match           (Convertible *object) = 0;
  protected: bool               match_found;
  };
  typedef Signal<Convertible, Convertible* (const InterfaceMatch&, const String&), CollectorWhile0<Convertible*> > SignalFindInterface;
private:
  /* implement InterfaceCast template */
  template<typename Type>
  struct InterfaceCast : InterfaceMatch {
    typedef Type  Interface;
    explicit      InterfaceCast () : instance (NULL) {}
    virtual bool  match         (Convertible *obj)
    {
      if (!instance)
        instance = dynamic_cast<Interface*> (obj);
      return match_found = instance != NULL;
    }
  protected:
    Interface *instance;
  };
  /* implement InterfaceType template */
  template<typename Type> struct InterfaceType : InterfaceCast<Type> {
    typedef Type& Result;
    Type&         result  (bool may_throw)
    {
      if (!this->instance && may_throw)
        throw NullInterface();
      return *this->instance;
    }
  };
  template<typename Type> struct InterfaceType<Type&> : InterfaceType<Type> {};
  template<typename Type> struct InterfaceType<Type*> : InterfaceCast<Type> {
    typedef Type* Result;
    Type*         result  (bool may_throw) { return InterfaceCast<Type>::instance; }
  };
public: /* user API */
  explicit                      Convertible     ();
  SignalFindInterface           sig_find_interface;
  virtual bool                  match_interface (InterfaceMatch &imatch, const String &ident);
  template<typename Type>
  typename
  InterfaceType<Type>::Result   interface       (const String &ident = String(), const std::nothrow_t &nt = dothrow)
  {
    InterfaceType<Type> interface_type;
    match_interface (interface_type, ident);
    return interface_type.result (&nt == &dothrow);
  }
  template<typename Type>
  typename
  InterfaceType<Type>::Result   interface       (const std::nothrow_t &nt) { return interface<Type> (String(), nt); }
};

} // Rapicorn

#endif  /* __RAPICORN_UTILITIES_HH__ */
