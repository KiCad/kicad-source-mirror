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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "tools/array_tool.h"

#include <array_options.h>
#include <array_pad_number_provider.h>
#include <dialogs/dialog_create_array.h>
#include <pad.h>
#include <pcb_generator.h>
#include <pcb_group.h>
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


ARRAY_TOOL::ARRAY_TOOL() : PCB_TOOL_BASE( "pcbnew.Array" ),
    m_dialog( nullptr )
{
}


ARRAY_TOOL::~ARRAY_TOOL()
{
}


void ARRAY_TOOL::Reset( RESET_REASON aReason )
{
}


bool ARRAY_TOOL::Init()
{
    return true;
}


int ARRAY_TOOL::CreateArray( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    // Be sure that there is at least one item that we can modify
    const PCB_SELECTION& selection = selectionTool->RequestSelection(
            []( const VECTOR2I&, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
            } );

    if( selection.Empty() )
        return 0;

    m_selection = std::make_unique<PCB_SELECTION>( selection );

    // we have a selection to work on now, so start the tool process
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    const bool enableArrayNumbering = m_isFootprintEditor;
    VECTOR2I   origin;

    if( m_selection->Size() == 1 )
        origin = m_selection->Items()[0]->GetPosition();
    else
        origin = m_selection->GetCenter();

    m_array_opts.reset();
    m_dialog = new DIALOG_CREATE_ARRAY( editFrame, m_array_opts, enableArrayNumbering, origin );

    m_dialog->Bind( wxEVT_CLOSE_WINDOW, &ARRAY_TOOL::onDialogClosed, this );

    // Show the dialog, but it's not modal - the user might ask to select a point, in which case
    // we'll exit and do to that.
    m_dialog->Show( true );

    return 0;
}

