/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

//FIXME: cannot include only this file in wxWidgets 2.9.3
// test if it works under stable release
// #include <wx/bitmap.h>   // only to define wxBitmap
class wxBitmap;     // only to define wxBitmap
class EDA_DRAW_FRAME;
class wxWindow;
class wxString;
struct BITMAP_OPAQUE;
class BITMAP_STORE;

enum class BITMAPS : unsigned int;

enum class BITMAP_TYPE
{
    PNG,
    JPG,
    BMP
};

BITMAP_STORE* GetBitmapStore();

/**
 * Construct a wxBitmap from an image identifier
 * Returns the image from the active theme if the image has multiple theme variants.
 * @param aBitmap is from the BITMAPS enum in bitmaps_list.h
 * @param aHeightTag is the requested height tag for multi-res bitmaps (-1 for any)
 */
wxBitmap KiBitmap( BITMAPS aBitmap, int aHeightTag = -1 );

/**
 * Compatibility shim for pcb_calculator until its images are pulled into the PNG pipeline
 */
wxBitmap KiBitmap( const BITMAP_OPAQUE* aBitmap );

/**
 * Wipes out the scaled bitmap cache so that the icon theme can be changed.
 * TODO: move scaling system into BITMAP_STORE so this global doesn't need to exist
 */
void ClearScaledBitmapCache();

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
wxBitmap KiScaledBitmap( BITMAPS aBitmap, wxWindow* aWindow, int aHeight = -1,
                         bool aQuantized = false );

/**
 * Overload of the above function that takes another wxBitmap as a parameter.
 *
 * @param aBitmap is the source bitmap to scale
 * @param aWindow target window for scaling context
 */
wxBitmap KiScaledBitmap( const wxBitmap& aBitmap, wxWindow* aWindow );

/**
 * Return the automatic scale factor that would be used for a given window by
 * KiScaledBitmap and KiScaledSeparator.
 */
int KiIconScale( wxWindow* aWindow );

/**
 * Allocate a wxBitmap on heap from a memory record, held in a BITMAPS.
 *
 * @param aBitmap is from the BITMAPS enum in bitmaps_list.h
 * @return wxBitmap* - caller owns it.
 */
wxBitmap* KiBitmapNew( BITMAPS aBitmap );

/**
 * Save the current view as an image file.
 *
 * @param aFrame The current draw frame view to save.
 * @param aFileName The file name to save the image.  This will overwrite an existing file.
 * @param aBitmapType The type of bitmap create as defined by wxImage.
 * @return True if the file was successfully saved or false if the file failed to be saved.
 */
bool SaveCanvasImageToFile( EDA_DRAW_FRAME* aFrame, const wxString& aFileName,
                            BITMAP_TYPE aBitmapType = BITMAP_TYPE::PNG );

#endif  // BITMAP_TYPES_H_
