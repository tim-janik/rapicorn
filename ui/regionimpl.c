// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

/* Rapicorn region handling
 * Copyright 2006 Tim Janik
 *
 * This file contains a version of xorg-server-X11R7.1-1.1.0/mi/miregion.c.
 * The copyright notices have been left intact.
 * Edits were made to support 64bit coordinates, to avoid symbol exports
 * and to remove/configure code portions.
 */
#include "regionimpl.h"
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* --- compat stuff --- */
typedef llint64_t               Xint64;
#define FALSE                   false
#define TRUE                    true
#define INLINE                  inline
#define _X_EXPORT               static
#define Bool                    bool
#define xalloc                  malloc
#define xrealloc                realloc
#define xfree                   free
#define min(a,b)                ((a) < (b) ? (a) : (b))
#define max(a,b)                ((a) > (b) ? (a) : (b))

/* --- DEC / Open Group stuff starts --- */
/***********************************************************

Copyright 1987, 1988, 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 

Copyright 1987, 1988, 1989 by 
Digital Equipment Corporation, Maynard, Massachusetts. 

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/


/* --- types & macros --- */
#define rgnOUT  RAPICORN_REGION_OUTSIDE
#define rgnIN   RAPICORN_REGION_INSIDE
#define rgnPART RAPICORN_REGION_PARTIAL
typedef struct _RapicornRegion  RegionRec;
typedef struct _RapicornRegion *RegionPtr;
typedef RapicornRegionBox       BoxRec;
typedef RapicornRegionBox      *BoxPtr;
typedef struct _RegData {
  long        size;
  long        numRects;
  /*  BoxRec      rects[size];   in memory but not explicitly declared */
} RegDataRec, *RegDataPtr;
struct _RapicornRegion {
  BoxRec      extents;
  RegDataPtr  data;
};
#define REGION_NIL(reg) ((reg)->data && !(reg)->data->numRects)
/* not a region */
#define REGION_NAR(reg) ((reg)->data == &miBrokenData)
#define REGION_NUM_RECTS(reg) ((reg)->data ? (reg)->data->numRects : 1)
#define REGION_SIZE(reg) ((reg)->data ? (reg)->data->size : 0)
#define REGION_RECTS(reg) ((reg)->data ? (BoxPtr)((reg)->data + 1) \
                                       : &(reg)->extents)
#define REGION_BOXPTR(reg) ((BoxPtr)((reg)->data + 1))
#define REGION_BOX(reg,i) (&REGION_BOXPTR(reg)[i])
#define REGION_TOP(reg) REGION_BOX(reg, (reg)->data->numRects)
#define REGION_END(reg) REGION_BOX(reg, (reg)->data->numRects - 1)
#define REGION_SZOF(n) (sizeof(RegDataRec) + ((n) * sizeof(BoxRec)))

#define ErrorF(...) fprintf (stderr, __VA_ARGS__)
#define good(reg) do { if (!miValidRegion(reg)) { ErrorF ("InvalidRegion: %p:\n", reg); miPrintRegion (reg); } assert(miValidRegion(reg)); } while (0)

/*
 * The functions in this file implement the Region abstraction used extensively
 * throughout the X11 sample server. A Region is simply a set of disjoint
 * (non-overlapping) rectangles, plus an "extent" rectangle which is the
 * smallest single rectangle that contains all the non-overlapping rectangles.
 *
 * A Region is implemented as a "y-x-banded" array of rectangles.  This array
 * imposes two degrees of order.  First, all rectangles are sorted by top side
 * y coordinate first (y1), and then by left side x coordinate (x1).
 *
 * Furthermore, the rectangles are grouped into "bands".  Each rectangle in a
 * band has the same top y coordinate (y1), and each has the same bottom y
 * coordinate (y2).  Thus all rectangles in a band differ only in their left
 * and right side (x1 and x2).  Bands are implicit in the array of rectangles:
 * there is no separate list of band start pointers.
 *
 * The y-x band representation does not minimize rectangles.  In particular,
 * if a rectangle vertically crosses a band (the rectangle has scanlines in 
 * the y1 to y2 area spanned by the band), then the rectangle may be broken
 * down into two or more smaller rectangles stacked one atop the other. 
 *
 *  -----------				    -----------
 *  |         |				    |         |		    band 0
 *  |         |  --------		    -----------  --------
 *  |         |  |      |  in y-x banded    |         |  |      |   band 1
 *  |         |  |      |  form is	    |         |  |      |
 *  -----------  |      |		    -----------  --------
 *               |      |				 |      |   band 2
 *               --------				 --------
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible: no two rectangles within a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course).
 *
 * Adam de Boor wrote most of the original region code.  Joel McCormack
 * substantially modified or rewrote most of the rcore arithmetic routines,
 * and added miRegionValidate in order to support several speed improvements
 * to miValidateTree.  Bob Scheifler changed the representation to be more
 * compact when empty or a single rectangle, and did a bunch of gratuitous
 * reformatting.
 */

/*  true iff two Boxes overlap */
#define EXTENTCHECK(r1,r2) \
      (!( ((r1)->x2 <= (r2)->x1)  || \
          ((r1)->x1 >= (r2)->x2)  || \
          ((r1)->y2 <= (r2)->y1)  || \
          ((r1)->y1 >= (r2)->y2) ) )

/* true iff (x,y) is in Box */
#define INBOX(r,x,y) \
      ( ((r)->x2 >  x) && \
        ((r)->x1 <= x) && \
        ((r)->y2 >  y) && \
        ((r)->y1 <= y) )

/* true iff Box r1 contains Box r2 */
#define SUBSUMES(r1,r2) \
      ( ((r1)->x1 <= (r2)->x1) && \
        ((r1)->x2 >= (r2)->x2) && \
        ((r1)->y1 <= (r2)->y1) && \
        ((r1)->y2 >= (r2)->y2) )

#define xallocData(n) (RegDataPtr)xalloc(REGION_SZOF(n))
#define xfreeData(reg) if ((reg)->data && (reg)->data->size) xfree((reg)->data)

#define RECTALLOC_BAIL(pReg,n,bail) \
if (!(pReg)->data || (((pReg)->data->numRects + (n)) > (pReg)->data->size)) \
    if (!miRectAlloc(pReg, n)) { goto bail; }

#define RECTALLOC(pReg,n) \
if (!(pReg)->data || (((pReg)->data->numRects + (n)) > (pReg)->data->size)) \
    if (!miRectAlloc(pReg, n)) { return FALSE; }

#define ADDRECT(pNextRect,nx1,ny1,nx2,ny2)	\
{						\
    pNextRect->x1 = nx1;			\
    pNextRect->y1 = ny1;			\
    pNextRect->x2 = nx2;			\
    pNextRect->y2 = ny2;			\
    pNextRect++;				\
}

#define NEWRECT(pReg,pNextRect,nx1,ny1,nx2,ny2)			\
{									\
    if (!(pReg)->data || ((pReg)->data->numRects == (pReg)->data->size))\
    {									\
	if (!miRectAlloc(pReg, 1))					\
	    return FALSE;						\
	pNextRect = REGION_TOP(pReg);					\
    }									\
    ADDRECT(pNextRect,nx1,ny1,nx2,ny2);					\
    pReg->data->numRects++;						\
    assert(pReg->data->numRects<=pReg->data->size);			\
}


