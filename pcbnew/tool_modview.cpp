/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file tool_modview.cpp
 * @brief Build the toolbars for the library browser.
 */

#include <fctsys.h>
#include <macros.h>
#include <pcbnew_id.h>

#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <hotkeys.h>
#include <dialog_helpers.h>
#include <modview_frame.h>


void FOOTPRINT_VIEWER_FRAME::ReCreateHToolbar()
{
    wxString msg;

    if( m_mainToolBar == NULL )
    {
        m_mainToolBar = new wxAuiToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          wxAUI_TB_DEFAULT_STYLE
                                            | wxAUI_TB_OVERFLOW );

        // Set up toolbar
        m_mainToolBar->AddTool( ID_MODVIEW_SELECT_LIB, wxEmptyString,
                                KiBitmap( library_xpm ),
                                _( "Select library to browse" ) );

        m_mainToolBar->AddTool( ID_MODVIEW_SELECT_PART, wxEmptyString,
                                KiBitmap( module_xpm ),
                                _( "Select footprint to browse" ) );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_MODVIEW_PREVIOUS, wxEmptyString,
                                KiBitmap( lib_previous_xpm ),
                                _( "Display previous footprint" ) );

        m_mainToolBar->AddTool( ID_MODVIEW_NEXT, wxEmptyString,
                                KiBitmap( lib_next_xpm ),
                                _( "Display next footprint" ) );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_MODVIEW_SHOW_3D_VIEW, wxEmptyString,
                                KiBitmap( three_d_xpm ),
                                _( "Show footprint in 3D viewer" ) );

        m_mainToolBar->AddSeparator();
        msg = AddHotkeyName( _( "Zoom in" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_IN, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                                KiBitmap( zoom_in_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom out" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_OUT, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                                KiBitmap( zoom_out_xpm ), msg );

        msg = AddHotkeyName( _( "Redraw view" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_REDRAW, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                             KiBitmap( zoom_redraw_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom auto" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_AUTO, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                                KiBitmap( zoom_fit_in_page_xpm ), msg );

        if( IsModal() )
        {
            m_mainToolBar->AddSeparator();
            m_mainToolBar->AddTool( ID_MODVIEW_FOOTPRINT_EXPORT_TO_BOARD, wxEmptyString,
                                    KiBitmap( export_footprint_names_xpm ),
                                    _( "Insert footprint in board" ) );
        }

        // after adding the buttons to the toolbar, must call Realize() to
        // reflect the changes
        m_mainToolBar->Realize();
    }

    m_mainToolBar->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateVToolbar()
{
}