void ARRAY_TOOL::onDialogClosed( wxCloseEvent& aEvent )
{
    // Now that the dialog has served it's purpose, we can get rid of it
    m_dialog->Destroy();

    // This means the dialog failed somehow
    if( m_array_opts == nullptr )
        return;

    wxCHECK( m_selection, /* void */ );

    PCB_SELECTION&  selection = *m_selection;
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
    BOARD_COMMIT    commit( editFrame );

    const int arraySize = m_array_opts->GetArraySize();

    if( m_array_opts->ShouldArrangeSelection() )
    {
        std::set<FOOTPRINT*> fpDeDupe;

        EDA_ITEMS sortedSelection = selection.GetItemsSortedBySelectionOrder();
        int       selectionIndex = 0;

        BOARD_ITEM* firstItem = nullptr;

        for( int arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex )
        {
            BOARD_ITEM* item = nullptr;

            // Get the next valid item to arrange
            for( ; selectionIndex < (int) sortedSelection.size(); selectionIndex++ )
            {
                item = nullptr;

                if( !sortedSelection[selectionIndex]->IsBOARD_ITEM() )
                    continue;

                item = static_cast<BOARD_ITEM*>( sortedSelection[selectionIndex] );

                FOOTPRINT* parentFootprint = item->GetParentFootprint();

                // If it is not the footprint editor, then move the parent footprint instead.
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
                        item = nullptr;
                        continue;
                    }
                }

                // Found a valid item
                selectionIndex++;
                break;
            }

            // Must be out of items to arrange, we're done
            if( item == nullptr )
                break;

            commit.Modify( item, nullptr, RECURSE_MODE::RECURSE );

            // Transform is a relative move, so when arranging the transform needs to start from
            // the same point for each item, e.g. the first item's position
            if( firstItem == nullptr )
                firstItem = item;
            else
                item->SetPosition( firstItem->GetPosition() );

            TransformItem( *m_array_opts, arrayIndex, *item );
        }

        // Make sure we did something...
        if( firstItem != nullptr )
            commit.Push( _( "Arrange selection" ) );

        return;
    }

    FOOTPRINT* const fp = m_isFootprintEditor ? editFrame->GetBoard()->GetFirstFootprint()
                                              : nullptr;

    // Collect a list of pad numbers that will _not_ be counted as "used"
    // when finding the next pad numbers.
    // Things that are selected are fair game, as they'll give up their numbers.
    // Keeps numbers used by both selected and unselected pads as "reserved".
    std::set<wxString> unchangingPadNumbers;
    if( fp )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( !pad->IsSelected() )
                unchangingPadNumbers.insert( pad->GetNumber() );
        }
    }

    ARRAY_PAD_NUMBER_PROVIDER pad_number_provider( unchangingPadNumbers, *m_array_opts );

    const bool will_reannotate = !m_isFootprintEditor && m_array_opts->ShouldReannotateFootprints();
    EDA_ITEMS  all_added_items;

    // Iterate in reverse so the original items go last, and we can
    // use them for the positions of the clones.
    for( int ptN = arraySize - 1; ptN >= 0; --ptN )
    {
        PCB_SELECTION        items_for_this_block;
        std::set<FOOTPRINT*> fpDeDupe;

        for( EDA_ITEM* eda_item : selection )

        {
            if( !eda_item->IsBOARD_ITEM() )
                continue;

            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( eda_item );

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

                commit.Modify( this_item, nullptr, RECURSE_MODE::RECURSE );

                TransformItem( *m_array_opts, arraySize - 1, *this_item );
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
                    this_item = fp->DuplicateItem( true, &commit, item );
                }
                else
                {
                    switch( item->Type() )
                    {
                    case PCB_FOOTPRINT_T:
                    case PCB_SHAPE_T:
                    case PCB_BARCODE_T:
                    case PCB_REFERENCE_IMAGE_T:
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
                    case PCB_POINT_T:
                    case PCB_TARGET_T:
                    case PCB_ZONE_T:
                        this_item = item->Duplicate( true, &commit );
                        break;

                    case PCB_GENERATOR_T:
                        this_item = static_cast<PCB_GENERATOR*>( item )->DeepClone();
                        break;

                    case PCB_GROUP_T:
                        this_item = static_cast<PCB_GROUP*>( item )->DeepDuplicate( true, &commit );
                        break;

                    default:
                        // Silently drop other items (such as footprint texts) from duplication
                        break;
                    }
                }

                if( this_item )
                {
                    // Because aItem is/can be created from a selected item, and inherits from
                    // it this state, reset the selected stated of aItem:
                    this_item->ClearSelected();

                    this_item->RunOnChildren(
                            []( BOARD_ITEM* aItem )
                            {
                                aItem->ClearSelected();
                            },
                            RECURSE_MODE::RECURSE );

                    // We're iterating backwards, so the first item is the last in the array
                    TransformItem( *m_array_opts, arraySize - ptN - 1, *this_item );

                    // If a group is duplicated, add also created members to the board
                    if( this_item->Type() == PCB_GROUP_T  ||
                        this_item->Type() == PCB_GENERATOR_T )
                    {
                        this_item->RunOnChildren(
                                [&]( BOARD_ITEM* aItem )
                                {
                                    commit.Add( aItem );
                                },
                                RECURSE_MODE::RECURSE );
                    }

                    commit.Add( this_item );
                }
            }

            // Add new items to selection (footprints in the selection will be reannotated)
            if( this_item )
                items_for_this_block.Add( this_item );

            // attempt to renumber items if the array parameters define
            // a complete numbering scheme to number by (as opposed to
            // implicit numbering by incrementing the items during creation
            if( this_item && m_array_opts->ShouldNumberItems() )
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

        // Do not reannotate the first item, or it will skip its own numbering and
        // the array annotations will shift by one cell.
        if( will_reannotate && ptN != arraySize - 1 )
        {
            m_toolMgr->GetTool<BOARD_REANNOTATE_TOOL>()->ReannotateDuplicates( items_for_this_block,
                                                                               all_added_items );
        }

        for( EDA_ITEM* item : items_for_this_block )
            all_added_items.push_back( item );
    }

    // Make sure original items are selected (e.g. interactive point select may clear it)
    for( EDA_ITEM* eda_item : selection )
    {
        all_added_items.push_back( eda_item );
    }

    m_toolMgr->RunAction( ACTIONS::selectionClear );
    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &all_added_items );

    commit.Push( _( "Create Array" ) );

    m_selection.reset();
}


void ARRAY_TOOL::setTransitions()
{
    // clang-format off
    Go( &ARRAY_TOOL::CreateArray,   PCB_ACTIONS::createArray.MakeEvent() );
    // clang-format on
}
