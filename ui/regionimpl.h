// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_REGION_IMPL_H__
#define __RAPICORN_REGION_IMPL_H__

#include <rcore/buildconfig.h>
#include <stdint.h>
#include <stdbool.h>

RAPICORN_EXTERN_C_BEGIN();

/* --- types & macros --- */
typedef signed long long int llint64_t;
typedef char llint64_size_assertion_t[-!(sizeof (llint64_t) == 8)];
typedef enum {
  RAPICORN_REGION_OUTSIDE = 0,
  RAPICORN_REGION_INSIDE  = 1,
  RAPICORN_REGION_PARTIAL = 2
} RapicornRegionCType;
typedef struct _RapicornRegion    RapicornRegion;
typedef struct {
  llint64_t x1, y1, x2, y2;
} RapicornRegionBox;
typedef struct {
  llint64_t x, y;
} RapicornRegionPoint;

/* --- functions --- */
RapicornRegion*		_rapicorn_region_create 	(void);
void			_rapicorn_region_free 		(RapicornRegion 	   *region);
void			_rapicorn_region_init		(RapicornRegion            *region,
							 int                        region_size);
void			_rapicorn_region_uninit		(RapicornRegion            *region);
void           		_rapicorn_region_copy 		(RapicornRegion		   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_clear 		(RapicornRegion 	   *region);
bool			_rapicorn_region_empty 		(const RapicornRegion 	   *region);
bool			_rapicorn_region_equal 		(const RapicornRegion 	   *region,
							 const RapicornRegion 	   *region2);
int			_rapicorn_region_cmp 		(const RapicornRegion 	   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_swap 		(RapicornRegion 	   *region,
							 RapicornRegion 	   *region2);
void			_rapicorn_region_extents 	(const RapicornRegion 	   *region,
							 RapicornRegionBox    	   *rect);
bool			_rapicorn_region_point_in 	(const RapicornRegion 	   *region,
							 const RapicornRegionPoint *point);
RapicornRegionCType	_rapicorn_region_rect_in 	(const RapicornRegion      *region,
							 const RapicornRegionBox   *rect);
RapicornRegionCType	_rapicorn_region_region_in 	(const RapicornRegion      *region,
							 const RapicornRegion      *region2);
int			_rapicorn_region_get_rects 	(const RapicornRegion 	   *region,
							 int                  	    n_rects,
							 RapicornRegionBox    	   *rects);
int			_rapicorn_region_get_rect_count	(const RapicornRegion 	   *region);
void			_rapicorn_region_union_rect	(RapicornRegion            *region,
							 const RapicornRegionBox   *rect);
void			_rapicorn_region_union 		(RapicornRegion       	   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_subtract 	(RapicornRegion       	   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_intersect 	(RapicornRegion       	   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_xor 		(RapicornRegion       	   *region,
							 const RapicornRegion 	   *region2);

RAPICORN_EXTERN_C_END();

#endif /* __RAPICORN_REGION_IMPL_H__ */
