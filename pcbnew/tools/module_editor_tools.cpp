/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
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

#include "module_editor_tools.h"
#include "selection_tool.h"
#include "pcb_actions.h"
#include <tool/tool_manager.h>

#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <view/view_group.h>
#include <pcb_painter.h>
#include <origin_viewitem.h>

#include <kicad_plugin.h>
#include <pcbnew_id.h>
#include <collectors.h>
#include <confirm.h>
#include <dialogs/dialog_enum_pads.h>
#include <hotkeys.h>
#include <bitmaps.h>

#include <wxPcbStruct.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <board_commit.h>

#include <tools/tool_event_utils.h>

#include <functional>
using namespace std::placeholders;
#include <wx/defs.h>

// Module editor tools
TOOL_ACTION PCB_ACTIONS::placePad( "pcbnew.ModuleEditor.placePad",
        AS_GLOBAL, 0,
        _( "Add Pad" ), _( "Add a pad" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::enumeratePads( "pcbnew.ModuleEditor.enumeratePads",
        AS_GLOBAL, 0,
        _( "Enumerate Pads" ), _( "Enumerate pads" ), pad_enumerate_xpm, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::copyItems( "pcbnew.ModuleEditor.copyItems",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_COPY_ITEM ),
        _( "Copy" ), _( "Copy items" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::pasteItems( "pcbnew.ModuleEditor.pasteItems",
        AS_GLOBAL, MD_CTRL + int( 'V' ),
        _( "Paste" ), _( "Paste items" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::moduleEdgeOutlines( "pcbnew.ModuleEditor.graphicOutlines",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::moduleTextOutlines( "pcbnew.ModuleEditor.textOutlines",
       AS_GLOBAL, 0,
       "", "" );


MODULE_EDITOR_TOOLS::MODULE_EDITOR_TOOLS() :
    TOOL_INTERACTIVE( "pcbnew.ModuleEditor" ), m_view( NULL ), m_controls( NULL ),
    m_board( NULL ), m_frame( NULL )
{
}


MODULE_EDITOR_TOOLS::~MODULE_EDITOR_TOOLS()
{
}


void MODULE_EDITOR_TOOLS::Reset( RESET_REASON aReason )
{
    // Init variables used by every drawing tool
    m_view = getView();
    m_controls = getViewControls();
    m_board = getModel<BOARD>();
    m_frame = getEditFrame<PCB_EDIT_FRAME>();
}


int MODULE_EDITOR_TOOLS::PlacePad( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_MODEDIT_PAD_TOOL, wxCURSOR_PENCIL, _( "Add pads" ) );

    assert( m_board->m_Modules );

    D_PAD* pad = new D_PAD( m_board->m_Modules );
    m_frame->Import_Pad_Settings( pad, false );     // use the global settings for pad

    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    pad->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    preview.Add( pad );
    m_view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
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
            m_view->Update( &preview );
        }

        else if( evt->Category() == TC_COMMAND )
        {
            if( TOOL_EVT_UTILS::IsRotateToolEvt( *evt ) )
            {
                const auto rotationAngle = TOOL_EVT_UTILS::GetEventRotationAngle(
                        *m_frame, *evt );
                pad->Rotate( pad->GetPosition(), rotationAngle );
                m_view->Update( &preview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::flip ) )
            {
                pad->Flip( pad->GetPosition() );
                m_view->Update( &preview );
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
            BOARD_COMMIT commit( m_frame );
            commit.Add( pad );

            m_board->m_Status_Pcb = 0;    // I have no clue why, but it is done in the legacy view

            // Take the next available pad number
            pad->IncrementPadName( true, true );

            // Handle the view aspect
            preview.Remove( pad );
            commit.Push( _( "Add a pad" ) );

            // Start placing next pad
            pad = new D_PAD( m_board->m_Modules );
            m_frame->Import_Pad_Settings( pad, false );
            pad->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.Add( pad );
        }
    }

    m_view->Remove( &preview );
    m_frame->SetNoToolSelected();

    return 0;
}


int MODULE_EDITOR_TOOLS::EnumeratePads( const TOOL_EVENT& aEvent )
{
    if( !m_board->m_Modules || !m_board->m_Modules->Pads() )
        return 0;

    Activate();

    GENERAL_COLLECTOR collector;
    const KICAD_T types[] = { PCB_PAD_T, EOT };

    GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
    guide.SetIgnoreMTextsMarkedNoShow( true );
    guide.SetIgnoreMTextsOnBack( true );
    guide.SetIgnoreMTextsOnFront( true );
    guide.SetIgnoreModulesVals( true );
    guide.SetIgnoreModulesRefs( true );

    DIALOG_ENUM_PADS settingsDlg( m_frame );

    if( settingsDlg.ShowModal() == wxID_CANCEL )
        return 0;

    int padNumber = settingsDlg.GetStartNumber();
    wxString padPrefix = settingsDlg.GetPrefix();

    m_frame->DisplayToolMsg( _( "Hold left mouse button and move cursor over pads to enumerate them" ) );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );

    KIGFX::VIEW* view = m_toolMgr->GetView();
    VECTOR2I oldCursorPos = m_controls->GetCursorPosition();
    std::list<D_PAD*> selectedPads;
    BOARD_COMMIT commit( m_frame );
    std::map<wxString, wxString> oldNames;

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            selectedPads.clear();
            VECTOR2I cursorPos = m_controls->GetCursorPosition();

            if( evt->IsClick( BUT_LEFT ) )
            {
                oldCursorPos = m_controls->GetCursorPosition();
                collector.Empty();
                collector.Collect( m_board, types, wxPoint( cursorPos.x, cursorPos.y ), guide );

                for( int i = 0; i < collector.GetCount(); ++i )
                {
                    if( collector[i]->Type() == PCB_PAD_T )
                        selectedPads.push_back( static_cast<D_PAD*>( collector[i] ) );
                }
            }
            else //evt->IsDrag( BUT_LEFT )
            {
                // wxWidgets deliver mouse move events not frequently enough, resulting in skipping
                // pads if the user moves cursor too fast. To solve it, create a line that approximates
                // the mouse move and select items intersecting with the line.
                int distance = ( cursorPos - oldCursorPos ).EuclideanNorm();
                int segments = distance / 100000 + 1;
                const wxPoint LINE_STEP( ( cursorPos - oldCursorPos ).x / segments,
                                         ( cursorPos - oldCursorPos ).y / segments );

                collector.Empty();
                for( int j = 0; j < segments; ++j ) {
                    collector.Collect( m_board, types,
                                       wxPoint( oldCursorPos.x, oldCursorPos.y ) + j * LINE_STEP,
                                       guide );

                    for( int i = 0; i < collector.GetCount(); ++i )
                    {
                        if( collector[i]->Type() == PCB_PAD_T )
                            selectedPads.push_back( static_cast<D_PAD*>( collector[i] ) );
                    }
                }

                selectedPads.unique();
            }

            for( D_PAD* pad : selectedPads )
            {
                // If pad was not selected, then enumerate it
                if( !pad->IsSelected() )
                {
                    commit.Modify( pad );

                    // Rename pad and store the old name
                    wxString newName = wxString::Format( wxT( "%s%d" ), padPrefix.c_str(), padNumber++ );
                    oldNames[newName] = pad->GetPadName();
                    pad->SetPadName( newName );
                    pad->SetSelected();
                    getView()->Update( pad );
                }

                // ..or restore the old name if it was enumerated and clicked again
                else if( pad->IsSelected() && evt->IsClick( BUT_LEFT ) )
                {
                    auto it = oldNames.find( pad->GetPadName() );
                    wxASSERT( it != oldNames.end() );

                    if( it != oldNames.end() )
                    {
                        pad->SetPadName( it->second );
                        oldNames.erase( it );
                    }

                    pad->ClearSelected();
                    getView()->Update( pad );
                }
            }

            oldCursorPos = cursorPos;
        }

        else if( ( evt->IsKeyPressed() && evt->KeyCode() == WXK_RETURN ) ||
                   evt->IsDblClick( BUT_LEFT ) )
        {
            commit.Push( _( "Enumerate pads" ) );
            break;
        }

        else if( evt->IsCancel() || evt->IsActivate() )
        {
            commit.Revert();
            break;
        }
    }

    for( D_PAD* p = m_board->m_Modules->Pads(); p; p = p->Next() )
    {
        p->ClearSelected();
        view->Update( p );
    }

    m_frame->DisplayToolMsg( wxEmptyString );

    return 0;
}


int MODULE_EDITOR_TOOLS::CopyItems( const TOOL_EVENT& aEvent )
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

        for( auto item : selection )
        {
            auto clone = static_cast<BOARD_ITEM*>( item->Clone() );

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

    return 0;
}


int MODULE_EDITOR_TOOLS::PasteItems( const TOOL_EVENT& aEvent )
{
    // Parse clipboard
    PCB_IO io( CTL_FOR_CLIPBOARD );
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
        return 0;
    }

    // Placement tool part
    VECTOR2I cursorPos = m_controls->GetCursorPosition();

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    pastedModule->SetParent( m_board );
    pastedModule->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
    pastedModule->RunOnChildren( std::bind( &KIGFX::VIEW_GROUP::Add,
                                                std::ref( preview ),  _1 ) );
    preview.Add( pastedModule );
    m_view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
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
            m_view->Update( &preview );
        }

        else if( evt->Category() == TC_COMMAND )
        {
            if( TOOL_EVT_UTILS::IsRotateToolEvt( *evt ) )
            {
                const auto rotationAngle = TOOL_EVT_UTILS::GetEventRotationAngle(
                        *m_frame, *evt );
                pastedModule->Rotate( pastedModule->GetPosition(), rotationAngle );
                m_view->Update( &preview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::flip ) )
            {
                pastedModule->Flip( pastedModule->GetPosition() );
                m_view->Update( &preview );
            }
            else if( evt->IsCancel() || evt->IsActivate() )
            {
                preview.Clear();
                break;
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            BOARD_COMMIT commit( m_frame );

            m_board->m_Status_Pcb = 0;    // I have no clue why, but it is done in the legacy view

            // MODULE::RunOnChildren is infeasible here: we need to create copies of items, do not
            // directly modify them

            for( D_PAD* pad = pastedModule->Pads(); pad; pad = pad->Next() )
            {
                D_PAD* clone = static_cast<D_PAD*>( pad->Clone() );
                commit.Add( clone );
            }

            for( BOARD_ITEM* drawing = pastedModule->GraphicalItems();
                    drawing; drawing = drawing->Next() )
            {
                BOARD_ITEM* clone = static_cast<BOARD_ITEM*>( drawing->Clone() );

                if( TEXTE_MODULE* text = dyn_cast<TEXTE_MODULE*>( clone ) )
                {
                    // Do not add reference/value - convert them to the common type
                    text->SetType( TEXTE_MODULE::TEXT_is_DIVERS );

                    // Whyyyyyyyyyyyyyyyyyyyyyy?! All other items conform to rotation performed
                    // on its parent module, but texts are so independent..
                    text->Rotate( text->GetPosition(), pastedModule->GetOrientation() );
                    commit.Add( text );
                }

                commit.Add( clone );
            }

            commit.Push( _( "Paste clipboard contents" ) );
            preview.Clear();

            break;
        }
    }

    delete pastedModule;
    m_view->Remove( &preview );

    return 0;
}


