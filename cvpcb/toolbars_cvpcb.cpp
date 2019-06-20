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

#include <fctsys.h>
#include <kiface_i.h>
#include <common.h>

#include <bitmaps.h>
#include <cvpcb_mainframe.h>
#include <cvpcb_id.h>


void CVPCB_MAINFRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                          KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    m_mainToolBar->AddTool( ID_CVPCB_LIB_TABLE_EDIT, wxEmptyString,
                            KiScaledBitmap( config_xpm, this ),
                            _( "Edit footprint library table" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_CVPCB_CREATE_SCREENCMP, wxEmptyString,
                            KiScaledBitmap( show_footprint_xpm, this ),
                            _( "View selected footprint" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_CVPCB_GOTO_PREVIOUSNA, wxEmptyString,
                            KiScaledBitmap( left_xpm, this ),
                            _( "Select previous unlinked symbol" ) );

    m_mainToolBar->AddTool( ID_CVPCB_GOTO_FIRSTNA, wxEmptyString,
                            KiScaledBitmap( right_xpm, this ),
                            _( "Select next unlinked symbol" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_CVPCB_AUTO_ASSOCIE, wxEmptyString,
                            KiScaledBitmap( auto_associe_xpm, this ),
                            _( "Perform automatic footprint association" ) );

    m_mainToolBar->AddTool( ID_CVPCB_DEL_ALL_ASSOCIATIONS, wxEmptyString,
                            KiScaledBitmap( delete_association_xpm, this ),
                            _( "Delete all footprint associations" ) );

    // Add tools for footprint names filtering:
    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddSpacer( 15 );
    wxStaticText* text = new wxStaticText( m_mainToolBar, wxID_ANY,
                                           _( "Footprint Filters:" ) );
	text->SetFont( m_mainToolBar->GetFont().Bold() );
    m_mainToolBar->AddControl( text );

    m_mainToolBar->AddTool( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST,
                            KiScaledBitmap( module_filtered_list_xpm, this ),
                            wxNullBitmap,
                            true, NULL,
                            _( "Filter footprint list by schematic symbol keywords" ),
                            wxEmptyString );

    m_mainToolBar->AddTool( ID_CVPCB_FOOTPRINT_DISPLAY_PIN_FILTERED_LIST,
                            KiScaledBitmap( module_pin_filtered_list_xpm, this ),
                            wxNullBitmap,
                            true, NULL,
                            _( "Filter footprint list by pin count" ),
                            wxEmptyString );

    m_mainToolBar->AddTool( ID_CVPCB_FOOTPRINT_DISPLAY_BY_LIBRARY_LIST,
                            KiScaledBitmap( module_library_list_xpm, this ),
                            wxNullBitmap, true, NULL,
                            _( "Filter footprint list by library" ),
                            wxEmptyString );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_CVPCB_FOOTPRINT_DISPLAY_BY_NAME,
                            KiScaledBitmap( module_name_filtered_list_xpm, this ),
                            wxNullBitmap, true, NULL,
                            _( "Filter footprint list using a partial name or a pattern" ),
                            wxEmptyString );

    m_tcFilterString = new wxTextCtrl( m_mainToolBar, ID_CVPCB_FILTER_TEXT_EDIT );

    m_mainToolBar->AddControl( m_tcFilterString );


    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}
