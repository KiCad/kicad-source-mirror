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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "schematic_text_var_adapter.h"

#include <algorithm>
#include <set>
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
            {
                return ExtractSourceKeys( aItem );
            } );
}


SCH_ITEM* SCHEMATIC_TEXT_VAR_ADAPTER::trackedItem( SCH_ITEM* aItem )
{
    // Fields, sheet pins and table cells live in their owner's storage and can move without
    // a notification, so only the screen item that owns them is ever indexed
    if( aItem && aItem->IsType( { SCH_FIELD_T, SCH_SHEET_PIN_T, SCH_TABLECELL_T } ) )
        return dynamic_cast<SCH_ITEM*>( aItem->GetParent() );

    return aItem;
}


std::vector<TEXT_VAR_REF_KEY> SCHEMATIC_TEXT_VAR_ADAPTER::collectKeys( SCH_ITEM* aItem )
{
    std::vector<TEXT_VAR_REF_KEY> keys;

    auto collect =
            [&]( SCH_ITEM* aTextItem )
            {
                EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aTextItem );

                if( !text )
                    return;

                for( const TEXT_VAR_REF_KEY& key : FilterTrackable( text->GetTextVarReferences() ) )
                {
                    if( std::find( keys.begin(), keys.end(), key ) == keys.end() )
                        keys.push_back( key );
                }
            };

    collect( aItem );

    // Group members are screen items in their own right
    if( aItem->Type() != SCH_GROUP_T )
        aItem->RunOnChildren( collect, RECURSE_MODE::NO_RECURSE );

    return keys;
}


std::vector<TEXT_VAR_REF_KEY> SCHEMATIC_TEXT_VAR_ADAPTER::trackKeys( SCH_ITEM* aItem )
{
    std::vector<TEXT_VAR_REF_KEY> keys = collectKeys( aItem );

    if( keys.empty() )
        m_registered.erase( aItem );
    else
        m_registered.insert( aItem );

    return keys;
}


void SCHEMATIC_TEXT_VAR_ADAPTER::registerItem( SCH_ITEM* aItem )
{
    m_tracker.RegisterItem( aItem, trackKeys( aItem ) );
}


void SCHEMATIC_TEXT_VAR_ADAPTER::handleItemChanged( SCH_ITEM* aItem )
{
    // Also fans out ${REFDES:FIELD} for symbols, which source cross-references
    m_tracker.HandleItemChanged( aItem, trackKeys( aItem ) );
}


void SCHEMATIC_TEXT_VAR_ADAPTER::noteSheets( const std::vector<SCH_ITEM*>& aItems )
{
    // A sheet add or remove forces a rebuild because a new screen can reuse a freed one's address.
    // Changed sheets keep their old screen alive in the undo image, so the screen set catches them
    for( SCH_ITEM* item : aItems )
    {
        if( item && item->Type() == SCH_SHEET_T )
            m_hierarchyChanged = true;
    }
}


void SCHEMATIC_TEXT_VAR_ADAPTER::OnSchItemsAdded( SCHEMATIC&, std::vector<SCH_ITEM*>& aItems )
{
    noteSheets( aItems );

    for( SCH_ITEM* item : aItems )
    {
        if( SCH_ITEM* tracked = trackedItem( item ) )
            registerItem( tracked );
    }
}


void SCHEMATIC_TEXT_VAR_ADAPTER::OnSchItemsRemoved( SCHEMATIC&, std::vector<SCH_ITEM*>& aItems )
{
    noteSheets( aItems );

    for( SCH_ITEM* item : aItems )
    {
        SCH_ITEM* tracked = trackedItem( item );

        if( tracked == item )
        {
            m_tracker.UnregisterItem( item );
            m_registered.erase( item );
        }
        else if( tracked )
        {
            // A child left its owner, which stays on the screen
            handleItemChanged( tracked );
        }
    }
}


void SCHEMATIC_TEXT_VAR_ADAPTER::OnSchItemsChanged( SCHEMATIC&, std::vector<SCH_ITEM*>& aItems )
{
    for( SCH_ITEM* item : aItems )
    {
        if( SCH_ITEM* tracked = trackedItem( item ) )
            handleItemChanged( tracked );
    }
}


std::set<SCH_SCREEN*> SCHEMATIC_TEXT_VAR_ADAPTER::hierarchyScreens() const
{
    std::set<SCH_SCREEN*> screens;

    for( const SCH_SHEET_PATH& path : m_schematic.Hierarchy() )
    {
        if( SCH_SCREEN* screen = path.LastScreen() )
            screens.insert( screen );
    }

    return screens;
}


void SCHEMATIC_TEXT_VAR_ADAPTER::SyncToHierarchy()
{
    if( !m_hierarchyChanged && hierarchyScreens() == m_indexedScreens )
        return;

    RebuildIndex();
}


void SCHEMATIC_TEXT_VAR_ADAPTER::RebuildIndex()
{
    // The drawing sheet shares this tracker, so drop only what this adapter owns.
    // Stale pointers are never dereferenced, only used as keys
    for( SCH_ITEM* item : m_registered )
        m_tracker.UnregisterItem( item );

    m_registered.clear();
    m_indexedScreens = hierarchyScreens();
    m_hierarchyChanged = false;

    for( SCH_SCREEN* screen : m_indexedScreens )
    {
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

    auto addKey =
            [&]( const wxString& aPrimary )
            {
                if( aPrimary.IsEmpty() )
                    return;

                TEXT_VAR_REF_KEY key;
                key.kind    = TEXT_VAR_REF_KEY::KIND::CROSS_REF;
                key.primary = aPrimary;

                if( std::find( out.begin(), out.end(), key ) == out.end() )
                    out.push_back( key );
            };

    // Must match the KIID path SCHEMATIC::ConvertRefsToKIIDs writes into stored text
    auto addInstance =
            [&]( const SCH_SHEET_PATH& aPath )
            {
                KIID_PATH path = aPath.Path();
                path.push_back( sym->m_Uuid );

                addKey( sym->GetRef( &aPath, false ) );
                addKey( path.AsString() );
            };

    const SCH_SHEET_LIST& hierarchy = m_schematic.Hierarchy();

    if( hierarchy.empty() )
    {
        addInstance( m_schematic.CurrentSheet() );
        return out;
    }

    // GetRef falls back to the REFERENCE field on paths that are not instances of this
    // symbol, so only visit paths that actually show the symbol's screen
    const SCH_SCREEN* parentScreen = dynamic_cast<const SCH_SCREEN*>( sym->GetParent() );

    for( const SCH_SHEET_PATH& path : hierarchy )
    {
        if( path.LastScreen() == parentScreen )
            addInstance( path );
    }

    return out;
}
