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

#include <fctsys.h>
#include <tool/conditional_menu.h>
#include <eda_3d_viewer.h>
#include <3d_canvas/cinfo3d_visu.h>
#include <menus_helpers.h>
#include <3d_viewer_id.h>
#include <3d_actions.h>
#include <tool/tool_manager.h>
#include <tool/conditional_menu.h>
#include <tool/common_control.h>
#include "help_common_strings.h"


TOOL_ACTION EDA_3D_ACTIONS::pivotCenter( "3DViewer.Control.pivotCenter", AS_GLOBAL, 
                                         ' ', "",  "Center pivot rotation (Middle mouse click)" );

TOOL_ACTION EDA_3D_ACTIONS::moveLeft( "3DViewer.Control.moveLeft", AS_GLOBAL,
                                      WXK_LEFT, "", "Move board Left" );

TOOL_ACTION EDA_3D_ACTIONS::moveRight( "3DViewer.Control.moveRight", AS_GLOBAL,
                                       WXK_RIGHT, "", "Move board Right" );

TOOL_ACTION EDA_3D_ACTIONS::moveUp( "3DViewer.Control.moveUp", AS_GLOBAL,
                                    WXK_UP, "", "Move board Up" );

TOOL_ACTION EDA_3D_ACTIONS::moveDown( "3DViewer.Control.moveDown", AS_GLOBAL,
                                      WXK_DOWN, "", "Move board Down" );

TOOL_ACTION EDA_3D_ACTIONS::homeView( "3DViewer.Control.homeView", AS_GLOBAL,
                                      WXK_HOME, "", "Home view" );

TOOL_ACTION EDA_3D_ACTIONS::resetView( "3DViewer.Control.resetView", AS_GLOBAL,
                                       'R', "", "Reset view" );

TOOL_ACTION EDA_3D_ACTIONS::viewFront( "3DViewer.Control.viewFront", AS_GLOBAL,
                                       'Y', "", "View Front" );

TOOL_ACTION EDA_3D_ACTIONS::viewBack( "3DViewer.Control.viewBack", AS_GLOBAL,
                                      MD_SHIFT + 'Y', "", "View Back" );

TOOL_ACTION EDA_3D_ACTIONS::viewLeft( "3DViewer.Control.viewLeft", AS_GLOBAL,
                                      MD_SHIFT + 'X', "", "View Left" );

TOOL_ACTION EDA_3D_ACTIONS::viewRight( "3DViewer.Control.viewRight", AS_GLOBAL,
                                       'X', "", "View Right" );

TOOL_ACTION EDA_3D_ACTIONS::viewTop( "3DViewer.Control.viewTop", AS_GLOBAL,
                                     'Z', "", "View Top" );

TOOL_ACTION EDA_3D_ACTIONS::viewBottom( "3DViewer.Control.viewBottom", AS_GLOBAL,
                                        MD_SHIFT + 'Z', "", "View Bottom" );

TOOL_ACTION EDA_3D_ACTIONS::rotate45axisZ( "3DViewer.Control.rotate45axisZ", AS_GLOBAL,
                                           WXK_TAB, "", "Rotate 45 degrees over Z axis" );

TOOL_ACTION EDA_3D_ACTIONS::zoomIn( "3DViewer.Control.zoomIn", AS_GLOBAL,
                                    WXK_F1, "", "Zoom in " );

TOOL_ACTION EDA_3D_ACTIONS::zoomOut( "3DViewer.Control.zoomOut", AS_GLOBAL,
                                     WXK_F2, "", "Zoom out" );

TOOL_ACTION EDA_3D_ACTIONS::attributesTHT( "3DViewer.Control.attributesTHT", AS_GLOBAL,
                                           'T', "", "Toggle 3D models with type Through Hole" );

TOOL_ACTION EDA_3D_ACTIONS::attributesSMD( "3DViewer.Control.attributesSMD", AS_GLOBAL,
                                           'S', "", "Toggle 3D models with type Surface Mount" );

TOOL_ACTION EDA_3D_ACTIONS::attributesVirtual( "3DViewer.Control.attributesVirtual", AS_GLOBAL,
                                               'V', "", "Toggle 3D models with type Virtual" );