#define DOWNSIZE(reg,numRects)						 \
if (((numRects) < ((reg)->data->size >> 1)) && ((reg)->data->size > 50)) \
{									 \
    RegDataPtr NewData;							 \
    NewData = (RegDataPtr)xrealloc((reg)->data, REGION_SZOF(numRects));	 \
    if (NewData)							 \
    {									 \
	NewData->size = (numRects);					 \
	(reg)->data = NewData;						 \
    }									 \
}


_X_EXPORT BoxRec miEmptyBox = {0, 0, 0, 0};
_X_EXPORT RegDataRec miEmptyData = {0, 0};

static RegDataRec  miBrokenData = {0, 0};
static RegionRec   miBrokenRegion = { { 0, 0, 0, 0 }, &miBrokenData };

static int
miPrintRegion(rgn)
    RegionPtr rgn;
{
    int num, size;
    register int i;
    BoxPtr rects;

    num = REGION_NUM_RECTS(rgn);
    size = REGION_SIZE(rgn);
    rects = REGION_RECTS(rgn);
    ErrorF("num: %d size: %d\n", num, size);
    ErrorF("extents: %lld %lld %lld %lld\n",
	   rgn->extents.x1, rgn->extents.y1, rgn->extents.x2, rgn->extents.y2);
    for (i = 0; i < num; i++)
      ErrorF("%lld %lld %lld %lld \n",
	     rects[i].x1, rects[i].y1, rects[i].x2, rects[i].y2);
    ErrorF("\n");
    return(num);
}

_X_EXPORT Bool
miRegionEqual(reg1, reg2)
    RegionPtr reg1;
    RegionPtr reg2;
{
    int i, num;
    BoxPtr rects1, rects2;

    if (reg1->extents.x1 != reg2->extents.x1) return FALSE;
    if (reg1->extents.x2 != reg2->extents.x2) return FALSE;
    if (reg1->extents.y1 != reg2->extents.y1) return FALSE;
    if (reg1->extents.y2 != reg2->extents.y2) return FALSE;

    num = REGION_NUM_RECTS(reg1);
    if (num != REGION_NUM_RECTS(reg2)) return FALSE;
    
    rects1 = REGION_RECTS(reg1);
    rects2 = REGION_RECTS(reg2);
    for (i = 0; i != num; i++) {
	if (rects1[i].x1 != rects2[i].x1) return FALSE;
	if (rects1[i].x2 != rects2[i].x2) return FALSE;
	if (rects1[i].y1 != rects2[i].y1) return FALSE;
	if (rects1[i].y2 != rects2[i].y2) return FALSE;
    }
    return TRUE;
}

static Bool
miValidRegion(reg)
    RegionPtr reg;
{
    register int i, numRects;

    if ((reg->extents.x1 > reg->extents.x2) ||
	(reg->extents.y1 > reg->extents.y2))
	return FALSE;
    numRects = REGION_NUM_RECTS(reg);
    if (!numRects)
	return ((reg->extents.x1 == reg->extents.x2) &&
		(reg->extents.y1 == reg->extents.y2) &&
		(reg->data->size || (reg->data == &miEmptyData)));
    else if (numRects == 1)
	return (!reg->data);
    else
    {
	register BoxPtr pboxP, pboxN;
	BoxRec box;

	pboxP = REGION_RECTS(reg);
	box = *pboxP;
	box.y2 = pboxP[numRects-1].y2;
	pboxN = pboxP + 1;
	for (i = numRects; --i > 0; pboxP++, pboxN++)
	{
	    if ((pboxN->x1 >= pboxN->x2) ||
		(pboxN->y1 >= pboxN->y2))
		return FALSE;
	    if (pboxN->x1 < box.x1)
	        box.x1 = pboxN->x1;
	    if (pboxN->x2 > box.x2)
		box.x2 = pboxN->x2;
	    if ((pboxN->y1 < pboxP->y1) ||
		((pboxN->y1 == pboxP->y1) &&
		 ((pboxN->x1 < pboxP->x2) || (pboxN->y2 != pboxP->y2))))
		return FALSE;
	}
	return ((box.x1 == reg->extents.x1) &&
		(box.x2 == reg->extents.x2) &&
		(box.y1 == reg->extents.y1) &&
		(box.y2 == reg->extents.y2));
    }
}


/*****************************************************************
 *   RegionCreate(rect, size)
 *     This routine does a simple malloc to make a structure of
 *     REGION of "size" number of rectangles.
 *****************************************************************/

_X_EXPORT RegionPtr
miRegionCreate(rect, size)
    BoxPtr rect;
    int size;
{
    register RegionPtr pReg;
   
    pReg = (RegionPtr)xalloc(sizeof(RegionRec));
    if (!pReg)
	return &miBrokenRegion;
    if (rect)
    {
	pReg->extents = *rect;
	pReg->data = (RegDataPtr)NULL;
    }
    else
    {
	pReg->extents = miEmptyBox;
	if ((size > 1) && (pReg->data = xallocData(size)))
	{
	    pReg->data->size = size;
	    pReg->data->numRects = 0;
	}
	else
	    pReg->data = &miEmptyData;
    }
    return(pReg);
}

/*****************************************************************
 *   RegionInit(pReg, rect, size)
 *     Outer region rect is statically allocated.
 *****************************************************************/

_X_EXPORT void
miRegionInit(pReg, rect, size)
    RegionPtr pReg;
    BoxPtr rect;
    int size;
{
    if (rect)
    {
	pReg->extents = *rect;
	pReg->data = (RegDataPtr)NULL;
    }
    else
    {
	pReg->extents = miEmptyBox;
	if ((size > 1) && (pReg->data = xallocData(size)))
	{
	    pReg->data->size = size;
	    pReg->data->numRects = 0;
	}
	else
	    pReg->data = &miEmptyData;
    }
}

_X_EXPORT void
miRegionDestroy(pReg)
    RegionPtr pReg;
{
    good(pReg);
    xfreeData(pReg);
    if (pReg != &miBrokenRegion)
	xfree(pReg);
}

_X_EXPORT void
miRegionUninit(pReg)
     RegionPtr pReg;
{
  good(pReg);
  xfreeData(pReg);
}

static Bool
miRegionBreak (pReg)
     RegionPtr pReg;
{
  xfreeData (pReg);
  pReg->extents = miEmptyBox;
  pReg->data = &miBrokenData;
  return FALSE;
}

