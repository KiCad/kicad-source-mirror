/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_sheet_path.h>
#include <transform.h>
#include <ee_collectors.h>
#include <sch_component.h>
#include <sch_line.h>
#include <sch_bus_entry.h>


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
    SCH_COMPONENT_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    SCH_BITMAP_T,
    SCH_LINE_T,
    EOT
};


const KICAD_T EE_COLLECTOR::ComponentsOnly[] = {
    SCH_COMPONENT_T,
    EOT
};


const KICAD_T EE_COLLECTOR::SheetsOnly[] = {
    SCH_SHEET_T,
    EOT
};


SEARCH_RESULT EE_COLLECTOR::Inspect( EDA_ITEM* aItem, void* aTestData )
{
    if( aItem->Type() == LIB_PIN_T )
    {
        // Special selection rules apply to pins of different units when edited in
        // synchronized pins mode.  Leave it to EE_SELECTION_TOOL::isSelectable() to
        // decide what to do with them.
    }
    else if( m_Unit || m_Convert )
    {
        LIB_ITEM* lib_item = dynamic_cast<LIB_ITEM*>( aItem );

        if( m_Unit && lib_item && lib_item->GetUnit() && lib_item->GetUnit() != m_Unit )
            return SEARCH_CONTINUE;

        if( m_Convert && lib_item && lib_item->GetConvert() && lib_item->GetConvert() != m_Convert )
            return SEARCH_CONTINUE;
    }

    if( aItem->HitTest( m_RefPos, m_Threshold ) )
        Append( aItem );

    return SEARCH_CONTINUE;
}


void EE_COLLECTOR::Collect( EDA_ITEM* aItem, const KICAD_T aFilterList[], const wxPoint& aPos,
                            int aUnit, int aConvert )
{
    Empty();        // empty the collection just in case

    SetScanTypes( aFilterList );
    m_Unit = aUnit;
    m_Convert = aConvert;

    // remember where the snapshot was taken from and pass refPos to the Inspect() function.
    SetRefPos( aPos );

    // aItem can be null for empty schematics
    if( aItem && aItem->Type() == LIB_PART_T )
        static_cast<LIB_PART*>( aItem )->Visit( m_inspector, nullptr, m_ScanTypes );
    else
        EDA_ITEM::IterateForward( aItem, m_inspector, nullptr, m_ScanTypes );
}


bool EE_COLLECTOR::IsCorner() const
{
    if( GetCount() != 2 )
        return false;

    bool is_busentry0 = (dynamic_cast<SCH_BUS_ENTRY_BASE*>( m_List[0] ) != NULL);
    bool is_busentry1 = (dynamic_cast<SCH_BUS_ENTRY_BASE*>( m_List[1] ) != NULL);

    if( (m_List[0]->Type() == SCH_LINE_T) && (m_List[1]->Type() == SCH_LINE_T) )
        return ( ( SCH_LINE* ) m_List[0])->GetLayer() == ( ( SCH_LINE* ) m_List[1])->GetLayer();

    if( (m_List[0]->Type() == SCH_LINE_T) && is_busentry1 )
        return true;

    if( is_busentry0 && (m_List[1]->Type() == SCH_LINE_T) )
        return true;

    return false;
}


bool EE_COLLECTOR::IsDraggableJunction() const
{
    for( size_t i = 0;  i < m_List.size();  i++ )
        if( ( (SCH_ITEM*) m_List[ i ] )->Type() == SCH_JUNCTION_T )
            return true;

    return false;
}


SEARCH_RESULT EE_TYPE_COLLECTOR::Inspect( EDA_ITEM* aItem, void* testData )
{
    // The Vist() function only visits the testItem if its type was in the
    // the scanList, so therefore we can collect anything given to us here.
    Append( aItem );

    return SEARCH_CONTINUE;
}


void EE_TYPE_COLLECTOR::Collect( EDA_ITEM* aItem, const KICAD_T aFilterList[] )
{
    Empty();        // empty the collection

    SetScanTypes( aFilterList );

    EDA_ITEM::IterateForward( aItem, m_inspector, NULL, m_ScanTypes );
}
