/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

// For page and paper size, values are in 1/1000 inch

#ifndef WORKSHEET_H_
#define WORKSHEET_H_

#include <colors.h>     // EDA_COLOR_T definition
#include <class_page_info.h>

// Forward declarations:
class EDA_DRAW_PANEL;
class EDA_RECT;
class TITLE_BLOCK;

/**
 * Function DrawPageLayout is a core function to draw the page layout with
 * the frame and the basic inscriptions.
 * @param aDC The device context.
 * @param aClipBox = the clipping rect, or NULL if no clipping.
 * @param aPageInfo for margins and page size (in mils).
 * @param aFullSheetName The sheetpath (full sheet name), for basic inscriptions.
 * @param aFileName The file name, for basic inscriptions.
 * @param aTitleBlock The sheet title block, for basic inscriptions.
 * @param aSheetCount The number of sheets (for basic inscriptions).
 * @param aSheetNumber The sheet number (for basic inscriptions).
 * @param aPenWidth the pen size The line width for drawing.
 * @param aScalar the scale factor to convert from mils to internal units.
 * @param aColor The color for drawing.
 * @param aAltColor The color for items which need to be "hightlighted".
 *
 * Parameters used in aPageInfo
 * - the size of the page layout.
 * - the LTmargin The left top margin of the page layout.
 * - the RBmargin The right bottom margin of the page layout.
 */
void DrawPageLayout( wxDC* aDC, EDA_RECT* aClipBox,
                     const PAGE_INFO& aPageInfo,
                     const wxString &aFullSheetName,
                     const wxString& aFileName,
                     TITLE_BLOCK& aTitleBlock,
                     int aSheetCount, int aSheetNumber,
                     int aPenWidth, double aScalar,
                     EDA_COLOR_T aColor, EDA_COLOR_T aAltColor );


#endif // WORKSHEET_H_
