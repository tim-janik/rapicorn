/* This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0 */
#ifndef __RAPICORN_SVG_TWEAK_H__
#define __RAPICORN_SVG_TWEAK_H__
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int      svg_tweak_debugging;

int             svg_tweak_point_tweak   (double vx, double vy, double *px, double *py,
                                         const double affine[6], const double iaffine[6]);
int             svg_tweak_point_simple  (double *px, double *py, const double affine[6], const double iaffine[6]);
cairo_matrix_t* svg_tweak_matrix        ();

#ifdef __cplusplus
};
#endif /* __cplusplus */
#endif /* __RAPICORN_SVG_TWEAK_H__ */
