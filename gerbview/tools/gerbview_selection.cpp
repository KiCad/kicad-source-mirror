/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_struct.h>
#include <view/view_group.h>
#include <gerber_draw_item.h>
#include <tools/gerbview_selection.h>

VECTOR2I GERBVIEW_SELECTION::GetCenter() const
{
    VECTOR2I centre;

    if( Size() == 1 )
    {
        centre = static_cast<GERBER_DRAW_ITEM*>( Front() )->GetPosition();
    }
    else
    {
        EDA_RECT bbox = Front()->GetBoundingBox();
        auto i = m_items.begin();
        ++i;

        for( ; i != m_items.end(); ++i )
            bbox.Merge( (*i)->GetBoundingBox() );

        centre = bbox.Centre();
    }

    return centre;
}


const BOX2I GERBVIEW_SELECTION::ViewBBox() const
{
    EDA_RECT eda_bbox;

    if( Size() == 1 )
    {
        eda_bbox = Front()->GetBoundingBox();
    }
    else if( Size() > 1 )
    {
        eda_bbox = Front()->GetBoundingBox();
        auto i = m_items.begin();
        ++i;

        for( ; i != m_items.end(); ++i )
            eda_bbox.Merge( (*i)->GetBoundingBox() );
    }

    return BOX2I( eda_bbox.GetOrigin(), eda_bbox.GetSize() );
}


