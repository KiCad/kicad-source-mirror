/* libwmf ("player/region.h"): library for wmf conversion
   Copyright (C) 2000 - various; see CREDITS, ChangeLog, and sources

   The libwmf Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The libwmf Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the libwmf Library; see the file COPYING.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#ifndef WMFPLAYER_REGION_H
#define WMFPLAYER_REGION_H

static wmfD_Rect* rgn_memchk (wmfAPI* API,wmfRegion* rgn)
{	wmfD_Rect* more = 0;

	if (rgn->numRects < (rgn->size - 1)) return (rgn->rects + rgn->numRects);

	more = wmf_realloc (API,rgn->rects,(rgn->size + 8) * sizeof (wmfD_Rect));

	if ((more == 0) || (ERR (API))) return (0); /* The two conditions are equivalent, I think */

	rgn->rects = more;
	rgn->size += 8; /* The exponential increase in memory allocation in MEMCHECK scares me */

	return (rgn->rects + rgn->numRects);
}

/*  1 if two RECTs overlap.
 *  0 if two RECTs do not overlap.
 */
#define EXTENTCHECK(r1,r2)      \
	(  ((r1)->BR.x > (r2)->TL.x) \
	&& ((r1)->TL.x < (r2)->BR.x) \
	&& ((r1)->BR.y > (r2)->TL.y) \
	&& ((r1)->TL.y < (r2)->BR.y) )

static void WmfSetRectRgn (wmfRegion* rgn,wmfD_Rect* rect)
{	if ((rect == 0) || (rect->TL.x == rect->BR.x) || (rect->TL.y == rect->BR.y)) /* EMPTY_REGION (rgn); */
	{	rgn->extents.TL.x = 0;
		rgn->extents.TL.y = 0;
		rgn->extents.BR.x = 0;
		rgn->extents.BR.y = 0;

		rgn->numRects = 0;
		rgn->type = NULLREGION;
	}
	else
	{	rgn->extents = (*rect);
		(*(rgn->rects)) = (*rect);

		rgn->numRects = 1;
		rgn->type = SIMPLEREGION;
	}
}

static void WmfCombineRgn (wmfAPI* API,wmfRegion* destObj,wmfRegion* src1Obj,wmfRegion* src2Obj,U16 mode)
{	if ((destObj == 0) || (src1Obj == 0)) return;

	if (mode == RGN_COPY)
	{	REGION_CopyRegion (API,destObj,src1Obj);
		return;
	}

	if (src2Obj)
	{	switch (mode)
		{
		case RGN_OR:
			REGION_UnionRegion (API,destObj,src1Obj,src2Obj);
		break;

		case RGN_AND:
			REGION_IntersectRegion (API,destObj,src1Obj,src2Obj);
		break;
#if 0
		case RGN_XOR:
			REGION_XorRegion (API,destObj,src1Obj,src2Obj);
		break;
#endif
		case RGN_DIFF:
			REGION_SubtractRegion (API,destObj,src1Obj,src2Obj);
		break;
		}
	}
}

