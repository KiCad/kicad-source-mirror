/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2007-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file tool_cvpcb.cpp
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <common.h>

#include <bitmaps.h>
#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <cvpcb_id.h>

#include <common_help_msg.h>


void CVPCB_MAINFRAME::ReCreateHToolbar()
{
    if( m_mainToolBar != NULL )
        return;

    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    m_mainToolBar->AddTool( wxID_SAVE, wxEmptyString, KiBitmap( save_xpm ), SAVE_HLP_MSG );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_LIB_TABLE_EDIT, wxEmptyString,
                            KiBitmap( config_xpm ),
                            _( "Edit footprint library table" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_CREATE_SCREENCMP, wxEmptyString,
                            KiBitmap( show_footprint_xpm ),
                            _( "View selected footprint" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_GOTO_PREVIOUSNA, wxEmptyString,
                            KiBitmap( left_xpm ),
                            _( "Select previous unlinked component" ) );

    m_mainToolBar->AddTool( ID_CVPCB_GOTO_FIRSTNA, wxEmptyString,
                            KiBitmap( right_xpm ),
                            _( "Select next unlinked component" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_AUTO_ASSOCIE, wxEmptyString,
                            KiBitmap( auto_associe_xpm ),
                            _( "Perform automatic footprint association" ) );

    m_mainToolBar->AddTool( ID_CVPCB_DEL_ASSOCIATIONS, wxEmptyString,
                            KiBitmap( delete_association_xpm ),
                            _( "Delete all footprint associations" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST,
                            KiBitmap( module_filtered_list_xpm ),
                            wxNullBitmap,
                            true, NULL,
                            _( "Filter footprint list by schematic components keywords" ),
                            wxEmptyString );

    m_mainToolBar->AddTool( ID_CVPCB_FOOTPRINT_DISPLAY_PIN_FILTERED_LIST,
                            KiBitmap( module_pin_filtered_list_xpm ),
                            wxNullBitmap,
                            true, NULL,
                            _( "Filter footprint list by pin count" ),
                            wxEmptyString );

    m_mainToolBar->AddTool( ID_CVPCB_FOOTPRINT_DISPLAY_BY_LIBRARY_LIST,
                            KiBitmap( module_library_list_xpm ),
                            wxNullBitmap, true, NULL,
                            _( "Filter footprint list by library" ),
                            wxEmptyString );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_FOOTPRINT_DISPLAY_BY_NAME,
                            KiBitmap( module_name_filtered_list_xpm ),
                            wxNullBitmap, true, NULL,
                            _( "Filter footprint list using a partial name or a pattern" ),
                            wxEmptyString );

    m_tcFilterString = new wxTextCtrl( m_mainToolBar, ID_CVPCB_FILTER_TEXT_EDIT );

    m_mainToolBar->AddControl( m_tcFilterString );


    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}
