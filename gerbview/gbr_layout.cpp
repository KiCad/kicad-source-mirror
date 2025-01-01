/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2018 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#include <gerbview_frame.h>
#include <gbr_layout.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>

GBR_LAYOUT::GBR_LAYOUT() :
    EDA_ITEM( nullptr, GERBER_LAYOUT_T )
{
}


GBR_LAYOUT::~GBR_LAYOUT()
{
}

// Accessor to the list of Gerber files (and drill files) images
GERBER_FILE_IMAGE_LIST* GBR_LAYOUT::GetImagesList() const
{
    return &GERBER_FILE_IMAGE_LIST::GetImagesList();
}


BOX2I GBR_LAYOUT::ComputeBoundingBox() const
{
    BOX2I bbox;     // Start with a fresh BOX2I so the Merge algorithm works

    for( unsigned layer = 0; layer < GetImagesList()->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = GetImagesList()->GetGbrImage( layer );

        if( gerber == nullptr )    // Graphic layer not yet used
            continue;

        for( GERBER_DRAW_ITEM* item : gerber->GetItems() )
            bbox.Merge( item->GetBoundingBox() );
    }

    bbox.Normalize();

    m_BoundingBox = bbox;
    return bbox;
}


INSPECT_RESULT GBR_LAYOUT::Visit( INSPECTOR inspector, void* testData,
                                  const std::vector<KICAD_T>& aScanTypes )
{
#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == GERBER_LAYOUT_T )
        {
            for( unsigned layer = 0; layer < GetImagesList()->ImagesMaxCount(); ++layer )
            {
                GERBER_FILE_IMAGE* gerber = GetImagesList()->GetGbrImage( layer );

                if( gerber == nullptr )    // Graphic layer not yet used
                    continue;

                if( gerber->Visit( inspector, testData, aScanTypes ) == INSPECT_RESULT::QUIT )
                    return INSPECT_RESULT::QUIT;
            }
        }
    }

    return INSPECT_RESULT::CONTINUE;
}
