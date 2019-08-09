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


void CVPCB_MAINFRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    m_mainToolBar->Add( ACTIONS::showFootprintLibTable );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( CVPCB_ACTIONS::showFootprintViewer );


    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( CVPCB_ACTIONS::gotoPreviousNA );
    m_mainToolBar->Add( CVPCB_ACTIONS::gotoNextNA );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( ACTIONS::undo );
    m_mainToolBar->Add( ACTIONS::redo );
    m_mainToolBar->Add( CVPCB_ACTIONS::autoAssociate );
    m_mainToolBar->Add( CVPCB_ACTIONS::deleteAll );

    // Add tools for footprint names filtering:
    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddSpacer( 15 );
    wxStaticText* text = new wxStaticText( m_mainToolBar, wxID_ANY,
                                           _( "Footprint Filters:" ) );
	text->SetFont( m_mainToolBar->GetFont().Bold() );
    m_mainToolBar->AddControl( text );

    m_mainToolBar->Add( CVPCB_ACTIONS::filterFPbyKeywords, true );
    m_mainToolBar->Add( CVPCB_ACTIONS::filterFPbyPin, true );
    m_mainToolBar->Add( CVPCB_ACTIONS::filterFPbyLibrary, true );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( CVPCB_ACTIONS::filterFPbyDisplayName, true );

    m_tcFilterString = new wxTextCtrl( m_mainToolBar, ID_CVPCB_FILTER_TEXT_EDIT );

    m_mainToolBar->AddControl( m_tcFilterString );


    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void CVPCB_MAINFRAME::SyncToolbars()
{
#define filterActive( filt ) ( m_filteringOptions & filt )

    m_mainToolBar->Toggle( ACTIONS::undo, m_undoList.size() > 0 );
    m_mainToolBar->Toggle( ACTIONS::redo, m_redoList.size() > 0 );

    m_mainToolBar->Toggle( CVPCB_ACTIONS::filterFPbyKeywords,
            filterActive( FOOTPRINTS_LISTBOX::FILTERING_BY_COMPONENT_KEYWORD ) );
    m_mainToolBar->Toggle( CVPCB_ACTIONS::filterFPbyLibrary,
            filterActive( FOOTPRINTS_LISTBOX::FILTERING_BY_LIBRARY ) );
    m_mainToolBar->Toggle( CVPCB_ACTIONS::filterFPbyPin,
            filterActive( FOOTPRINTS_LISTBOX::FILTERING_BY_PIN_COUNT ) );
    m_mainToolBar->Toggle( CVPCB_ACTIONS::filterFPbyDisplayName,
            filterActive( FOOTPRINTS_LISTBOX::FILTERING_BY_NAME ) );
    m_mainToolBar->Refresh();
}