_X_EXPORT Bool
miRectAlloc(
            register RegionPtr pRgn,
            int n)
{
  RegDataPtr  data;

  if (!pRgn->data)
    {
      n++;
      pRgn->data = xallocData(n);
      if (!pRgn->data)
        return miRegionBreak (pRgn);
      pRgn->data->numRects = 1;
      *REGION_BOXPTR(pRgn) = pRgn->extents;
    }
  else if (!pRgn->data->size)
    {
      pRgn->data = xallocData(n);
      if (!pRgn->data)
        return miRegionBreak (pRgn);
      pRgn->data->numRects = 0;
    }
  else
    {
      if (n == 1)
        {
          n = pRgn->data->numRects;
          if (n > 500) /* XXX pick numbers out of a hat */
            n = 250;
        }
      n += pRgn->data->numRects;
      data = (RegDataPtr)xrealloc(pRgn->data, REGION_SZOF(n));
      if (!data)
        return miRegionBreak (pRgn);
      pRgn->data = data;
    }
  pRgn->data->size = n;
  return TRUE;
}

_X_EXPORT Bool
miRegionCopy(dst, src)
     register RegionPtr dst;
     register RegionPtr src;
{
  good(dst);
  good(src);
  if (dst == src)
    return TRUE;
  dst->extents = src->extents;
  if (!src->data || !src->data->size)
    {
      xfreeData(dst);
      dst->data = src->data;
      return TRUE;
    }
  if (!dst->data || (dst->data->size < src->data->numRects))
    {
      xfreeData(dst);
      dst->data = xallocData(src->data->numRects);
      if (!dst->data)
        return miRegionBreak (dst);
      dst->data->size = src->data->numRects;
    }
  dst->data->numRects = src->data->numRects;
  memmove((char *)REGION_BOXPTR(dst),(char *)REGION_BOXPTR(src),
          dst->data->numRects * sizeof(BoxRec));
  return TRUE;
}


/*======================================================================
 *	    Generic Region Operator
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miCoalesce --
 *	Attempt to merge the boxes in the current band with those in the
 *	previous one.  We are guaranteed that the current band extends to
 *      the end of the rects array.  Used only by miRegionOp.
 *
 * Results:
 *	The new index for the previous band.
 *
 * Side Effects:
 *	If coalescing takes place:
 *	    - rectangles in the previous band will have their y2 fields
 *	      altered.
 *	    - pReg->data->numRects will be decreased.
 *
 *-----------------------------------------------------------------------
 */
INLINE static int
miCoalesce (
    register RegionPtr	pReg,	    	/* Region to coalesce		     */
    int	    	  	prevStart,  	/* Index of start of previous band   */
    int	    	  	curStart)   	/* Index of start of current band    */
{
    register BoxPtr	pPrevBox;   	/* Current box in previous band	     */
    register BoxPtr	pCurBox;    	/* Current box in current band       */
    register int  	numRects;	/* Number rectangles in both bands   */
    register Xint64	y2;		/* Bottom of current band	     */
    /*
     * Figure out how many rectangles are in the band.
     */
    numRects = curStart - prevStart;
    assert(numRects == pReg->data->numRects - curStart);

    if (!numRects) return curStart;

    /*
     * The bands may only be coalesced if the bottom of the previous
     * matches the top scanline of the current.
     */
    pPrevBox = REGION_BOX(pReg, prevStart);
    pCurBox = REGION_BOX(pReg, curStart);
    if (pPrevBox->y2 != pCurBox->y1) return curStart;

    /*
     * Make sure the bands have boxes in the same places. This
     * assumes that boxes have been added in such a way that they
     * cover the most area possible. I.e. two boxes in a band must
     * have some horizontal space between them.
     */
    y2 = pCurBox->y2;

    do {
	if ((pPrevBox->x1 != pCurBox->x1) || (pPrevBox->x2 != pCurBox->x2)) {
	    return (curStart);
	}
	pPrevBox++;
	pCurBox++;
	numRects--;
    } while (numRects);

    /*
     * The bands may be merged, so set the bottom y of each box
     * in the previous band to the bottom y of the current band.
     */
    numRects = curStart - prevStart;
    pReg->data->numRects -= numRects;
    do {
	pPrevBox--;
	pPrevBox->y2 = y2;
	numRects--;
    } while (numRects);
    return prevStart;
}


/* Quicky macro to avoid trivial reject procedure calls to miCoalesce */

#define Coalesce(newReg, prevBand, curBand)				\
    if (curBand - prevBand == newReg->data->numRects - curBand) {	\
	prevBand = miCoalesce(newReg, prevBand, curBand);		\
    } else {								\
	prevBand = curBand;						\
    }

/*-
 *-----------------------------------------------------------------------
 * miAppendNonO --
 *	Handle a non-overlapping band for the union and subtract operations.
 *      Just adds the (top/bottom-clipped) rectangles into the region.
 *      Doesn't have to check for subsumption or anything.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	pReg->data->numRects is incremented and the rectangles overwritten
 *	with the rectangles we're passed.
 *
 *-----------------------------------------------------------------------
 */

INLINE static Bool
miAppendNonO (
    register RegionPtr	pReg,
    register BoxPtr	r,
    BoxPtr  	  	rEnd,
    register Xint64  	y1,
    register Xint64  	y2)
{
    register BoxPtr	pNextRect;
    register int	newRects;

    newRects = rEnd - r;

    assert(y1 < y2);
    assert(newRects != 0);

    /* Make sure we have enough space for all rectangles to be added */
    RECTALLOC(pReg, newRects);
    pNextRect = REGION_TOP(pReg);
    pReg->data->numRects += newRects;
    do {
	assert(r->x1 < r->x2);
	ADDRECT(pNextRect, r->x1, y1, r->x2, y2);
	r++;
    } while (r != rEnd);

    return TRUE;
}

#define FindBand(r, rBandEnd, rEnd, ry1)		    \
{							    \
    ry1 = r->y1;					    \
    rBandEnd = r+1;					    \
    while ((rBandEnd != rEnd) && (rBandEnd->y1 == ry1)) {   \
	rBandEnd++;					    \
    }							    \
}

#define	AppendRegions(newReg, r, rEnd)					\
{									\
    int newRects;							\
    if ((newRects = rEnd - r)) {					\
	RECTALLOC(newReg, newRects);					\
	memmove((char *)REGION_TOP(newReg),(char *)r, 			\
              newRects * sizeof(BoxRec));				\
	newReg->data->numRects += newRects;				\
    }									\
}

/*-
 *-----------------------------------------------------------------------
 * miRegionOp --
 *	Apply an operation to two regions. Called by miUnion, miInverse,
 *	miSubtract, miIntersect....  Both regions MUST have at least one
 *      rectangle, and cannot be the same object.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *	The new region is overwritten.
 *	pOverlap set to TRUE if overlapFunc ever returns TRUE.
 *
 * Notes:
 *	The idea behind this function is to view the two regions as sets.
 *	Together they cover a rectangle of area that this function divides
 *	into horizontal bands where points are covered only by one region
 *	or by both. For the first case, the nonOverlapFunc is called with
 *	each the band and the band's upper and lower extents. For the
 *	second, the overlapFunc is called to process the entire band. It
 *	is responsible for clipping the rectangles in the band, though
 *	this function provides the boundaries.
 *	At the end of each band, the new region is coalesced, if possible,
 *	to reduce the number of rectangles in the region.
 *
 *-----------------------------------------------------------------------
 */

