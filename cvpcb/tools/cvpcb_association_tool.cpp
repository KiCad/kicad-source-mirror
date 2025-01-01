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
#include <wx/clipbrd.h>
#include <wx/log.h>

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
    LIB_ID fpid;

    if( m_frame->GetFocusedControl() == CVPCB_MAINFRAME::CONTROL_FOOTPRINT )
        fpid.Parse( m_frame->GetSelectedFootprint() );
    else if( m_frame->GetSelectedComponent() )
        fpid = m_frame->GetSelectedComponent()->GetFPID();
    else
        return 0;

    // if no valid fpid, then skip
    if( !fpid.IsValid() )
        return 0;

    wxLogNull raiiDoNotLog; // disable logging of failed clipboard actions

    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->SetData( new wxTextDataObject( fpid.GetUniStringLibId() ) );
        wxTheClipboard->Flush();
        wxTheClipboard->Close();
    }

    return 0;
}


int CVPCB_ASSOCIATION_TOOL::CutAssoc( const TOOL_EVENT& aEvent )
{
    // If using the keyboard, only cut in the component frame
    if( m_frame->GetFocusedControl()
            && m_frame->GetFocusedControl() != CVPCB_MAINFRAME::CONTROL_COMPONENT )
    {
        return 0;
    }

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
    {
        wxLogNull doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            if( !wxTheClipboard->SetData( new wxTextDataObject( fpid.GetUniStringLibId() ) ) )
            {
                wxTheClipboard->Close();
                return 0;
            }

            wxTheClipboard->Flush();
            wxTheClipboard->Close();
        }
        else
        {
            return 0;
        }
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

    {
        wxLogNull doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            wxTheClipboard->GetData( data );
            wxTheClipboard->Close();
        }
        else
        {
            return 0;
        }
    }

    if( fpid.Parse( data.GetText() ) >= 0 )
        return 0;

    // Assign the fpid to the selections
    bool firstAssoc = true;

    for( unsigned int i : idx )
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
    fpid.Parse( fp );

    // Ignore the action if the footprint is empty (nothing selected)
    if( fpid.empty() )
        return 0;

    // Test for validity of the requested footprint
    if( !fpid.IsValid() )
    {
        wxString msg = wxString::Format( _( "'%s' is not a valid footprint." ),
                                         fpid.Format().wx_str() );
        DisplayErrorMessage( m_frame, msg );
    }

    // Get all the components that are selected and associate them with the current footprint
    bool firstAssoc = true;

    for( unsigned int i : m_frame->GetComponentIndices( CVPCB_MAINFRAME::SEL_COMPONENTS ) )
    {
        CVPCB_ASSOCIATION newfp( i, fpid );
        m_frame->AssociateFootprint( newfp, firstAssoc );
        firstAssoc = false;
    }

    // Move to the next not associated component
    m_toolMgr->PostAction( CVPCB_ACTIONS::gotoNextNA );

    return 0;
}


int CVPCB_ASSOCIATION_TOOL::AutoAssociate( const TOOL_EVENT& aEvent )
{
    m_frame->AutomaticFootprintMatching();

    return 0;
}


int CVPCB_ASSOCIATION_TOOL::DeleteAssoc( const TOOL_EVENT& aEvent )
{
    // Delete all the selected components' associations
    bool firstAssoc = true;

    for( unsigned int i : m_frame->GetComponentIndices( CVPCB_MAINFRAME::SEL_COMPONENTS ) )
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

        bool firstAssoc = true;

        for( unsigned int i : m_frame->GetComponentIndices( CVPCB_MAINFRAME::ALL_COMPONENTS ) )
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
