/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/// @file easypc_design_settings.h Title block and page size, shared by the board and schematic builders

#ifndef EASYPC_DESIGN_SETTINGS_H
#define EASYPC_DESIGN_SETTINGS_H

#include <math/box2.h>
#include <page_info.h>

class TITLE_BLOCK;


namespace EASYPC
{

struct DESIGN_DOCUMENT;


/**
 * Fill a title block from the design's SummaryInformation property set: title from Title, date from the last save,
 * comments 1 to 4 from Subject, Author, Keywords and Comments.
 */
void FillTitleBlock( const DESIGN_DOCUMENT& aDoc, TITLE_BLOCK& aTitleBlock );


/// Margin added on every side of the drawn extents before choosing a page, in millimetres
constexpr double PAGE_MARGIN_MM = 10.0;


/**
 * Easy-PC stores no page size, so the first of A4 to A0 then A to E, landscape before portrait, that holds the
 * extents and margins is chosen, else a User page of exactly that size rounded up to whole millimetres.
 */
PAGE_INFO PageForExtents( const BOX2L& aExtentsNm );

} // namespace EASYPC

#endif // EASYPC_DESIGN_SETTINGS_H
