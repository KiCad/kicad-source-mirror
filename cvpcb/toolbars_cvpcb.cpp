/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2007-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file tool_cvpcb.cpp
 */

#include <bitmaps.h>
#include <tool/action_toolbar.h>
#include <tool/actions.h>

#include <cvpcb_id.h>
#include <cvpcb_mainframe.h>
#include <tools/cvpcb_actions.h>
#include <wx/stattext.h>


void CVPCB_MAINFRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
    {
        m_mainToolBar->ClearToolbar();
    }
    else
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );
        m_mainToolBar->SetAuiManager( &m_auimgr );
    }

    m_mainToolBar->Add( CVPCB_ACTIONS::saveAssociations );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::showFootprintLibTable );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( CVPCB_ACTIONS::showFootprintViewer );


    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( CVPCB_ACTIONS::gotoPreviousNA );
    m_mainToolBar->Add( CVPCB_ACTIONS::gotoNextNA );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::undo );
    m_mainToolBar->Add( ACTIONS::redo );
    m_mainToolBar->Add( CVPCB_ACTIONS::autoAssociate );
    m_mainToolBar->Add( CVPCB_ACTIONS::deleteAll );

    // Add tools for footprint names filtering:
    m_mainToolBar->AddScaledSeparator( this );

    // wxGTK with GTK3 has a serious issue with bold texts: strings are incorrectly sized
    // and truncated after the first space.
    // so use SetLabelMarkup is a trick to fix this issue.
    m_mainToolBar->AddSpacer( 15 );
    wxString msg_bold = _( "Footprint Filters:" );
    wxStaticText* text = new wxStaticText( m_mainToolBar, wxID_ANY, msg_bold );
	text->SetFont( m_mainToolBar->GetFont().Bold() );
#ifdef __WXGTK3__
    text->SetLabelMarkup( "<b>" + msg_bold + "</b>" );
#endif
    m_mainToolBar->AddControl( text );

    m_mainToolBar->Add( CVPCB_ACTIONS::FilterFPbyFPFilters, ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->Add( CVPCB_ACTIONS::filterFPbyPin,       ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->Add( CVPCB_ACTIONS::FilterFPbyLibrary,   ACTION_TOOLBAR::TOGGLE );

    m_mainToolBar->AddScaledSeparator( this );

    m_tcFilterString = new wxTextCtrl( m_mainToolBar, ID_CVPCB_FILTER_TEXT_EDIT, wxEmptyString,
                                       wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );

    m_tcFilterString->Bind( wxEVT_TEXT_ENTER, &CVPCB_MAINFRAME::OnEnterFilteringText, this );

    m_mainToolBar->AddControl( m_tcFilterString );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}