static void REGION_UnionRegion (wmfAPI* API,wmfRegion* newReg,wmfRegion* reg1,wmfRegion* reg2)
{	/* checks all the simple cases */

	/* Region 1 and 2 are the same or region 1 is empty
	 */
	if ((reg1 == reg2) || (!(reg1->numRects)))
	{	if (newReg != reg2) REGION_CopyRegion (API,newReg,reg2);
		return;
	}

	/* If nothing to union (region 2 empty)
	 */
	if (reg2->numRects == 0)
	{	if (newReg != reg1) REGION_CopyRegion (API,newReg,reg1);
		return;
	}

	/* Region 1 completely subsumes region 2
	 */
	if ( (reg1->numRects == 1)
	  && (reg1->extents.TL.x <= reg2->extents.TL.x)
	  && (reg1->extents.TL.y <= reg2->extents.TL.y)
	  && (reg1->extents.BR.x >= reg2->extents.BR.x)
	  && (reg1->extents.BR.y >= reg2->extents.BR.y))
	{	if (newReg != reg1) REGION_CopyRegion (API,newReg,reg1);
		return;
	}

	/* Region 2 completely subsumes region 1
	 */
	if ( (reg2->numRects == 1)
	  && (reg2->extents.TL.x <= reg1->extents.TL.x)
	  && (reg2->extents.TL.y <= reg1->extents.TL.y)
	  && (reg2->extents.BR.x >= reg1->extents.BR.x)
	  && (reg2->extents.BR.y >= reg1->extents.BR.y))
	{	if (newReg != reg2) REGION_CopyRegion (API,newReg,reg2);
		return;
	}

	REGION_RegionOp (API,newReg,reg1,reg2,REGION_UnionO,REGION_UnionNonO,REGION_UnionNonO);

	newReg->extents.TL.x = MIN (reg1->extents.TL.x,reg2->extents.TL.x);
	newReg->extents.TL.y = MIN (reg1->extents.TL.y,reg2->extents.TL.y);
	newReg->extents.BR.x = MAX (reg1->extents.BR.x,reg2->extents.BR.x);
	newReg->extents.BR.y = MAX (reg1->extents.BR.y,reg2->extents.BR.y);

	newReg->type = ((newReg->numRects)?(COMPLEXREGION):(NULLREGION));
}

static void REGION_CopyRegion (wmfAPI* API,wmfRegion* dst,wmfRegion* src)
{	if (dst != src) /*  don't want to copy to itself */
	{	if (dst->size < src->numRects)
		{	dst->rects = wmf_realloc (API,dst->rects,src->numRects * sizeof (wmfD_Rect));

			if (ERR (API))
			{	WMF_DEBUG (API,"bailing...");
				return;
			}

			dst->size = src->numRects;
		}
		dst->numRects = src->numRects;

		dst->extents.TL.x = src->extents.TL.x;
		dst->extents.TL.y = src->extents.TL.y;
		dst->extents.BR.x = src->extents.BR.x;
		dst->extents.BR.y = src->extents.BR.y;

		dst->type = src->type;

		memcpy ((char*) dst->rects,(char*) src->rects,src->numRects * sizeof (wmfD_Rect));
	}
}

/*****************************************************************************
 *           REGION_RegionOp
 *
 *      Apply an operation to two regions. Called by REGION_Union,
 *      REGION_Inverse, REGION_Subtract, REGION_Intersect...
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The new region is overwritten.
 *
 * Notes:
 *      The idea behind this function is to view the two regions as sets.
 *      Together they cover a rectangle of area that this function divides
 *      into horizontal bands where points are covered only by one region
 *      or by both. For the first case, the nonOverlapFunc is called with
 *      each the band and the band's upper and lower extents. For the
 *      second, the overlapFunc is called to process the entire band. It
 *      is responsible for clipping the rectangles in the band, though
 *      this function provides the boundaries.
 *      At the end of each band, the new region is coalesced, if possible,
 *      to reduce the number of rectangles in the region.
 *
 */
