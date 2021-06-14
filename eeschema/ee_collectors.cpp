/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <ee_collectors.h>
#include <lib_item.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <transform.h>
#include "sch_reference_list.h"


const KICAD_T EE_COLLECTOR::AllItems[] = {
    SCH_LOCATE_ANY_T,
    EOT
};


const KICAD_T EE_COLLECTOR::EditableItems[] = {
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_FIELD_T,
    SCH_SYMBOL_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    SCH_BITMAP_T,
    SCH_LINE_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_JUNCTION_T,
    EOT
};


const KICAD_T EE_COLLECTOR::SymbolsOnly[] = {
    SCH_SYMBOL_T,
    EOT
};


const KICAD_T EE_COLLECTOR::SheetsOnly[] = {
    SCH_SHEET_T,
    EOT
};


const KICAD_T EE_COLLECTOR::MovableItems[] =
{
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_LINE_T,
    SCH_BITMAP_T,
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_FIELD_T,
    SCH_SYMBOL_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    EOT
};


const KICAD_T EE_COLLECTOR::WiresOnly[] = {
    SCH_LINE_T,
    EOT
};


const KICAD_T EE_COLLECTOR::FieldOwners[] = {
    SCH_SYMBOL_T,
    SCH_SHEET_T,
    SCH_GLOBAL_LABEL_T,
    EOT
};


SEARCH_RESULT EE_COLLECTOR::Inspect( EDA_ITEM* aItem, void* aTestData )
{
    if( aItem->Type() == LIB_PIN_T )
    {
        // Special selection rules apply to pins of different units when edited in
        // synchronized pins mode.  Leave it to EE_SELECTION_TOOL::Selectable() to
        // decide what to do with them.
    }
    else if( m_Unit || m_Convert )
    {
        LIB_ITEM* lib_item = dynamic_cast<LIB_ITEM*>( aItem );

        if( m_Unit && lib_item && lib_item->GetUnit() && lib_item->GetUnit() != m_Unit )
            return SEARCH_RESULT::CONTINUE;

        if( m_Convert && lib_item && lib_item->GetConvert() && lib_item->GetConvert() != m_Convert )
            return SEARCH_RESULT::CONTINUE;
    }

    if( aItem->HitTest( m_refPos, m_Threshold ) )
        Append( aItem );

    return SEARCH_RESULT::CONTINUE;
}


void EE_COLLECTOR::Collect( SCH_SCREEN* aScreen, const KICAD_T aFilterList[], const wxPoint& aPos,
                            int aUnit, int aConvert )
{
    Empty(); // empty the collection just in case

    SetScanTypes( aFilterList );
    m_Unit    = aUnit;
    m_Convert = aConvert;

    // remember where the snapshot was taken from and pass refPos to the Inspect() function.
    SetRefPos( aPos );

    if( aScreen )
    {
        // Symbols and sheets own their own children so have to be visited even if
        // they're not in the filter list
        bool symbolsVisited = false;
        bool sheetsVisited = false;
        bool globalLabelsVisited = false;

        for( const KICAD_T* filter = aFilterList; *filter != EOT; ++filter )
        {
            for( SCH_ITEM* item : aScreen->Items().OfType( *filter ) )
            {
                if( *filter == SCH_SYMBOL_T || *filter == SCH_LOCATE_ANY_T )
                    symbolsVisited = true;

                if( *filter == SCH_SHEET_T || *filter == SCH_LOCATE_ANY_T )
                    sheetsVisited = true;

                if( *filter == SCH_GLOBAL_LABEL_T || *filter == SCH_LOCATE_ANY_T )
                    globalLabelsVisited = true;

                item->Visit( m_inspector, nullptr, m_scanTypes );
            }
        }

        if( !symbolsVisited )
        {
            for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
                item->Visit( m_inspector, nullptr, m_scanTypes );
        }

        if( !sheetsVisited )
        {
            for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SHEET_T ) )
                item->Visit( m_inspector, nullptr, m_scanTypes );
        }

        if( !globalLabelsVisited )
        {
            for( SCH_ITEM* item : aScreen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
                item->Visit( m_inspector, nullptr, m_scanTypes );
        }
    }
}


void EE_COLLECTOR::Collect( LIB_ITEMS_CONTAINER& aItems, const KICAD_T aFilterList[],
                            const wxPoint& aPos, int aUnit, int aConvert )
{
    Empty();        // empty the collection just in case

    SetScanTypes( aFilterList );
    m_Unit = aUnit;
    m_Convert = aConvert;

    // remember where the snapshot was taken from and pass refPos to the Inspect() function.
    SetRefPos( aPos );

    for( LIB_ITEM& item : aItems )
    {
        if( item.Visit( m_inspector, nullptr, m_scanTypes ) == SEARCH_RESULT::QUIT )
            break;
    }
}


bool EE_COLLECTOR::IsCorner() const
{
    if( GetCount() != 2 )
        return false;

    bool is_busentry0 = ( dynamic_cast<SCH_BUS_ENTRY_BASE*>( m_list[0] ) != NULL);
    bool is_busentry1 = ( dynamic_cast<SCH_BUS_ENTRY_BASE*>( m_list[1] ) != NULL);

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

        if( symbol.GetRef() == aRef && symbol.GetSymbol()->GetLibId() == aLibId
          && symbol.GetUnit() != aUnit )
            otherUnits->push_back( symbol.GetSymbol() );
    }
}