int MODULE_EDITOR_TOOLS::ModuleTextOutlines( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    const LAYER_NUM layers[] = { LAYER_MOD_TEXT_BK,
                                 LAYER_MOD_TEXT_FR,
                                 LAYER_MOD_TEXT_INVISIBLE,
                                 LAYER_MOD_REFERENCES,
                                 LAYER_MOD_VALUES };

    bool enable = !settings->GetSketchMode( layers[0] );

    for( LAYER_NUM layer : layers )
        settings->SetSketchMode( layer, enable );

    for( MODULE* module = getModel<BOARD>()->m_Modules; module; module = module->Next() )
    {
        for( BOARD_ITEM* item = module->GraphicalItems(); item; item = item ->Next() )
        {
            if( item->Type() == PCB_MODULE_TEXT_T )
                view->Update( item, KIGFX::GEOMETRY );
        }

        view->Update( &module->Reference(), KIGFX::GEOMETRY );
        view->Update( &module->Value(), KIGFX::GEOMETRY );
    }

    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


int MODULE_EDITOR_TOOLS::ModuleEdgeOutlines( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    const PCB_LAYER_ID layers[] = { F_Adhes, B_Adhes, F_Paste, B_Paste,
            F_SilkS, B_SilkS, F_Mask, B_Mask,
            Dwgs_User, Cmts_User, Eco1_User, Eco2_User, Edge_Cuts };

    bool enable = !settings->GetSketchMode( layers[0] );

    for( LAYER_NUM layer : layers )
        settings->SetSketchMode( layer, enable );

    for( MODULE* module = getModel<BOARD>()->m_Modules; module; module = module->Next() )
    {
        for( BOARD_ITEM* item = module->GraphicalItems(); item; item = item ->Next() )
        {
            if( item->Type() == PCB_MODULE_EDGE_T )
                view->Update( item, KIGFX::GEOMETRY );
        }
    }

    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


void MODULE_EDITOR_TOOLS::SetTransitions()
{
    Go( &MODULE_EDITOR_TOOLS::PlacePad,            PCB_ACTIONS::placePad.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::EnumeratePads,       PCB_ACTIONS::enumeratePads.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::CopyItems,           PCB_ACTIONS::copyItems.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::PasteItems,          PCB_ACTIONS::pasteItems.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::ModuleTextOutlines,  PCB_ACTIONS::moduleTextOutlines.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::ModuleEdgeOutlines,  PCB_ACTIONS::moduleEdgeOutlines.MakeEvent() );
}
