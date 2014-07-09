/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "module_tools.h"
#include "selection_tool.h"
#include "common_actions.h"

#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <view/view_group.h>

#include <kicad_plugin.h>
#include <pcbnew_id.h>

#include <wxPcbStruct.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <boost/bind.hpp>

MODULE_TOOLS::MODULE_TOOLS() :
    TOOL_INTERACTIVE( "pcbnew.ModuleEditor" )
{
}


void MODULE_TOOLS::Reset( RESET_REASON aReason )
{
    // Init variables used by every drawing tool
    m_view = getView();
    m_controls = getViewControls();
    m_board = getModel<BOARD>();
    m_frame = getEditFrame<PCB_EDIT_FRAME>();
}


bool MODULE_TOOLS::Init()
{
    setTransitions();

    return true;
}


static wxString getNextPadName( MODULE* aModule )
{
    std::set<int> usedNumbers;

    // Create a set of used pad numbers
    for( D_PAD* pad = aModule->Pads(); pad; pad = pad->Next() )
    {
        wxString padName = pad->GetPadName();
        int padNumber = 0;
        int base = 1;

        // Trim and extract the trailing numeric part
        while( padName.Len() && padName.Last() >= '0' && padName.Last() <= '9' )
        {
            padNumber += ( padName.Last() - '0' ) * base;
            padName.RemoveLast();
            base *= 10;
        }

        usedNumbers.insert( padNumber );
    }

    int candidate = *usedNumbers.begin();

    // Look for a gap in pad numbering
    for( std::set<int>::iterator it = usedNumbers.begin(),
            itEnd = usedNumbers.end(); it != itEnd; ++it )
    {
        if( *it - candidate > 1 )
            break;

        candidate = *it;
    }

    return wxString::Format( wxT( "%i" ), ++candidate );
}


int MODULE_TOOLS::PlacePad( TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_MODEDIT_PAD_TOOL, wxCURSOR_PENCIL, _( "Add pads" ) );

    MODULE* module = m_board->m_Modules;
    assert( module );

    D_PAD* pad = new D_PAD( module );
    m_frame->Import_Pad_Settings( pad, false );     // use the global settings for pad

    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    pad->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    preview.Add( pad );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition();

        if( evt->IsMotion() )
        {
            pad->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.ViewUpdate();
        }

        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                pad->Rotate( pad->GetPosition(), m_frame->GetRotationAngle() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )
            {
                pad->Flip( pad->GetPosition() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsCancel() || evt->IsActivate() )
            {
                preview.Clear();
                delete pad;
                break;
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( module, UR_MODEDIT );

            m_board->m_Status_Pcb = 0;    // I have no clue why, but it is done in the legacy view
            module->SetLastEditTime();
            module->Pads().PushBack( pad );

            pad->SetNetCode( NETINFO_LIST::UNCONNECTED );

            // Set the relative pad position
            // ( pad position for module orient, 0, and relative to the module position)
            pad->SetLocalCoord();

            /* NPTH pads take empty pad number (since they can't be connected),
             * other pads get incremented from the last one edited */
            wxString padName;

            if( pad->GetAttribute() != PAD_HOLE_NOT_PLATED )
                padName = getNextPadName( module );

            pad->SetPadName( padName );

            // Handle the view aspect
            preview.Remove( pad );
            m_view->Add( pad );

            // Start placing next pad
            pad = new D_PAD( module );
            m_frame->Import_Pad_Settings( pad, false );
            preview.Add( pad );
        }
    }

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int MODULE_TOOLS::CopyItems( TOOL_EVENT& aEvent )
{
    const SELECTION_TOOL::SELECTION& selection = m_toolMgr->GetTool<SELECTION_TOOL>()->GetSelection();

    Activate();

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    controls->SetSnapping( true );
    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    PCB_BASE_FRAME* frame = getEditFrame<PCB_BASE_FRAME>();
    frame->DisplayToolMsg( _( "Select reference point" ) );

    bool cancelled = false;
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition();

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsMotion() )
        {
            cursorPos = getViewControls()->GetCursorPosition();
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            break;
        }
        else if( evt->IsCancel() || evt->IsActivate() )
        {
            cancelled = true;
            break;
        }
    }

    if( !cancelled )
    {
        PCB_IO io( CTL_FOR_CLIPBOARD );

        // Create a temporary module that contains selected items to ease serialization
        MODULE module( getModel<BOARD>() );

        for( int i = 0; i < selection.Size(); ++i )
        {
            BOARD_ITEM* clone = static_cast<BOARD_ITEM*>( selection.Item<BOARD_ITEM>( i )->Clone() );

            // Do not add reference/value - convert them to the common type
            if( TEXTE_MODULE* text = dyn_cast<TEXTE_MODULE*>( clone ) )
                text->SetType( TEXTE_MODULE::TEXT_is_DIVERS );

            module.Add( clone );
        }

        // Set the new relative internal local coordinates of copied items
        MODULE* editedModule = getModel<BOARD>()->m_Modules;
        wxPoint moveVector = module.GetPosition() + editedModule->GetPosition() -
                             wxPoint( cursorPos.x, cursorPos.y );
        module.MoveAnchorPosition( moveVector );

        io.Format( &module, 0 );
        std::string data = io.GetStringOutput( true );
        m_toolMgr->SaveClipboard( data );
    }

    frame->DisplayToolMsg( wxString::Format( _( "Copied %d item(s)" ), selection.Size() ) );
    controls->SetSnapping( false );
    controls->ShowCursor( false );
    controls->SetAutoPan( false );

    setTransitions();

    return 0;
}


