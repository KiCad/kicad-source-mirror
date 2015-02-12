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
#include <pcb_painter.h>

#include <kicad_plugin.h>
#include <pcbnew_id.h>
#include <collectors.h>
#include <confirm.h>
#include <dialogs/dialog_enum_pads.h>
#include <dialogs/dialog_create_array.h>

#include <wxPcbStruct.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <wx/defs.h>

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
    // Find the selection tool, so they can cooperate
    SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    if( !selectionTool )
    {
        DisplayError( NULL, wxT( "pcbnew.InteractiveSelection tool is not available" ) );
        return false;
    }

    selectionTool->AddMenuItem( COMMON_ACTIONS::enumeratePads );

    setTransitions();

    return true;
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

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );
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

            // Take the next available pad number
            pad->IncrementPadName( true, true );

            // Handle the view aspect
            preview.Remove( pad );
            m_view->Add( pad );

            // Start placing next pad
            pad = new D_PAD( module );
            m_frame->Import_Pad_Settings( pad, false );
            pad->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
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


int MODULE_TOOLS::EnumeratePads( TOOL_EVENT& aEvent )
{
    std::list<D_PAD*> pads;
    std::set<D_PAD*> allPads;
    MODULE* module = m_board->m_Modules;

    GENERAL_COLLECTOR collector;
    const KICAD_T types[] = { PCB_PAD_T, EOT };

    GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
    guide.SetIgnoreMTextsMarkedNoShow( true );
    guide.SetIgnoreMTextsOnBack( true );
    guide.SetIgnoreMTextsOnFront( true );
    guide.SetIgnoreModulesVals( true );
    guide.SetIgnoreModulesRefs( true );

    // Create a set containing all pads (to avoid double adding to a list)
    for( D_PAD* p = module->Pads(); p; p = p->Next() )
        allPads.insert( p );

    DIALOG_ENUM_PADS settingsDlg( m_frame );

    if( settingsDlg.ShowModal() == wxID_CANCEL )
    {
        setTransitions();

        return 0;
    }

    int padNumber = settingsDlg.GetStartNumber();
    wxString padPrefix = settingsDlg.GetPrefix();

    m_frame->DisplayToolMsg( _( "Hold left mouse button and move cursor over pads to enumerate them" ) );

    Activate();

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            // Add pads to the list according to the selection order
            VECTOR2I cursorPos = m_controls->GetCursorPosition();

            collector.Empty();
            collector.Collect( m_board, types, wxPoint( cursorPos.x, cursorPos.y ), guide );

            for( int i = 0; i < collector.GetCount(); ++i )
            {
                if( collector[i]->Type() == PCB_PAD_T )
                {
                    D_PAD* pad = static_cast<D_PAD*>( collector[i] );

                    std::set<D_PAD*>::iterator it = allPads.find( pad );

                    // Add the pad to the list, if it was not selected previously..
                    if( it != allPads.end() )
                    {
                        allPads.erase( it );
                        pads.push_back( pad );
                        pad->SetSelected();
                    }

                    // ..or remove it from the list if it was clicked
                    else if( evt->IsClick( BUT_LEFT ) )
                    {
                        allPads.insert( pad );
                        pads.remove( pad );
                        pad->ClearSelected();
                    }
                }
            }
        }

        else if( ( evt->IsKeyPressed() && evt->KeyCode() == WXK_RETURN ) ||
                   evt->IsDblClick( BUT_LEFT ) )
        {
            // Accept changes
            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( module, UR_MODEDIT );

            BOOST_FOREACH( D_PAD* pad, pads )
                pad->SetPadName( wxString::Format( wxT( "%s%d" ), padPrefix.c_str(), padNumber++ ) );

            break;
        }

        else if( evt->IsCancel() || evt->IsActivate() )
        {
            break;
        }
    }

    BOOST_FOREACH( D_PAD* pad, pads )
        pad->ClearSelected();

    m_frame->DisplayToolMsg( wxEmptyString );
    m_controls->ShowCursor( false );

    setTransitions();

    return 0;
}


int MODULE_TOOLS::CopyItems( TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_toolMgr->GetTool<SELECTION_TOOL>()->GetSelection();

    Activate();

    m_controls->SetSnapping( true );
    m_controls->ShowCursor( true );
    m_controls->SetAutoPan( true );

    m_frame->DisplayToolMsg( _( "Select reference point" ) );

    bool cancelled = false;
    VECTOR2I cursorPos = m_controls->GetCursorPosition();

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsMotion() )
        {
            cursorPos = m_controls->GetCursorPosition();
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
        MODULE module( m_board );

        for( int i = 0; i < selection.Size(); ++i )
        {
            BOARD_ITEM* clone = static_cast<BOARD_ITEM*>( selection.Item<BOARD_ITEM>( i )->Clone() );

            // Do not add reference/value - convert them to the common type
            if( TEXTE_MODULE* text = dyn_cast<TEXTE_MODULE*>( clone ) )
                text->SetType( TEXTE_MODULE::TEXT_is_DIVERS );

            module.Add( clone );
        }

        // Set the new relative internal local coordinates of copied items
        MODULE* editedModule = m_board->m_Modules;
        wxPoint moveVector = module.GetPosition() + editedModule->GetPosition() -
                             wxPoint( cursorPos.x, cursorPos.y );
        module.MoveAnchorPosition( moveVector );

        io.Format( &module, 0 );
        std::string data = io.GetStringOutput( true );
        m_toolMgr->SaveClipboard( data );
    }

    m_frame->DisplayToolMsg( wxString::Format( _( "Copied %d item(s)" ), selection.Size() ) );
    m_controls->SetSnapping( false );
    m_controls->ShowCursor( false );
    m_controls->SetAutoPan( false );

    setTransitions();

    return 0;
}