static void REGION_RegionOp (
	wmfAPI* API,
	wmfRegion* newReg,         /* Place to store result */
	wmfRegion* reg1,           /* First region in operation */
	wmfRegion* reg2,           /* 2nd region in operation */
	pProcO overlapFunc,        /* Function to call for over-lapping bands */
	pProcNonO nonOverlap1Func, /* Function to call for non-overlapping bands in region 1 */
	pProcNonO nonOverlap2Func  /* Function to call for non-overlapping bands in region 2 */
)
{	wmfD_Rect* r1;                    /* Pointer into first region */
	wmfD_Rect* r2;                    /* Pointer into 2d region */
	wmfD_Rect* r1End;                 /* End of 1st region */
	wmfD_Rect* r2End;                 /* End of 2d region */
	wmfD_Rect* oldRects;              /* Old rects for newReg */
	wmfD_Rect* r1BandEnd;             /* End of current band in r1 */
	wmfD_Rect* r2BandEnd;             /* End of current band in r2 */
	wmfD_Rect* prev_rects;

	float ytop;                       /* Top of intersection */
	float ybot;                       /* Bottom of intersection */
	float top;                        /* Top of non-overlapping band */
	float bot;                        /* Bottom of non-overlapping band */

	unsigned int prevBand;            /* Index of start of previous band in newReg */
	unsigned int curBand;             /* Index of start of current band in newReg */

/*Initialization:
 * 
 * set r1, r2, r1End and r2End appropriately, preserve the important
 * parts of the destination region until the end in case it's one of
 * the two source regions, then mark the "new" region empty, allocating
 * another array of rectangles for it to use.
 */
	r1 = reg1->rects;
	r2 = reg2->rects;

	r1End = r1 + reg1->numRects;
	r2End = r2 + reg2->numRects;

/* newReg may be one of the src regions so we can't empty it. We keep a 
 * note of its rects pointer (so that we can free them later), preserve its
 * extents and simply set numRects to zero. 
 */
	oldRects = newReg->rects;
	newReg->numRects = 0;

/* Allocate a reasonable number of rectangles for the new region. The idea
 * is to allocate enough so the individual functions don't need to
 * reallocate and copy the array, which is time consuming, yet we don't
 * have to worry about using too much memory. I hope to be able to
 * nuke the Xrealloc() at the end of this function eventually.
 */
	newReg->size = MAX (reg1->numRects,reg2->numRects) * 2;

	if ((newReg->rects = wmf_malloc (API,sizeof (wmfD_Rect) * newReg->size)) == 0)
	{	newReg->size = 0;
		return;
	}

/* Initialize ybot and ytop.
 *
 * In the upcoming loop, ybot and ytop serve different functions depending
 * on whether the band being handled is an overlapping or non-overlapping
 * band.
 *
 * In the case of a non-overlapping band (only one of the regions
 * has points in the band), ybot is the bottom of the most recent
 * intersection and thus clips the top of the rectangles in that band.
 * ytop is the top of the next intersection between the two regions and
 * serves to clip the bottom of the rectangles in the current band.
 *
 * For an overlapping band (where the two regions intersect), ytop clips
 * the top of the rectangles of both regions and ybot clips the bottoms.
 */
	if (reg1->extents.TL.y < reg2->extents.TL.y)
	{	ybot = reg1->extents.TL.y;
	}
	else
	{	ybot = reg2->extents.TL.y;
	}
	 
/* prevBand serves to mark the start of the previous band so rectangles
 * can be coalesced into larger rectangles. qv. miCoalesce, above.
 * In the beginning, there is no previous band, so prevBand == curBand
 * (curBand is set later on, of course, but the first band will always
 * start at index 0). prevBand and curBand must be indices because of
 * the possible expansion, and resultant moving, of the new region's
 * array of rectangles.
 */
	prevBand = 0;
	 
	do
	{	curBand = newReg->numRects;

/* This algorithm proceeds one source-band (as opposed to a
 * destination band, which is determined by where the two regions
 * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
 * rectangle after the last one in the current band for their
 * respective regions.
 */
		r1BandEnd = r1;
		while ((r1BandEnd != r1End) && (r1BandEnd->TL.y == r1->TL.y)) r1BandEnd++;

		r2BandEnd = r2;
		while ((r2BandEnd != r2End) && (r2BandEnd->TL.y == r2->TL.y)) r2BandEnd++;

/* First handle the band that doesn't intersect, if any.
 *
 * Note that attention is restricted to one band in the
 * non-intersecting region at once, so if a region has n
 * bands between the current position and the next place it overlaps
 * the other, this entire loop will be passed through n times.
 */
		if (r1->TL.y < r2->TL.y)
		{	top = MAX (r1->TL.y,ybot);
			bot = MIN (r1->BR.y,r2->TL.y);

			if ((top != bot) && (nonOverlap1Func))
			{	(*nonOverlap1Func) (API,newReg,r1,r1BandEnd,top,bot);
			}

			ytop = r2->TL.y;
		}
		else if (r2->TL.y < r1->TL.y)
		{	top = MAX (r2->TL.y,ybot);
			bot = MIN (r2->BR.y,r1->TL.y);

			if ((top != bot) && (nonOverlap2Func))
			{	(*nonOverlap2Func) (API,newReg,r2,r2BandEnd,top,bot);
			}

			ytop = r1->TL.y;
		}
		else
		{	ytop = r1->TL.y;
		}

/* If any rectangles got added to the region, try and coalesce them
 * with rectangles from the previous band. Note we could just do
 * this test in miCoalesce, but some machines incur a not
 * inconsiderable cost for function calls, so...
 */
		if (newReg->numRects != curBand)
		{	prevBand = REGION_Coalesce (newReg,prevBand,curBand);
		}

/* Now see if we've hit an intersecting band. The two bands only
 * intersect if ybot > ytop
 */
		ybot = MIN (r1->BR.y,r2->BR.y);
		curBand = newReg->numRects;
		if ((ybot > ytop) && (overlapFunc))
		{	(*overlapFunc) (API,newReg,r1,r1BandEnd,r2,r2BandEnd,ytop,ybot);
		}

		if (newReg->numRects != curBand)
		{	prevBand = REGION_Coalesce (newReg,prevBand,curBand);
		}

/* If we've finished with a band (bottom == ybot) we skip forward
 * in the region to the next band.
 */
		if (r1->BR.y == ybot) r1 = r1BandEnd;
		if (r2->BR.y == ybot) r2 = r2BandEnd;

	} while ((r1 != r1End) && (r2 != r2End));

/* Deal with whichever region still has rectangles left.
 */
	curBand = newReg->numRects;
	if (r1 != r1End)
	{	if (nonOverlap1Func)
		{	do
			{	r1BandEnd = r1;
				while ((r1BandEnd < r1End) && (r1BandEnd->TL.y == r1->TL.y)) r1BandEnd++;

				(*nonOverlap1Func) (API,newReg,r1,r1BandEnd,MAX (r1->TL.y,ybot),r1->BR.y);

				r1 = r1BandEnd;
			} while (r1 != r1End);
		}
	}
	else if ((r2 != r2End) && (nonOverlap2Func))
	{	do
		{	r2BandEnd = r2;
			while ((r2BandEnd < r2End) && (r2BandEnd->TL.y == r2->TL.y)) r2BandEnd++;

			(*nonOverlap2Func) (API,newReg,r2,r2BandEnd,MAX (r2->TL.y,ybot),r2->BR.y);

			r2 = r2BandEnd;
		} while (r2 != r2End);
	}

	if (newReg->numRects != curBand)
	{	REGION_Coalesce (newReg,prevBand,curBand);
	}

/* A bit of cleanup. To keep regions from growing without bound,
 * we shrink the array of rectangles to match the new number of
 * rectangles in the region. This never goes to 0, however...
 *
 * Only do this stuff if the number of rectangles allocated is more than
 * twice the number of rectangles in the region (a simple optimization...).
 */
	if (newReg->numRects < (newReg->size >> 1))
	{	if (newReg->numRects) /* REGION_NOT_EMPTY */
		{	prev_rects = newReg->rects;
			newReg->size = newReg->numRects;
			newReg->rects = wmf_realloc (API,newReg->rects,sizeof(wmfD_Rect) * newReg->size);
			if (newReg->rects == 0) newReg->rects = prev_rects;
		}
		else
		{	/* No point in doing the extra work involved in an Xrealloc if
			 * the region is empty
			 */
			newReg->size = 1;
			wmf_free (API,newReg->rects);
			newReg->rects = wmf_malloc (API,sizeof (wmfD_Rect));
		}
	}

	wmf_free (API,oldRects);
}

