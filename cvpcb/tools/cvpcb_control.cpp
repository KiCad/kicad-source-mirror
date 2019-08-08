/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Ian McInerney <Ian.S.McInerney@ieee.org>
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_i.h>
#include <kiway_express.h>
#include <lib_id.h>
#include <tool/actions.h>

#include <cvpcb_mainframe.h>
#include <dialogs/dialog_config_equfiles.h>
#include <display_footprints_frame.h>
#include <listboxes.h>
#include <tools/cvpcb_actions.h>
#include <tools/cvpcb_control.h>

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


int CVPCB_CONTROL::ShowFootprintViewer( const TOOL_EVENT& aEvent )
{

    DISPLAY_FOOTPRINTS_FRAME* fpframe = m_frame->GetFootprintViewerFrame();

    if( !fpframe )
    {
        fpframe = (DISPLAY_FOOTPRINTS_FRAME*) m_frame->Kiway().Player(
                FRAME_CVPCB_DISPLAY, true, m_frame );
        fpframe->Show( true );
    }
    else
    {
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
    m_frame->SetFootprintFilter(
            static_cast<FOOTPRINTS_LISTBOX::FP_FILTER_T>( aEvent.Parameter<intptr_t>() ),
            CVPCB_MAINFRAME::FILTER_TOGGLE );

    return 0;
}


int CVPCB_CONTROL::Undo( const TOOL_EVENT& aEvent )
{
    m_frame->UndoAssociation();

    return 0;
}


int CVPCB_CONTROL::Redo( const TOOL_EVENT& aEvent )
{
    m_frame->RedoAssociation();

    return 0;
}


int CVPCB_CONTROL::Associate( const TOOL_EVENT& aEvent )
{
    // Get the currently selected footprint
    LIB_ID   fpid;
    wxString fp = m_frame->GetSelectedFootprint();
    fpid.Parse( fp, LIB_ID::ID_PCB );

    // Ignore the action if the footprint is empty (nothing selected)
    if( fpid.empty() )
        return 0;

    // Test for validity of the requested footprint
    if( !fpid.IsValid() )
    {
        wxString msg =
                wxString::Format( _( "\"%s\" is not a valid footprint." ), fpid.Format().wx_str() );
        DisplayErrorMessage( m_frame, msg );
    }

    // Get all the components that are selected and associate them with the current footprint
    std::vector<unsigned int> sel = m_frame->GetComponentIndices( CVPCB_MAINFRAME::SEL_COMPONENTS );

    bool firstAssoc = true;
    for( auto i : sel )
    {
        CVPCB_ASSOCIATION newfp( i, fpid );
        m_frame->AssociateFootprint( newfp, firstAssoc );
        firstAssoc = false;
    }

    return 0;
}


int CVPCB_CONTROL::AutoAssociate( const TOOL_EVENT& aEvent )
{
    m_frame->AutomaticFootprintMatching();

    return 0;
}


int CVPCB_CONTROL::DeleteAll( const TOOL_EVENT& aEvent )
{
    if( IsOK( m_frame, _( "Delete all associations?" ) ) )
    {
        // Remove all selections to avoid issues when setting the fpids
        m_frame->SetSelectedComponent( -1, true );
        std::vector<unsigned int> idx
                = m_frame->GetComponentIndices( CVPCB_MAINFRAME::ALL_COMPONENTS );

        bool firstAssoc = true;
        for( auto i : idx )
        {
            m_frame->AssociateFootprint( CVPCB_ASSOCIATION( i, LIB_ID() ), firstAssoc );
            firstAssoc = false;
        }

        // Remove all selections after setting the fpids and select the first component
        m_frame->SetSelectedComponent( -1, true );
        m_frame->SetSelectedComponent( 0 );
    }

    // Update the status display
    m_frame->DisplayStatus();

    return 0;
}


int CVPCB_CONTROL::ShowEquFileTable( const TOOL_EVENT& aEvent )
{
    DIALOG_CONFIG_EQUFILES dlg( m_frame );
    dlg.ShowModal();

    return 0;
}


int CVPCB_CONTROL::SaveAssociations( const TOOL_EVENT& aEvent )
{
    m_frame->SaveFootprintAssociation( true );
    return 0;
}


int CVPCB_CONTROL::ToNA( const TOOL_EVENT& aEvent )
{
    int tmp = aEvent.Parameter<intptr_t>();
    CVPCB_MAINFRAME::ITEM_DIR dir =
            static_cast<CVPCB_MAINFRAME::ITEM_DIR>( tmp );

    std::vector<unsigned int> naComp = m_frame->GetComponentIndices( CVPCB_MAINFRAME::NA_COMPONENTS );
    std::vector<unsigned int> tempSel = m_frame->GetComponentIndices( CVPCB_MAINFRAME::SEL_COMPONENTS );

    // No unassociated components
    if( naComp.empty() )
        return 0;

    // Extract the current selection
    unsigned int curSel = -1;
    unsigned int newSel = -1;
    switch( dir )
    {
    case CVPCB_MAINFRAME::ITEM_NEXT:
        if( !tempSel.empty() )
            newSel = tempSel.front();

        // Find the next index in the component list
        for( unsigned int i : naComp )
        {
            if( i > newSel )
            {
                newSel = i;
                break;
            }
        }

        break;

    case CVPCB_MAINFRAME::ITEM_PREV:
        if( !tempSel.empty() )
        {
            newSel = tempSel.front();
            curSel = newSel - 1;        // Break one before the current selection
        }

        break;

    default:
        wxASSERT_MSG( false, "Invalid direction" );
    }

    // Find the next index in the component list
    for( unsigned int i : naComp )
    {
        if( i >= curSel )
        {
            newSel = i;
            break;
        }
    }

    // Set the component selection
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
    // Update the menu
    Go( &CVPCB_CONTROL::UpdateMenu,            ACTIONS::updateMenu.MakeEvent() );

    // Run the footprint viewer
    Go( &CVPCB_CONTROL::ShowFootprintViewer,   CVPCB_ACTIONS::showFootprintViewer.MakeEvent() );

    // Management actions
    Go( &CVPCB_CONTROL::ShowEquFileTable,      CVPCB_ACTIONS::showEquFileTable.MakeEvent() );
    Go( &CVPCB_CONTROL::SaveAssociations,      CVPCB_ACTIONS::saveAssociations.MakeEvent() );
    Go( &CVPCB_CONTROL::DeleteAll,             CVPCB_ACTIONS::deleteAll.MakeEvent() );

    // Navigation actions
    Go( &CVPCB_CONTROL::ToNA,                  CVPCB_ACTIONS::gotoNextNA.MakeEvent() );
    Go( &CVPCB_CONTROL::ToNA,                  CVPCB_ACTIONS::gotoPreviousNA.MakeEvent() );

    // Footprint association actions
    Go( &CVPCB_CONTROL::Undo,                  ACTIONS::undo.MakeEvent() );
    Go( &CVPCB_CONTROL::Redo,                  ACTIONS::redo.MakeEvent() );
    Go( &CVPCB_CONTROL::Associate,             CVPCB_ACTIONS::associate.MakeEvent() );
    Go( &CVPCB_CONTROL::AutoAssociate,         CVPCB_ACTIONS::autoAssociate.MakeEvent() );

    // Filter the footprints
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::filterFPbyKeywords.MakeEvent() );
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::filterFPbyLibrary.MakeEvent() );
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::filterFPbyPin.MakeEvent() );
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::filterFPbyDisplayName.MakeEvent() );
}
