/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <menus_helpers.h>
#include <3d_viewer_id.h>
#include <3d_viewer/tools/3d_actions.h>
#include <tool/tool_manager.h>
#include <tool/common_control.h>
#include <widgets/wx_menubar.h>


void EDA_3D_VIEWER::CreateMenuBar()
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::CreateMenuBar" );

    COMMON_CONTROL* tool    = m_toolManager->GetTool<COMMON_CONTROL>();
    WX_MENUBAR*     menuBar = new WX_MENUBAR();


    //-- File menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU*   fileMenu = new CONDITIONAL_MENU( false, tool );

    fileMenu->AddItem( ID_MENU_SCREENCOPY_PNG, _( "Export Current View as PNG..." ), "",
                       export_xpm,                     SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_MENU_SCREENCOPY_JPEG, _( "Export Current View as JPEG..." ), "",
                       export_xpm,                     SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddClose( _( "3D Viewer" ) );

    fileMenu->Resolve();

    //-- Edit menu -------------------------------------------------------
    // Avoid to translate hotkey modifiers like Ctrl and Shift.
    // The translated modifiers do not always work
    CONDITIONAL_MENU* editMenu = new CONDITIONAL_MENU( false, tool );

    editMenu->AddItem( ID_TOOL_SCREENCOPY_TOCLIBBOARD, _( "Copy 3D Image" ), "",
                       copy_xpm,                       SELECTION_CONDITIONS::ShowAlways );

    editMenu->Resolve();

    //-- View menu -------------------------------------------------------
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, tool );

    viewMenu->AddItem( ACTIONS::zoomIn,                SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOut,               SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,         SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,            SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( EDA_3D_ACTIONS::rotateXCW,      SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( EDA_3D_ACTIONS::rotateXCCW,     SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( EDA_3D_ACTIONS::rotateYCW,      SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( EDA_3D_ACTIONS::rotateYCCW,     SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( EDA_3D_ACTIONS::rotateZCW,      SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( EDA_3D_ACTIONS::rotateZCCW,     SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( EDA_3D_ACTIONS::moveLeft,       SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( EDA_3D_ACTIONS::moveRight,      SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( EDA_3D_ACTIONS::moveUp,         SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( EDA_3D_ACTIONS::moveDown,       SELECTION_CONDITIONS::ShowAlways );

    viewMenu->Resolve();

    //-- Preferences menu -----------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, tool );

    //clang-format off
    auto raytracingCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.RenderEngineGet() != RENDER_ENGINE::OPENGL_LEGACY;
    };

    auto NormalModeCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.MaterialModeGet() == MATERIAL_MODE::NORMAL;
    };

    auto DiffuseModeCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.MaterialModeGet() == MATERIAL_MODE::DIFFUSE_ONLY;
    };

    auto CADModeCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.MaterialModeGet() == MATERIAL_MODE::CAD_MODE;
    };

    auto boundingBoxesCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX );
    };

    auto renderShadowsCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_SHADOWS );
    };

    auto proceduralTexturesCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES );
    };

    auto showFloorCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_BACKFLOOR );
    };

    auto useRefractionsCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_REFRACTIONS );
    };

    auto useReflectionsCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_REFLECTIONS );
    };

    auto antiAliasingCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING );
    };

    auto postProcessCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING );
    };

    auto showAxesCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GetFlag( FL_AXIS );
    };
    //clang-format on

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

    optsSubmenu->AddCheckItem( EDA_3D_ACTIONS::showBoundingBoxes,   boundingBoxesCondition );

    // Raytracing  submenu
    CONDITIONAL_MENU* raySubmenu = new CONDITIONAL_MENU( false, tool );
    raySubmenu->SetTitle( _( "Raytracing Options" ) );
    raySubmenu->SetIcon( tools_xpm );

    raySubmenu->AddCheckItem( EDA_3D_ACTIONS::renderShadows,        renderShadowsCondition );
    raySubmenu->AddCheckItem( EDA_3D_ACTIONS::proceduralTextures,   proceduralTexturesCondition );
    raySubmenu->AddCheckItem( EDA_3D_ACTIONS::addFloor,             showFloorCondition );
    raySubmenu->AddCheckItem( EDA_3D_ACTIONS::showRefractions,      useRefractionsCondition );
    raySubmenu->AddCheckItem( EDA_3D_ACTIONS::showReflections,      useReflectionsCondition );
    raySubmenu->AddCheckItem( EDA_3D_ACTIONS::antiAliasing,         antiAliasingCondition );

    raySubmenu->AddCheckItem( EDA_3D_ACTIONS::postProcessing,       postProcessCondition );

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

    // Only allow the stackup to be used in the PCB editor, since it isn't editable in the other frames
    if( Parent()->IsType( FRAME_PCB_EDITOR ) )
    {
        colorSubmenu->AddItem( ID_MENU3D_STACKUP_COLORS, _( "Get colors from physical stackup" ), "",
                               nullptr, SELECTION_CONDITIONS::ShowAlways );
    }

    prefsMenu->AddMenu( colorSubmenu );

    prefsMenu->AddCheckItem( EDA_3D_ACTIONS::showAxis,         showAxesCondition );

    // Grid  submenu
    CONDITIONAL_MENU* gridSubmenu = new CONDITIONAL_MENU( false, tool );
    gridSubmenu->SetTitle( _( "3D Grid" ) );
    gridSubmenu->SetIcon( grid_xpm );

    //clang-format off
    auto noGridCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GridGet() == GRID3D_TYPE::NONE;
    };

    auto grid10mmCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GridGet() == GRID3D_TYPE::GRID_10MM;
    };

    auto grid5mmCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GridGet() == GRID3D_TYPE::GRID_5MM;
    };

    auto grid2p5mmCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GridGet() == GRID3D_TYPE::GRID_2P5MM;
    };

    auto grid_1mmCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GridGet() == GRID3D_TYPE::GRID_1MM;
    };
    //clang-format on

    gridSubmenu->AddCheckItem( EDA_3D_ACTIONS::noGrid,         noGridCondition );
    gridSubmenu->AddCheckItem( EDA_3D_ACTIONS::show10mmGrid,   grid10mmCondition );
    gridSubmenu->AddCheckItem( EDA_3D_ACTIONS::show5mmGrid,    grid5mmCondition );
    gridSubmenu->AddCheckItem( EDA_3D_ACTIONS::show2_5mmGrid,  grid2p5mmCondition );
    gridSubmenu->AddCheckItem( EDA_3D_ACTIONS::show1mmGrid,    grid_1mmCondition );

    prefsMenu->AddMenu( gridSubmenu,                           SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    prefsMenu->AddItem( ID_MENU3D_RESET_DEFAULTS, _( "Reset to Default Settings" ), "",
                        tools_xpm,                             SELECTION_CONDITIONS::ShowAlways );

#ifdef __APPLE__    // Note: will get moved to Apple menu by wxWidgets
    prefsMenu->AddItem( wxID_PREFERENCES,
                        _( "Preferences...\tCTRL+," ),
                        _( "Show preferences for all open tools" ),
                        preference_xpm,                        SELECTION_CONDITIONS::ShowAlways );
#endif

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