/*****************************************************************************
 *           REGION_Coalesce
 *
 *      Attempt to merge the rects in the current band with those in the
 *      previous one. Used only by REGION_RegionOp.
 *
 * Results:
 *      The new index for the previous band.
 *
 * Side Effects:
 *      If coalescing takes place:
 *          - rectangles in the previous band will have their bottom fields
 *            altered.
 *          - pReg->numRects will be decreased.
 *
 */
static unsigned int REGION_Coalesce (
	wmfRegion* pReg,           /* Region to coalesce */
	unsigned int prevStart,    /* Index of start of previous band */
	unsigned int curStart      /* Index of start of current band */
)
{	wmfD_Rect* pPrevRect;      /* Current rect in previous band */
	wmfD_Rect* pCurRect;       /* Current rect in current band */
	wmfD_Rect* pRegEnd;        /* End of region */

	unsigned int curNumRects;  /* Number of rectangles in current band */
	unsigned int prevNumRects; /* Number of rectangles in previous band */

	float bandtop;             /* top coordinate for current band */

	pRegEnd = pReg->rects + pReg->numRects;
	pPrevRect = pReg->rects + prevStart;
	prevNumRects = curStart - prevStart;

/* Figure out how many rectangles are in the current band. Have to do
 * this because multiple bands could have been added in REGION_RegionOp
 * at the end when one region has been exhausted.
 */
	pCurRect = pReg->rects + curStart;
	bandtop = pCurRect->TL.y;

	for (curNumRects = 0; (pCurRect != pRegEnd) && (pCurRect->TL.y == bandtop); curNumRects++)
	{	pCurRect++;
	}
	 
	if (pCurRect != pRegEnd)
	{	/* If more than one band was added, we have to find the start
		 * of the last band added so the next coalescing job can start
		 * at the right place... (given when multiple bands are added,
		 * this may be pointless -- see above).
		 */
		pRegEnd--;
		while (pRegEnd[-1].TL.y == pRegEnd->TL.y) pRegEnd--;

		curStart = pRegEnd - pReg->rects;
		pRegEnd = pReg->rects + pReg->numRects;
	}
	
	if ((curNumRects == prevNumRects) && (curNumRects != 0))
	{	pCurRect -= curNumRects;

		/* The bands may only be coalesced if the bottom of the previous
		 * matches the top scanline of the current.
		 */
		if (pPrevRect->BR.y == pCurRect->TL.y)
		{	/* Make sure the bands have rects in the same places. This
			 * assumes that rects have been added in such a way that they
			 * cover the most area possible. I.e. two rects in a band must
			 * have some horizontal space between them.
			 */
			do
			{	if ( (pPrevRect->TL.x  != pCurRect->TL.x )
				  || (pPrevRect->BR.x != pCurRect->BR.x))
				{	/* The bands don't line up so they can't be coalesced.
					 */
					return (curStart);
				}
				pPrevRect++;
				pCurRect++;
				prevNumRects -= 1;
			} while (prevNumRects != 0);

			pReg->numRects -= curNumRects;
			pCurRect -= curNumRects;
			pPrevRect -= curNumRects;

			/* The bands may be merged, so set the bottom of each rect
			 * in the previous band to that of the corresponding rect in
			 * the current band.
			 */
			do
			{	pPrevRect->BR.y = pCurRect->BR.y;
				pPrevRect++;
				pCurRect++;
				curNumRects -= 1;
			} while (curNumRects != 0);

			/* If only one band was added to the region, we have to backup
			 * curStart to the start of the previous band.
			 *
			 * If more than one band was added to the region, copy the
			 * other bands down. The assumption here is that the other bands
			 * came from the same region as the current one and no further
			 * coalescing can be done on them since it's all been done
			 * already... curStart is already in the right place.
			 */
			if (pCurRect == pRegEnd)
			{	curStart = prevStart;
			}
			else
			{	do
				{	*pPrevRect++ = *pCurRect++;
				} while (pCurRect != pRegEnd);
			}
		}
	}

	return (curStart);
}