void EDA_3D_VIEWER::CreateMenuBar()
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::CreateMenuBar" );

    COMMON_CONTROL* tool = m_toolManager->GetTool<COMMON_CONTROL>();
    wxMenuBar* menuBar   = new wxMenuBar;


    //-- File menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU*   fileMenu = new CONDITIONAL_MENU( false, tool );

    fileMenu->AddItem( ID_MENU_SCREENCOPY_PNG, _( "Export Current View as PNG..." ), "",
                       export_xpm,                     SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_MENU_SCREENCOPY_JPEG, _( "Export Current View as JPEG..." ), "",
                       export_xpm,                     SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    // Don't use ACTIONS::quit; wxWidgets moves this on OSX and expects to find it via wxID_EXIT
    fileMenu->AddItem( wxID_EXIT, _( "Quit" ), "", exit_xpm, SELECTION_CONDITIONS::ShowAlways );

    //-- Edit menu -------------------------------------------------------
    //
    CONDITIONAL_MENU* editMenu = new CONDITIONAL_MENU( false, tool );

    editMenu->AddItem( ID_TOOL_SCREENCOPY_TOCLIBBOARD, _( "Copy 3D Image" ), "",
                       copy_xpm,                       SELECTION_CONDITIONS::ShowAlways );

    //-- View menu -------------------------------------------------------
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, tool );

    viewMenu->AddItem( ID_ZOOM_IN, _( "Zoom In\tF1" ), HELP_ZOOM_IN,
                       zoom_in_xpm,                    SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_ZOOM_OUT, _( "Zoom Out\tF2" ), HELP_ZOOM_OUT,
                       zoom_out_xpm,                   SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_ZOOM_PAGE, _( "Zoom to Fit" ), HELP_ZOOM_FIT,
                       zoom_fit_in_page_xpm,           SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_ZOOM_REDRAW, _( "Redraw\tR" ), HELP_ZOOM_REDRAW,
                       zoom_redraw_xpm,                SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ID_ROTATE3D_X_NEG, _( "Rotate X Clockwise\tShift+X" ), "",
                       rotate_neg_x_xpm,               SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_ROTATE3D_X_POS, _( "Rotate X Counterclockwise\tX" ), "",
                       rotate_pos_x_xpm,               SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ID_ROTATE3D_Y_NEG, _( "Rotate Y Clockwise\tShift+Y" ), "",
                       rotate_neg_y_xpm,               SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_ROTATE3D_Y_POS, _( "Rotate Y Counterclockwise\tY" ), "",
                       rotate_pos_y_xpm,               SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ID_ROTATE3D_Z_NEG, _( "Rotate Z Clockwise\tShift+Z" ), "",
                       rotate_neg_z_xpm,               SELECTION_CONDITIONS::ShowAlways );;

    viewMenu->AddItem( ID_ROTATE3D_Z_POS, _( "Rotate Z Counterclockwise\tZ" ), "",
                       rotate_pos_z_xpm,               SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AppendSeparator();
    viewMenu->AddItem( ID_MOVE3D_LEFT, _( "Move Left\tLeft" ), "",
                       left_xpm,                       SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_MOVE3D_RIGHT, _( "Move Right\tRight" ), "",
                       right_xpm,                      SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_MOVE3D_UP, _( "Move Up\tUp" ), "",
                       up_xpm,                         SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_MOVE3D_DOWN, _( "Move Down\tDown" ), "",
                       down_xpm,                       SELECTION_CONDITIONS::ShowAlways );

    viewMenu->Resolve();

    //-- Preferences menu -----------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, tool );

    auto raytracingCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.RenderEngineGet() != RENDER_ENGINE_OPENGL_LEGACY;
    };
    auto NormalModeCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.MaterialModeGet() == MATERIAL_MODE_NORMAL;
    };
    auto DiffuseModeCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.MaterialModeGet() == MATERIAL_MODE_DIFFUSE_ONLY;
    };
    auto CADModeCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.MaterialModeGet() == MATERIAL_MODE_CAD_MODE;
    };
    auto copperThicknessCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS );
    };
    auto boundingBoxesCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX );
    };
    auto renderShadowsCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GetFlag( FL_RENDER_RAYTRACING_SHADOWS );
    };
    auto proceduralTexturesCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES );
    };
    auto showFloorCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GetFlag( FL_RENDER_RAYTRACING_BACKFLOOR );
    };
    auto useRefractionsCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GetFlag( FL_RENDER_RAYTRACING_REFRACTIONS );
    };
    auto useReflectionsCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GetFlag( FL_RENDER_RAYTRACING_REFLECTIONS );
    };
    auto antiAliasingCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING );
    };
    auto postProcessCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING );
    };
    auto showAxesCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GetFlag( FL_AXIS );
    };
    
    prefsMenu->AddItem( ID_TOOL_SET_VISIBLE_ITEMS, _( "Display Options" ), "",
                        read_setup_xpm,                SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddCheckItem( ID_RENDER_CURRENT_VIEW, _( "Raytracing" ), "",
                             tools_xpm,                raytracingCondition );

    // Render options submenu
    CONDITIONAL_MENU* optsSubmenu = new CONDITIONAL_MENU( false, tool );
    optsSubmenu->SetTitle( _( "Render Options" ) );
    optsSubmenu->SetIcon( options_3drender_xpm );

    // Material properties submenu
    CONDITIONAL_MENU* propsSubmenu = new CONDITIONAL_MENU( false, tool );
    propsSubmenu->SetTitle( _( "Material Properties" ) );
    propsSubmenu->SetIcon( color_materials_xpm );

    propsSubmenu->AddCheckItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_NORMAL,
                                _( "Use All Properties" ),
                                _( "Use all material properties from each 3D model file" ),
                                nullptr,                       NormalModeCondition );

    propsSubmenu->AddCheckItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_DIFFUSE_ONLY,
                                _( "Use Diffuse Only" ),
                                _( "Use only the diffuse color property from model 3D model file" ),
                                nullptr,                       DiffuseModeCondition );

    propsSubmenu->AddCheckItem( ID_MENU3D_FL_RENDER_MATERIAL_MODE_CAD_MODE,
                                _( "CAD Color Style" ),
                                _( "Use a CAD color style based on the diffuse color of the material" ),
                                nullptr,                       CADModeCondition );

    optsSubmenu->AddMenu( propsSubmenu,                        SELECTION_CONDITIONS::ShowAlways );

    optsSubmenu->AddCheckItem( ID_MENU3D_FL_OPENGL_RENDER_COPPER_THICKNESS,
                               _( "Show Copper Thickness" ),
                               _( "Shows the copper thickness on copper layers (slower loading)" ),
                               use_3D_copper_thickness_xpm,    copperThicknessCondition );

    optsSubmenu->AddCheckItem( ID_MENU3D_FL_OPENGL_RENDER_SHOW_MODEL_BBOX,
                               _( "Show Model Bounding Boxes" ), "",
                               ortho_xpm,                      boundingBoxesCondition );

    // Raytracing  submenu
    CONDITIONAL_MENU* raySubmenu = new CONDITIONAL_MENU( false, tool );
    raySubmenu->SetTitle( _( "Raytracing Options" ) );
    raySubmenu->SetIcon( tools_xpm );

    raySubmenu->AddCheckItem( ID_MENU3D_FL_RAYTRACING_RENDER_SHADOWS,
                 _( "Render Shadows" ), "",
                  green_xpm,                                   renderShadowsCondition );

    raySubmenu->AddCheckItem( ID_MENU3D_FL_RAYTRACING_PROCEDURAL_TEXTURES,
                 _( "Procedural Textures" ),
                 _( "Apply procedural textures to materials (slow)"),
                   green_xpm,                                  proceduralTexturesCondition );

    raySubmenu->AddCheckItem( ID_MENU3D_FL_RAYTRACING_BACKFLOOR,
                 _( "Add Floor" ),
                 _( "Adds a floor plane below the board (slow)"),
                   green_xpm,                                  showFloorCondition );

    raySubmenu->AddCheckItem( ID_MENU3D_FL_RAYTRACING_REFRACTIONS,
                 _( "Refractions" ),
                 _( "Render materials with refractions properties on final render (slow)"),
                   green_xpm,                                  useRefractionsCondition );

    raySubmenu->AddCheckItem( ID_MENU3D_FL_RAYTRACING_REFLECTIONS,
                _( "Reflections" ),
                _( "Render materials with reflections properties on final render (slow)"),
                  green_xpm,                                   useReflectionsCondition );

    raySubmenu->AddCheckItem( ID_MENU3D_FL_RAYTRACING_ANTI_ALIASING,
                 _( "Anti-aliasing" ),
                 _( "Render with improved quality on final render (slow)"),
                   green_xpm,                                  antiAliasingCondition );

    raySubmenu->AddCheckItem( ID_MENU3D_FL_RAYTRACING_POST_PROCESSING,
                 _( "Post-processing" ),
                 _( "Apply Screen Space Ambient Occlusion and Global Illumination reflections on final render (slow)"),
                   green_xpm,                                  postProcessCondition );

    optsSubmenu->AddMenu( raySubmenu,                          SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddMenu( optsSubmenu,                           SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();

    // Color  submenu
    CONDITIONAL_MENU* colorSubmenu = new CONDITIONAL_MENU( false, tool );
    colorSubmenu->SetTitle( _( "Choose Colors" ) );
    colorSubmenu->SetIcon( palette_xpm );

    colorSubmenu->AddItem( ID_MENU3D_BGCOLOR_TOP, _( "Background Top Color..." ), "",
                           setcolor_3d_bg_xpm,                 SELECTION_CONDITIONS::ShowAlways );

    colorSubmenu->AddItem( ID_MENU3D_BGCOLOR_BOTTOM, _( "Background Bottom Color..." ), "",
                           setcolor_3d_bg_xpm,                 SELECTION_CONDITIONS::ShowAlways );

    colorSubmenu->AddItem( ID_MENU3D_SILKSCREEN_COLOR, _( "Silkscreen Color..." ), "",
                           setcolor_silkscreen_xpm,            SELECTION_CONDITIONS::ShowAlways );

    colorSubmenu->AddItem( ID_MENU3D_SOLDERMASK_COLOR, _( "Solder Mask Color..." ), "",
                           setcolor_soldermask_xpm,            SELECTION_CONDITIONS::ShowAlways );

    colorSubmenu->AddItem( ID_MENU3D_SOLDERPASTE_COLOR, _( "Solder Paste Color..." ), "",
                           setcolor_solderpaste_xpm,           SELECTION_CONDITIONS::ShowAlways );

    colorSubmenu->AddItem( ID_MENU3D_COPPER_COLOR, _( "Copper/Surface Finish Color..." ), "",
                           setcolor_copper_xpm,                SELECTION_CONDITIONS::ShowAlways );

    colorSubmenu->AddItem( ID_MENU3D_PCB_BODY_COLOR, _( "Board Body Color..." ), "",
                           setcolor_board_body_xpm,            SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddMenu( colorSubmenu );
    
    prefsMenu->AddCheckItem( ID_MENU3D_AXIS_ONOFF, _( "Show 3D &Axis" ), "",
                             axis3d_front_xpm,                 showAxesCondition );

    // Grid  submenu
    CONDITIONAL_MENU* gridSubmenu = new CONDITIONAL_MENU( false, tool );
    gridSubmenu->SetTitle( _( "3D Grid" ) );
    gridSubmenu->SetIcon( grid_xpm );

    auto noGridCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GridGet() == GRID3D_NONE;
    };
    auto grid10mmCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GridGet() == GRID3D_10MM;
    };
    auto grid5mmCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GridGet() == GRID3D_5MM;
    };
    auto grid2p5mmCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GridGet() == GRID3D_2P5MM;
    };
    auto grid_1mmCondition = [ this ] ( const SELECTION& aSel ) {
        return m_settings.GridGet() == GRID3D_1MM;
    };

    gridSubmenu->AddItem( ID_MENU3D_GRID_NOGRID, _( "No 3D Grid" ), "",
                          nullptr,                             noGridCondition );
    gridSubmenu->AddItem( ID_MENU3D_GRID_10_MM, _( "3D Grid 10mm" ), "",
                          nullptr,                             grid10mmCondition );
    gridSubmenu->AddItem( ID_MENU3D_GRID_5_MM, _( "3D Grid 5mm" ), "",
                          nullptr,                             grid5mmCondition );
    gridSubmenu->AddItem( ID_MENU3D_GRID_2P5_MM, _( "3D Grid 2.5mm" ), "",
                          nullptr,                             grid2p5mmCondition );
    gridSubmenu->AddItem( ID_MENU3D_GRID_1_MM, _( "3D Grid 1mm" ), "",
                          nullptr,                             grid_1mmCondition );
    
    prefsMenu->AddMenu( gridSubmenu,                           SELECTION_CONDITIONS::ShowAlways );
    
    prefsMenu->AppendSeparator();
    prefsMenu->AddItem( ID_MENU3D_RESET_DEFAULTS, _( "Reset to Default Settings" ), "",
                        tools_xpm,                             SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->Resolve();
    
    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( prefsMenu, _( "&Preferences" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
}
