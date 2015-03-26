/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <menus_helpers.h>
#include <hotkeys.h>
#include <dialog_helpers.h>
#include <modview_frame.h>
#include <help_common_strings.h>


void FOOTPRINT_VIEWER_FRAME::ReCreateHToolbar()
{
    wxString msg;

    if( m_mainToolBar == NULL )
    {
        m_mainToolBar = new wxAuiToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT
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
        msg = AddHotkeyName( _( "Zoom in" ), g_Module_Viewer_Hokeys_Descr,
                             HK_ZOOM_IN, IS_COMMENT );
        m_mainToolBar->AddTool( ID_VIEWER_ZOOM_IN, wxEmptyString,
                                KiBitmap( zoom_in_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom out" ), g_Module_Viewer_Hokeys_Descr,
                             HK_ZOOM_OUT, IS_COMMENT );
        m_mainToolBar->AddTool( ID_VIEWER_ZOOM_OUT, wxEmptyString,
                                KiBitmap( zoom_out_xpm ), msg );

        msg = AddHotkeyName( _( "Redraw view" ), g_Module_Viewer_Hokeys_Descr,
                             HK_ZOOM_REDRAW );
        m_mainToolBar->AddTool( ID_VIEWER_ZOOM_REDRAW, wxEmptyString,
                             KiBitmap( zoom_redraw_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom auto" ), g_Module_Viewer_Hokeys_Descr,
                             HK_ZOOM_AUTO );
        m_mainToolBar->AddTool( ID_VIEWER_ZOOM_PAGE, wxEmptyString,
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


// Virtual function
void FOOTPRINT_VIEWER_FRAME::ReCreateMenuBar( void )
{
    // Create and try to get the current menubar
    wxMenuBar* menuBar = GetMenuBar();

    if( !menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();

    while( menuBar->GetMenuCount() )
        delete menuBar->Remove( 0 );

    // Recreate all menus:
    wxString text;

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // Active library selection
    AddMenuItem( fileMenu, ID_MODVIEW_SELECT_LIB, _("Set Current Library"),
                 _( "Select library to be displayed" ), KiBitmap( open_library_xpm ) );
    fileMenu->AppendSeparator();

    // Close viewer
    AddMenuItem( fileMenu, wxID_EXIT,
                 _( "Cl&ose" ),
                 _( "Close footprint viewer" ),
                 KiBitmap( exit_xpm ) );

    // View menu
    wxMenu* viewMenu = new wxMenu;

    text = AddHotkeyName( _( "Zoom &In" ), g_Module_Viewer_Hokeys_Descr,
                          HK_ZOOM_IN, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_VIEWER_ZOOM_IN, text, HELP_ZOOM_IN, KiBitmap( zoom_in_xpm ) );

    text = AddHotkeyName( _( "Zoom &Out" ), g_Module_Viewer_Hokeys_Descr,
                          HK_ZOOM_OUT, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_VIEWER_ZOOM_OUT, text, HELP_ZOOM_OUT, KiBitmap( zoom_out_xpm ) );

    text = AddHotkeyName( _( "&Fit on Screen" ), g_Module_Viewer_Hokeys_Descr,
                          HK_ZOOM_AUTO  );
    AddMenuItem( viewMenu, ID_VIEWER_ZOOM_PAGE, text, HELP_ZOOM_FIT,
                 KiBitmap( zoom_fit_in_page_xpm ) );

    text = AddHotkeyName( _( "&Redraw" ), g_Module_Viewer_Hokeys_Descr, HK_ZOOM_REDRAW );
    AddMenuItem( viewMenu, ID_VIEWER_ZOOM_REDRAW, text,
                 HELP_ZOOM_REDRAW, KiBitmap( zoom_redraw_xpm ) );

    viewMenu->AppendSeparator();

    // 3D view
    text = AddHotkeyName( _( "3&D Viewer" ), g_Module_Viewer_Hokeys_Descr, HK_3D_VIEWER );
    AddMenuItem( viewMenu, ID_MODVIEW_SHOW_3D_VIEW, text, _( "Show footprint in 3D viewer" ),
                 KiBitmap( three_d_xpm ) );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    AddMenuItem( helpMenu, wxID_HELP,
                 _( "P&cbnew Manual" ),
                 _( "Open the Pcbnew manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open the \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    // About Pcbnew
    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, wxID_ABOUT,
                 _( "&About Pcbnew" ),
                 _( "About Pcbnew PCB designer" ),
                 KiBitmap( info_xpm ) );

    // Append menus to the menubar
    menuBar->Append( fileMenu, _( "&File" ) );

    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