/*****************************************************************************
 *       REGION_UnionO
 *
 *      Handle an overlapping band for the union operation. Picks the
 *      left-most rectangle each time and merges it into the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles are overwritten in pReg->rects and pReg->numRects will
 *      be changed.
 *
 */
static void REGION_UnionO (
	wmfAPI* API,
	wmfRegion* pReg,
	wmfD_Rect* r1,
	wmfD_Rect* r1End,
	wmfD_Rect* r2,
	wmfD_Rect* r2End,
	float top,
	float bottom
)
{	while ((r1 != r1End) && (r2 != r2End))
	{	if (r1->TL.x < r2->TL.x)
		{	rect_merge (API,pReg,r1,top,bottom);
			if (ERR (API)) return;
			r1++;
		}
		else
		{	rect_merge (API,pReg,r2,top,bottom);
			if (ERR (API)) return;
			r2++;
		}
	}

	if (r1 != r1End)
	{	do
		{	rect_merge (API,pReg,r1,top,bottom);
			if (ERR (API)) return;
			r1++;
		} while (r1 != r1End);
	}
	else
	{	while (r2 != r2End)
		{	rect_merge (API,pReg,r2,top,bottom);
			if (ERR (API)) return;
			r2++;
		}
	}
}

static void rect_merge (
	wmfAPI* API,
	wmfRegion* rgn,
	wmfD_Rect* r,
	float top,
	float bottom
)
{	wmfD_Rect* pNextRect = 0;
	wmfD_Rect* pPrior = 0;

	if ((pNextRect = rgn_memchk (API,rgn)) == 0) return;

	if (rgn->numRects == 0)
	{	rgn->numRects++;

		pNextRect->TL.x = r->TL.x;
		pNextRect->TL.y = top;
		pNextRect->BR.x = r->BR.x;
		pNextRect->BR.y = bottom;
		
		pNextRect++;
	}
	else
	{	pPrior = pNextRect - 1;
		if ((pPrior->TL.y == top) && (pPrior->BR.y == bottom) && (pPrior->BR.x >= r->TL.x))
		{	if (pPrior->BR.x < r->BR.x) pPrior->BR.x = r->BR.x;
		}
		else
		{	rgn->numRects++;

			pNextRect->TL.x = r->TL.x;
			pNextRect->TL.y = top;
			pNextRect->BR.x = r->BR.x;
			pNextRect->BR.y = bottom;
		
			pNextRect++;
		}
	}
}

