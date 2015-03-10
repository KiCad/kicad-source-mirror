/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file 3d_toolbar.cpp
 */

#include <fctsys.h>

#include <3d_viewer.h>
#include <info3d_visu.h>
#include <menus_helpers.h>
#include <3d_viewer_id.h>


void EDA_3D_FRAME::ReCreateMainToolbar()
{
    if( m_mainToolBar != NULL )
    {
        // Simple update to the list of old files.
        SetToolbars();
        return;
    }

    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                   wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_mainToolBar->AddTool( ID_RELOAD3D_BOARD, wxEmptyString,
                            KiBitmap( import3d_xpm ), _( "Reload board" ) );

#if (defined(__WINDOWS__) || defined(__APPLE__ ) )

    // Does not work properly under linux
    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_TOOL_SCREENCOPY_TOCLIBBOARD, wxEmptyString,
                         KiBitmap( copy_button_xpm ),
                         _( "Copy 3D Image to Clipboard" ) );
#endif

    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_TOOL_SET_VISIBLE_ITEMS, wxEmptyString,
                         KiBitmap( read_setup_xpm ),
                         _( "Set display options, and some layers visibility" ) );
    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ),
                         _( "Zoom in" ) );

    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ),
                         _( "Zoom out" ) );

    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                         KiBitmap( zoom_redraw_xpm ),
                         _( "Redraw view" ) );

    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ),
                         _( "Fit in page" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_ROTATE3D_X_NEG, wxEmptyString,
                         KiBitmap( rotate_neg_x_xpm ),
                         _( "Rotate X <-" ) );

    m_mainToolBar->AddTool( ID_ROTATE3D_X_POS, wxEmptyString,
                         KiBitmap( rotate_pos_x_xpm ),
                         _( "Rotate X ->" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_ROTATE3D_Y_NEG, wxEmptyString,
                         KiBitmap( rotate_neg_y_xpm ),
                         _( "Rotate Y <-" ) );

    m_mainToolBar->AddTool( ID_ROTATE3D_Y_POS, wxEmptyString,
                         KiBitmap( rotate_pos_y_xpm ),
                         _( "Rotate Y ->" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_ROTATE3D_Z_NEG, wxEmptyString,
                         KiBitmap( rotate_neg_z_xpm ),
                         _( "Rotate Z <-" ) );

    m_mainToolBar->AddTool( ID_ROTATE3D_Z_POS, wxEmptyString,
                         KiBitmap( rotate_pos_z_xpm ),
                         _( "Rotate Z ->" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_MOVE3D_LEFT, wxEmptyString, KiBitmap( left_xpm ),
                         _( "Move left" ) );

    m_mainToolBar->AddTool( ID_MOVE3D_RIGHT, wxEmptyString, KiBitmap( right_xpm ),
                         _( "Move right" ) );

    m_mainToolBar->AddTool( ID_MOVE3D_UP, wxEmptyString, KiBitmap( up_xpm ),
                         _( "Move up" ) );

    m_mainToolBar->AddTool( ID_MOVE3D_DOWN, wxEmptyString, KiBitmap( down_xpm ),
                         _( "Move down" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_ORTHO, wxEmptyString, KiBitmap( ortho_xpm ),
                         _( "Enable/Disable orthographic projection" ),
                         wxITEM_CHECK );

    m_mainToolBar->Realize();
}


void EDA_3D_FRAME::CreateMenuBar()
{
    wxMenuBar* menuBar   = new wxMenuBar;
    wxMenu*    fileMenu  = new wxMenu;
    wxMenu*    prefsMenu = new wxMenu;

    menuBar->Append( fileMenu, _( "&File" ) );

    fileMenu->Append( ID_MENU_SCREENCOPY_PNG, _( "Create Image (png format)" ) );
    fileMenu->Append( ID_MENU_SCREENCOPY_JPEG, _( "Create Image (jpeg format)" ) );

#if (defined(__WINDOWS__) || defined(__APPLE__ ) )
    // Does not work properly under linux
    fileMenu->AppendSeparator();
    fileMenu->Append( ID_TOOL_SCREENCOPY_TOCLIBBOARD, _( "Copy 3D Image to Clipboard" ) );
#endif

    fileMenu->AppendSeparator();
    fileMenu->Append( wxID_EXIT, _( "&Exit" ) );

    menuBar->Append( prefsMenu, _( "&Preferences" ) );

    AddMenuItem( prefsMenu, ID_MENU3D_REALISTIC_MODE,
                 _( "Realistic Mode" ),
                 KiBitmap( use_3D_copper_thickness_xpm ), wxITEM_CHECK );

    wxMenu * renderOptionsMenu = new wxMenu;
    AddMenuItem( prefsMenu, renderOptionsMenu, ID_MENU3D_COLOR,
           _( "Render Options" ), KiBitmap( tools_xpm ) );

    AddMenuItem( renderOptionsMenu, ID_MENU3D_FL_RENDER_SHADOWS,
        _( "Render Shadows" ),
        KiBitmap( green_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu, ID_MENU3D_FL_RENDER_SHOW_HOLES_IN_ZONES,
        _( "Show Holes in Zones" ),
        _( "Holes inside a copper layer copper zones are shown, "
            "but the calculation time is longer" ),
        KiBitmap( green_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu, ID_MENU3D_FL_RENDER_TEXTURES,
        _( "Render Textures" ),
        _( "Apply a grid/cloud textures to Board, Solder Mask and Silkscreen" ),
        KiBitmap( green_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu, ID_MENU3D_FL_RENDER_SMOOTH_NORMALS,
        _( "Render Smooth Normals" ),
        KiBitmap( green_xpm ), wxITEM_CHECK );
    
    AddMenuItem( renderOptionsMenu, ID_MENU3D_FL_RENDER_USE_MODEL_NORMALS,
        _( "Use Model Normals" ),
        KiBitmap( green_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu, ID_MENU3D_FL_RENDER_MATERIAL,
        _( "Render Material Properties" ),
        KiBitmap( green_xpm ), wxITEM_CHECK );

    prefsMenu->AppendSeparator();

    wxMenu * backgrounColorMenu = new wxMenu;

    // Add submenu Choose Colors
    AddMenuItem( prefsMenu, backgrounColorMenu, ID_MENU3D_COLOR,
           _( "Choose Colors" ), KiBitmap( palette_xpm ) );

    AddMenuItem( backgrounColorMenu, ID_MENU3D_BGCOLOR_TOP_SELECTION,
                 _( "Background Top Color" ), KiBitmap( palette_xpm ) );

    AddMenuItem( backgrounColorMenu, ID_MENU3D_BGCOLOR_SELECTION,
                 _( "Background Bottom Color" ), KiBitmap( palette_xpm ) );

    AddMenuItem( prefsMenu, ID_MENU3D_AXIS_ONOFF,
                 _( "Show 3D &Axis" ), KiBitmap( axis3d_front_xpm ), wxITEM_CHECK );

    // Creates grid menu
    wxMenu * gridlistMenu = new wxMenu;
    AddMenuItem( prefsMenu, gridlistMenu, ID_MENU3D_GRID,
           _( "3D Grid" ), KiBitmap( grid_xpm ) );
    gridlistMenu->AppendCheckItem( ID_MENU3D_GRID_NOGRID, _( "No 3D Grid" ), wxEmptyString );
    gridlistMenu->AppendCheckItem( ID_MENU3D_GRID_10_MM, _( "3D Grid 10 mm" ), wxEmptyString );
    gridlistMenu->AppendCheckItem( ID_MENU3D_GRID_5_MM, _( "3D Grid 5 mm" ), wxEmptyString );
    gridlistMenu->AppendCheckItem( ID_MENU3D_GRID_2P5_MM, _( "3D Grid 2.5 mm" ), wxEmptyString );
    gridlistMenu->AppendCheckItem( ID_MENU3D_GRID_1_MM, _( "3D Grid 1 mm" ), wxEmptyString );

    // If the grid is on, check the corresponding menuitem showing the grid  size
    if( IsEnabled( FL_GRID ) )
    {
        gridlistMenu->Check( ID_MENU3D_GRID_10_MM, GetPrm3DVisu().m_3D_Grid == 10.0 );
        gridlistMenu->Check( ID_MENU3D_GRID_5_MM, GetPrm3DVisu().m_3D_Grid == 5.0 );
        gridlistMenu->Check( ID_MENU3D_GRID_2P5_MM, GetPrm3DVisu().m_3D_Grid == 2.5 );
        gridlistMenu->Check( ID_MENU3D_GRID_1_MM, GetPrm3DVisu().m_3D_Grid == 1.0 );
    }
    else
        gridlistMenu->Check( ID_MENU3D_GRID_NOGRID, true );

    prefsMenu->AppendSeparator();

    AddMenuItem( prefsMenu, ID_MENU3D_SHOW_BOARD_BODY,
           _( "Show Board Bod&y" ), KiBitmap( use_3D_copper_thickness_xpm ), wxITEM_CHECK );

    AddMenuItem( prefsMenu, ID_MENU3D_USE_COPPER_THICKNESS,
           _( "Show Copper &Thickness" ), KiBitmap( use_3D_copper_thickness_xpm ), wxITEM_CHECK );

    AddMenuItem( prefsMenu, ID_MENU3D_MODULE_ONOFF,
           _( "Show 3D F&ootprints" ), KiBitmap( shape_3d_xpm ), wxITEM_CHECK );

    AddMenuItem( prefsMenu, ID_MENU3D_ZONE_ONOFF,
           _( "Show Zone &Filling" ), KiBitmap( add_zone_xpm ), wxITEM_CHECK );

    prefsMenu->AppendSeparator();

    wxMenu * layersMenu = new wxMenu;
    AddMenuItem( prefsMenu, layersMenu, ID_MENU3D_LAYERS,
           _( "Show &Layers" ), KiBitmap( tools_xpm ) );

    AddMenuItem( layersMenu, ID_MENU3D_ADHESIVE_ONOFF,
           _( "Show &Adhesive Layers" ), KiBitmap( tools_xpm ), wxITEM_CHECK );

    AddMenuItem( layersMenu, ID_MENU3D_SILKSCREEN_ONOFF,
           _( "Show &Silkscreen Layer" ), KiBitmap( add_text_xpm ), wxITEM_CHECK );

    AddMenuItem( layersMenu, ID_MENU3D_SOLDER_MASK_ONOFF,
           _( "Show Solder &Mask Layers" ), KiBitmap( pads_mask_layers_xpm ), wxITEM_CHECK );

    AddMenuItem( layersMenu, ID_MENU3D_SOLDER_PASTE_ONOFF,
           _( "Show Solder &Paste Layers" ), KiBitmap( pads_mask_layers_xpm ), wxITEM_CHECK );

    AddMenuItem( layersMenu, ID_MENU3D_COMMENTS_ONOFF,
           _( "Show &Comments and Drawings Layer" ), KiBitmap( edit_sheet_xpm ), wxITEM_CHECK );

    AddMenuItem( layersMenu, ID_MENU3D_ECO_ONOFF,
           _( "Show &Eco Layers" ), KiBitmap( edit_sheet_xpm ), wxITEM_CHECK );

    SetMenuBar( menuBar );
    SetMenuBarOptionsState();
}

void EDA_3D_FRAME::SetMenuBarOptionsState()
{
    wxMenuBar* menuBar = GetMenuBar();

    if( menuBar == NULL )
        return;

    wxMenuItem* item;
    // Set the state of toggle menus according to the current display options
    item = menuBar->FindItem( ID_MENU3D_REALISTIC_MODE );
    item->Check( GetPrm3DVisu().IsRealisticMode() );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_SHADOWS );
    item->Check( GetPrm3DVisu().GetFlag( FL_RENDER_SHADOWS ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_SHADOWS );
    item->Check( GetPrm3DVisu().GetFlag( FL_RENDER_SHADOWS ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_SHOW_HOLES_IN_ZONES );
    item->Check( GetPrm3DVisu().GetFlag( FL_RENDER_SHOW_HOLES_IN_ZONES ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_TEXTURES );
    item->Check( GetPrm3DVisu().GetFlag( FL_RENDER_TEXTURES ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_SMOOTH_NORMALS );
    item->Check( GetPrm3DVisu().GetFlag( FL_RENDER_SMOOTH_NORMALS ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_USE_MODEL_NORMALS );
    item->Check( GetPrm3DVisu().GetFlag( FL_RENDER_USE_MODEL_NORMALS ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_MATERIAL );
    item->Check( GetPrm3DVisu().GetFlag( FL_RENDER_MATERIAL ) );

    item = menuBar->FindItem( ID_MENU3D_SHOW_BOARD_BODY );
    item->Check( GetPrm3DVisu().GetFlag( FL_SHOW_BOARD_BODY ) );

    item = menuBar->FindItem( ID_MENU3D_USE_COPPER_THICKNESS );
    item->Check( GetPrm3DVisu().GetFlag( FL_USE_COPPER_THICKNESS ) );

    item = menuBar->FindItem( ID_MENU3D_MODULE_ONOFF );
    item->Check( GetPrm3DVisu().GetFlag( FL_MODULE ) );

    item = menuBar->FindItem( ID_MENU3D_ZONE_ONOFF );
    item->Check( GetPrm3DVisu().GetFlag( FL_ZONE ) );

    item = menuBar->FindItem( ID_MENU3D_AXIS_ONOFF );
    item->Check( GetPrm3DVisu().GetFlag( FL_AXIS ) );

    item = menuBar->FindItem( ID_MENU3D_ADHESIVE_ONOFF );
    item->Check( GetPrm3DVisu().GetFlag( FL_ADHESIVE ) );

    item = menuBar->FindItem( ID_MENU3D_SILKSCREEN_ONOFF );
    item->Check( GetPrm3DVisu().GetFlag( FL_SILKSCREEN ) );

    item = menuBar->FindItem( ID_MENU3D_SOLDER_MASK_ONOFF );
    item->Check( GetPrm3DVisu().GetFlag( FL_SOLDERMASK ) );

    item = menuBar->FindItem( ID_MENU3D_SOLDER_PASTE_ONOFF );
    item->Check( GetPrm3DVisu().GetFlag( FL_SOLDERPASTE ) );

    item = menuBar->FindItem( ID_MENU3D_COMMENTS_ONOFF );
    item->Check( GetPrm3DVisu().GetFlag( FL_COMMENTS ) );

    item = menuBar->FindItem( ID_MENU3D_ECO_ONOFF );
    item->Check( GetPrm3DVisu().GetFlag( FL_ECO ));
}

void EDA_3D_FRAME::SetToolbars()
{
}
