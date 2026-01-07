/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef BITMAP_TYPES_H_
#define BITMAP_TYPES_H_

#include <kicommon.h>

//FIXME: cannot include only this file in wxWidgets 2.9.3
// test if it works under stable release
// #include <wx/bitmap.h>   // only to define wxBitmap
class wxBitmap;     // only to define wxBitmap
class wxBitmapBundle;
class wxWindow;
class wxString;
class BITMAP_STORE;

enum class BITMAPS : unsigned int;

enum class BITMAP_TYPE
{
    PNG,
    JPG,
    BMP
};

KICOMMON_API BITMAP_STORE* GetBitmapStore();

/**
 * Construct a wxBitmap from an image identifier
 * Returns the image from the active theme if the image has multiple theme variants.
 * @param aBitmap is from the BITMAPS enum in bitmaps_list.h
 * @param aHeightTag is the requested height tag for multi-res bitmaps (-1 for any)
 */
KICOMMON_API wxBitmap KiBitmap( BITMAPS aBitmap, int aHeightTag = -1 );

KICOMMON_API wxBitmapBundle KiBitmapBundle( BITMAPS aBitmap, int aMinHeight = -1 );

/**
 * Constructs and returns a bitmap bundle for the given icon ID, with the default
 * bitmap size being aDefHeight. Rescaling is applied if no bitmap of that size
 * is available.
 * @param aBitmapId is from the BITMAPS enum in bitmaps_list.h
 * @param aDefHeight is the desired height of the default bitmap in the bundle.
 */
KICOMMON_API wxBitmapBundle KiBitmapBundleDef( BITMAPS aBitmap, int aDefHeight );

KICOMMON_API wxBitmapBundle KiDisabledBitmapBundle( BITMAPS aBitmap, int aMinHeight = -1 );

KICOMMON_API wxBitmapBundle KiDisabledBitmapBundleDef( BITMAPS aBitmap, int aDefHeight );

/**
 * Wipes out the scaled bitmap cache so that the icon theme can be changed.
 * TODO: move scaling system into BITMAP_STORE so this global doesn't need to exist
 */
KICOMMON_API void ClearScaledBitmapCache();

/**
 * Construct a wxBitmap from a memory record, scaling it if device DPI demands it.
 *
 * Internally this may use caching to avoid scaling the same image twice. If caching
 * is used, it's guaranteed threadsafe.
 *
 * @param aBitmap is from the BITMAPS enum in bitmaps_list.h
 * @param aWindow target window for scaling context
 * @param aHeight is the requested image height for the source bitmap, or -1 for any height
 * @param aQuantized if true scaling will be rounded to integers (2X, 3X, etc.).
 */
KICOMMON_API wxBitmap KiScaledBitmap( BITMAPS aBitmap, wxWindow* aWindow, int aHeight = -1,
                                      bool aQuantized = false );

/**
 * Overload of the above function that takes another wxBitmap as a parameter.
 *
 * @param aBitmap is the source bitmap to scale
 * @param aWindow target window for scaling context
 */
KICOMMON_API wxBitmap KiScaledBitmap( const wxBitmap& aBitmap, wxWindow* aWindow );

/**
 * Return the automatic scale factor that would be used for a given window by
 * KiScaledBitmap and KiScaledSeparator.
 */
KICOMMON_API int KiIconScale( wxWindow* aWindow );

/**
 * Allocate a wxBitmap on heap from a memory record, held in a BITMAPS.
 *
 * @param aBitmap is from the BITMAPS enum in bitmaps_list.h
 * @return wxBitmap* - caller owns it.
 */
KICOMMON_API wxBitmap* KiBitmapNew( BITMAPS aBitmap );

#endif  // BITMAP_TYPES_H_
