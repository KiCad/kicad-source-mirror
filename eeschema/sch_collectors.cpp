/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 Kicad Developers, see change_log.txt for contributors.
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

#include "general.h"
#include "transform.h"
#include "sch_collectors.h"
#include "sch_component.h"
#include "sch_line.h"


const KICAD_T SCH_COLLECTOR::AllItems[] = {
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_POLYLINE_T,
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIERARCHICAL_LABEL_T,
    SCH_FIELD_T,
    SCH_COMPONENT_T,
    LIB_PIN_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    EOT
};


const KICAD_T SCH_COLLECTOR::AllItemsButPins[] = {
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_POLYLINE_T,
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIERARCHICAL_LABEL_T,
    SCH_FIELD_T,
    SCH_COMPONENT_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    EOT
};


const KICAD_T SCH_COLLECTOR::EditableItems[] = {
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIERARCHICAL_LABEL_T,
    SCH_FIELD_T,
    SCH_COMPONENT_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    EOT
};


const KICAD_T SCH_COLLECTOR::MovableItems[] = {
    SCH_MARKER_T,
//    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_ENTRY_T,
//    SCH_LINE_T,
    SCH_POLYLINE_T,
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIERARCHICAL_LABEL_T,
    SCH_FIELD_T,
    SCH_COMPONENT_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    EOT
};


const KICAD_T SCH_COLLECTOR::DraggableItems[] = {
    SCH_JUNCTION_T,
    SCH_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_POLYLINE_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIERARCHICAL_LABEL_T,
    SCH_COMPONENT_T,
    SCH_SHEET_T,
    EOT
};


const KICAD_T SCH_COLLECTOR::RotatableItems[] = {
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIERARCHICAL_LABEL_T,
    SCH_FIELD_T,
    SCH_COMPONENT_T,
    SCH_SHEET_T,
    EOT
};


const KICAD_T SCH_COLLECTOR::ParentItems[] = {
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_POLYLINE_T,
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIERARCHICAL_LABEL_T,
    SCH_COMPONENT_T,
    SCH_SHEET_T,
    EOT
};


const KICAD_T SCH_COLLECTOR::ComponentsOnly[] = {
    SCH_COMPONENT_T,
    EOT
};


const KICAD_T SCH_COLLECTOR::SheetsOnly[] = {
    SCH_SHEET_T,
    EOT
};


const KICAD_T SCH_COLLECTOR::SheetsAndSheetLabels[] = {
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    EOT
};


SEARCH_RESULT SCH_COLLECTOR::Inspect( EDA_ITEM* aItem, const void* aTestData )
{
    if( aItem->Type() != LIB_PIN_T && !aItem->HitTest( m_RefPos ) )
        return SEARCH_CONTINUE;

    // Pins have special hit testing requirements that are relative to their parent
    // SCH_COMPONENT item.
    if( aItem->Type() == LIB_PIN_T )
    {
        wxCHECK_MSG( aTestData && ( (EDA_ITEM*) aTestData )->Type() == SCH_COMPONENT_T,
                     SEARCH_CONTINUE, wxT( "Cannot inspect invalid data.  Bad programmer!" ) );

        // Pin hit testing is relative to the components position and orientation in the
        // schematic.  The hit test position must be converted to library coordinates.
        SCH_COMPONENT* component = (SCH_COMPONENT*) aTestData;
        TRANSFORM transform = component->GetTransform().InverseTransform();
        wxPoint position = transform.TransformCoordinate( m_RefPos - component->m_Pos );

        position.y *= -1;   // Y axis polarity in schematic is inverted from library.

        if( !aItem->HitTest( position ) )
            return SEARCH_CONTINUE;
    }

    Append( aItem );

    return SEARCH_CONTINUE;
}


void SCH_COLLECTOR::Collect( SCH_ITEM* aItem, const KICAD_T aFilterList[],
                             const wxPoint& aPosition )
{
    Empty();        // empty the collection just in case

    SetScanTypes( aFilterList );

    // remember where the snapshot was taken from and pass refPos to the Inspect() function.
    SetRefPos( aPosition );

    EDA_ITEM::IterateForward( aItem, this, NULL, m_ScanTypes );
}


bool SCH_COLLECTOR::IsCorner() const
{
    if( GetCount() != 2 )
        return false;

    if( (m_List[0]->Type() == SCH_LINE_T) && (m_List[1]->Type() == SCH_LINE_T) )
        return true;

    if( (m_List[0]->Type() == SCH_LINE_T) && (m_List[1]->Type() == SCH_BUS_ENTRY_T) )
        return true;

    if( (m_List[0]->Type() == SCH_BUS_ENTRY_T) && (m_List[1]->Type() == SCH_LINE_T) )
        return true;

    return false;
}


bool SCH_COLLECTOR::IsNode( bool aIncludePins ) const
{
    for( size_t i = 0;  i < m_List.size();  i++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) m_List[ i ];
        KICAD_T type = item->Type();

        if( type == SCH_JUNCTION_T )
            continue;

        if( type == SCH_LINE_T )
        {
            if( item->GetLayer() != LAYER_WIRE )
                return false;

            continue;
        }

        if( type == LIB_PIN_T )
        {
            if( !aIncludePins )
                return false;

            continue;
        }

        // Any other item types indicate that this collection is not a node.
        return false;
    }

    return true;
}


bool SCH_COLLECTOR::IsDraggableJunction() const
{
    int wireEndCount = 0;
    int wireMidPoint = 0;
    int junctionCount = 0;

    for( size_t i = 0;  i < m_List.size();  i++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) m_List[ i ];
        KICAD_T type = item->Type();

        if( type == SCH_JUNCTION_T )
        {
            junctionCount++;
            continue;
        }

        if( type == SCH_LINE_T )
        {
            if( item->GetLayer() != LAYER_WIRE )
                return false;

            SCH_LINE* line = (SCH_LINE*) item;

            if( line->IsEndPoint( m_RefPos ) )
                wireEndCount++;
            else
                wireMidPoint++;

            continue;
        }

        // Any other item types indicate that this collection is not a draggable junction.
        return false;
    }

    return (wireEndCount >= 3) || ((wireEndCount >= 1) && (wireMidPoint == 1))
        || ((wireMidPoint >= 2) && (junctionCount == 1));
}
