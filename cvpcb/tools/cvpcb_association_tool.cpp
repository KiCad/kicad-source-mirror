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
#include <tool/tool_manager.h>
#include <wx/clipbrd.h>

#include <cvpcb_association.h>
#include <cvpcb_mainframe.h>
#include <dialogs/dialog_config_equfiles.h>
#include <display_footprints_frame.h>
#include <listboxes.h>
#include <tools/cvpcb_actions.h>
#include <tools/cvpcb_association_tool.h>

using namespace std::placeholders;


CVPCB_ASSOCIATION_TOOL::CVPCB_ASSOCIATION_TOOL() :
        TOOL_INTERACTIVE( "cvpcb.Association" ),
        m_frame( nullptr )
{
}


int CVPCB_ASSOCIATION_TOOL::CopyAssoc( const TOOL_EVENT& aEvent )
{
    COMPONENT* comp;
    LIB_ID     fpid;
    switch( m_frame->GetFocusedControl() )
    {
    case CVPCB_MAINFRAME::CONTROL_FOOTPRINT:
        fpid.Parse( m_frame->GetSelectedFootprint(), LIB_ID::ID_PCB );
        break;

    case CVPCB_MAINFRAME::CONTROL_COMPONENT:
        // Get the selection
        comp = m_frame->GetSelectedComponent();

        if( !comp )
            return 0;

        // Get the fpid and save it to the clipboard
        fpid = comp->GetFPID();
        break;

    default:
        // Do nothing
        break;
    }

    // if no valid fpid, then skip
    if( !fpid.IsValid() )
        return 0;

    if( wxTheClipboard->Open() )
    {
        if( !wxTheClipboard->SetData( new wxTextDataObject( fpid.GetUniStringLibId() ) ) )
            wxLogDebug( "Failed to copy data to clipboard" );

        wxTheClipboard->Flush();
        wxTheClipboard->Close();
    }

    return 0;
}


int CVPCB_ASSOCIATION_TOOL::CutAssoc( const TOOL_EVENT& aEvent )
{
    // Only cut when in the component frame
    if( m_frame->GetFocusedControl() != CVPCB_MAINFRAME::CONTROL_COMPONENT )
        return 0;

    // Get the selection, but only use the first one
    COMPONENT*                comp = m_frame->GetSelectedComponent();
    std::vector<unsigned int> idx = m_frame->GetComponentIndices( CVPCB_MAINFRAME::SEL_COMPONENTS );

    if( idx.empty() || !comp )
        return 0;

    // Get the fpid
    LIB_ID fpid;
    fpid = comp->GetFPID();

    // if no valid fpid, then skip
    if( !fpid.IsValid() )
        return 0;

    // Save it to the clipboard
    if( wxTheClipboard->Open() )
    {
        if( !wxTheClipboard->SetData( new wxTextDataObject( fpid.GetUniStringLibId() ) ) )
        {
            wxLogDebug( "Failed to cut data to clipboard" );
            wxTheClipboard->Close();
            return 0;
        }

        wxTheClipboard->Flush();
        wxTheClipboard->Close();
    }

    // Remove the association
    m_frame->AssociateFootprint( CVPCB_ASSOCIATION( idx.front(), "" ) );

    return 0;
}


int CVPCB_ASSOCIATION_TOOL::PasteAssoc( const TOOL_EVENT& aEvent )
{
    // Get the selection
    std::vector<unsigned int> idx = m_frame->GetComponentIndices( CVPCB_MAINFRAME::SEL_COMPONENTS );

    if( idx.empty() )
        return 0;

    // Get the clipboard data and ensure it is valid
    LIB_ID           fpid;
    wxTextDataObject data;

    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->GetData( data );
        wxTheClipboard->Close();
    }

    if( fpid.Parse( data.GetText(), LIB_ID::ID_PCB ) >= 0 )
        return 0;

    // Assign the fpid to the selections
    bool firstAssoc = true;
    for( auto i : idx )
    {
        m_frame->AssociateFootprint( CVPCB_ASSOCIATION( i, fpid ), firstAssoc );
        firstAssoc = false;
    }

    return 0;
}

void CVPCB_ASSOCIATION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<CVPCB_MAINFRAME>();
}


int CVPCB_ASSOCIATION_TOOL::Undo( const TOOL_EVENT& aEvent )
{
    m_frame->UndoAssociation();

    return 0;
}


int CVPCB_ASSOCIATION_TOOL::Redo( const TOOL_EVENT& aEvent )
{
    m_frame->RedoAssociation();

    return 0;
}


int CVPCB_ASSOCIATION_TOOL::Associate( const TOOL_EVENT& aEvent )
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

    // Move to the next not associated component
    m_toolMgr->RunAction( CVPCB_ACTIONS::gotoNextNA );

    return 0;
}


int CVPCB_ASSOCIATION_TOOL::AutoAssociate( const TOOL_EVENT& aEvent )
{
    m_frame->AutomaticFootprintMatching();

    return 0;
}


int CVPCB_ASSOCIATION_TOOL::DeleteAssoc( const TOOL_EVENT& aEvent )
{
    // Get all the components that are selected
    std::vector<unsigned int> sel = m_frame->GetComponentIndices( CVPCB_MAINFRAME::SEL_COMPONENTS );

    // Delete the association
    bool firstAssoc = true;
    for( auto i : sel )
    {
        m_frame->AssociateFootprint( CVPCB_ASSOCIATION( i, LIB_ID() ), firstAssoc );
        firstAssoc = false;
    }

    return 0;
}


int CVPCB_ASSOCIATION_TOOL::DeleteAll( const TOOL_EVENT& aEvent )
{
    if( IsOK( m_frame, _( "Delete all associations?" ) ) )
    {
        // Remove all selections to avoid issues when setting the fpids
        m_frame->SetSelectedComponent( -1, true );
        std::vector<unsigned int> idx =
                m_frame->GetComponentIndices( CVPCB_MAINFRAME::ALL_COMPONENTS );

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


void CVPCB_ASSOCIATION_TOOL::setTransitions()
{
    // Association
    Go( &CVPCB_ASSOCIATION_TOOL::Associate,        CVPCB_ACTIONS::associate.MakeEvent() );
    Go( &CVPCB_ASSOCIATION_TOOL::AutoAssociate,    CVPCB_ACTIONS::autoAssociate.MakeEvent() );

    // Deletion
    Go( &CVPCB_ASSOCIATION_TOOL::DeleteAll,        CVPCB_ACTIONS::deleteAll.MakeEvent() );
    Go( &CVPCB_ASSOCIATION_TOOL::DeleteAssoc,      CVPCB_ACTIONS::deleteAssoc.MakeEvent() );

    // Helpers
    Go( &CVPCB_ASSOCIATION_TOOL::Undo,             ACTIONS::undo.MakeEvent() );
    Go( &CVPCB_ASSOCIATION_TOOL::Redo,             ACTIONS::redo.MakeEvent() );

    // Clipboard
    Go( &CVPCB_ASSOCIATION_TOOL::CutAssoc,         ACTIONS::cut.MakeEvent() );
    Go( &CVPCB_ASSOCIATION_TOOL::CopyAssoc,        ACTIONS::copy.MakeEvent() );
    Go( &CVPCB_ASSOCIATION_TOOL::PasteAssoc,       ACTIONS::paste.MakeEvent() );
}
