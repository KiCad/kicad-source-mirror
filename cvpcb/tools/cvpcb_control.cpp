/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Ian McInerney <Ian.S.McInerney@ieee.org>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <confirm.h>
#include <cstdint>
#include <functional>
#include <kiface_base.h>
#include <kiway_express.h>
#include <lib_id.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>

#include <cvpcb_mainframe.h>
#include <dialogs/dialog_config_equfiles.h>
#include <display_footprints_frame.h>
#include <listboxes.h>
#include <tools/cvpcb_actions.h>
#include <tools/cvpcb_control.h>
#include <wx/settings.h>
#include <wx/msgdlg.h>

using namespace std::placeholders;


CVPCB_CONTROL::CVPCB_CONTROL() :
        TOOL_INTERACTIVE( "cvpcb.Control" ),
        m_frame( nullptr )
{
}


void CVPCB_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<CVPCB_MAINFRAME>();
}


int CVPCB_CONTROL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        bool handled = false;

        // The escape key maps to the cancel event, which is used to close the window
        if( evt->IsCancel() )
        {
            m_frame->Close( false );
            handled = true;
        }
        else if( evt->IsKeyPressed() )
        {
            switch( evt->KeyCode() )
            {
            // The right arrow moves focus to the focusable object to the right
            case WXK_RIGHT:
                m_toolMgr->PostAction( CVPCB_ACTIONS::changeFocusRight );
                handled = true;
                break;

            // The left arrow moves focus to the focusable object to the left
            case WXK_LEFT:
                m_toolMgr->PostAction( CVPCB_ACTIONS::changeFocusLeft );
                handled = true;
                break;

            default:
                // Let every other key continue processing to the controls of the window
                break;
            }
        }

        if( !handled )
            evt->SetPassEvent();
    }

    return 0;
}


int CVPCB_CONTROL::ChangeFocus( const TOOL_EVENT& aEvent )
{
    CVPCB_MAINFRAME::FOCUS_DIR dir = aEvent.Parameter<CVPCB_MAINFRAME::FOCUS_DIR>();

    switch( dir )
    {
    case CVPCB_MAINFRAME::CHANGE_FOCUS_RIGHT:
        switch( m_frame->GetFocusedControl() )
        {
        case CVPCB_MAINFRAME::CONTROL_LIBRARY:
            m_frame->SetFocusedControl( CVPCB_MAINFRAME::CONTROL_COMPONENT );
            break;

        case CVPCB_MAINFRAME::CONTROL_COMPONENT:
            m_frame->SetFocusedControl( CVPCB_MAINFRAME::CONTROL_FOOTPRINT );
            break;

        case CVPCB_MAINFRAME::CONTROL_FOOTPRINT:
            m_frame->SetFocusedControl( CVPCB_MAINFRAME::CONTROL_LIBRARY );
            break;

        case CVPCB_MAINFRAME::CONTROL_NONE:
        default:
            break;
        }

        break;

    case CVPCB_MAINFRAME::CHANGE_FOCUS_LEFT:
        switch( m_frame->GetFocusedControl() )
        {
        case CVPCB_MAINFRAME::CONTROL_LIBRARY:
            m_frame->SetFocusedControl( CVPCB_MAINFRAME::CONTROL_FOOTPRINT );
            break;

        case CVPCB_MAINFRAME::CONTROL_COMPONENT:
            m_frame->SetFocusedControl( CVPCB_MAINFRAME::CONTROL_LIBRARY );
            break;

        case CVPCB_MAINFRAME::CONTROL_FOOTPRINT:
            m_frame->SetFocusedControl( CVPCB_MAINFRAME::CONTROL_COMPONENT );
            break;

        case CVPCB_MAINFRAME::CONTROL_NONE:
        default:
            break;
        }

        break;

    default:
        break;
    }

    return 0;
}


int CVPCB_CONTROL::ShowFootprintViewer( const TOOL_EVENT& aEvent )
{

    DISPLAY_FOOTPRINTS_FRAME* fpframe = m_frame->GetFootprintViewerFrame();

    if( !fpframe )
    {
        // DISPLAY_FOOTPRINTS_FRAME needs a PCBNEW_SETTINGS because most of its settings
        // come from it, and needs _pcbnew.dll/so code. So load it if the dll is not yet loaded
        // (for instance if the board editor was never loaded)
        if( !m_frame->Kiway().KiFACE( KIWAY::FACE_PCB, false ) )
        {
            try
            {
                m_frame->Kiway().KiFACE( KIWAY::FACE_PCB, true );
            }
            catch( ... )
            {
                // _pcbnew.dll/so is not available (install problem).
            }
        }

        fpframe = (DISPLAY_FOOTPRINTS_FRAME*) m_frame->Kiway().Player( FRAME_CVPCB_DISPLAY, true,
                                                                       m_frame );

        // If Kiway() cannot create the Eeschema frame, it shows a error message, and
        // frame is null
        if( !fpframe )
            return 0;

        fpframe->Show( true );
    }
    else
    {
        if( wxWindow* blocking_win = fpframe->Kiway().GetBlockingDialog() )
            blocking_win->Close( true );

        if( fpframe->IsIconized() )
            fpframe->Iconize( false );

        // The display footprint window might be buried under some other
        // windows, so CreateScreenCmp() on an existing window would not
        // show any difference, leaving the user confused.
        // So we want to put it to front, second after our CVPCB_MAINFRAME.
        // We do this by a little dance of bringing it to front then the main
        // frame back.
        wxWindow* focus = m_frame->FindFocus();

        fpframe->Raise(); // Make sure that is visible.
        m_frame->Raise(); // .. but still we want the focus.

        if( focus )
            focus->SetFocus();
    }

    fpframe->InitDisplay();

    return 0;
}