typedef Bool (*OverlapProcPtr)(
    RegionPtr	pReg,
    BoxPtr	r1,
    BoxPtr   	r1End,
    BoxPtr	r2,
    BoxPtr   	r2End,
    Xint64    	y1,
    Xint64    	y2,
    Bool	*pOverlap);

static Bool
miRegionOp(
    RegionPtr       newReg,		    /* Place to store result	     */
    RegionPtr       reg1,		    /* First region in operation     */
    RegionPtr       reg2,		    /* 2d region in operation        */
    OverlapProcPtr  overlapFunc,            /* Function to call for over-
					     * lapping bands		     */
    Bool	    appendNon1,		    /* Append non-overlapping bands  */
					    /* in region 1 ? */
    Bool	    appendNon2,		    /* Append non-overlapping bands  */
					    /* in region 2 ? */
    Bool	    *pOverlap)
{
    register BoxPtr r1;			    /* Pointer into first region     */
    register BoxPtr r2;			    /* Pointer into 2d region	     */
    BoxPtr	    r1End;		    /* End of 1st region	     */
    BoxPtr	    r2End;		    /* End of 2d region		     */
    Xint64	    ybot;		    /* Bottom of intersection	     */
    Xint64	    ytop;		    /* Top of intersection	     */
    RegDataPtr	    oldData;		    /* Old data for newReg	     */
    int		    prevBand;		    /* Index of start of
					     * previous band in newReg       */
    int		    curBand;		    /* Index of start of current
					     * band in newReg		     */
    register BoxPtr r1BandEnd;		    /* End of current band in r1     */
    register BoxPtr r2BandEnd;		    /* End of current band in r2     */
    Xint64          top;		    /* Top of non-overlapping band   */
    Xint64	    bot;		    /* Bottom of non-overlapping band*/
    register Xint64 r1y1;		    /* Temps for r1->y1 and r2->y1   */
    register Xint64 r2y1;
    int		    newSize;
    int		    numRects;

    /*
     * Break any region computed from a broken region
     */
    if (REGION_NAR (reg1) || REGION_NAR(reg2))
	return miRegionBreak (newReg);
    
    /*
     * Initialization:
     *	set r1, r2, r1End and r2End appropriately, save the rectangles
     * of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */

    r1 = REGION_RECTS(reg1);
    newSize = REGION_NUM_RECTS(reg1);
    r1End = r1 + newSize;
    numRects = REGION_NUM_RECTS(reg2);
    r2 = REGION_RECTS(reg2);
    r2End = r2 + numRects;
    assert(r1 != r1End);
    assert(r2 != r2End);

    oldData = (RegDataPtr)NULL;
    if (((newReg == reg1) && (newSize > 1)) ||
	((newReg == reg2) && (numRects > 1)))
    {
	oldData = newReg->data;
	newReg->data = &miEmptyData;
    }
    /* guess at new size */
    if (numRects > newSize)
	newSize = numRects;
    newSize <<= 1;
    if (!newReg->data)
	newReg->data = &miEmptyData;
    else if (newReg->data->size)
	newReg->data->numRects = 0;
    if (newSize > newReg->data->size)
	if (!miRectAlloc(newReg, newSize))
	    return FALSE;

    /*
     * Initialize ybot.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     * 	In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *	For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms.
     */

    ybot = min(r1->y1, r2->y1);
    
    /*
     * prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. miCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prevBand = 0;
    
    do {
	/*
	 * This algorithm proceeds one source-band (as opposed to a
	 * destination band, which is determined by where the two regions
	 * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
	 * rectangle after the last one in the current band for their
	 * respective regions.
	 */
	assert(r1 != r1End);
	assert(r2 != r2End);
    
	FindBand(r1, r1BandEnd, r1End, r1y1);
	FindBand(r2, r2BandEnd, r2End, r2y1);

	/*
	 * First handle the band that doesn't intersect, if any.
	 *
	 * Note that attention is restricted to one band in the
	 * non-intersecting region at once, so if a region has n
	 * bands between the current position and the next place it overlaps
	 * the other, this entire loop will be passed through n times.
	 */
	if (r1y1 < r2y1) {
	    if (appendNon1) {
		top = max(r1y1, ybot);
		bot = min(r1->y2, r2y1);
		if (top != bot)	{
		    curBand = newReg->data->numRects;
		    miAppendNonO(newReg, r1, r1BandEnd, top, bot);
		    Coalesce(newReg, prevBand, curBand);
		}
	    }
	    ytop = r2y1;
	} else if (r2y1 < r1y1) {
	    if (appendNon2) {
		top = max(r2y1, ybot);
		bot = min(r2->y2, r1y1);
		if (top != bot) {
		    curBand = newReg->data->numRects;
		    miAppendNonO(newReg, r2, r2BandEnd, top, bot);
		    Coalesce(newReg, prevBand, curBand);
		}
	    }
	    ytop = r1y1;
	} else {
	    ytop = r1y1;
	}

	/*
	 * Now see if we've hit an intersecting band. The two bands only
	 * intersect if ybot > ytop
	 */
	ybot = min(r1->y2, r2->y2);
	if (ybot > ytop) {
	    curBand = newReg->data->numRects;
	    (* overlapFunc)(newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot,
			    pOverlap);
	    Coalesce(newReg, prevBand, curBand);
	}

	/*
	 * If we've finished with a band (y2 == ybot) we skip forward
	 * in the region to the next band.
	 */
	if (r1->y2 == ybot) r1 = r1BandEnd;
	if (r2->y2 == ybot) r2 = r2BandEnd;

    } while (r1 != r1End && r2 != r2End);

    /*
     * Deal with whichever region (if any) still has rectangles left.
     *
     * We only need to worry about banding and coalescing for the very first
     * band left.  After that, we can just group all remaining boxes,
     * regardless of how many bands, into one final append to the list.
     */

    if ((r1 != r1End) && appendNon1) {
	/* Do first nonOverlap1Func call, which may be able to coalesce */
	FindBand(r1, r1BandEnd, r1End, r1y1);
	curBand = newReg->data->numRects;
	miAppendNonO(newReg, r1, r1BandEnd, max(r1y1, ybot), r1->y2);
	Coalesce(newReg, prevBand, curBand);
	/* Just append the rest of the boxes  */
	AppendRegions(newReg, r1BandEnd, r1End);

    } else if ((r2 != r2End) && appendNon2) {
	/* Do first nonOverlap2Func call, which may be able to coalesce */
	FindBand(r2, r2BandEnd, r2End, r2y1);
	curBand = newReg->data->numRects;
	miAppendNonO(newReg, r2, r2BandEnd, max(r2y1, ybot), r2->y2);
	Coalesce(newReg, prevBand, curBand);
	/* Append rest of boxes */
	AppendRegions(newReg, r2BandEnd, r2End);
    }

    if (oldData)
	xfree(oldData);

    if (!(numRects = newReg->data->numRects))
    {
	xfreeData(newReg);
	newReg->data = &miEmptyData;
    }
    else if (numRects == 1)
    {
	newReg->extents = *REGION_BOXPTR(newReg);
	xfreeData(newReg);
	newReg->data = (RegDataPtr)NULL;
    }
    else
    {
	DOWNSIZE(newReg, numRects);
    }

    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * miSetExtents --
 *	Reset the extents of a region to what they should be. Called by
 *	miSubtract and miIntersect as they can't figure it out along the
 *	way or do so easily, as miUnion can.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The region's 'extents' structure is overwritten.
 *
 *-----------------------------------------------------------------------
 */