/*****************************************************************************
 *       REGION_UnionNonO
 *
 *      Handle a non-overlapping band for the union operation. Just
 *      Adds the rectangles into the region. Doesn't have to check for
 *      subsumption or anything.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg->numRects is incremented and the final rectangles overwritten
 *      with the rectangles we're passed.
 *
 */
static void REGION_UnionNonO (
	wmfAPI* API,
	wmfRegion* pReg,
	wmfD_Rect* r,
	wmfD_Rect* rEnd,
	float top,
	float bottom
)
{	wmfD_Rect* pNextRect = 0;

	while (r != rEnd)
	{	if ((pNextRect = rgn_memchk (API,pReg)) == 0) return;

		pReg->numRects++;

		pNextRect->TL.x = r->TL.x;
		pNextRect->TL.y = top;
		pNextRect->BR.x = r->BR.x;
		pNextRect->BR.y = bottom;

		pNextRect++;
		r++;
	}
}

/*****************************************************************************
 *       REGION_SubtractNonO1
 *
 *      Deal with non-overlapping band for subtraction. Any parts from
 *      region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg may be affected.
 *
 */
static void REGION_SubtractNonO1 (
	wmfAPI* API,
	wmfRegion* pReg,
	wmfD_Rect* r,
	wmfD_Rect* rEnd,
	float top,
	float bottom
)
{	wmfD_Rect* pNextRect = 0;

	while (r != rEnd)
	{	if ((pNextRect = rgn_memchk (API,pReg)) == 0) return;

		pReg->numRects++;

		pNextRect->TL.x = r->TL.x;
		pNextRect->TL.y = top;
		pNextRect->BR.x = r->BR.x;
		pNextRect->BR.y = bottom;

		pNextRect++;
		r++;
	}
}

/*****************************************************************************
 *       REGION_SubtractRegion
 *
 *      Subtract regS from regM and leave the result in regD.
 *      S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *      TRUE.
 *
 * Side Effects:
 *      regD is overwritten.
 *
 */
