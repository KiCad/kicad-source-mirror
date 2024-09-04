/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Created on: 11 Mar 2016, author John Beard
 * Copyright (C) 2016-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "array_creator.h"

#include <array_pad_number_provider.h>
#include <board_commit.h>
#include <pcb_group.h>
#include <pad.h>
#include <dialogs/dialog_create_array.h>
#include <tool/tool_manager.h>
#include <tools/board_reannotate_tool.h>
#include <tools/pcb_selection_tool.h>

/**
 * Transform a #BOARD_ITEM from the given #ARRAY_OPTIONS and an index into the array.
 *
 * @param aArrOpts The array options that describe the array
 * @param aIndex   The index in the array of this item
 * @param aItem    The item to transform
 */
static void TransformItem( const ARRAY_OPTIONS& aArrOpts, int aIndex, BOARD_ITEM& aItem )
{
    const ARRAY_OPTIONS::TRANSFORM transform = aArrOpts.GetTransform( aIndex, aItem.GetPosition() );

    aItem.Move( transform.m_offset );
    aItem.Rotate( aItem.GetPosition(), transform.m_rotation );
}


void ARRAY_CREATOR::Invoke()
{
    // bail out if no items
    if( m_selection.Size() == 0 )
        return;

    FOOTPRINT* const fp = m_isFootprintEditor ? m_parent.GetBoard()->GetFirstFootprint() : nullptr;

    const bool enableArrayNumbering = m_isFootprintEditor;
    VECTOR2I   origin;

    if( m_selection.Size() == 1 )
        origin = m_selection.Items()[0]->GetPosition();
    else
        origin = m_selection.GetCenter();

    std::unique_ptr<ARRAY_OPTIONS> array_opts;

    DIALOG_CREATE_ARRAY dialog( &m_parent, array_opts, enableArrayNumbering, origin );

    int ret = dialog.ShowModal();

    if( ret != wxID_OK || array_opts == nullptr )
        return;

    BOARD_COMMIT commit( &m_parent );

    ARRAY_PAD_NUMBER_PROVIDER pad_number_provider( fp, *array_opts );

    EDA_ITEMS all_added_items;

    // The first item in list is the original item. We do not modify it
    for( int ptN = 0; ptN < array_opts->GetArraySize(); ptN++ )
    {
        PCB_SELECTION items_for_this_block;
        std::set<FOOTPRINT*> fpDeDupe;

        for ( int i = 0; i < m_selection.Size(); ++i )
        {
            if( !m_selection[i]->IsBOARD_ITEM() )
                continue;

            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( m_selection[ i ] );

            FOOTPRINT* parentFootprint = item->GetParentFootprint();

            // If it is not the footprint editor, then duplicate the parent footprint instead.
            // This check assumes that the footprint child objects are correctly parented, if
            // they are not, this will segfault.
            if( !m_isFootprintEditor && parentFootprint )
            {
                // It is possible to select multiple footprint child objects in the board editor.
                // Do not create multiple copies of the same footprint when this occurs.
                if( fpDeDupe.count( parentFootprint ) == 0 )
                {
                    fpDeDupe.emplace( parentFootprint );
                    item = parentFootprint;
                }
                else
                {
                    continue;
                }
            }

            BOARD_ITEM* this_item = nullptr;

            if( ptN == 0 )
            {
                // the first point: we don't own this or add it, but
                // we might still modify it (position or label)
                this_item = item;

                commit.Modify( this_item );
                TransformItem( *array_opts, ptN, *this_item );
            }
            else
            {
                if( m_isFootprintEditor )
                {
                    // Fields cannot be duplicated, especially mandatory fields.
                    // A given field is unique for the footprint
                    if( item->Type() == PCB_FIELD_T )
                        continue;

                    // Don't bother incrementing pads: the footprint won't update until commit,
                    // so we can only do this once
                    this_item = fp->DuplicateItem( item );
                }
                else
                {
                    switch( item->Type() )
                    {
                    case PCB_FOOTPRINT_T:
                    case PCB_SHAPE_T:
                    case PCB_REFERENCE_IMAGE_T:
                    case PCB_GENERATOR_T:
                    case PCB_TEXT_T:
                    case PCB_TEXTBOX_T:
                    case PCB_TABLE_T:
                    case PCB_TRACE_T:
                    case PCB_ARC_T:
                    case PCB_VIA_T:
                    case PCB_DIM_ALIGNED_T:
                    case PCB_DIM_CENTER_T:
                    case PCB_DIM_RADIAL_T:
                    case PCB_DIM_ORTHOGONAL_T:
                    case PCB_DIM_LEADER_T:
                    case PCB_TARGET_T:
                    case PCB_ZONE_T:
                        this_item = item->Duplicate();
                        break;

                    case PCB_GROUP_T:
                        this_item = static_cast<PCB_GROUP*>( item )->DeepDuplicate();
                        break;

                    default:
                        // Silently drop other items (such as footprint texts) from duplication
                        break;
                    }
                }

                // Add new items to selection (footprints in the selection will be reannotated)
                items_for_this_block.Add( this_item );

                if( this_item )
                {
                    // Because aItem is/can be created from a selected item, and inherits from
                    // it this state, reset the selected stated of aItem:
                    this_item->ClearSelected();

                    this_item->RunOnDescendants(
                            []( BOARD_ITEM* aItem )
                            {
                                aItem->ClearSelected();
                            } );

                    TransformItem( *array_opts, ptN, *this_item );

                    // If a group is duplicated, add also created members to the board
                    if( this_item->Type() == PCB_GROUP_T )
                    {
                        this_item->RunOnDescendants(
                                [&]( BOARD_ITEM* aItem )
                                {
                                    commit.Add( aItem );
                                } );
                    }

                    commit.Add( this_item );
                }
            }

            // attempt to renumber items if the array parameters define
            // a complete numbering scheme to number by (as opposed to
            // implicit numbering by incrementing the items during creation
            if( this_item && array_opts->ShouldNumberItems() )
            {
                // Renumber non-aperture pads.
                if( this_item->Type() == PCB_PAD_T )
                {
                    PAD& pad = static_cast<PAD&>( *this_item );

                    if( pad.CanHaveNumber() )
                    {
                        wxString newNumber = pad_number_provider.GetNextPadNumber();
                        pad.SetNumber( newNumber );
                    }
                }
            }
        }

        if( !m_isFootprintEditor && array_opts->ShouldReannotateFootprints() )
        {
            m_toolMgr->GetTool<BOARD_REANNOTATE_TOOL>()->ReannotateDuplicates( items_for_this_block,
                                                                               all_added_items );
        }

        for( EDA_ITEM* item : items_for_this_block )
            all_added_items.push_back( item );
    }

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );
    m_toolMgr->RunAction<EDA_ITEMS*>( PCB_ACTIONS::selectItems, &all_added_items );

    commit.Push( _( "Create Array" ) );
}