static void
miSetExtents (pReg)
    register RegionPtr pReg;
{
    register BoxPtr pBox, pBoxEnd;

    if (!pReg->data)
	return;
    if (!pReg->data->size)
    {
	pReg->extents.x2 = pReg->extents.x1;
	pReg->extents.y2 = pReg->extents.y1;
	return;
    }

    pBox = REGION_BOXPTR(pReg);
    pBoxEnd = REGION_END(pReg);

    /*
     * Since pBox is the first rectangle in the region, it must have the
     * smallest y1 and since pBoxEnd is the last rectangle in the region,
     * it must have the largest y2, because of banding. Initialize x1 and
     * x2 from  pBox and pBoxEnd, resp., as good things to initialize them
     * to...
     */
    pReg->extents.x1 = pBox->x1;
    pReg->extents.y1 = pBox->y1;
    pReg->extents.x2 = pBoxEnd->x2;
    pReg->extents.y2 = pBoxEnd->y2;

    assert(pReg->extents.y1 < pReg->extents.y2);
    while (pBox <= pBoxEnd) {
	if (pBox->x1 < pReg->extents.x1)
	    pReg->extents.x1 = pBox->x1;
	if (pBox->x2 > pReg->extents.x2)
	    pReg->extents.x2 = pBox->x2;
	pBox++;
    };

    assert(pReg->extents.x1 < pReg->extents.x2);
}

/*======================================================================
 *	    Region Intersection
 *====================================================================*/
/*-
 *-----------------------------------------------------------------------
 * miIntersectO --
 *	Handle an overlapping band for miIntersect.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *	Rectangles may be added to the region.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Bool
miIntersectO (
    register RegionPtr	pReg,
    register BoxPtr	r1,
    BoxPtr  	  	r1End,
    register BoxPtr	r2,
    BoxPtr  	  	r2End,
    Xint64    	  	y1,
    Xint64    	  	y2,
    Bool		*pOverlap)
{
    register Xint64  	x1;
    register Xint64  	x2;
    register BoxPtr	pNextRect;

    pNextRect = REGION_TOP(pReg);

    assert(y1 < y2);
    assert(r1 != r1End && r2 != r2End);

    do {
	x1 = max(r1->x1, r2->x1);
	x2 = min(r1->x2, r2->x2);

	/*
	 * If there's any overlap between the two rectangles, add that
	 * overlap to the new region.
	 */
	if (x1 < x2)
	    NEWRECT(pReg, pNextRect, x1, y1, x2, y2);

	/*
	 * Advance the pointer(s) with the leftmost right side, since the next
	 * rectangle on that list may still overlap the other region's
	 * current rectangle.
	 */
	if (r1->x2 == x2) {
	    r1++;
	}
	if (r2->x2 == x2) {
	    r2++;
	}
    } while ((r1 != r1End) && (r2 != r2End));

    return TRUE;
}


_X_EXPORT Bool
miIntersect(newReg, reg1, reg2)
    register RegionPtr 	newReg;     /* destination Region */
    register RegionPtr 	reg1;
    register RegionPtr	reg2;       /* source regions     */
{
    good(reg1);
    good(reg2);
    good(newReg);
   /* check for trivial reject */
    if (REGION_NIL(reg1)  || REGION_NIL(reg2) ||
	!EXTENTCHECK(&reg1->extents, &reg2->extents))
    {
	/* Covers about 20% of all cases */
	xfreeData(newReg);
	newReg->extents.x2 = newReg->extents.x1;
	newReg->extents.y2 = newReg->extents.y1;
	if (REGION_NAR(reg1) || REGION_NAR(reg2))
	{
	    newReg->data = &miBrokenData;
	    return FALSE;
	}
	else
	    newReg->data = &miEmptyData;
    }
    else if (!reg1->data && !reg2->data)
    {
	/* Covers about 80% of cases that aren't trivially rejected */
	newReg->extents.x1 = max(reg1->extents.x1, reg2->extents.x1);
	newReg->extents.y1 = max(reg1->extents.y1, reg2->extents.y1);
	newReg->extents.x2 = min(reg1->extents.x2, reg2->extents.x2);
	newReg->extents.y2 = min(reg1->extents.y2, reg2->extents.y2);
	xfreeData(newReg);
	newReg->data = (RegDataPtr)NULL;
    }
    else if (!reg2->data && SUBSUMES(&reg2->extents, &reg1->extents))
    {
	return miRegionCopy(newReg, reg1);
    }
    else if (!reg1->data && SUBSUMES(&reg1->extents, &reg2->extents))
    {
	return miRegionCopy(newReg, reg2);
    }
    else if (reg1 == reg2)
    {
	return miRegionCopy(newReg, reg1);
    }
    else
    {
	/* General purpose intersection */
	Bool overlap; /* result ignored */
	if (!miRegionOp(newReg, reg1, reg2, miIntersectO, FALSE, FALSE,
			&overlap))
	    return FALSE;
	miSetExtents(newReg);
    }

    good(newReg);
    return(TRUE);
}

#define MERGERECT(r)						\
{								\
    if (r->x1 <= x2) {						\
	/* Merge with current rectangle */			\
	if (r->x1 < x2) *pOverlap = TRUE;				\
	if (x2 < r->x2) x2 = r->x2;				\
    } else {							\
	/* Add current rectangle, start new one */		\
	NEWRECT(pReg, pNextRect, x1, y1, x2, y2);		\
	x1 = r->x1;						\
	x2 = r->x2;						\
    }								\
    r++;							\
}

/*======================================================================
 *	    Region Union
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miUnionO --
 *	Handle an overlapping band for the union operation. Picks the
 *	left-most rectangle each time and merges it into the region.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *	pReg is overwritten.
 *	pOverlap is set to TRUE if any boxes overlap.
 *
 *-----------------------------------------------------------------------
 */
static Bool
miUnionO (
    register RegionPtr	pReg,
    register BoxPtr	r1,
	     BoxPtr  	r1End,
    register BoxPtr	r2,
	     BoxPtr  	r2End,
	     Xint64	y1,
	     Xint64	y2,
	     Bool	*pOverlap)
{
    register BoxPtr     pNextRect;
    register Xint64     x1;     /* left and right side of current union */
    register Xint64     x2;

    assert (y1 < y2);
    assert(r1 != r1End && r2 != r2End);

    pNextRect = REGION_TOP(pReg);

    /* Start off current rectangle */
    if (r1->x1 < r2->x1)
    {
	x1 = r1->x1;
	x2 = r1->x2;
	r1++;
    }
    else
    {
	x1 = r2->x1;
	x2 = r2->x2;
	r2++;
    }
    while (r1 != r1End && r2 != r2End)
    {
	if (r1->x1 < r2->x1) MERGERECT(r1) else MERGERECT(r2);
    }

    /* Finish off whoever (if any) is left */
    if (r1 != r1End)
    {
	do
	{
	    MERGERECT(r1);
	} while (r1 != r1End);
    }
    else if (r2 != r2End)
    {
	do
	{
	    MERGERECT(r2);
	} while (r2 != r2End);
    }
    
    /* Add current rectangle */
    NEWRECT(pReg, pNextRect, x1, y1, x2, y2);

    return TRUE;
}

