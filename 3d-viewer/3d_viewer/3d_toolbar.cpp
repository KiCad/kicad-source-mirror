/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_3d_viewer.h>
#include <3d_canvas/cinfo3d_visu.h>
#include <menus_helpers.h>
#include <3d_viewer_id.h>


void EDA_3D_VIEWER::ReCreateMainToolbar()
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::ReCreateMainToolbar" ) );

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

    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_TOOL_SCREENCOPY_TOCLIBBOARD, wxEmptyString,
                            KiBitmap( copy_button_xpm ),
                            _( "Copy 3D image to clipboard" ) );

    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_RENDER_CURRENT_VIEW, wxEmptyString,
                            KiBitmap( render_mode_xpm ),
                            _( "Render current view using Raytracing" ) );

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


void EDA_3D_VIEWER::CreateMenuBar()
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::CreateMenuBar" ) );

    wxMenuBar* menuBar   = new wxMenuBar;
    wxMenu*    fileMenu  = new wxMenu;
    wxMenu*    prefsMenu = new wxMenu;
    wxMenu*    helpMenu  = new wxMenu;

    menuBar->Append( fileMenu, _( "&File" ) );

    AddMenuItem( fileMenu, ID_MENU_SCREENCOPY_PNG,
                  _( "Create Image (png format)" ),
                  KiBitmap( export_xpm ) );

    AddMenuItem( fileMenu, ID_MENU_SCREENCOPY_JPEG,
                 _( "Create Image (jpeg format)" ),
                 KiBitmap( export_xpm ) );

    fileMenu->AppendSeparator();
    AddMenuItem( fileMenu, ID_TOOL_SCREENCOPY_TOCLIBBOARD,
                 _( "Copy 3D Image to Clipboard" ),
                 KiBitmap( copy_button_xpm ) );

    fileMenu->AppendSeparator();
    AddMenuItem( fileMenu, wxID_EXIT,
                 _( "&Exit" ),
                 KiBitmap( exit_xpm ) );

    menuBar->Append( prefsMenu, _( "&Preferences" ) );

    AddMenuItem( prefsMenu, ID_MENU3D_MOUSEWHEEL_PANNING,
                _( "Use Touchpad to Pan" ),
                KiBitmap( tools_xpm ), wxITEM_CHECK );

    prefsMenu->AppendSeparator();

    AddMenuItem( prefsMenu, ID_MENU3D_REALISTIC_MODE,
                 _( "Realistic Mode" ),
                 KiBitmap( use_3D_copper_thickness_xpm ), wxITEM_CHECK );

    wxMenu * renderEngineList = new wxMenu;
    AddMenuItem( prefsMenu, renderEngineList, ID_MENU3D_ENGINE,
                _( "Render Engine" ), KiBitmap( render_mode_xpm ) );

    renderEngineList->AppendRadioItem( ID_MENU3D_ENGINE_OPENGL_LEGACY,
                                       _( "OpenGL" ),
                                       wxEmptyString );

    renderEngineList->AppendRadioItem( ID_MENU3D_ENGINE_RAYTRACING,
                                       _( "Raytracing" ),
                                       wxEmptyString );

    renderEngineList->Check( ID_MENU3D_ENGINE_OPENGL_LEGACY,
                             m_settings.RenderEngineGet() == RENDER_ENGINE_OPENGL_LEGACY );

    renderEngineList->Check( ID_MENU3D_ENGINE_RAYTRACING,
                             m_settings.RenderEngineGet() == RENDER_ENGINE_RAYTRACING );

    wxMenu * renderOptionsMenu = new wxMenu;
    AddMenuItem( prefsMenu, renderOptionsMenu, ID_MENU3D_FL,
                _( "Render Options" ), KiBitmap( options_3drender_xpm ) );

    AddMenuItem( renderOptionsMenu, ID_MENU3D_FL_RENDER_SHOW_HOLES_IN_ZONES,
                _( "Show Holes in Zones" ),
                _( "Holes inside a copper layer copper zones are shown, "
                   "but the calculation time is longer" ),
                KiBitmap( green_xpm ), wxITEM_CHECK );

    wxMenu * materialsList = new wxMenu;
    AddMenuItem( renderOptionsMenu, materialsList, ID_MENU3D_FL_RENDER_MATERIAL,
                _( "Material Properties" ), KiBitmap( color_materials_xpm ) );

    materialsList->AppendRadioItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_NORMAL,
                                    _( "Use all properties" ),
                                    _( "Use all material properties from each 3D model file" ) );

    materialsList->AppendRadioItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_DIFFUSE_ONLY,
                                    _( "Use diffuse only" ),
                                    _( "Use only the diffuse color property from model 3D model file " ) );

    materialsList->AppendRadioItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_CAD_MODE,
                                    _( "CAD color style" ),
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
                KiBitmap( green_xpm ), wxITEM_CHECK );


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
                _( "Add floor" ),
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
                _( "Render with improoved quality on final render (slow)"),
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
                 _( "Background Top Color" ), KiBitmap( setcolor_3d_bg_xpm ) );

    AddMenuItem( setBgColorMenu, ID_MENU3D_BGCOLOR_BOTTOM_SELECTION,
                 _( "Background Bottom Color" ), KiBitmap( setcolor_3d_bg_xpm ) );

    AddMenuItem( setColorMenu, ID_MENU3D_SILKSCREEN_COLOR_SELECTION,
                 _( "Silkscreen Color" ), KiBitmap( setcolor_silkscreen_xpm ) );

    AddMenuItem( setColorMenu, ID_MENU3D_SOLDERMASK_COLOR_SELECTION,
                 _( "Solder Mask Color" ), KiBitmap( setcolor_soldermask_xpm ) );

    AddMenuItem( setColorMenu, ID_MENU3D_SOLDERPASTE_COLOR_SELECTION,
                 _( "Solder Paste Color" ), KiBitmap( setcolor_solderpaste_xpm ) );

    AddMenuItem( setColorMenu, ID_MENU3D_COPPER_COLOR_SELECTION,
                 _( "Copper/Surface Finish Color" ), KiBitmap( setcolor_copper_xpm ) );

    AddMenuItem( setColorMenu, ID_MENU3D_PCB_BODY_COLOR_SELECTION,
                 _( "Board Body Color" ), KiBitmap( setcolor_board_body_xpm ) );

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


    // Display elements options
    // /////////////////////////////////////////////////////////////////////////
    prefsMenu->AppendSeparator();

    AddMenuItem( prefsMenu, ID_MENU3D_SHOW_BOARD_BODY,
                _( "Show Board Bod&y" ), KiBitmap( use_3D_copper_thickness_xpm ), wxITEM_CHECK );

    AddMenuItem( prefsMenu, ID_MENU3D_ZONE_ONOFF,
                _( "Show Zone &Filling" ), KiBitmap( add_zone_xpm ), wxITEM_CHECK );

    wxMenu * moduleAttributes = new wxMenu;
    AddMenuItem( prefsMenu, moduleAttributes, ID_MENU3D_MODULE_ONOFF,
                _( "Show 3D M&odels" ), KiBitmap( shape_3d_xpm ) );
    moduleAttributes->AppendCheckItem( ID_MENU3D_MODULE_ONOFF_ATTRIBUTES_NORMAL,
                                       _( "Normal" ),
                                       _( "Footprint Properties -> Attributes -> Normal (eg: THT parts)" ) );

    moduleAttributes->AppendCheckItem( ID_MENU3D_MODULE_ONOFF_ATTRIBUTES_NORMAL_INSERT,
                                       _( "Normal+Insert" ),
                                       _( "Footprint Properties -> Attributes -> Normal+Insert (eg: SMD parts)" ) );

    moduleAttributes->AppendCheckItem( ID_MENU3D_MODULE_ONOFF_ATTRIBUTES_VIRTUAL,
                                       _( "Virtual" ),
                                       _( "Footprint Properties -> Attributes -> Virtual (eg: edge connectors, test points, mechanical parts)" ) );

    // Layer options
    // /////////////////////////////////////////////////////////////////////////
    prefsMenu->AppendSeparator();

    wxMenu * layersMenu = new wxMenu;
    AddMenuItem( prefsMenu, layersMenu, ID_MENU3D_LAYERS,
                _( "Show &Layers" ), KiBitmap( tools_xpm ) );

    AddMenuItem( layersMenu, ID_MENU3D_ADHESIVE_ONOFF,
                _( "Show &Adhesive Layers" ), KiBitmap( tools_xpm ), wxITEM_CHECK );

    AddMenuItem( layersMenu, ID_MENU3D_SILKSCREEN_ONOFF,
                _( "Show &Silkscreen Layers" ), KiBitmap( add_text_xpm ), wxITEM_CHECK );

    AddMenuItem( layersMenu, ID_MENU3D_SOLDER_MASK_ONOFF,
                _( "Show Solder &Mask Layers" ), KiBitmap( pads_mask_layers_xpm ), wxITEM_CHECK );

    AddMenuItem( layersMenu, ID_MENU3D_SOLDER_PASTE_ONOFF,
                _( "Show Solder &Paste Layers" ), KiBitmap( pads_mask_layers_xpm ), wxITEM_CHECK );

    // Other layers are not "board" layers, and are not shown in realistic mode
    // These menus will be disabled in in realistic mode
    AddMenuItem( layersMenu, ID_MENU3D_COMMENTS_ONOFF,
                _( "Show &Comments and Drawings Layers" ), KiBitmap( edit_sheet_xpm ), wxITEM_CHECK );

    AddMenuItem( layersMenu, ID_MENU3D_ECO_ONOFF,
                _( "Show &Eco Layers" ), KiBitmap( edit_sheet_xpm ), wxITEM_CHECK );

    // Reset options
    // /////////////////////////////////////////////////////////////////////////
    prefsMenu->AppendSeparator();

    AddMenuItem( prefsMenu, ID_MENU3D_RESET_DEFAULTS,
                _( "Reset to default settings" ),
                KiBitmap( tools_xpm ) );

    // Help menu
    // /////////////////////////////////////////////////////////////////////////
    menuBar->Append( helpMenu, _( "&Help" ) );

    AddMenuItem( helpMenu, ID_MENU3D_HELP_HOTKEY_SHOW_CURRENT_LIST,
                 _( "&List Hotkeys" ),
                 _( "Displays the current hotkeys list and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    SetMenuBar( menuBar );
    SetMenuBarOptionsState();
}


void EDA_3D_VIEWER::SetMenuBarOptionsState()
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::SetMenuBarOptionsState" ) );

    wxMenuBar* menuBar = GetMenuBar();

    if( menuBar == NULL )
    {
        wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::SetMenuBarOptionsState menuBar == NULL" ) );

        return;
    }

    wxMenuItem* item;
    // Set the state of toggle menus according to the current display options
    item = menuBar->FindItem( ID_MENU3D_MOUSEWHEEL_PANNING );
    item->Check( m_settings.GetFlag( FL_MOUSEWHEEL_PANNING ) );


    item = menuBar->FindItem( ID_MENU3D_ENGINE_OPENGL_LEGACY );
    item->Check( m_settings.RenderEngineGet() == RENDER_ENGINE_OPENGL_LEGACY );

    item = menuBar->FindItem( ID_MENU3D_ENGINE_RAYTRACING );
    item->Check( m_settings.RenderEngineGet() == RENDER_ENGINE_RAYTRACING );

    item = menuBar->FindItem( ID_MENU3D_REALISTIC_MODE );
    item->Check(  m_settings.GetFlag( FL_USE_REALISTIC_MODE ) );
    item = menuBar->FindItem( ID_MENU3D_COMMENTS_ONOFF );
    item->Enable( !m_settings.GetFlag( FL_USE_REALISTIC_MODE ) );
    item = menuBar->FindItem( ID_MENU3D_ECO_ONOFF );
    item->Enable( !m_settings.GetFlag( FL_USE_REALISTIC_MODE ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_SHOW_HOLES_IN_ZONES );
    item->Check( m_settings.GetFlag( FL_RENDER_SHOW_HOLES_IN_ZONES ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_NORMAL );
    item->Check( m_settings.MaterialModeGet() == MATERIAL_MODE_NORMAL );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_DIFFUSE_ONLY );
    item->Check( m_settings.MaterialModeGet() == MATERIAL_MODE_DIFFUSE_ONLY );

    item = menuBar->FindItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_CAD_MODE );
    item->Check( m_settings.MaterialModeGet() == MATERIAL_MODE_CAD_MODE );

    // OpenGL
    item = menuBar->FindItem( ID_MENU3D_FL_OPENGL_RENDER_COPPER_THICKNESS );
    item->Check( m_settings.GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) );

    item = menuBar->FindItem( ID_MENU3D_FL_OPENGL_RENDER_SHOW_MODEL_BBOX );
    item->Check( m_settings.GetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX ) );

    // Raytracing
    item = menuBar->FindItem( ID_MENU3D_FL_RAYTRACING_RENDER_SHADOWS );
    item->Check( m_settings.GetFlag( FL_RENDER_RAYTRACING_SHADOWS ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RAYTRACING_BACKFLOOR );
    item->Check( m_settings.GetFlag( FL_RENDER_RAYTRACING_BACKFLOOR ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RAYTRACING_REFRACTIONS );
    item->Check( m_settings.GetFlag( FL_RENDER_RAYTRACING_REFRACTIONS ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RAYTRACING_REFLECTIONS );
    item->Check( m_settings.GetFlag( FL_RENDER_RAYTRACING_REFLECTIONS ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RAYTRACING_POST_PROCESSING );
    item->Check( m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RAYTRACING_ANTI_ALIASING );
    item->Check( m_settings.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) );

    item = menuBar->FindItem( ID_MENU3D_FL_RAYTRACING_PROCEDURAL_TEXTURES );
    item->Check( m_settings.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES ) );


    item = menuBar->FindItem( ID_MENU3D_SHOW_BOARD_BODY );
    item->Check( m_settings.GetFlag( FL_SHOW_BOARD_BODY ) );

    item = menuBar->FindItem( ID_MENU3D_MODULE_ONOFF_ATTRIBUTES_NORMAL );
    item->Check( m_settings.GetFlag( FL_MODULE_ATTRIBUTES_NORMAL ) );

    item = menuBar->FindItem( ID_MENU3D_MODULE_ONOFF_ATTRIBUTES_NORMAL_INSERT );
    item->Check( m_settings.GetFlag( FL_MODULE_ATTRIBUTES_NORMAL_INSERT ) );

    item = menuBar->FindItem( ID_MENU3D_MODULE_ONOFF_ATTRIBUTES_VIRTUAL );
    item->Check( m_settings.GetFlag( FL_MODULE_ATTRIBUTES_VIRTUAL ) );

    item = menuBar->FindItem( ID_MENU3D_ZONE_ONOFF );
    item->Check( m_settings.GetFlag( FL_ZONE ) );

    item = menuBar->FindItem( ID_MENU3D_AXIS_ONOFF );
    item->Check( m_settings.GetFlag( FL_AXIS ) );

    item = menuBar->FindItem( ID_MENU3D_ADHESIVE_ONOFF );
    item->Check( m_settings.GetFlag( FL_ADHESIVE ) );

    item = menuBar->FindItem( ID_MENU3D_SILKSCREEN_ONOFF );
    item->Check( m_settings.GetFlag( FL_SILKSCREEN ) );

    item = menuBar->FindItem( ID_MENU3D_SOLDER_MASK_ONOFF );
    item->Check( m_settings.GetFlag( FL_SOLDERMASK ) );

    item = menuBar->FindItem( ID_MENU3D_SOLDER_PASTE_ONOFF );
    item->Check( m_settings.GetFlag( FL_SOLDERPASTE ) );

    item = menuBar->FindItem( ID_MENU3D_COMMENTS_ONOFF );
    item->Check( m_settings.GetFlag( FL_COMMENTS ) );

    item = menuBar->FindItem( ID_MENU3D_ECO_ONOFF );
    item->Check( m_settings.GetFlag( FL_ECO ));
}


void EDA_3D_VIEWER::SetToolbars()
{
}
