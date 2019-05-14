/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <dialog_helpers.h>
#include <macros.h>
#include <menus_helpers.h>

#include "class_library.h"
#include "eeschema_id.h"
#include "general.h"
#include "help_common_strings.h"
#include "ee_hotkeys.h"
#include "viewlib_frame.h"
#include <symbol_lib_table.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/lib_control.h>

void LIB_VIEW_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR,
                                            wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    ACTION_TOOLBAR* toolbar = static_cast<ACTION_TOOLBAR*>( m_mainToolBar );
    wxString msg;

    m_mainToolBar->AddTool( ID_LIBVIEW_SELECT_PART, wxEmptyString,
            KiScaledBitmap( add_component_xpm, this ),
            _( "Select symbol to browse" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_LIBVIEW_PREVIOUS, wxEmptyString,
            KiScaledBitmap( lib_previous_xpm, this ),
            _( "Display previous symbol" ) );

    m_mainToolBar->AddTool( ID_LIBVIEW_NEXT, wxEmptyString,
            KiScaledBitmap( lib_next_xpm, this ),
            _( "Display next symbol" ) );

    toolbar->AddSeparator();
    toolbar->Add( ACTIONS::zoomRedraw );
    toolbar->Add( ACTIONS::zoomInCenter );
    toolbar->Add( ACTIONS::zoomOutCenter );
    toolbar->Add( ACTIONS::zoomFitScreen );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, wxEmptyString,
            KiScaledBitmap( morgan1_xpm, this ),
            _( "Show as \"De Morgan\" normal symbol" ),
            wxITEM_CHECK );

    m_mainToolBar->AddTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, wxEmptyString,
            KiScaledBitmap( morgan2_xpm, this ),
            _( "Show as \"De Morgan\" convert symbol" ),
            wxITEM_CHECK );

    KiScaledSeparator( m_mainToolBar, this );

    m_unitChoice = new wxChoice( m_mainToolBar, ID_LIBVIEW_SELECT_PART_NUMBER,
            wxDefaultPosition, wxSize( 150, -1 ) );
    m_mainToolBar->AddControl( m_unitChoice );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_LIBVIEW_VIEWDOC, wxEmptyString,
            KiScaledBitmap( datasheet_xpm, this ),
            _( "View symbol documents" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_ADD_PART_TO_SCHEMATIC, wxEmptyString,
            KiScaledBitmap( export_xpm, this ),
            _( "Add symbol to schematic" ) );

    // after adding the buttons to the toolbar, must call Realize() to
    // reflect the changes
    m_mainToolBar->Realize();

    m_mainToolBar->Refresh();
}


void LIB_VIEW_FRAME::ReCreateVToolbar()
{
}


// Virtual function
void LIB_VIEW_FRAME::ReCreateMenuBar()
{
    LIB_CONTROL* libControl = m_toolManager->GetTool<LIB_CONTROL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();
    wxString   text;

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    AddMenuItem( fileMenu, wxID_EXIT,
                 _( "Cl&ose" ),
                 _( "Close schematic symbol viewer" ),
                 KiBitmap( exit_xpm ) );

    // View menu
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, libControl );

    auto gridShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsGridVisible();
    };

    viewMenu->AddItem( ACTIONS::zoomInCenter,             EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,            EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,            EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,               EE_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,          gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,           EE_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( EE_ACTIONS::showElectricalTypes,   EE_CONDITIONS::ShowAlways );

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

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
