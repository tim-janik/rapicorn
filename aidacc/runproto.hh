// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
namespace Aida {

template<class R, class A1, class A2>
class FieldTools<R (A1, A2)> {
  template<class Y> struct Type           { typedef Y T; };
  template<class Y> struct Type<Y&>       { typedef Y T; };
  template<class Y> struct Type<const Y&> { typedef Y T; };
public:
  template<class Signal> static inline R
  emit (Aida::FieldBuffer &fb, Signal &sig) /// Unpack FieldBuffer as signal arguments and issue emit().
  {
    const size_t NARGS = 2;
    Aida::FieldReader fbr (fb);
    fbr.skip_msgid(); // FIXME: check msgid
    fbr.skip();       // skip m_handler_id
    if (AIDA_UNLIKELY (fbr.remaining() != NARGS))
      Aida::error_printf ("invalid number of signal arguments");
    typename Type<A1>::T a1; typename Type<A2>::T a2;
    fbr >>= a1; fbr >>= a2;
    return sig.emit (a1, a2);
  }
};

} // Aida
