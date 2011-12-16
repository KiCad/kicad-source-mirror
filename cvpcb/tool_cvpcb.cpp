/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2007-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"

#include "bitmaps.h"
#include "cvpcb.h"
#include "cvpcb_mainframe.h"
#include "cvpcb_id.h"


void CVPCB_MAINFRAME::ReCreateHToolbar()
{
    wxConfig* config = wxGetApp().GetSettings();

    if( m_mainToolBar != NULL )
        return;

    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    m_mainToolBar->AddTool( ID_CVPCB_READ_INPUT_NETLIST, wxEmptyString,
                            KiBitmap( open_document_xpm ),
                            _( "Open a net list file" ) );

    m_mainToolBar->AddTool( wxID_SAVE, wxEmptyString, KiBitmap( save_xpm ),
                            _( "Save net list and footprint files" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_CREATE_CONFIGWINDOW, wxEmptyString,
                            KiBitmap( config_xpm ),
                            _( "Configuration" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_CREATE_SCREENCMP, wxEmptyString,
                            KiBitmap( show_footprint_xpm ),
                            _( "View selected footprint" ) );

    m_mainToolBar->AddTool( ID_CVPCB_AUTO_ASSOCIE, wxEmptyString,
                            KiBitmap( auto_associe_xpm ),
                            _( "Perform automatic footprint association" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_GOTO_PREVIOUSNA, wxEmptyString,
                            KiBitmap( left_xpm ),
                            _( "Select previous free component" ) );

    m_mainToolBar->AddTool( ID_CVPCB_GOTO_FIRSTNA, wxEmptyString,
                            KiBitmap( right_xpm ),
                            _( "Select next free component" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_DEL_ASSOCIATIONS, wxEmptyString,
                            KiBitmap( delete_association_xpm ),
                            _( "Delete all associations" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_CREATE_STUFF_FILE, wxEmptyString,
                            KiBitmap( export_footprint_names_xpm ),
                            _( "Create export file (component/footprint list, \
used by Eeschema to fill the footprint field of components)" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_PCB_DISPLAY_FOOTPRINT_DOC, wxEmptyString,
                            KiBitmap( datasheet_xpm ),
                            _( "Display footprints list documentation" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST,
                            KiBitmap( module_filtered_list_xpm ),
                            wxNullBitmap,
                            true, NULL,
                            _( "Display the filtered footprint list for the current component" ),
                            wxEmptyString );

    m_mainToolBar->AddTool( ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST,
                            KiBitmap( module_full_list_xpm ),
                            wxNullBitmap, true, NULL,
                            _( "Display the full footprint list (without filtering)" ),
                            wxEmptyString );

    if( config )
    {
        wxString key = wxT( FILTERFOOTPRINTKEY );
        int      opt = config->Read( key, (long) 1 );
        m_mainToolBar->ToggleTool( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST, opt );
        m_mainToolBar->ToggleTool( ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST, !opt );
    }

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}
