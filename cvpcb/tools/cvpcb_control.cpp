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

#include <cstdint>
#include <functional>
#include <kiface_i.h>
#include <kiway_express.h>
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


int CVPCB_CONTROL::AutoAssociate( const TOOL_EVENT& aEvent )
{
    m_frame->AutomaticFootprintMatching();

    return 0;
}


int CVPCB_CONTROL::DeleteAll( const TOOL_EVENT& aEvent )
{
    m_frame->DeleteAll();

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


int CVPCB_CONTROL::ToNextNA( const TOOL_EVENT& aEvent )
{
    m_frame->ToNextNA();
    return 0;
}


int CVPCB_CONTROL::ToPreviousNA( const TOOL_EVENT& aEvent )
{
    m_frame->ToPreviousNA();
    return 0;
}


void CVPCB_CONTROL::setTransitions()
{
    // Run the footprint viewer
    Go( &CVPCB_CONTROL::ShowFootprintViewer,   CVPCB_ACTIONS::showFootprintViewer.MakeEvent() );

    // Management actions
    Go( &CVPCB_CONTROL::ShowEquFileTable,      CVPCB_ACTIONS::showEquFileTable.MakeEvent() );
    Go( &CVPCB_CONTROL::SaveAssociations,      CVPCB_ACTIONS::saveAssociations.MakeEvent() );
    Go( &CVPCB_CONTROL::DeleteAll,             CVPCB_ACTIONS::deleteAll.MakeEvent() );

    // Navigation actions
    Go( &CVPCB_CONTROL::ToNextNA,              CVPCB_ACTIONS::gotoNextNA.MakeEvent() );
    Go( &CVPCB_CONTROL::ToPreviousNA,          CVPCB_ACTIONS::gotoPreviousNA.MakeEvent() );

    // Footprint association actions
    Go( &CVPCB_CONTROL::AutoAssociate,         CVPCB_ACTIONS::autoAssociate.MakeEvent() );

    // Filter the footprints
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::filterFPbyKeywords.MakeEvent() );
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::filterFPbyLibrary.MakeEvent() );
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::filterFPbyPin.MakeEvent() );
    Go( &CVPCB_CONTROL::ToggleFootprintFilter, CVPCB_ACTIONS::filterFPbyDisplayName.MakeEvent() );
}
