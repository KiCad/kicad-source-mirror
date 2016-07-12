/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

#include <macros.h>
#include <general.h>
#include <transform.h>
#include <lib_collectors.h>


const KICAD_T LIB_COLLECTOR::AllItems[] = {
    LIB_ARC_T,
    LIB_CIRCLE_T,
    LIB_TEXT_T,
    LIB_RECTANGLE_T,
    LIB_POLYLINE_T,
    LIB_BEZIER_T,
    LIB_PIN_T,
    LIB_FIELD_T,
    EOT
};


const KICAD_T LIB_COLLECTOR::AllItemsButPins[] = {
    LIB_ARC_T,
    LIB_CIRCLE_T,
    LIB_TEXT_T,
    LIB_RECTANGLE_T,
    LIB_POLYLINE_T,
    LIB_BEZIER_T,
    LIB_FIELD_T,
    EOT
};


const KICAD_T LIB_COLLECTOR::EditableItems[] = {
    LIB_ARC_T,
    LIB_CIRCLE_T,
    LIB_TEXT_T,
    LIB_RECTANGLE_T,
    LIB_POLYLINE_T,
    LIB_BEZIER_T,
    LIB_PIN_T,
    LIB_FIELD_T,
    EOT
};


const KICAD_T LIB_COLLECTOR::MovableItems[] = {
    LIB_ARC_T,
    LIB_CIRCLE_T,
    LIB_TEXT_T,
    LIB_RECTANGLE_T,
    LIB_POLYLINE_T,
    LIB_BEZIER_T,
    LIB_PIN_T,
    LIB_FIELD_T,
    EOT
};


const KICAD_T LIB_COLLECTOR::RotatableItems[] = {
    LIB_ARC_T,
    LIB_CIRCLE_T,
    LIB_TEXT_T,
    LIB_RECTANGLE_T,
    LIB_POLYLINE_T,
    LIB_BEZIER_T,
    LIB_PIN_T,
    LIB_FIELD_T,
    EOT
};


SEARCH_RESULT LIB_COLLECTOR::Inspect( EDA_ITEM* aItem, void* testData )
{
    LIB_ITEM* item = (LIB_ITEM*) aItem;

//    wxLogDebug( wxT( "Inspecting item %s, unit %d, convert %d" ),
//                GetChars( item->GetSelectMenuText() ), item->GetUnit(), item->GetConvert() );

    if( ( m_data.m_unit && item->GetUnit() && ( m_data.m_unit != item->GetUnit() ) )
        || ( m_data.m_convert && item->GetConvert() && ( m_data.m_convert != item->GetConvert() ) )
        || !item->HitTest( m_RefPos, -1, DefaultTransform ) )
        return SEARCH_CONTINUE;

    Append( aItem );

    return SEARCH_CONTINUE;
}


void LIB_COLLECTOR::Collect( LIB_ITEMS& aItems, const KICAD_T aFilterList[],
                             const wxPoint& aPosition, int aUnit, int aConvert )
{
    Empty();        // empty the collection just in case

    SetScanTypes( aFilterList );

    // remember where the snapshot was taken from and pass refPos to the Inspect() function.
    SetRefPos( aPosition );

    m_data.m_unit = aUnit;
    m_data.m_convert = aConvert;

    for( size_t i = 0;  i < aItems.size();  i++ )
    {
        if( SEARCH_QUIT == aItems[i].Visit( m_inspector, NULL, m_ScanTypes ) )
            break;
    }
}
