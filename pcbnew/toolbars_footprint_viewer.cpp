/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <macros.h>
#include <bitmaps.h>
#include <tool/action_toolbar.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <tools/selection_tool.h>
#include <tools/pcb_actions.h>
#include "footprint_viewer_frame.h"
#include "pcbnew_id.h"


void FOOTPRINT_VIEWER_FRAME::ReCreateHToolbar()
{
    // Note:
    // To rebuild the aui toolbar, the more easy way is to clear ( calling m_mainToolBar.Clear() )
    // all wxAuiToolBarItems.
    // However the wxAuiToolBarItems are not the owners of controls managed by
    // them ( m_zoomSelectBox and m_gridSelectBox ), and therefore do not delete them
    // So we do not recreate them after clearing the tools.

    wxString msg;

    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT  );

    // Set up toolbar
    m_mainToolBar->AddTool( ID_MODVIEW_OPTIONS, wxEmptyString,
                            KiScaledBitmap( config_xpm, this ),
                            _( "Display options" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_MODVIEW_PREVIOUS, wxEmptyString,
                            KiScaledBitmap( lib_previous_xpm, this ),
                            _( "Display previous footprint" ) );
    m_mainToolBar->AddTool( ID_MODVIEW_NEXT, wxEmptyString,
                            KiScaledBitmap( lib_next_xpm, this ),
                            _( "Display next footprint" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( ACTIONS::show3DViewer );
    m_mainToolBar->AddTool( ID_ADD_FOOTPRINT_TO_BOARD, wxEmptyString,
                            KiScaledBitmap( export_xpm, this ),
                            _( "Insert footprint in board" ) );

    KiScaledSeparator( m_mainToolBar, this );

    // Grid selection choice box.
    if( m_gridSelectBox == nullptr )
        m_gridSelectBox = new wxChoice( m_mainToolBar, ID_ON_GRID_SELECT,
                                    wxDefaultPosition, wxDefaultSize, 0, NULL );

    UpdateGridSelectBox();
    m_mainToolBar->AddControl( m_gridSelectBox );

    KiScaledSeparator( m_mainToolBar, this );

    // Zoom selection choice box.
    if( m_zoomSelectBox == nullptr )
        m_zoomSelectBox = new wxChoice( m_mainToolBar, ID_ON_ZOOM_SELECT,
                                    wxDefaultPosition, wxDefaultSize, 0, NULL );

    updateZoomSelectBox();
    m_mainToolBar->AddControl( m_zoomSelectBox );

    // after adding the buttons to the toolbar, must call Realize() to
    // reflect the changes
    m_mainToolBar->Realize();
    m_mainToolBar->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateVToolbar()
{
}


void FOOTPRINT_VIEWER_FRAME::ReCreateMenuBar()
{
    SELECTION_TOOL* selTool = m_toolManager->GetTool<SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();

    //----- File menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU* fileMenu = new CONDITIONAL_MENU( false, selTool );

    fileMenu->AddClose( _( "Footprint Viewer" ) );

    fileMenu->Resolve();

    //----- View menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, selTool );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,     SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,    SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,    SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,       SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::show3DViewer,     SELECTION_CONDITIONS::ShowAlways );

    viewMenu->Resolve();

    //----- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
