/* Rapicorn region handling
 * Copyright (C) 2006 Tim Janik
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
#ifndef __RAPICORN_REGION_IMPL_H__
#define __RAPICORN_REGION_IMPL_H__

#include <rapicorn/birnetcdefs.h>

BIRNET_EXTERN_C_BEGIN();

/* --- types & macros --- */
typedef enum {
  RAPICORN_REGION_OUTSIDE = 0,
  RAPICORN_REGION_INSIDE  = 1,
  RAPICORN_REGION_PARTIAL = 2
} RapicornRegionCType;
typedef struct _RapicornRegion    RapicornRegion;
typedef struct {
  BirnetInt64 x1, y1, x2, y2;
} RapicornRegionBox;
typedef struct {
  BirnetInt64 x, y;
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
void			_rapicorn_region_debug 		(const char 		   *format,
							 ...) BIRNET_PRINTF (1, 2);

BIRNET_EXTERN_C_END();

#endif /* __RAPICORN_REGION_IMPL_H__ */