static void REGION_SubtractRegion (
	wmfAPI* API,
	wmfRegion* regD,
	wmfRegion* regM,
	wmfRegion* regS
)
{	/* check for trivial reject
	 */
	if ( (regM->numRects == 0)
	  || (regS->numRects == 0)
	  || (!EXTENTCHECK (&regM->extents,&regS->extents)) )
	{	REGION_CopyRegion (API,regD,regM);
		return;
	}

	REGION_RegionOp (API,regD,regM,regS,REGION_SubtractO,REGION_SubtractNonO1,0);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

/* Can't alter newReg's extents before we call miRegionOp because
 * it might be one of the source regions and miRegionOp depends
 * on the extents of those regions being the unaltered. Besides, this
 * way there's no checking against rectangles that will be nuked
 * due to coalescing, so we have to examine fewer rectangles.
 */
	REGION_SetExtents (regD);
	regD->type = ((regD->numRects) ? COMPLEXREGION : NULLREGION);
}

/*****************************************************************************
 *		  REGION_SubtractO
 *
 *      Overlapping band subtraction. x1 is the left-most point not yet
 *      checked.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg may have rectangles added to it.
 *
 */
static void REGION_SubtractO (
	wmfAPI* API,
	wmfRegion* pReg,
	wmfD_Rect* r1,
	wmfD_Rect* r1End,
	wmfD_Rect* r2,
	wmfD_Rect* r2End,
	float top,
	float bottom
)
{	wmfD_Rect* pNextRect = 0;

	float left = r1->TL.x;

	if ((pNextRect = rgn_memchk (API,pReg)) == 0) return;

	while ((r1 != r1End) && (r2 != r2End))
	{	if (r2->BR.x <= left)
		{	/* Subtrahend missed the boat: go to next subtrahend.
			 */
			r2++;
		}
		else if (r2->TL.x <= left)
		{	/* Subtrahend preceeds minuend: nuke left edge of minuend.
			 */
			left = r2->BR.x;
			if (left >= r1->BR.x)
			{	/* Minuend completely covered: advance to next minuend and
				 * reset left fence to edge of new minuend.
				 */
				r1++;
				if (r1 != r1End) left = r1->TL.x;
			}
			else
			{	/* Subtrahend now used up since it doesn't extend beyond
				 * minuend
				 */
				r2++;
			}
		}
		else if (r2->TL.x < r1->BR.x)
		{	/* Left part of subtrahend covers part of minuend: add uncovered
			 * part of minuend to region and skip to next subtrahend.
			 */
			if ((pNextRect = rgn_memchk (API,pReg)) == 0) return;

			pReg->numRects++;

			pNextRect->TL.x = left;
			pNextRect->TL.y = top;
			pNextRect->BR.x = r2->TL.x;
			pNextRect->BR.y = bottom;

			pNextRect++;

			left = r2->BR.x;

			if (left >= r1->BR.x)
			{	/* Minuend used up: advance to new...
				 */
				r1++;
				if (r1 != r1End) left = r1->TL.x;
			}
			else
			{	/* Subtrahend used up
				 */
				r2++;
			}
		}
		else
		{	/* Minuend used up: add any remaining piece before advancing.
			 */
			if (r1->BR.x > left)
			{	if ((pNextRect = rgn_memchk (API,pReg)) == 0) return;

				pReg->numRects++;

				pNextRect->TL.x = left;
				pNextRect->TL.y = top;
				pNextRect->BR.x = r1->BR.x;
				pNextRect->BR.y = bottom;

				pNextRect++;
			}
			r1++;
			if (r1 != r1End) left = r1->TL.x;
		}
	}

/* Add remaining minuend rectangles to region.
 */
	while (r1 != r1End)
	{	if ((pNextRect = rgn_memchk (API,pReg)) == 0) return;

		pReg->numRects++;

		pNextRect->TL.x = left;
		pNextRect->TL.y = top;
		pNextRect->BR.x = r1->BR.x;
		pNextRect->BR.y = bottom;

		pNextRect++;
		r1++;

		if (r1 != r1End) left = r1->TL.x;
	}
}

/*****************************************************************************
 *           REGION_SetExtents
 *           Re-calculate the extents of a region
 */
