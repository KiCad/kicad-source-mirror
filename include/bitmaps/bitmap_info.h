
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_BITMAP_INFO_H
#define KICAD_BITMAP_INFO_H

#include <kicommon.h>
#include <vector>
#include <unordered_map>
#include <bitmaps/bitmaps_list.h>

#include <wx/string.h>

struct KICOMMON_API BITMAP_INFO
{
    BITMAPS  id;
    wxString filename;
    int      height;
    wxString theme;

    BITMAP_INFO( BITMAPS aId, const wxString& aFilename, int aHeight, const wxString& aTheme ) :
        id( aId ),
        filename( aFilename ),
        height( aHeight ),
        theme( aTheme )
    {};

};


extern KICOMMON_API void BuildBitmapInfo( std::unordered_map<BITMAPS,
                             std::vector<BITMAP_INFO>>& aBitmapInfoCache );

#endif // KICAD_BITMAP_INFO_H
