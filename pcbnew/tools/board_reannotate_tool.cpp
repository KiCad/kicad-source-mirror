/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Brian Piccioni <brian@documenteddesigns.com>
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

#include <pcb_group.h>
#include <refdes_utils.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <wx/filedlg.h>
#include <tools/board_reannotate_tool.h>


BOARD_REANNOTATE_TOOL::BOARD_REANNOTATE_TOOL() :
     PCB_TOOL_BASE( "pcbnew.ReannotateTool" ),
     m_selectionTool( nullptr ),
     m_frame( nullptr )
{
}


bool BOARD_REANNOTATE_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    return true;
}


void BOARD_REANNOTATE_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_EDIT_FRAME>();
}


int BOARD_REANNOTATE_TOOL::ShowReannotateDialog( const TOOL_EVENT& aEvent )
{
    DIALOG_BOARD_REANNOTATE dialog( m_frame );
    dialog.ShowModal();
    return 0;
}


int BOARD_REANNOTATE_TOOL::ReannotateDuplicatesInSelection()
{
    PCB_SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Empty() )
        return 0;

    return ReannotateDuplicates( selection, std::vector<EDA_ITEM*>() );
}

int BOARD_REANNOTATE_TOOL::ReannotateDuplicates( const PCB_SELECTION& aSelectionToReannotate,
                                                 const std::vector<EDA_ITEM*>& aAdditionalFootprints )
{
    if( aSelectionToReannotate.Empty() )
        return 0;

    // 1. Build list of designators on the board & the additional footprints
    FOOTPRINTS                    fpOnBoard = m_frame->GetBoard()->Footprints();
    std::multimap<wxString, KIID> usedDesignatorsMap;

    for( EDA_ITEM* item : aAdditionalFootprints )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            fpOnBoard.push_back( static_cast<FOOTPRINT*>( item ) );

        if( item->Type() == PCB_GROUP_T )
        {
            PCB_GROUP* group = static_cast<PCB_GROUP*>( item );

            group->RunOnChildren(
                    [&]( BOARD_ITEM* aGroupItem )
                    {
                        if( aGroupItem->Type() == PCB_FOOTPRINT_T )
                            fpOnBoard.push_back( static_cast<FOOTPRINT*>( aGroupItem ) );
                    },
                    RECURSE_MODE::RECURSE );
        }
    }

    for( FOOTPRINT* fp : fpOnBoard )
        usedDesignatorsMap.insert( { fp->GetReference(), fp->m_Uuid } );

    // 2. Get a sorted list of footprints from the selection
    FOOTPRINTS fpInSelection;

    for( EDA_ITEM* item : aSelectionToReannotate )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            fpInSelection.push_back( static_cast<FOOTPRINT*>( item ) );

        if( item->Type() == PCB_GROUP_T )
        {
            PCB_GROUP* group = static_cast<PCB_GROUP*>( item );

            group->RunOnChildren(
                    [&]( BOARD_ITEM* aGroupItem )
                    {
                        if( aGroupItem->Type() == PCB_FOOTPRINT_T )
                            fpInSelection.push_back( static_cast<FOOTPRINT*>( aGroupItem ) );
                    },
                    RECURSE_MODE::RECURSE );
        }
    }

    std::sort( fpInSelection.begin(), fpInSelection.end(),
               []( const FOOTPRINT* aA, const FOOTPRINT* aB ) -> bool
               {
                   int ii = StrNumCmp( aA->GetReference(), aB->GetReference(), true );

                   if( ii == 0 )
                   {
                       // Sort by position: x, then y
                       if( aA->GetPosition().y == aB->GetPosition().y )
                       {
                           if( aA->GetPosition().x == aB->GetPosition().x )
                               return aA->m_Uuid < aB->m_Uuid; // ensure a deterministic sort
                           else
                               return aA->GetPosition().x < aB->GetPosition().x;
                       }
                       else
                       {
                           return aA->GetPosition().y > aB->GetPosition().y;
                       }
                   }

                   return ii < 0;
               } );

    // 3. Iterate through the sorted list of footprints
    for( FOOTPRINT* fp : fpInSelection )
    {
        wxString stem = UTIL::GetRefDesPrefix( fp->GetReference() );
        int      value = UTIL::GetRefDesNumber( fp->GetReference() );
        bool     duplicate = false;

        while( usedDesignatorsMap.find( fp->GetReference() ) != usedDesignatorsMap.end() )
        {
            auto result = usedDesignatorsMap.equal_range( fp->GetReference() );

            for( auto& it = result.first; it != result.second; it++ )
            {
                if( it->second != fp->m_Uuid )
                {
                    duplicate = true;
                    break;
                }
            }

            if( !duplicate )
                break; // The only designator in the board with this reference is the selected one

            if( value < 0 )
                value = 1;
            else
                ++value;

            fp->SetReference( stem + std::to_string( value ) );
        }

        if( duplicate )
            usedDesignatorsMap.insert( { fp->GetReference(), fp->m_Uuid } );
    }

    return 0;
}


void BOARD_REANNOTATE_TOOL::setTransitions()
{
    Go( &BOARD_REANNOTATE_TOOL::ShowReannotateDialog, PCB_ACTIONS::boardReannotate.MakeEvent() );
}