static void REGION_SetExtents (
	wmfRegion* pReg
)
{	wmfD_Rect* pRect;
	wmfD_Rect* pRectEnd;
	wmfD_Rect* pExtents;

	if (pReg->numRects == 0)
	{	pReg->extents.TL.x = 0;
		pReg->extents.TL.y = 0;
		pReg->extents.BR.x = 0;
		pReg->extents.BR.y = 0;

		return;
	}

	pExtents = &pReg->extents;
	pRect = pReg->rects;
	pRectEnd = pRect + pReg->numRects - 1;

/* Since pRect is the first rectangle in the region, it must have the
 * smallest top and since pRectEnd is the last rectangle in the region,
 * it must have the largest bottom, because of banding. Initialize left and
 * right from pRect and pRectEnd, resp., as good things to initialize them
 * to...
 */
	pExtents->TL.x = pRect->TL.x;
	pExtents->TL.y = pRect->TL.y;
	pExtents->BR.x = pRectEnd->BR.x;
	pExtents->BR.y = pRectEnd->BR.y;

	while (pRect <= pRectEnd)
	{	if (pRect->TL.x < pExtents->TL.x) pExtents->TL.x = pRect->TL.x;
		if (pRect->BR.x > pExtents->BR.x) pExtents->BR.x = pRect->BR.x;

		pRect++;
	}
}

/*****************************************************************************
 *       REGION_IntersectRegion
 */
static void REGION_IntersectRegion (
	wmfAPI* API,
	wmfRegion* newReg,
	wmfRegion* reg1,
	wmfRegion* reg2
)
{	/* check for trivial reject
	 */
	if ( (!(reg1->numRects)) || (!(reg2->numRects))  || (!EXTENTCHECK (&reg1->extents,&reg2->extents)))
	{	newReg->numRects = 0;
	}
	else
	{	REGION_RegionOp (API,newReg,reg1,reg2,REGION_IntersectO,0,0);
	}

/* Can't alter newReg's extents before we call miRegionOp because
 * it might be one of the source regions and miRegionOp depends
 * on the extents of those regions being the same. Besides, this
 * way there's no checking against rectangles that will be nuked
 * due to coalescing, so we have to examine fewer rectangles.
 */
	REGION_SetExtents (newReg);

	newReg->type = ((newReg->numRects) ? COMPLEXREGION : NULLREGION);
}

/*****************************************************************************
 *       REGION_IntersectO
 *
 * Handle an overlapping band for REGION_Intersect.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles may be added to the region.
 *
 */
static void REGION_IntersectO (
	wmfAPI* API,
	wmfRegion* pReg,
	wmfD_Rect* r1,
	wmfD_Rect* r1End,
	wmfD_Rect* r2,
	wmfD_Rect* r2End,
	float top,
	float bottom
)
{	float left;
	float right;

	wmfD_Rect* pNextRect;

	while ((r1 != r1End) && (r2 != r2End))
	{	left  = MAX (r1->TL.x,r2->TL.x);
		right = MIN (r1->BR.x,r2->BR.x);

/* If there's any overlap between the two rectangles, add that
 * overlap to the new region.
 * There's no need to check for subsumption because the only way
 * such a need could arise is if some region has two rectangles
 * right next to each other. Since that should never happen...
 */
		if (left < right)
		{	if ((pNextRect = rgn_memchk (API,pReg)) == 0) return;

			pReg->numRects++;

			pNextRect->TL.x = left;
			pNextRect->TL.y = top;
			pNextRect->BR.x = right;
			pNextRect->BR.y = bottom;

			pNextRect++;
		}

/* Need to advance the pointers. Shift the one that extends
 * to the right the least, since the other still has a chance to
 * overlap with that region's next rectangle, if you see what I mean.
 */
		if (r1->BR.x < r2->BR.x)
		{	r1++;
		}
		else if (r2->BR.x < r1->BR.x)
		{	r2++;
		}
		else
		{	r1++;
			r2++;
		}
	}
}

#endif /* ! WMFPLAYER_REGION_H */