int MODULE_TOOLS::PasteItems( TOOL_EVENT& aEvent )
{
    // Parse clipboard
    PCB_IO io( CTL_FOR_CLIPBOARD );
    MODULE* currentModule = getModel<BOARD>()->m_Modules;
    MODULE* pastedModule = NULL;

    try
    {
        BOARD_ITEM* item = io.Parse( wxString( m_toolMgr->GetClipboard().c_str(), wxConvUTF8 ) );
        assert( item->Type() == PCB_MODULE_T );
        pastedModule = dyn_cast<MODULE*>( item );
    }
    catch( ... )
    {
        m_frame->DisplayToolMsg( _( "Invalid clipboard contents" ) );
        setTransitions();

        return 0;
    }

    // Placement tool part
    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>();
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME>();
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition();

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    pastedModule->SetParent( board );
    pastedModule->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
    pastedModule->RunOnChildren( boost::bind( &KIGFX::VIEW_GROUP::Add, boost::ref( preview ), _1 ) );
    preview.Add( pastedModule );
    view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    controls->ShowCursor( true );
    controls->SetSnapping( true );
    controls->SetAutoPan( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = controls->GetCursorPosition();

        if( evt->IsMotion() )
        {
            pastedModule->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.ViewUpdate();
        }

        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                pastedModule->Rotate( pastedModule->GetPosition(), frame->GetRotationAngle() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )
            {
                pastedModule->Flip( pastedModule->GetPosition() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsCancel() || evt->IsActivate() )
            {
                preview.Clear();
                break;
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            frame->OnModify();
            frame->SaveCopyInUndoList( currentModule, UR_MODEDIT );

            board->m_Status_Pcb = 0;    // I have no clue why, but it is done in the legacy view
            currentModule->SetLastEditTime();

            // MODULE::RunOnChildren is infeasible here: we need to create copies of items, do not
            // directly modify them

            for( D_PAD* pad = pastedModule->Pads(); pad; pad = pad->Next() )
            {
                D_PAD* clone = static_cast<D_PAD*>( pad->Clone() );

                currentModule->Add( clone );
                clone->SetLocalCoord();
                view->Add( clone );
            }

            for( BOARD_ITEM* drawing = pastedModule->GraphicalItems();
                    drawing; drawing = drawing->Next() )
            {
                BOARD_ITEM* clone = static_cast<BOARD_ITEM*>( drawing->Clone() );

                if( TEXTE_MODULE* text = dyn_cast<TEXTE_MODULE*>( clone ) )
                {
                    // Do not add reference/value - convert them to the common type
                    text->SetType( TEXTE_MODULE::TEXT_is_DIVERS );
                    currentModule->Add( text );
                    text->SetLocalCoord();

                    // Whyyyyyyyyyyyyyyyyyyyyyy?! All other items conform to rotation performed
                    // on its parent module, but texts are so independent..
                    text->Rotate( text->GetPosition(), pastedModule->GetOrientation() );
                }
                else if( EDGE_MODULE* edge = dyn_cast<EDGE_MODULE*>( clone ) )
                {
                    currentModule->Add( edge );
                    edge->SetLocalCoord();
                }

                view->Add( clone );
            }

            preview.Clear();

            break;
        }
    }

    delete pastedModule;
    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );
    view->Remove( &preview );

    setTransitions();

    return 0;
}


void MODULE_TOOLS::setTransitions()
{
    Go( &MODULE_TOOLS::PlacePad,        COMMON_ACTIONS::placePad.MakeEvent() );
    Go( &MODULE_TOOLS::CopyItems,       COMMON_ACTIONS::copyItems.MakeEvent() );
    Go( &MODULE_TOOLS::PasteItems,      COMMON_ACTIONS::pasteItems.MakeEvent() );
}
