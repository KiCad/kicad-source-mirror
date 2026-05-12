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

#include "board_text_var_adapter.h"

#include <eda_text.h>
#include <footprint.h>
#include <pcb_barcode.h>
#include <pcb_field.h>
#include <pcb_text.h>
#include <pcb_textbox.h>


BOARD_TEXT_VAR_ADAPTER::BOARD_TEXT_VAR_ADAPTER( BOARD& aBoard ) :
        m_board( aBoard )
{
    m_tracker.SetSourceKeyExtractor(
            [this]( EDA_ITEM* aItem ) -> std::vector<TEXT_VAR_REF_KEY>
            { return ExtractSourceKeys( aItem ); } );
}


void BOARD_TEXT_VAR_ADAPTER::registerItem( BOARD_ITEM* aItem )
{
    if( !aItem )
        return;

    // Route the dynamic_cast through the pcbnew DSO where RTTI for EDA_TEXT is
    // visible. The kicommon-side tracker stays RTTI-free.
    EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

    if( !text )
    {
        // PCB_BARCODE wraps an EDA_TEXT internally; register the barcode itself
        // (not its inner PCB_TEXT) so invalidation callbacks receive the barcode
        // as the dependent and the view can repaint it.
        if( PCB_BARCODE* bc = dynamic_cast<PCB_BARCODE*>( aItem ) )
        {
            m_tracker.RegisterItem( aItem, FilterTrackable( bc->GetTextVarReferences() ) );
            return;
        }

        // FOOTPRINTs aren't themselves EDA_TEXT. Walk both their fields AND
        // their graphical items — silkscreen/copper/courtyard layers can
        // carry PCB_TEXT/PCB_TEXTBOX that reference `${...}`.
        if( FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aItem ) )
        {
            for( PCB_FIELD* field : fp->GetFields() )
                registerItem( field );

            for( BOARD_ITEM* child : fp->GraphicalItems() )
                registerItem( child );
        }

        return;
    }

    m_tracker.RegisterItem( aItem, FilterTrackable( text->GetTextVarReferences() ) );
}


void BOARD_TEXT_VAR_ADAPTER::unregisterItem( BOARD_ITEM* aItem )
{
    if( !aItem )
        return;

    m_tracker.UnregisterItem( aItem );

    if( FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aItem ) )
    {
        for( PCB_FIELD* field : fp->GetFields() )
            m_tracker.UnregisterItem( field );

        for( BOARD_ITEM* child : fp->GraphicalItems() )
            unregisterItem( child );
    }
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemAdded( BOARD&, BOARD_ITEM* aItem )
{
    registerItem( aItem );
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemsAdded( BOARD&, std::vector<BOARD_ITEM*>& aItems )
{
    for( BOARD_ITEM* item : aItems )
        registerItem( item );
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemRemoved( BOARD&, BOARD_ITEM* aItem )
{
    unregisterItem( aItem );
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemsRemoved( BOARD&, std::vector<BOARD_ITEM*>& aItems )
{
    for( BOARD_ITEM* item : aItems )
        unregisterItem( item );
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemChanged( BOARD&, BOARD_ITEM* aItem )
{
    if( !aItem )
        return;

    // Re-register the item's own refs (its text may have changed), then fan
    // out any cross-ref keys this item could source so dependents refresh.
    EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

    if( text )
    {
        std::vector<TEXT_VAR_REF_KEY> updated = FilterTrackable( text->GetTextVarReferences() );
        m_tracker.HandleItemChanged( aItem, updated );
    }
    else if( PCB_BARCODE* bc = dynamic_cast<PCB_BARCODE*>( aItem ) )
    {
        m_tracker.HandleItemChanged( aItem, FilterTrackable( bc->GetTextVarReferences() ) );
    }
    else
    {
        // Non-text item (e.g., FOOTPRINT) — its own children were registered
        // separately on add, so we only need the fan-out for source keys.
        m_tracker.HandleItemChanged( aItem, {} );

        if( FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aItem ) )
        {
            // Footprint fields may themselves have been edited (value/ref
            // change). Re-register each so the dependency graph stays current.
            for( PCB_FIELD* field : fp->GetFields() )
            {
                std::vector<TEXT_VAR_REF_KEY> fieldRefs = FilterTrackable( field->GetTextVarReferences() );
                m_tracker.RegisterItem( field, fieldRefs );
            }

            // Graphical items (silkscreen text etc.) may also reference
            // text vars and may have been edited out-of-band.
            for( BOARD_ITEM* child : fp->GraphicalItems() )
            {
                if( EDA_TEXT* childText = dynamic_cast<EDA_TEXT*>( child ) )
                {
                    m_tracker.RegisterItem( child,
                            FilterTrackable( childText->GetTextVarReferences() ) );
                }
            }
        }
    }
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems )
{
    for( BOARD_ITEM* item : aItems )
        OnBoardItemChanged( aBoard, item );
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardCompositeUpdate( BOARD&                    aBoard,
                                                    std::vector<BOARD_ITEM*>& aAdded,
                                                    std::vector<BOARD_ITEM*>& aRemoved,
                                                    std::vector<BOARD_ITEM*>& aChanged )
{
    // Apply in order: remove, add, change. Removals first so re-added items
    // with recycled pointers don't collide in the index. Changes last so
    // fan-outs see the updated board state.
    for( BOARD_ITEM* item : aRemoved )
        unregisterItem( item );

    for( BOARD_ITEM* item : aAdded )
        registerItem( item );

    for( BOARD_ITEM* item : aChanged )
        OnBoardItemChanged( aBoard, item );
}


void BOARD_TEXT_VAR_ADAPTER::RebuildIndex()
{
    m_tracker.Clear();

    for( FOOTPRINT* fp : m_board.Footprints() )
        registerItem( fp );

    for( BOARD_ITEM* item : m_board.Drawings() )
        registerItem( item );
}


std::vector<TEXT_VAR_REF_KEY> BOARD_TEXT_VAR_ADAPTER::ExtractSourceKeys( EDA_ITEM* aItem ) const
{
    std::vector<TEXT_VAR_REF_KEY> out;

    FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aItem );

    if( !fp )
        return out;

    // A footprint U1 sources `${U1:<FIELDNAME>}` for every named field it
    // carries. This is a conservative over-approximation — an edit to any
    // footprint field fans out to every dependent on ${U1:*}, even if the
    // specific field they reference wasn't the one that changed. The blast
    // radius is bounded by the number of actual dependents, so over-
    // invalidation is cheap relative to the alternative (per-field diff
    // against a pre-image snapshot, which would couple the adapter to the
    // commit system).
    const wxString refdes = fp->GetReference();

    if( refdes.IsEmpty() )
        return out;

    out.reserve( fp->GetFields().size() );

    for( PCB_FIELD* field : fp->GetFields() )
    {
        TEXT_VAR_REF_KEY key;
        key.kind      = TEXT_VAR_REF_KEY::KIND::CROSS_REF;
        key.primary   = refdes;
        key.secondary = field->GetName();
        out.push_back( key );
    }

    return out;
}