int CVPCB_CONTROL::ToggleFootprintFilter( const TOOL_EVENT& aEvent )
{
    m_frame->SetFootprintFilter( aEvent.Parameter<FOOTPRINTS_LISTBOX::FP_FILTER_T>(),
                                 CVPCB_MAINFRAME::FILTER_TOGGLE );
    return 0;
}


int CVPCB_CONTROL::ShowEquFileTable( const TOOL_EVENT& aEvent )
{
    DIALOG_CONFIG_EQUFILES dlg( m_frame );
    dlg.ShowModal();

    return 0;
}


int CVPCB_CONTROL::SaveAssociationsToFile( const TOOL_EVENT& aEvent )
{
    m_frame->SaveFootprintAssociation( true );
    return 0;
}


int CVPCB_CONTROL::SaveAssociationsToSchematic( const TOOL_EVENT& aEvent )
{
    m_frame->SaveFootprintAssociation( false );
    return 0;
}


int CVPCB_CONTROL::ToNA( const TOOL_EVENT& aEvent )
{
    CVPCB_MAINFRAME::ITEM_DIR dir = aEvent.Parameter<CVPCB_MAINFRAME::ITEM_DIR>();

    std::vector<unsigned int> naComp =
            m_frame->GetComponentIndices( CVPCB_MAINFRAME::NA_COMPONENTS );
    std::vector<unsigned int> tempSel =
            m_frame->GetComponentIndices( CVPCB_MAINFRAME::SEL_COMPONENTS );

    // No unassociated components
    if( naComp.empty() )
        return 0;

    // Extract the current selection
    bool changeSel = false;
    unsigned int newSel = UINT_MAX;

    switch( dir )
    {
    case CVPCB_MAINFRAME::ITEM_NEXT:
        if( !tempSel.empty() )
            newSel = tempSel.front();

        // Find the next index in the component list
        for( unsigned int idx : naComp )
        {
            if( idx > newSel )
            {
                changeSel = true;
                newSel = idx;
                break;
            }
        }

        break;

    case CVPCB_MAINFRAME::ITEM_PREV:
        if( !tempSel.empty() )
        {
            newSel = tempSel.front();

            // Find the previous index in the component list
            for( int jj = naComp.size()-1; jj >= 0; jj-- )
            {
                unsigned idx = naComp[jj];

                if( idx < newSel )
                {
                    changeSel = true;
                    newSel = idx;
                    break;
                }
            }
        }

        break;

    default:
        wxASSERT_MSG( false, "Invalid direction" );
    }

    // Set the new component selection
    if( changeSel )
        m_frame->SetSelectedComponent( newSel );

    return 0;
}


int CVPCB_CONTROL::UpdateMenu( const TOOL_EVENT& aEvent )
{
    ACTION_MENU*      actionMenu = aEvent.Parameter<ACTION_MENU*>();
    CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( actionMenu );
    SELECTION         dummySel;

    if( conditionalMenu )
        conditionalMenu->Evaluate( dummySel );

    if( actionMenu )
        actionMenu->UpdateAll();

    return 0;
}


void CVPCB_CONTROL::setTransitions()
{
    // Control actions
    Go( &CVPCB_CONTROL::UpdateMenu,            ACTIONS::updateMenu.MakeEvent() );
    Go( &CVPCB_CONTROL::Main,                  CVPCB_ACTIONS::controlActivate.MakeEvent() );
    Go( &CVPCB_CONTROL::ChangeFocus,           CVPCB_ACTIONS::changeFocusRight.MakeEvent() );
    Go( &CVPCB_CONTROL::ChangeFocus,           CVPCB_ACTIONS::changeFocusLeft.MakeEvent() );

    // Run the footprint viewer
    Go( &CVPCB_CONTROL::ShowFootprintViewer,   CVPCB_ACTIONS::showFootprintViewer.MakeEvent() );

    // Management actions
    Go( &CVPCB_CONTROL::ShowEquFileTable,      CVPCB_ACTIONS::showEquFileTable.MakeEvent() );
    Go( &CVPCB_CONTROL::SaveAssociationsToSchematic,
        CVPCB_ACTIONS::saveAssociationsToSchematic.MakeEvent() );
    Go( &CVPCB_CONTROL::SaveAssociationsToFile,CVPCB_ACTIONS::saveAssociationsToFile.MakeEvent() );

    // Navigation actions
    Go( &CVPCB_CONTROL::ToNA,                  CVPCB_ACTIONS::gotoNextNA.MakeEvent() );
    Go( &CVPCB_CONTROL::ToNA,                  CVPCB_ACTIONS::gotoPreviousNA.MakeEvent() );

    // Filter the footprints
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::FilterFPbyFPFilters.MakeEvent() );
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::FilterFPbyLibrary.MakeEvent() );
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::filterFPbyPin.MakeEvent() );
}
