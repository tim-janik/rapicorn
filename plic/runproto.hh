// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
namespace Plic {

template<class A1, class A2>
struct FieldTools<void (A1, A2)> {
  template<class Signal> static inline void
  emit (Plic::FieldBuffer &fb, Signal &sig)
  {
    const size_t NARGS = 2;
    Plic::FieldReader fbr (fb);
    fbr.skip_msgid(); // FIXME: check msgid
    fbr.skip();       // skip m_handler_id
    if (PLIC_UNLIKELY (fbr.remaining() != NARGS))
      Plic::error_printf ("invalid number of signal arguments");
    A1 a1; A2 a2;
    fbr >> a1; fbr >> a2;
    sig.emit (a1, a2);
  }
  A1 m_a1; A2 m_a2;
  FieldTools (A1 a1, A2 a2)
    : m_a1 (a1), m_a2 (a2)
  {
    FieldTools (m_a1, m_a2);
  }
};

} // Plic
