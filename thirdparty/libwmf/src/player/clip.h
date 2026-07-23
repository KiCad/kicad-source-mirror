/* libwmf ("player/clip.h"): library for wmf conversion
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


#ifndef WMFPLAYER_CLIP_H
#define WMFPLAYER_CLIP_H

static void Clipping (wmfAPI* API,wmfRegion* clip,wmfRegion* vis,wmfD_Rect* rect,U16 flags)
{	wmfRegion rgn;

	rgn.rects = 0;

	if (clip->numRects == 0)
	{	if (clip->rects)
		{	rgn.rects = clip->rects;
			rgn.size = clip->size;

			clip->rects = 0;
			clip->size = 0;
		}
	}

	if (rgn.rects == 0)
	{	rgn.rects = (wmfD_Rect*) wmf_malloc (API,8 * sizeof (wmfD_Rect));
		rgn.size = 8;

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}
	}

	WmfSetRectRgn (&rgn,rect);

	if ((clip->numRects == 0) && (flags & CLIP_INTERSECT))
	{	(*clip) = rgn;
		return;
	}
/* else if (flags & CLIP_EXCLUDE) ...
 */
	if (clip->numRects == 0)
	{	if (clip->rects == 0)
		{	clip->rects = (wmfD_Rect*) wmf_malloc (API,8 * sizeof (wmfD_Rect));
			clip->size = 8;

			if (ERR (API))
			{	WMF_DEBUG (API,"bailing...");
				return;
			}
		}

		WmfSetRectRgn (clip,0);
		WmfCombineRgn (API,clip,vis,0,RGN_COPY);

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}
	}

	WmfCombineRgn (API,&rgn,clip,&rgn,(U16)((flags & CLIP_EXCLUDE) ? RGN_DIFF : RGN_AND));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	(*clip) = rgn; /* What about what *was* in clip ?? Check WmfCombineRgn */
}

#endif /* ! WMFPLAYER_CLIP_H */
