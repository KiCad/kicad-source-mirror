/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2019 CERN
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
#include <trace_helpers.h>
#include <sch_collectors.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <transform.h>
#include "sch_reference_list.h"


const std::vector<KICAD_T> SCH_COLLECTOR::EditableItems = {
    SCH_SHAPE_T,
    SCH_TEXT_T,
    SCH_TEXTBOX_T,
    SCH_TABLECELL_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_DIRECTIVE_LABEL_T,
    SCH_FIELD_T,
    SCH_SYMBOL_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    SCH_BITMAP_T,
    SCH_LINE_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_JUNCTION_T,
    SCH_RULE_AREA_T,
    SCH_GROUP_T
};


const std::vector<KICAD_T> SCH_COLLECTOR::MovableItems =
{
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_LINE_T,
    SCH_BITMAP_T,
    SCH_SHAPE_T,
    SCH_TEXT_T,
    SCH_TEXTBOX_T,
    SCH_TABLE_T,
    SCH_TABLECELL_T,    // will be promoted to parent table(s)
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_DIRECTIVE_LABEL_T,
    SCH_FIELD_T,
    SCH_SYMBOL_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    SCH_RULE_AREA_T,
    SCH_GROUP_T
};


const std::vector<KICAD_T> SCH_COLLECTOR::FieldOwners =
{
    SCH_SYMBOL_T,
    SCH_SHEET_T,
    SCH_LABEL_LOCATE_ANY_T
};


const std::vector<KICAD_T> SCH_COLLECTOR::DeletableItems =
{
    LIB_SYMBOL_T,
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_LINE_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_SHAPE_T,
    SCH_RULE_AREA_T,
    SCH_TEXT_T,
    SCH_TEXTBOX_T,
    SCH_TABLECELL_T,    // Clear contents
    SCH_TABLE_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_DIRECTIVE_LABEL_T,
    SCH_NO_CONNECT_T,
    SCH_SHEET_T,
    SCH_SHEET_PIN_T,
    SCH_SYMBOL_T,
    SCH_FIELD_T,        // Will be hidden
    SCH_BITMAP_T,
    SCH_GROUP_T
};


INSPECT_RESULT SCH_COLLECTOR::Inspect( EDA_ITEM* aItem, void* aTestData )
{
    if( m_Unit || m_BodyStyle )
    {
        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( aItem );

        // Special selection rules apply to pins of different units when edited in synchronized
        // pins mode.  Leave it to SCH_SELECTION_TOOL::Selectable() to decide what to do with them.

        if( schItem && schItem->Type() != SCH_PIN_T )
        {
            if( m_Unit && schItem->GetUnit() && schItem->GetUnit() != m_Unit )
                return INSPECT_RESULT::CONTINUE;

            if( m_BodyStyle && schItem->GetBodyStyle() && schItem->GetBodyStyle() != m_BodyStyle )
                return INSPECT_RESULT::CONTINUE;
        }
    }

    if( m_ShowPinElectricalTypes )
        aItem->SetFlags( SHOW_ELEC_TYPE );

    if( aItem->HitTest( m_refPos, m_Threshold ) )
        Append( aItem );

    aItem->ClearFlags( SHOW_ELEC_TYPE );

    return INSPECT_RESULT::CONTINUE;
}


void SCH_COLLECTOR::Collect( SCH_SCREEN* aScreen, const std::vector<KICAD_T>& aFilterList,
                             const VECTOR2I& aPos, int aUnit, int aBodyStyle )
{
    Empty(); // empty the collection just in case

    SetScanTypes( aFilterList );
    m_Unit = aUnit;
    m_BodyStyle = aBodyStyle;

    // remember where the snapshot was taken from and pass refPos to the Inspect() function.
    SetRefPos( aPos );

    if( aScreen )
    {
        for( SCH_ITEM *item : aScreen->Items().Overlapping( SCH_LOCATE_ANY_T, aPos, m_Threshold ) )
            item->Visit( m_inspector, nullptr, m_scanTypes );
    }
}


void SCH_COLLECTOR::Collect( LIB_ITEMS_CONTAINER& aItems, const std::vector<KICAD_T>& aFilterList,
                             const VECTOR2I& aPos, int aUnit, int aBodyStyle )
{
    Empty();        // empty the collection just in case

    SetScanTypes( aFilterList );
    m_Unit = aUnit;
    m_BodyStyle = aBodyStyle;

    // remember where the snapshot was taken from and pass refPos to the Inspect() function.
    SetRefPos( aPos );

    for( SCH_ITEM& item : aItems )
    {
        if( item.Visit( m_inspector, nullptr, m_scanTypes ) == INSPECT_RESULT::QUIT )
            break;
    }
}


bool SCH_COLLECTOR::IsCorner() const
{
    if( GetCount() != 2 )
        return false;

    bool is_busentry0 = ( dynamic_cast<SCH_BUS_ENTRY_BASE*>( m_list[0] ) != nullptr );
    bool is_busentry1 = ( dynamic_cast<SCH_BUS_ENTRY_BASE*>( m_list[1] ) != nullptr );

    if(( m_list[0]->Type() == SCH_LINE_T) && ( m_list[1]->Type() == SCH_LINE_T) )
        return ( ( SCH_LINE* ) m_list[0])->GetLayer() == ( ( SCH_LINE* ) m_list[1])->GetLayer();

    if(( m_list[0]->Type() == SCH_LINE_T) && is_busentry1 )
        return true;

    if( is_busentry0 && ( m_list[1]->Type() == SCH_LINE_T) )
        return true;

    return false;
}


void CollectOtherUnits( const wxString& aRef, int aUnit, const LIB_ID& aLibId,
                        SCH_SHEET_PATH& aSheet, std::vector<SCH_SYMBOL*>* otherUnits )
{
    SCH_REFERENCE_LIST symbols;
    aSheet.GetSymbols( symbols );

    for( unsigned i = 0; i < symbols.GetCount(); i++ )
    {
        SCH_REFERENCE symbol = symbols[i];

        if( symbol.GetRef() == aRef
                && symbol.GetSymbol()->GetLibId() == aLibId
                && symbol.GetUnit() != aUnit )
        {
            otherUnits->push_back( symbol.GetSymbol() );
        }
    }
}


