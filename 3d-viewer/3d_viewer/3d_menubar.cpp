/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file 3d_menubar.cpp
 */

#include <fctsys.h>

#include <eda_3d_viewer.h>
#include <3d_canvas/cinfo3d_visu.h>
#include <menus_helpers.h>
#include <3d_viewer_id.h>
#include "help_common_strings.h"

void EDA_3D_VIEWER::CreateMenuBar()
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::CreateMenuBar" );

    wxMenuBar* menuBar   = new wxMenuBar;
    wxMenu*    fileMenu  = new wxMenu;
    wxMenu*    editMenu  = new wxMenu;
    wxMenu*    viewMenu  = new wxMenu;
    wxMenu*    prefsMenu = new wxMenu;
    wxMenu*    helpMenu  = new wxMenu;

    menuBar->Append( fileMenu, _( "&File" ) );

    AddMenuItem( fileMenu, ID_MENU_SCREENCOPY_PNG,
                 _( "Export Current View as PNG..." ),
                 KiBitmap( export_xpm ) );

    AddMenuItem( fileMenu, ID_MENU_SCREENCOPY_JPEG,
                 _( "Export Current View as JPEG..." ),
                 KiBitmap( export_xpm ) );

    fileMenu->AppendSeparator();
    AddMenuItem( fileMenu, wxID_EXIT,
                 _( "&Exit" ),
                 KiBitmap( exit_xpm ) );

    menuBar->Append( editMenu, _( "&Edit" ) );

    AddMenuItem( editMenu, ID_TOOL_SCREENCOPY_TOCLIBBOARD,
                 _( "Copy 3D Image" ),
                 KiBitmap( copy_xpm ) );

    menuBar->Append( viewMenu, _( "&View" ) );

    AddMenuItem( viewMenu, ID_ZOOM_IN,
                 _( "Zoom &In" ), HELP_ZOOM_IN,
                 KiBitmap( zoom_in_xpm ) );

    AddMenuItem( viewMenu, ID_ZOOM_OUT,
                 _( "Zoom &Out" ), HELP_ZOOM_OUT,
                 KiBitmap( zoom_out_xpm ) );

    AddMenuItem( viewMenu, ID_ZOOM_PAGE,
                 _( "Zoom to &Fit" ), HELP_ZOOM_FIT,
                 KiBitmap( zoom_fit_in_page_xpm ) );

    AddMenuItem( viewMenu, ID_ZOOM_REDRAW,
                 _( "&Redraw" ), HELP_ZOOM_REDRAW,
                 KiBitmap( zoom_redraw_xpm ) );

    viewMenu->AppendSeparator();

    AddMenuItem( viewMenu, ID_ROTATE3D_X_NEG,
                 _( "Rotate X Clockwise" ),
                 KiBitmap( rotate_neg_x_xpm ) );

    AddMenuItem( viewMenu, ID_ROTATE3D_X_POS,
                 _( "Rotate X Counterclockwise" ),
                 KiBitmap( rotate_pos_x_xpm ) );

    viewMenu->AppendSeparator();

    AddMenuItem( viewMenu, ID_ROTATE3D_Y_NEG,
                 _( "Rotate Y Clockwise" ),
                 KiBitmap( rotate_neg_y_xpm ) );

    AddMenuItem( viewMenu, ID_ROTATE3D_Y_POS,
                 _( "Rotate Y Counterclockwise" ),
                 KiBitmap( rotate_pos_y_xpm ) );

    viewMenu->AppendSeparator();

    AddMenuItem( viewMenu, ID_ROTATE3D_Z_NEG,
                 _( "Rotate Z Clockwise" ),
                 KiBitmap( rotate_neg_z_xpm ) );

    AddMenuItem( viewMenu, ID_ROTATE3D_Z_POS,
                 _( "Rotate Z Counterclockwise" ),
                 KiBitmap( rotate_pos_z_xpm ) );

    viewMenu->AppendSeparator();

    AddMenuItem( viewMenu, ID_MOVE3D_LEFT,
                 _( "Move left" ),
                 KiBitmap( left_xpm ) );

    AddMenuItem( viewMenu, ID_MOVE3D_RIGHT,
                 _( "Move right" ),
                 KiBitmap( right_xpm ) );

    AddMenuItem( viewMenu, ID_MOVE3D_UP,
                 _( "Move up" ),
                 KiBitmap( up_xpm ) );

    AddMenuItem( viewMenu, ID_MOVE3D_DOWN,
                 _( "Move down" ),
                 KiBitmap( down_xpm ) );

    menuBar->Append( prefsMenu, _( "&Preferences" ) );

    AddMenuItem( prefsMenu, ID_TOOL_SET_VISIBLE_ITEMS,
                 _( "Display Options" ),
                 KiBitmap( read_setup_xpm ) );

    prefsMenu->AppendCheckItem( ID_RENDER_CURRENT_VIEW, _( "Raytracing" ) );
    prefsMenu->Check( ID_RENDER_CURRENT_VIEW,
                      m_settings.RenderEngineGet() != RENDER_ENGINE_OPENGL_LEGACY );

    wxMenu * renderOptionsMenu = new wxMenu;
    AddMenuItem( prefsMenu, renderOptionsMenu, ID_MENU3D_FL,
                _( "Render Options" ), KiBitmap( options_3drender_xpm ) );

    wxMenu * materialsList = new wxMenu;
    AddMenuItem( renderOptionsMenu, materialsList, ID_MENU3D_FL_RENDER_MATERIAL,
                _( "Material Properties" ), KiBitmap( color_materials_xpm ) );

    materialsList->AppendRadioItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_NORMAL,
                                    _( "Use All Properties" ),
                                    _( "Use all material properties from each 3D model file" ) );

    materialsList->AppendRadioItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_DIFFUSE_ONLY,
                                    _( "Use Diffuse Only" ),
                                    _( "Use only the diffuse color property from model 3D model file" ) );

    materialsList->AppendRadioItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_CAD_MODE,
                                    _( "CAD Color Style" ),
                                    _( "Use a CAD color style based on the diffuse color of the material" ) );

    // Add specific preferences for OpenGL
    // /////////////////////////////////////////////////////////////////////////
    wxMenu * renderOptionsMenu_OPENGL = new wxMenu;

    AddMenuItem( renderOptionsMenu, renderOptionsMenu_OPENGL, ID_MENU3D_FL_OPENGL,
                _( "OpenGL Options" ), KiBitmap( tools_xpm ) );

    AddMenuItem( renderOptionsMenu_OPENGL, ID_MENU3D_FL_OPENGL_RENDER_COPPER_THICKNESS,
                _( "Show Copper Thickness" ),
                _( "Shows the copper thickness on copper layers (slower loading)"),
                KiBitmap( use_3D_copper_thickness_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu_OPENGL, ID_MENU3D_FL_OPENGL_RENDER_SHOW_MODEL_BBOX,
                _( "Show Model Bounding Boxes" ),
                KiBitmap( ortho_xpm ), wxITEM_CHECK );


    // Add specific preferences for Raytracing
    // /////////////////////////////////////////////////////////////////////////
    wxMenu * renderOptionsMenu_RAYTRACING = new wxMenu;
    AddMenuItem( renderOptionsMenu, renderOptionsMenu_RAYTRACING, ID_MENU3D_FL_RAYTRACING,
                 _( "Raytracing Options" ), KiBitmap( tools_xpm ) );

    AddMenuItem( renderOptionsMenu_RAYTRACING, ID_MENU3D_FL_RAYTRACING_RENDER_SHADOWS,
                 _( "Render Shadows" ),
                 KiBitmap( green_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu_RAYTRACING, ID_MENU3D_FL_RAYTRACING_PROCEDURAL_TEXTURES,
                 _( "Procedural Textures" ),
                 _( "Apply procedural textures to materials (slow)"),
                 KiBitmap( green_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu_RAYTRACING, ID_MENU3D_FL_RAYTRACING_BACKFLOOR,
                 _( "Add Floor" ),
                 _( "Adds a floor plane below the board (slow)"),
                 KiBitmap( green_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu_RAYTRACING, ID_MENU3D_FL_RAYTRACING_REFRACTIONS,
                 _( "Refractions" ),
                 _( "Render materials with refractions properties on final render (slow)"),
                 KiBitmap( green_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu_RAYTRACING, ID_MENU3D_FL_RAYTRACING_REFLECTIONS,
                _( "Reflections" ),
                _( "Render materials with reflections properties on final render (slow)"),
                KiBitmap( green_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu_RAYTRACING, ID_MENU3D_FL_RAYTRACING_ANTI_ALIASING,
                 _( "Anti-aliasing" ),
                 _( "Render with improved quality on final render (slow)"),
                 KiBitmap( green_xpm ), wxITEM_CHECK );

    AddMenuItem( renderOptionsMenu_RAYTRACING, ID_MENU3D_FL_RAYTRACING_POST_PROCESSING,
                 _( "Post-processing" ),
                 _( "Apply Screen Space Ambient Occlusion and Global Illumination reflections on final render (slow)"),
                 KiBitmap( green_xpm ), wxITEM_CHECK );

    prefsMenu->AppendSeparator();


    // Colors, axis and grid elements
    // /////////////////////////////////////////////////////////////////////////

    // Add submenu set Colors
    wxMenu * setColorMenu = new wxMenu;
    AddMenuItem( prefsMenu, setColorMenu, ID_MENU3D_COLOR,
                 _( "Choose Colors" ), KiBitmap( palette_xpm ) );

    wxMenu * setBgColorMenu = new wxMenu;
    AddMenuItem( setColorMenu, setBgColorMenu, ID_MENU3D_BGCOLOR,
                 _( "Background Color" ), KiBitmap( palette_xpm ) );

    AddMenuItem( setBgColorMenu, ID_MENU3D_BGCOLOR_TOP_SELECTION,
                 _( "Background Top Color..." ), KiBitmap( setcolor_3d_bg_xpm ) );

    AddMenuItem( setBgColorMenu, ID_MENU3D_BGCOLOR_BOTTOM_SELECTION,
                 _( "Background Bottom Color..." ), KiBitmap( setcolor_3d_bg_xpm ) );

    AddMenuItem( setColorMenu, ID_MENU3D_SILKSCREEN_COLOR_SELECTION,
                 _( "Silkscreen Color..." ), KiBitmap( setcolor_silkscreen_xpm ) );

    AddMenuItem( setColorMenu, ID_MENU3D_SOLDERMASK_COLOR_SELECTION,
                 _( "Solder Mask Color..." ), KiBitmap( setcolor_soldermask_xpm ) );

    AddMenuItem( setColorMenu, ID_MENU3D_SOLDERPASTE_COLOR_SELECTION,
                 _( "Solder Paste Color..." ), KiBitmap( setcolor_solderpaste_xpm ) );

    AddMenuItem( setColorMenu, ID_MENU3D_COPPER_COLOR_SELECTION,
                 _( "Copper/Surface Finish Color..." ), KiBitmap( setcolor_copper_xpm ) );

    AddMenuItem( setColorMenu, ID_MENU3D_PCB_BODY_COLOR_SELECTION,
                 _( "Board Body Color..." ), KiBitmap( setcolor_board_body_xpm ) );

    AddMenuItem( prefsMenu, ID_MENU3D_AXIS_ONOFF,
                 _( "Show 3D &Axis" ), KiBitmap( axis3d_front_xpm ), wxITEM_CHECK );


    // Creates grid menu
    // /////////////////////////////////////////////////////////////////////////

    wxMenu * gridlistMenu = new wxMenu;
    AddMenuItem( prefsMenu, gridlistMenu, ID_MENU3D_GRID,
                _( "3D Grid" ), KiBitmap( grid_xpm ) );
    gridlistMenu->AppendRadioItem( ID_MENU3D_GRID_NOGRID, _( "No 3D Grid" ),     wxEmptyString );
    gridlistMenu->AppendRadioItem( ID_MENU3D_GRID_10_MM,  _( "3D Grid 10 mm" ),  wxEmptyString );
    gridlistMenu->AppendRadioItem( ID_MENU3D_GRID_5_MM,   _( "3D Grid 5 mm" ),   wxEmptyString );
    gridlistMenu->AppendRadioItem( ID_MENU3D_GRID_2P5_MM, _( "3D Grid 2.5 mm" ), wxEmptyString );
    gridlistMenu->AppendRadioItem( ID_MENU3D_GRID_1_MM,   _( "3D Grid 1 mm" ),   wxEmptyString );

    // If the grid is on, check the corresponding menuitem showing the grid  size
    if( m_settings.GridGet() != GRID3D_NONE )
    {
        gridlistMenu->Check( ID_MENU3D_GRID_10_MM,  m_settings.GridGet() == GRID3D_10MM );
        gridlistMenu->Check( ID_MENU3D_GRID_5_MM,   m_settings.GridGet() == GRID3D_5MM );
        gridlistMenu->Check( ID_MENU3D_GRID_2P5_MM, m_settings.GridGet() == GRID3D_2P5MM );
        gridlistMenu->Check( ID_MENU3D_GRID_1_MM,   m_settings.GridGet() == GRID3D_1MM );
    }
    else
        gridlistMenu->Check( ID_MENU3D_GRID_NOGRID, true );

    // Reset options
    // /////////////////////////////////////////////////////////////////////////
    prefsMenu->AppendSeparator();

    AddMenuItem( prefsMenu, ID_MENU3D_RESET_DEFAULTS,
                 _( "Reset to Default Settings" ),
                 KiBitmap( tools_xpm ) );

    // Help menu
    // /////////////////////////////////////////////////////////////////////////
    menuBar->Append( helpMenu, _( "&Help" ) );

    AddMenuItem( helpMenu, wxID_HELP,
                 _( "Pcbnew &Manual" ),
                 _( "Open Pcbnew Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    wxString text = AddHotkeyName( _( "&List Hotkeys..." ), GetHotkeyConfig(), HK_HELP );
    AddMenuItem( helpMenu, ID_MENU3D_HELP_HOTKEY_SHOW_CURRENT_LIST,
                 text,
                 _( "Displays the current hotkeys list and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    helpMenu->AppendSeparator();

    AddMenuItem( helpMenu, ID_HELP_GET_INVOLVED,
                 _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    helpMenu->AppendSeparator();

    AddMenuItem( helpMenu, wxID_ABOUT,
                 _( "&About KiCad" ),
                 _( "Display KiCad About dialog" ),
                 KiBitmap( about_xpm ) );

    SetMenuBar( menuBar );
}