_X_EXPORT Bool 
miUnion(newReg, reg1, reg2)
    RegionPtr		newReg;                  /* destination Region */
    register RegionPtr 	reg1;
    register RegionPtr	reg2;             /* source regions     */
{
    Bool overlap; /* result ignored */

    /* Return TRUE if some overlap between reg1, reg2 */
    good(reg1);
    good(reg2);
    good(newReg);
    /*  checks all the simple cases */

    /*
     * Region 1 and 2 are the same
     */
    if (reg1 == reg2)
    {
	return miRegionCopy(newReg, reg1);
    }

    /*
     * Region 1 is empty
     */
    if (REGION_NIL(reg1))
    {
	if (REGION_NAR(reg1))
	    return miRegionBreak (newReg);
        if (newReg != reg2)
	    return miRegionCopy(newReg, reg2);
        return TRUE;
    }

    /*
     * Region 2 is empty
     */
    if (REGION_NIL(reg2))
    {
	if (REGION_NAR(reg2))
	    return miRegionBreak (newReg);
        if (newReg != reg1)
	    return miRegionCopy(newReg, reg1);
        return TRUE;
    }

    /*
     * Region 1 completely subsumes region 2
     */
    if (!reg1->data && SUBSUMES(&reg1->extents, &reg2->extents))
    {
        if (newReg != reg1)
	    return miRegionCopy(newReg, reg1);
        return TRUE;
    }

    /*
     * Region 2 completely subsumes region 1
     */
    if (!reg2->data && SUBSUMES(&reg2->extents, &reg1->extents))
    {
        if (newReg != reg2)
	    return miRegionCopy(newReg, reg2);
        return TRUE;
    }

    if (!miRegionOp(newReg, reg1, reg2, miUnionO, TRUE, TRUE, &overlap))
	return FALSE;

    newReg->extents.x1 = min(reg1->extents.x1, reg2->extents.x1);
    newReg->extents.y1 = min(reg1->extents.y1, reg2->extents.y1);
    newReg->extents.x2 = max(reg1->extents.x2, reg2->extents.x2);
    newReg->extents.y2 = max(reg1->extents.y2, reg2->extents.y2);
    good(newReg);
    return TRUE;
}

/*======================================================================
 * 	    	  Region Subtraction
 *====================================================================*/


/*-
 *-----------------------------------------------------------------------
 * miSubtractO --
 *	Overlapping band subtraction. x1 is the left-most point not yet
 *	checked.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *	pReg may have rectangles added to it.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Bool
miSubtractO (
    register RegionPtr	pReg,
    register BoxPtr	r1,
    BoxPtr  	  	r1End,
    register BoxPtr	r2,
    BoxPtr  	  	r2End,
    register Xint64  	y1,
             Xint64  	y2,
    Bool		*pOverlap)
{
    register BoxPtr	pNextRect;
    register Xint64  	x1;

    x1 = r1->x1;
    
    assert(y1<y2);
    assert(r1 != r1End && r2 != r2End);

    pNextRect = REGION_TOP(pReg);

    do
    {
	if (r2->x2 <= x1)
	{
	    /*
	     * Subtrahend entirely to left of minuend: go to next subtrahend.
	     */
	    r2++;
	}
	else if (r2->x1 <= x1)
	{
	    /*
	     * Subtrahend preceeds minuend: nuke left edge of minuend.
	     */
	    x1 = r2->x2;
	    if (x1 >= r1->x2)
	    {
		/*
		 * Minuend completely covered: advance to next minuend and
		 * reset left fence to edge of new minuend.
		 */
		r1++;
		if (r1 != r1End)
		    x1 = r1->x1;
	    }
	    else
	    {
		/*
		 * Subtrahend now used up since it doesn't extend beyond
		 * minuend
		 */
		r2++;
	    }
	}
	else if (r2->x1 < r1->x2)
	{
	    /*
	     * Left part of subtrahend covers part of minuend: add uncovered
	     * part of minuend to region and skip to next subtrahend.
	     */
	    assert(x1<r2->x1);
	    NEWRECT(pReg, pNextRect, x1, y1, r2->x1, y2);

	    x1 = r2->x2;
	    if (x1 >= r1->x2)
	    {
		/*
		 * Minuend used up: advance to new...
		 */
		r1++;
		if (r1 != r1End)
		    x1 = r1->x1;
	    }
	    else
	    {
		/*
		 * Subtrahend used up
		 */
		r2++;
	    }
	}
	else
	{
	    /*
	     * Minuend used up: add any remaining piece before advancing.
	     */
	    if (r1->x2 > x1)
		NEWRECT(pReg, pNextRect, x1, y1, r1->x2, y2);
	    r1++;
	    if (r1 != r1End)
		x1 = r1->x1;
	}
    } while ((r1 != r1End) && (r2 != r2End));


    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 != r1End)
    {
	assert(x1<r1->x2);
	NEWRECT(pReg, pNextRect, x1, y1, r1->x2, y2);
	r1++;
	if (r1 != r1End)
	    x1 = r1->x1;
    }
    return TRUE;
}
	
/*-
 *-----------------------------------------------------------------------
 * miSubtract --
 *	Subtract regS from regM and leave the result in regD.
 *	S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *	regD is overwritten.
 *
 *-----------------------------------------------------------------------
 */
