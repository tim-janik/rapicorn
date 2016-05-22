// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CAIROUTILS_HH__
#define __RAPICORN_CAIROUTILS_HH__

#include <cairo.h>

// === Convenience Macro Abbreviations ===
#ifdef RAPICORN_CONVENIENCE
#define CAIRO_CHECK_STATUS(cairox)              RAPICORN_CAIRO_CHECK_STATUS (cairox)
#endif // RAPICORN_CONVENIENCE

#define RAPICORN_CAIRO_CHECK_STATUS(cairox)             ({              \
    cairo_status_t ___s = Rapicorn::cairo_status_generic (cairox);      \
    const bool ___status_success = ___s == CAIRO_STATUS_SUCCESS;        \
    if (!___status_success)                                             \
      RAPICORN_CRITICAL ("cairo status (%s): %s", #cairox,              \
                         cairo_status_to_string (___s));                \
    ___status_success;                                                  \
  })

namespace Rapicorn {

template<class T> static inline
cairo_status_t            cairo_status_generic (T t);
template<> cairo_status_t cairo_status_generic (cairo_status_t c)          { return c; }
template<> cairo_status_t cairo_status_generic (cairo_rectangle_list_t *c) { return c ? c->status : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_generic (cairo_font_options_t *c)   { return c ? cairo_font_options_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_generic (cairo_font_face_t *c)      { return c ? cairo_font_face_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_generic (cairo_scaled_font_t *c)    { return c ? cairo_scaled_font_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_generic (cairo_path_t *c)           { return c ? c->status : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_generic (cairo_t *c)                { return c ? cairo_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_generic (cairo_device_t *c)         { return c ? cairo_device_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_generic (cairo_surface_t *c)        { return c ? cairo_surface_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_generic (cairo_pattern_t *c)        { return c ? cairo_pattern_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_generic (const cairo_region_t *c)   { return c ? cairo_region_status (c) : CAIRO_STATUS_NULL_POINTER; }

} // Rapicorn

#endif /* __RAPICORN_CAIROUTILS_HH__ */
