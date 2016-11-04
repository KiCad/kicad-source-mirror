/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2016 KiCad Developers, see change_log.txt for contributors.
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
 * @file tool_viewlib.cpp
 * @brief Build the toolbars for the library browser.
 */


#include <dialog_helpers.h>
#include <macros.h>
#include <menus_helpers.h>

#include "class_library.h"
#include "eeschema_id.h"
#include "general.h"
#include "help_common_strings.h"
#include "hotkeys.h"
#include "viewlib_frame.h"


void LIB_VIEW_FRAME::ReCreateHToolbar()
{
    wxString    msg;
    LIB_PART*   part = NULL;

    if( m_mainToolBar == NULL )
    {
        m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                          wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

        m_mainToolBar->AddTool( ID_LIBVIEW_SELECT_PART, wxEmptyString,
                                KiBitmap( add_component_xpm ),
                                _( "Select component to browse" ) );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_LIBVIEW_PREVIOUS, wxEmptyString,
                                KiBitmap( lib_previous_xpm ),
                                _( "Display previous component" ) );

        m_mainToolBar->AddTool( ID_LIBVIEW_NEXT, wxEmptyString,
                                KiBitmap( lib_next_xpm ),
                                _( "Display next component" ) );

        m_mainToolBar->AddSeparator();
        msg = AddHotkeyName( _( "Zoom in" ), g_Viewlib_Hokeys_Descr,
                             HK_ZOOM_IN, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                                KiBitmap( zoom_in_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom out" ), g_Viewlib_Hokeys_Descr,
                             HK_ZOOM_OUT, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                                KiBitmap( zoom_out_xpm ), msg );

        msg = AddHotkeyName( _( "Redraw view" ), g_Viewlib_Hokeys_Descr,
                             HK_ZOOM_REDRAW, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                             KiBitmap( zoom_redraw_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom auto" ), g_Viewlib_Hokeys_Descr,
                             HK_ZOOM_AUTO, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                                KiBitmap( zoom_fit_in_page_xpm ), msg );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, wxEmptyString,
                                KiBitmap( morgan1_xpm ),
                                _( "Show as \"De Morgan\" normal part" ),
                                wxITEM_CHECK );

        m_mainToolBar->AddTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, wxEmptyString,
                                KiBitmap( morgan2_xpm ),
                                _( "Show as \"De Morgan\" convert part" ),
                                wxITEM_CHECK );

        m_mainToolBar->AddSeparator();

        m_selpartBox = new wxComboBox( m_mainToolBar, ID_LIBVIEW_SELECT_PART_NUMBER,
                                       wxEmptyString, wxDefaultPosition,
                                       wxSize( 150, -1 ), 0, NULL, wxCB_READONLY );
        m_mainToolBar->AddControl( m_selpartBox );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_LIBVIEW_VIEWDOC, wxEmptyString,
                                KiBitmap( datasheet_xpm ),
                                _( "View component documents" ) );
        m_mainToolBar->EnableTool( ID_LIBVIEW_VIEWDOC, false );

        if( IsModal() )
        {
            m_mainToolBar->AddSeparator();
            m_mainToolBar->AddTool( ID_LIBVIEW_CMP_EXPORT_TO_SCHEMATIC,
                                    wxEmptyString, KiBitmap( export_xpm ),
                                    _( "Insert component in schematic" ) );
        }

        // after adding the buttons to the toolbar, must call Realize() to
        // reflect the changes
        m_mainToolBar->Realize();
    }

    if( m_libraryName.size() && m_entryName.size() )
    {
        if( PART_LIB* lib = Prj().SchLibs()->FindLibrary( m_libraryName ) )
        {
            part = lib->FindPart( m_entryName );
        }
    }

    /// @todo Move updating the symbol units in the combobox to the symbol select function
    ///       and stop calling this function to update the toolbar.  All of the other toolbar
    ///       updates are handled by wxUpdateUIEvents.
    int parts_count = 1;

    if( part )
        parts_count = std::max( part->GetUnitCount(), 1 );

    m_selpartBox->Clear();

    for( int ii = 0; ii < parts_count; ii++ )
    {
        msg.Printf( _( "Unit %c" ), 'A' + ii );
        m_selpartBox->Append( msg );
    }

    m_selpartBox->SetSelection( m_unit > 0 ? m_unit - 1 : 0 );
    m_selpartBox->Enable( parts_count > 1 );

    m_mainToolBar->Refresh();
}


void LIB_VIEW_FRAME::ReCreateVToolbar()
{
}


// Virtual function
void LIB_VIEW_FRAME::ReCreateMenuBar( void )
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

    AddMenuItem( fileMenu, wxID_EXIT,
                 _( "Cl&ose" ),
                 _( "Close schematic component viewer" ),
                 KiBitmap( exit_xpm ) );

    // View menu
    wxMenu* viewMenu = new wxMenu;

    text = AddHotkeyName( _( "Zoom &In" ), g_Viewlib_Hokeys_Descr,
                          HK_ZOOM_IN, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, KiBitmap( zoom_in_xpm ) );

    text = AddHotkeyName( _( "Zoom &Out" ), g_Viewlib_Hokeys_Descr,
                          HK_ZOOM_OUT, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, KiBitmap( zoom_out_xpm ) );

    text = AddHotkeyName( _( "&Fit on Screen" ), g_Viewlib_Hokeys_Descr,
                          HK_ZOOM_AUTO  );
    AddMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT,
                 KiBitmap( zoom_fit_in_page_xpm ) );

    text = AddHotkeyName( _( "&Redraw" ), g_Viewlib_Hokeys_Descr, HK_ZOOM_REDRAW );
    AddMenuItem( viewMenu, ID_ZOOM_REDRAW, text,
                 HELP_ZOOM_REDRAW, KiBitmap( zoom_redraw_xpm ) );

    viewMenu->AppendSeparator();
    AddMenuItem( viewMenu, ID_LIBVIEW_SHOW_ELECTRICAL_TYPE, _( "&Show Pin Electrical Type" ),
                 wxEmptyString, KiBitmap( pin_show_etype_xpm ), wxITEM_CHECK );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Contents
    AddMenuItem( helpMenu, wxID_HELP,
                 _( "Eeschema &Manual" ),
                 _( "Open Eeschema manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open the \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, ID_HELP_GET_INVOLVED,
                 _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, wxID_ABOUT,
                 _( "&About Eeschema" ),
                 _( "About Eeschema schematic designer" ),
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