_X_EXPORT Bool
miSubtract(regD, regM, regS)
    register RegionPtr	regD;               
    register RegionPtr 	regM;
    register RegionPtr	regS;          
{
    Bool overlap; /* result ignored */

    good(regM);
    good(regS);
    good(regD);
   /* check for trivial rejects */
    if (REGION_NIL(regM) || REGION_NIL(regS) ||
	!EXTENTCHECK(&regM->extents, &regS->extents))
    {
	if (REGION_NAR (regS))
	    return miRegionBreak (regD);
	return miRegionCopy(regD, regM);
    }
    else if (regM == regS)
    {
	xfreeData(regD);
	regD->extents.x2 = regD->extents.x1;
	regD->extents.y2 = regD->extents.y1;
	regD->data = &miEmptyData;
	return TRUE;
    }
 
    /* Add those rectangles in region 1 that aren't in region 2,
       do yucky substraction for overlaps, and
       just throw away rectangles in region 2 that aren't in region 1 */
    if (!miRegionOp(regD, regM, regS, miSubtractO, TRUE, FALSE, &overlap))
	return FALSE;

    /*
     * Can't alter RegD's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    miSetExtents(regD);
    good(regD);
    return TRUE;
}

/*
 *   RectIn(region, rect)
 *   This routine takes a pointer to a region and a pointer to a box
 *   and determines if the box is outside/inside/partly inside the region.
 *
 *   The idea is to travel through the list of rectangles trying to cover the
 *   passed box with them. Anytime a piece of the rectangle isn't covered
 *   by a band of rectangles, partOut is set TRUE. Any time a rectangle in
 *   the region covers part of the box, partIn is set TRUE. The process ends
 *   when either the box has been completely covered (we reached a band that
 *   doesn't overlap the box, partIn is TRUE and partOut is false), the
 *   box has been partially covered (partIn == partOut == TRUE -- because of
 *   the banding, the first time this is true we know the box is only
 *   partially in the region) or is outside the region (we reached a band
 *   that doesn't overlap the box at all and partIn is false)
 */

_X_EXPORT int
miRectIn(region, prect)
    register RegionPtr  region;
    register BoxPtr     prect;
{
    register Xint64	x;
    register Xint64	y;
    register BoxPtr     pbox;
    register BoxPtr     pboxEnd;
    Bool		partIn, partOut;
    int			numRects;

    good(region);
    numRects = REGION_NUM_RECTS(region);
    /* useful optimization */
    if (!numRects || !EXTENTCHECK(&region->extents, prect))
        return(rgnOUT);

    if (numRects == 1)
    {
	/* We know that it must be rgnIN or rgnPART */
	if (SUBSUMES(&region->extents, prect))
	    return(rgnIN);
	else
	    return(rgnPART);
    }

    partOut = FALSE;
    partIn = FALSE;

    /* (x,y) starts at upper left of rect, moving to the right and down */
    x = prect->x1;
    y = prect->y1;

    /* can stop when both partOut and partIn are TRUE, or we reach prect->y2 */
    for (pbox = REGION_BOXPTR(region), pboxEnd = pbox + numRects;
         pbox != pboxEnd;
         pbox++)
    {

        if (pbox->y2 <= y)
           continue;    /* getting up to speed or skipping remainder of band */

        if (pbox->y1 > y)
        {
           partOut = TRUE;      /* missed part of rectangle above */
           if (partIn || (pbox->y1 >= prect->y2))
              break;
           y = pbox->y1;        /* x guaranteed to be == prect->x1 */
        }

        if (pbox->x2 <= x)
           continue;            /* not far enough over yet */

        if (pbox->x1 > x)
        {
           partOut = TRUE;      /* missed part of rectangle to left */
           if (partIn)
              break;
        }

        if (pbox->x1 < prect->x2)
        {
            partIn = TRUE;      /* definitely overlap */
            if (partOut)
               break;
        }

        if (pbox->x2 >= prect->x2)
        {
           y = pbox->y2;        /* finished with this band */
           if (y >= prect->y2)
              break;
           x = prect->x1;       /* reset x out to left again */
        }
	else
	{
	    /*
	     * Because boxes in a band are maximal width, if the first box
	     * to overlap the rectangle doesn't completely cover it in that
	     * band, the rectangle must be partially out, since some of it
	     * will be uncovered in that band. partIn will have been set true
	     * by now...
	     */
	    partOut = TRUE;
	    break;
	}
    }

    return(partIn ? ((y < prect->y2) ? rgnPART : rgnIN) : rgnOUT);
}

_X_EXPORT Bool
miPointInRegion(pReg, x, y, box)
    register RegionPtr pReg;
    register Xint64 x, y;
    BoxPtr box;     /* "return" value */
{
    register BoxPtr pbox, pboxEnd;
    int numRects;

    good(pReg);
    numRects = REGION_NUM_RECTS(pReg);
    if (!numRects || !INBOX(&pReg->extents, x, y))
        return(FALSE);
    if (numRects == 1)
    {
	*box = pReg->extents;
	return(TRUE);
    }
    for (pbox = REGION_BOXPTR(pReg), pboxEnd = pbox + numRects;
	 pbox != pboxEnd;
	 pbox++)
    {
        if (y >= pbox->y2)
	   continue;		/* not there yet */
	if ((y < pbox->y1) || (x < pbox->x1))
	   break;		/* missed it */
	if (x >= pbox->x2)
	   continue;		/* not there yet */
	*box = *pbox;
	return(TRUE);
    }
    return(FALSE);
}

_X_EXPORT Bool
miRegionNotEmpty(pReg)
     RegionPtr pReg;
{
  good(pReg);
  return(!REGION_NIL(pReg));
}

static Bool
miRegionBroken(RegionPtr pReg)
{
    good(pReg);
    return (REGION_NAR(pReg));
}

_X_EXPORT void
miRegionEmpty(pReg)
     RegionPtr pReg;
{
  good(pReg);
  xfreeData(pReg);
  pReg->extents.x2 = pReg->extents.x1;
  pReg->extents.y2 = pReg->extents.y1;
  pReg->data = &miEmptyData;
}

_X_EXPORT BoxPtr
miRegionExtents(pReg)
    RegionPtr pReg;
{
    good(pReg);
    return(&pReg->extents);
}
/* --- DEC / Open Group stuff ends --- */

/* --- Rapicorn region C API (internal) --- */
static inline RegionPtr
rmutable (const RapicornRegion *region)
{
  return (RapicornRegion*) region;
}

static inline RapicornRegionBox*
bmutable (const RapicornRegionBox *box)
{
  return (RapicornRegionBox*) box;
}

static void
fix_empty_region (RapicornRegion *region)
{
  /* fix up empty regions to always contain the same extents */
  if (region->extents.x1 == region->extents.x2 &&
      region->extents.y1 == region->extents.y2)
    {
      if (miRegionNotEmpty (region)) { ErrorF ("Invalid empty region: %p:\n", region); miPrintRegion (region); }
      assert (miRegionNotEmpty (region) == FALSE);
      region->extents.x1 = region->extents.x2 = 0;
      region->extents.y1 = region->extents.y2 = 0;
    }
}

RapicornRegion*
_rapicorn_region_create (void)
{
  RapicornRegion *region = miRegionCreate (NULL, 0);
  assert (!miRegionBroken (region));
  fix_empty_region (region);
  return region;
}

void
_rapicorn_region_free (RapicornRegion *region)
{
  assert (region != NULL);
  miRegionDestroy (region);
}

void
_rapicorn_region_init (RapicornRegion *region,
                       int             region_size)
{
  assert (region_size == sizeof (region[0]));
  miRegionInit (region, NULL, 0);
  fix_empty_region (region);
}

void
_rapicorn_region_uninit (RapicornRegion *region)
{
  miRegionUninit (region);
}

void
_rapicorn_region_copy (RapicornRegion       *region,
                       const RapicornRegion *region2)
{
  assert (region != NULL);
  assert (region2 != NULL);
  miRegionCopy (region, region2);
  fix_empty_region (region);
}

/* Alter region so that it additionally covers all of the rectangle. */
void
_rapicorn_region_union_rect (RapicornRegion          *region,
                             const RapicornRegionBox *rect)
{
  assert (region != NULL);
  if (rect->x1 != rect->x2 && rect->y1 != rect->y2)
    {
      const BoxRec brec = {
        .x1 = min (rect->x1, rect->x2),
        .y1 = min (rect->y1, rect->y2),
        .x2 = max (rect->x1, rect->x2),
        .y2 = max (rect->y1, rect->y2),
      };
      RapicornRegion tregion;
      miRegionInit (&tregion, bmutable (&brec), 1);
      miUnion (region, region, &tregion);
      miRegionUninit (&tregion);
    }
  fix_empty_region (region);
}

/* Alter region so that it additionally covers all of region2. */
void
_rapicorn_region_union (RapicornRegion       *region,
                        const RapicornRegion *region2)
{
  assert (region != NULL);
  assert (region2 != NULL);

  miUnion (region, region, region2);
  fix_empty_region (region);
}

/* Alter region so that all of region2 is removed from it. */
void
_rapicorn_region_subtract (RapicornRegion       *region,
                           const RapicornRegion *region2)
{
  assert (region != NULL);
  assert (region2 != NULL);

  miSubtract (region, region, region2);
  fix_empty_region (region);
}

/* Alter region so that everything not in region2 is removed from it. */
void
_rapicorn_region_intersect (RapicornRegion       *region,
                            const RapicornRegion *region2)
{
  assert (region != NULL);
  assert (region2 != NULL);

  miIntersect (region, region, rmutable (region2));
  fix_empty_region (region);
}

/* Alter region so that everything also in region2 is removed from it,
 * and everything only in region2 is added to it.
 */
void
_rapicorn_region_xor (RapicornRegion       *region,
                      const RapicornRegion *region2)
{
  assert (region != NULL);
  assert (region2 != NULL);

  RapicornRegion tregion;
  miRegionInit (&tregion, NULL, 0);
  miSubtract (&tregion, region2, region);
  miSubtract (region, region, region2);
  miUnion (region, region, &tregion);
  miRegionUninit (&tregion);
  fix_empty_region (region);
}

/* Alter region so that it is empty. */
void
_rapicorn_region_clear (RapicornRegion *region)
{
  miRegionEmpty (region);
  fix_empty_region (region);
}

/* Check whether region is empty. */
bool
_rapicorn_region_empty (const RapicornRegion *region)
{
  return region->extents.x1 == region->extents.x2 && region->extents.y1 == region->extents.y2;
}

bool
_rapicorn_region_equal (const RapicornRegion *region,
                        const RapicornRegion *region2)
{
  if (_rapicorn_region_empty (region) && _rapicorn_region_empty (region2))
    return TRUE;        /* empty regions are equal regardless of their zero-size extents */
  return miRegionEqual (rmutable (region), rmutable (region2));
}

#define RETURN_SIGN_IF_UNEQUAL(lhs,rhs) { if (lhs < rhs) return -1; else if (rhs < lhs) return +1; }

int
_rapicorn_region_cmp (const RapicornRegion *region,
                      const RapicornRegion *region2)
{
  RETURN_SIGN_IF_UNEQUAL (region->extents.y1, region2->extents.y1);
  RETURN_SIGN_IF_UNEQUAL (region->extents.x1, region2->extents.x1);
  RETURN_SIGN_IF_UNEQUAL (region->extents.y2, region2->extents.y2);
  RETURN_SIGN_IF_UNEQUAL (region->extents.x2, region2->extents.x2);

  int n1 = REGION_NUM_RECTS (region);
  int n2 = REGION_NUM_RECTS (region2);
  RETURN_SIGN_IF_UNEQUAL (n1, n2);

  const RapicornRegionBox *rs1 = REGION_RECTS (region);
  const RapicornRegionBox *rs2 = REGION_RECTS (region2);
  int i;
  for (i = 0; i < n1; i++)
    {
      RETURN_SIGN_IF_UNEQUAL (rs1[i].y1, rs2[i].y1);
      RETURN_SIGN_IF_UNEQUAL (rs1[i].x1, rs2[i].x1);
      RETURN_SIGN_IF_UNEQUAL (rs1[i].y2, rs2[i].y2);
      RETURN_SIGN_IF_UNEQUAL (rs1[i].x2, rs2[i].x2);
  }
  return 0; /* equality */
}

void
_rapicorn_region_swap (RapicornRegion *region,
                       RapicornRegion *region2)
{
  RapicornRegion tregion = *region;
  *region = *region2;
  *region2 = tregion;
  fix_empty_region (region);
  fix_empty_region (region2);
}

void
_rapicorn_region_extents (const RapicornRegion *region,
                          RapicornRegionBox    *rect)
{
  *rect = *miRegionExtents (rmutable (region));
}

bool
_rapicorn_region_point_in (const RapicornRegion      *region,
                           const RapicornRegionPoint *point)
{
  BoxRec brec;
  assert (region != NULL);
  return miPointInRegion (rmutable (region), point->x, point->y, &brec); /* 0=Out, 1=In */
}

RapicornRegionCType
_rapicorn_region_rect_in (const RapicornRegion    *region,
                          const RapicornRegionBox *rect)
{
  assert (region != NULL);
  if (rect->x1 == rect->x2 && rect->y1 == rect->y2)
    return RAPICORN_REGION_INSIDE;
  BoxRec brec = {
    .x1 = min (rect->x1, rect->x2),
    .y1 = min (rect->y1, rect->y2),
    .x2 = max (rect->x1, rect->x2),
    .y2 = max (rect->y1, rect->y2),
  };
  return miRectIn (rmutable (region), &brec); /* 0=Out, 1=In, 2=Partial */
}

RapicornRegionCType
_rapicorn_region_region_in (const RapicornRegion      *region,
                            const RapicornRegion      *region2)
{
  if (_rapicorn_region_empty (region2) || region == region2)
    return RAPICORN_REGION_INSIDE;
  int i, n = REGION_NUM_RECTS (region);
  const RapicornRegionBox *rects = REGION_RECTS (region);
  if (n < 1)
    return RAPICORN_REGION_OUTSIDE;
  RapicornRegionCType contains = miRectIn (rmutable (region2), bmutable (&rects[0]));
  for (i = 1; i < n && contains != RAPICORN_REGION_PARTIAL; i++)
    {
      RapicornRegionCType ct = miRectIn (rmutable (region2), bmutable (&rects[i]));
      if (ct == RAPICORN_REGION_OUTSIDE)
        contains = contains == RAPICORN_REGION_INSIDE ? RAPICORN_REGION_PARTIAL : contains;
      else if (ct == RAPICORN_REGION_INSIDE)
        contains = contains == RAPICORN_REGION_OUTSIDE ? RAPICORN_REGION_PARTIAL : contains;
      else /* (ct == RAPICORN_REGION_PARTIAL) */
        contains = RAPICORN_REGION_PARTIAL;
    }
  return contains;
}

int
_rapicorn_region_get_rect_count (const RapicornRegion *region)
{
  return REGION_NUM_RECTS (region);
}

int
_rapicorn_region_get_rects (const RapicornRegion *region,
                            int                   n_rects,
                            RapicornRegionBox    *rects)
{
  int i, n = n_rects;
  n = min (n, REGION_NUM_RECTS (region));
  for (i = 0; i < n; i++)
    rects[i] = REGION_RECTS (region)[i];
  return REGION_NUM_RECTS (region);
}
