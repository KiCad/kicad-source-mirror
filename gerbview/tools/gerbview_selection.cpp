/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#include <eda_item.h>
#include <view/view_group.h>
#include <gerber_draw_item.h>
#include <tools/gerbview_selection.h>

VECTOR2I GERBVIEW_SELECTION::GetCenter() const
{
    VECTOR2I centre;

    if( Size() == 1 )
    {
        centre = Front()->GetPosition();
    }
    else
    {
        BOX2I bbox;

        for( EDA_ITEM* item : m_items )
            bbox.Merge( item->GetBoundingBox() );

        centre = bbox.Centre();
    }

    return centre;
}


const BOX2I GERBVIEW_SELECTION::ViewBBox() const
{
    BOX2I bbox;

    if( Size() == 1 )
    {
        bbox = Front()->GetBoundingBox();
    }
    else if( Size() > 1 )
    {
        for( EDA_ITEM* item : m_items )
            bbox.Merge( item->GetBoundingBox() );
    }

    return bbox;
}


