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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#include "schematic_text_var_adapter.h"

#include <algorithm>
#include <eda_text.h>
#include <sch_field.h>
#include <sch_item.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>


SCHEMATIC_TEXT_VAR_ADAPTER::SCHEMATIC_TEXT_VAR_ADAPTER( SCHEMATIC& aSchematic ) :
        m_schematic( aSchematic )
{
    m_tracker.SetSourceKeyExtractor(
            [this]( EDA_ITEM* aItem ) -> std::vector<TEXT_VAR_REF_KEY>
            { return ExtractSourceKeys( aItem ); } );
}


void SCHEMATIC_TEXT_VAR_ADAPTER::registerItem( SCH_ITEM* aItem )
{
    if( !aItem )
        return;

    // SCH_SYMBOL: register its constituent SCH_FIELDs (not the symbol itself).
    // The symbol is a cross-ref source, not a dependent.
    if( SCH_SYMBOL* sym = dynamic_cast<SCH_SYMBOL*>( aItem ) )
    {
        for( SCH_FIELD& field : sym->GetFields() )
            registerItem( &field );

        return;
    }

    // SCH_SHEET: its fields (sheet name, file name) can carry text vars.
    // Sheet pins are separate sch items and flow through the listener on
    // their own when added/removed.
    if( SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( aItem ) )
    {
        for( SCH_FIELD& field : sheet->GetFields() )
            registerItem( &field );

        return;
    }

    EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

    if( !text )
        return;

    m_tracker.RegisterItem( aItem, FilterTrackable( text->GetTextVarReferences() ) );
}


void SCHEMATIC_TEXT_VAR_ADAPTER::unregisterItem( SCH_ITEM* aItem )
{
    if( !aItem )
        return;

    m_tracker.UnregisterItem( aItem );

    if( SCH_SYMBOL* sym = dynamic_cast<SCH_SYMBOL*>( aItem ) )
    {
        for( SCH_FIELD& field : sym->GetFields() )
            m_tracker.UnregisterItem( &field );
    }
    else if( SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( aItem ) )
    {
        for( SCH_FIELD& field : sheet->GetFields() )
            m_tracker.UnregisterItem( &field );
    }
}


void SCHEMATIC_TEXT_VAR_ADAPTER::handleItemChanged( SCH_ITEM* aItem )
{
    if( !aItem )
        return;

    // A SCH_SYMBOL change covers both "its fields were edited" (re-register
    // each field) and "this symbol's values source cross-refs" (fan out
    // ${REFDES:FIELD} keys).
    if( SCH_SYMBOL* sym = dynamic_cast<SCH_SYMBOL*>( aItem ) )
    {
        for( SCH_FIELD& field : sym->GetFields() )
        {
            std::vector<TEXT_VAR_REF_KEY> refs = FilterTrackable( field.GetTextVarReferences() );
            m_tracker.RegisterItem( &field, refs );
        }

        m_tracker.HandleItemChanged( aItem, {} );
        return;
    }

    if( SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( aItem ) )
    {
        for( SCH_FIELD& field : sheet->GetFields() )
        {
            std::vector<TEXT_VAR_REF_KEY> refs = FilterTrackable( field.GetTextVarReferences() );
            m_tracker.RegisterItem( &field, refs );
        }

        return;
    }

    if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem ) )
    {
        std::vector<TEXT_VAR_REF_KEY> updated = FilterTrackable( text->GetTextVarReferences() );
        m_tracker.HandleItemChanged( aItem, updated );
    }
}


void SCHEMATIC_TEXT_VAR_ADAPTER::OnSchItemsAdded( SCHEMATIC&,
                                                 std::vector<SCH_ITEM*>& aItems )
{
    for( SCH_ITEM* item : aItems )
        registerItem( item );
}


void SCHEMATIC_TEXT_VAR_ADAPTER::OnSchItemsRemoved( SCHEMATIC&,
                                                   std::vector<SCH_ITEM*>& aItems )
{
    for( SCH_ITEM* item : aItems )
        unregisterItem( item );
}


void SCHEMATIC_TEXT_VAR_ADAPTER::OnSchItemsChanged( SCHEMATIC&,
                                                   std::vector<SCH_ITEM*>& aItems )
{
    for( SCH_ITEM* item : aItems )
        handleItemChanged( item );
}


void SCHEMATIC_TEXT_VAR_ADAPTER::RebuildIndex()
{
    m_tracker.Clear();

    // Walk the hierarchy. Each screen can be referenced by multiple sheet
    // paths, but the SCH_ITEM pointers are shared — we only register each
    // item once via the screen traversal.
    for( const SCH_SHEET_PATH& path : m_schematic.Hierarchy() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items() )
            registerItem( item );
    }
}


std::vector<TEXT_VAR_REF_KEY> SCHEMATIC_TEXT_VAR_ADAPTER::ExtractSourceKeys( EDA_ITEM* aItem ) const
{
    std::vector<TEXT_VAR_REF_KEY> out;

    SCH_SYMBOL* sym = dynamic_cast<SCH_SYMBOL*>( aItem );

    if( !sym )
        return out;

    // Repeated-sheet instances: a single SCH_SYMBOL can carry different
    // reference designators on each SCH_SHEET_PATH it participates in.
    // Collect every distinct refdes the symbol currently has across the
    // hierarchy so each ${REFDES:FIELD} dependent fan-out reaches the right
    // dependents. Over-approximation (firing U1:Value when U2 — same symbol
    // on a different sheet — is the actual edit target) is the acceptable
    // tradeoff for not yet carrying sheet-path identity in the key itself.
    std::vector<wxString> refdesList;
    const SCH_SHEET_LIST& hierarchy = m_schematic.Hierarchy();

    if( hierarchy.empty() )
    {
        // No hierarchy yet (e.g., bare SCHEMATIC before sheets added) —
        // fall back to the current sheet context.
        const wxString refdes = sym->GetRef( &m_schematic.CurrentSheet(), false );

        if( !refdes.IsEmpty() )
            refdesList.push_back( refdes );
    }
    else
    {
        // SCH_SYMBOL::GetRef falls back to its REFERENCE field when the query
        // path is not one of its instances, so iterating every path and
        // calling GetRef would pollute the list with the same refdes from
        // unrelated sheets. Filter to paths whose last screen matches the
        // symbol's parent screen — those are the ones where this symbol
        // actually lives.
        const SCH_SCREEN* parentScreen = dynamic_cast<const SCH_SCREEN*>( sym->GetParent() );

        for( const SCH_SHEET_PATH& path : hierarchy )
        {
            if( path.LastScreen() != parentScreen )
                continue;

            const wxString refdes = sym->GetRef( &path, false );

            if( refdes.IsEmpty() )
                continue;

            if( std::find( refdesList.begin(), refdesList.end(), refdes ) == refdesList.end() )
                refdesList.push_back( refdes );
        }
    }

    if( refdesList.empty() )
        return out;

    out.reserve( refdesList.size() * sym->GetFields().size() );

    for( const wxString& refdes : refdesList )
    {
        for( const SCH_FIELD& field : sym->GetFields() )
        {
            TEXT_VAR_REF_KEY key;
            key.kind      = TEXT_VAR_REF_KEY::KIND::CROSS_REF;
            key.primary   = refdes;
            key.secondary = field.GetCanonicalName();
            out.push_back( key );
        }
    }

    return out;
}
