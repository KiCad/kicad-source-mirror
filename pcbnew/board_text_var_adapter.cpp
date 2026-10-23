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
            {
                return ExtractSourceKeys( aItem );
            } );
}


std::vector<TEXT_VAR_REF_KEY> BOARD_TEXT_VAR_ADAPTER::collectKeys( BOARD_ITEM* aItem )
{
    std::vector<TEXT_VAR_REF_KEY> keys;

    auto collect =
            [&]( BOARD_ITEM* item )
            {
                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( item ) )
                {
                    for( const TEXT_VAR_REF_KEY& key : FilterTrackable( text->GetTextVarReferences() ) )
                    {
                        if( std::find( keys.begin(), keys.end(), key ) == keys.end() )
                            keys.push_back( key );
                    }
                }
                // PCB_BARCODE wraps an EDA_TEXT internally
                else if( PCB_BARCODE* barcode = dynamic_cast<PCB_BARCODE*>( aItem ) )
                {
                    for( const TEXT_VAR_REF_KEY& key : FilterTrackable( barcode->GetTextVarReferences() ) )
                    {
                        if( std::find( keys.begin(), keys.end(), key ) == keys.end() )
                            keys.push_back( key );
                    }
                }
            };

    collect( aItem );

    // Group members participate in the COMMIT architecture as first-class items.  FOOTPRINT and
    // PCB_TABLE children do not, and must therefore be collected into their parent's registration
    // list
    if ( aItem->Type() != PCB_GROUP_T )
        aItem->RunOnChildren( collect, RECURSE_MODE::RECURSE );

    return keys;
}


void BOARD_TEXT_VAR_ADAPTER::registerItem( BOARD_ITEM* aItem )
{
    if( aItem )
        m_tracker.RegisterItem( aItem, collectKeys( aItem ) );
}


void BOARD_TEXT_VAR_ADAPTER::unregisterItem( BOARD_ITEM* aItem )
{
    if( aItem )
        m_tracker.UnregisterItem( aItem );
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemAdded( BOARD&, BOARD_ITEM* aItem )
{
    // Register the item, AND handle any fan-out from other items using this item as a source.
    // You might think no one could yet be referencing something that was just added, but if the
    // add is the result of an undo of a delete, then any dangling cross-reference should get a
    // chance to re-link to the item.
    m_tracker.HandleItemChanged( aItem, collectKeys( aItem ) );
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems )
{
    for( BOARD_ITEM* item : aItems )
        OnBoardItemAdded( aBoard, item );
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemRemoved( BOARD&, BOARD_ITEM* aItem )
{
    // Handle any fan-out from other items using this item as a source, and then unregister it.
    m_tracker.HandleItemChanged( aItem, collectKeys( aItem ) );
    unregisterItem( aItem );
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems )
{
    for( BOARD_ITEM* item : aItems )
        OnBoardItemRemoved( aBoard, item );
}


void BOARD_TEXT_VAR_ADAPTER::OnBoardItemChanged( BOARD&, BOARD_ITEM* aItem )
{
    // Re-register the item's own refs (its text may have changed), then fan out any cross-ref keys
    // this item could source so dependents refresh.
    m_tracker.HandleItemChanged( aItem, collectKeys( aItem ) );
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
        OnBoardItemRemoved( aBoard, item );

    for( BOARD_ITEM* item : aAdded )
        OnBoardItemAdded( aBoard, item );

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

    FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aItem );

    if( PCB_FIELD* field = dynamic_cast<PCB_FIELD*>( aItem ) )
        fp = field->GetParentFootprint();

    if( !fp )
        return out;

    // Any field edit fans out to every ${U1:*} dependent, which is cheaper than coupling to the
    // commit system for a per-field diff against a pre-image
    const wxString refdes = fp->GetReference();

    if( !refdes.IsEmpty() )
        addKey( refdes );

    addKey( fp->m_Uuid.AsString() );

    return out;
}