int MODULE_TOOLS::PasteItems( TOOL_EVENT& aEvent )
{
    // Parse clipboard
    PCB_IO io( CTL_FOR_CLIPBOARD );
    MODULE* currentModule = m_board->m_Modules;
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
    VECTOR2I cursorPos = m_controls->GetCursorPosition();

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    pastedModule->SetParent( m_board );
    pastedModule->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
    pastedModule->RunOnChildren( boost::bind( &KIGFX::VIEW_GROUP::Add, boost::ref( preview ), _1 ) );
    preview.Add( pastedModule );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->SetAutoPan( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition();

        if( evt->IsMotion() )
        {
            pastedModule->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.ViewUpdate();
        }

        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                pastedModule->Rotate( pastedModule->GetPosition(), m_frame->GetRotationAngle() );
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
            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( currentModule, UR_MODEDIT );

            m_board->m_Status_Pcb = 0;    // I have no clue why, but it is done in the legacy view
            currentModule->SetLastEditTime();

            // MODULE::RunOnChildren is infeasible here: we need to create copies of items, do not
            // directly modify them

            for( D_PAD* pad = pastedModule->Pads(); pad; pad = pad->Next() )
            {
                D_PAD* clone = static_cast<D_PAD*>( pad->Clone() );

                currentModule->Add( clone );
                clone->SetLocalCoord();
                m_view->Add( clone );
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

                m_view->Add( clone );
            }

            preview.Clear();

            break;
        }
    }

    delete pastedModule;
    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();

    return 0;
}


int MODULE_TOOLS::ModuleTextOutlines( TOOL_EVENT& aEvent )
{
    KIGFX::PCB_PAINTER* painter =
            static_cast<KIGFX::PCB_PAINTER*>( m_frame->GetGalCanvas()->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );

    const LAYER_NUM layers[] = { ITEM_GAL_LAYER( MOD_TEXT_BK_VISIBLE ),
                                 ITEM_GAL_LAYER( MOD_TEXT_FR_VISIBLE ),
                                 ITEM_GAL_LAYER( MOD_TEXT_INVISIBLE ),
                                 ITEM_GAL_LAYER( MOD_REFERENCES_VISIBLE ),
                                 ITEM_GAL_LAYER( MOD_VALUES_VISIBLE ) };

    bool enable = !settings->GetSketchMode( layers[0] );

    BOOST_FOREACH( LAYER_NUM layer, layers )
        settings->SetSketchMode( layer, enable );

    for( MODULE* module = getModel<BOARD>()->m_Modules; module; module = module->Next() )
    {
        for( BOARD_ITEM* item = module->GraphicalItems(); item; item = item ->Next() )
        {
            if( item->Type() == PCB_MODULE_TEXT_T )
                item->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }

        module->Reference().ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        module->Value().ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }

    m_frame->GetGalCanvas()->Refresh();
    setTransitions();

    return 0;
}


int MODULE_TOOLS::ModuleEdgeOutlines( TOOL_EVENT& aEvent )
{
    KIGFX::PCB_PAINTER* painter =
            static_cast<KIGFX::PCB_PAINTER*>( m_frame->GetGalCanvas()->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );

    const LAYER_ID layers[] = { F_Adhes, B_Adhes, F_Paste, B_Paste,
            F_SilkS, B_SilkS, F_Mask, B_Mask,
            Dwgs_User, Cmts_User, Eco1_User, Eco2_User, Edge_Cuts };

    bool enable = !settings->GetSketchMode( layers[0] );

    BOOST_FOREACH( LAYER_NUM layer, layers )
        settings->SetSketchMode( layer, enable );

    for( MODULE* module = getModel<BOARD>()->m_Modules; module; module = module->Next() )
    {
        for( BOARD_ITEM* item = module->GraphicalItems(); item; item = item ->Next() )
        {
            if( item->Type() == PCB_MODULE_EDGE_T )
                item->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_frame->GetGalCanvas()->Refresh();
    setTransitions();

    return 0;
}


void MODULE_TOOLS::setTransitions()
{
    Go( &MODULE_TOOLS::PlacePad,            COMMON_ACTIONS::placePad.MakeEvent() );
    Go( &MODULE_TOOLS::EnumeratePads,       COMMON_ACTIONS::enumeratePads.MakeEvent() );
    Go( &MODULE_TOOLS::CopyItems,           COMMON_ACTIONS::copyItems.MakeEvent() );
    Go( &MODULE_TOOLS::PasteItems,          COMMON_ACTIONS::pasteItems.MakeEvent() );
    Go( &MODULE_TOOLS::ModuleTextOutlines,  COMMON_ACTIONS::moduleTextOutlines.MakeEvent() );
    Go( &MODULE_TOOLS::ModuleEdgeOutlines,  COMMON_ACTIONS::moduleEdgeOutlines.MakeEvent() );
}
