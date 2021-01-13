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
    ACTION_MENU* fileMenu = new ACTION_MENU( false, tool );

    fileMenu->Add( _( "Export Current View as PNG..." ),
                   "",
                   ID_MENU_SCREENCOPY_PNG,
                   export_png_xpm );

    fileMenu->Add( _( "Export Current View as JPEG..." ),
                   "",
                   ID_MENU_SCREENCOPY_JPEG,
                   export_xpm );

    fileMenu->AppendSeparator();
    fileMenu->AddClose( _( "3D Viewer" ) );


    //-- Edit menu -------------------------------------------------------
    // Avoid to translate hotkey modifiers like Ctrl and Shift.
    // The translated modifiers do not always work
    ACTION_MENU* editMenu = new ACTION_MENU( false, tool );

    editMenu->Add( _( "Copy 3D Image" ),
                   "",
                   ID_TOOL_SCREENCOPY_TOCLIBBOARD,
                   copy_xpm );


    //-- View menu -------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, tool );

    viewMenu->Add( ACTIONS::zoomIn );
    viewMenu->Add( ACTIONS::zoomOut );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( EDA_3D_ACTIONS::rotateXCW );
    viewMenu->Add( EDA_3D_ACTIONS::rotateXCCW );

    viewMenu->AppendSeparator();
    viewMenu->Add( EDA_3D_ACTIONS::rotateYCW );
    viewMenu->Add( EDA_3D_ACTIONS::rotateYCCW );

    viewMenu->AppendSeparator();
    viewMenu->Add( EDA_3D_ACTIONS::rotateZCW );
    viewMenu->Add( EDA_3D_ACTIONS::rotateZCCW );

    viewMenu->AppendSeparator();
    viewMenu->Add( EDA_3D_ACTIONS::moveLeft );
    viewMenu->Add( EDA_3D_ACTIONS::moveRight );
    viewMenu->Add( EDA_3D_ACTIONS::moveUp );
    viewMenu->Add( EDA_3D_ACTIONS::moveDown );


    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, tool );

    prefsMenu->Add( _( "Display Options" ), "",
                    ID_TOOL_SET_VISIBLE_ITEMS,
                    config_xpm );

    prefsMenu->Add( _( "Raytracing" ), "",
                    ID_RENDER_CURRENT_VIEW,
                    tools_xpm,
                    ACTION_MENU::CHECK );

    // Render options submenu
    ACTION_MENU* optsSubmenu = new ACTION_MENU( false, tool );
    optsSubmenu->SetTitle( _( "Render Options" ) );
    optsSubmenu->SetIcon( options_3drender_xpm );

    // Material properties submenu
    ACTION_MENU* propsSubmenu = new ACTION_MENU( false, tool );
    propsSubmenu->SetTitle( _( "Material Properties" ) );

    propsSubmenu->Add( EDA_3D_ACTIONS::materialNormal,  ACTION_MENU::CHECK );
    propsSubmenu->Add( EDA_3D_ACTIONS::materialDiffuse, ACTION_MENU::CHECK );
    propsSubmenu->Add( EDA_3D_ACTIONS::materialCAD,     ACTION_MENU::CHECK );

    optsSubmenu->Add( propsSubmenu );

    optsSubmenu->Add( EDA_3D_ACTIONS::showBoundingBoxes, ACTION_MENU::CHECK );

    // Raytracing  submenu
    ACTION_MENU* raySubmenu = new ACTION_MENU( false, tool );
    raySubmenu->SetTitle( _( "Raytracing Options" ) );

    raySubmenu->Add( EDA_3D_ACTIONS::renderShadows,      ACTION_MENU::CHECK );
    raySubmenu->Add( EDA_3D_ACTIONS::proceduralTextures, ACTION_MENU::CHECK );
    raySubmenu->Add( EDA_3D_ACTIONS::addFloor,           ACTION_MENU::CHECK );
    raySubmenu->Add( EDA_3D_ACTIONS::showRefractions,    ACTION_MENU::CHECK );
    raySubmenu->Add( EDA_3D_ACTIONS::showReflections,    ACTION_MENU::CHECK );
    raySubmenu->Add( EDA_3D_ACTIONS::antiAliasing,       ACTION_MENU::CHECK );
    raySubmenu->Add( EDA_3D_ACTIONS::postProcessing,     ACTION_MENU::CHECK );

    optsSubmenu->Add( raySubmenu );
    prefsMenu->Add( optsSubmenu );

    prefsMenu->AppendSeparator();

    // Color  submenu
    ACTION_MENU* colorSubmenu = new ACTION_MENU( false, tool );
    colorSubmenu->SetTitle( _( "Choose Colors" ) );
    colorSubmenu->SetIcon( color_materials_xpm );

    colorSubmenu->Add( _( "Background Top Color..." ),
                       ID_MENU3D_BGCOLOR_TOP,
                       nullptr );

    colorSubmenu->Add( _( "Background Bottom Color..." ),
                       ID_MENU3D_BGCOLOR_BOTTOM,
                       nullptr );

    colorSubmenu->Add( _( "Silkscreen Color..." ),
                       ID_MENU3D_SILKSCREEN_COLOR,
                       nullptr );

    colorSubmenu->Add( _( "Solder Mask Color..." ),
                       ID_MENU3D_SOLDERMASK_COLOR,
                       nullptr );

    colorSubmenu->Add( _( "Solder Paste Color..." ),
                       ID_MENU3D_SOLDERPASTE_COLOR,
                       nullptr );

    colorSubmenu->Add( _( "Copper/Surface Finish Color..." ),
                       ID_MENU3D_COPPER_COLOR,
                       nullptr );

    colorSubmenu->Add( _( "Board Body Color..." ),
                       ID_MENU3D_PCB_BODY_COLOR,
                       nullptr );

    // Only allow the stackup to be used in the PCB editor, since it isn't editable in the other frames
    if( Parent()->IsType( FRAME_PCB_EDITOR ) )
    {
        colorSubmenu->Add( _( "Get colors from physical stackup" ),
                           ID_MENU3D_STACKUP_COLORS,
                           nullptr );
    }

    prefsMenu->Add( colorSubmenu );

    prefsMenu->Add( EDA_3D_ACTIONS::showAxis, ACTION_MENU::CHECK );

    // Grid submenu
    ACTION_MENU* gridSubmenu = new ACTION_MENU( false, tool );
    gridSubmenu->SetTitle( _( "3D Grid" ) );
    gridSubmenu->SetIcon( grid_xpm );

    gridSubmenu->Add( EDA_3D_ACTIONS::noGrid,        ACTION_MENU::CHECK);
    gridSubmenu->Add( EDA_3D_ACTIONS::show10mmGrid,  ACTION_MENU::CHECK);
    gridSubmenu->Add( EDA_3D_ACTIONS::show5mmGrid,   ACTION_MENU::CHECK);
    gridSubmenu->Add( EDA_3D_ACTIONS::show2_5mmGrid, ACTION_MENU::CHECK);
    gridSubmenu->Add( EDA_3D_ACTIONS::show1mmGrid,   ACTION_MENU::CHECK);

    prefsMenu->Add( gridSubmenu );

    prefsMenu->AppendSeparator();
    prefsMenu->Add( _( "Reset to Default Settings" ), ID_MENU3D_RESET_DEFAULTS, tools_xpm );

#ifdef __APPLE__    // Note: will get moved to Apple menu by wxWidgets
    prefsMenu->Add( _( "Preferences..." ) + "\tCtrl+,",
                    _( "Show preferences for all open tools" ),
                    wxID_PREFERENCES,
                    preference_xpm );
#endif

    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu,  _( "&File" ) );
    menuBar->Append( editMenu,  _( "&Edit" ) );
    menuBar->Append( viewMenu,  _( "&View" ) );
    menuBar->Append( prefsMenu, _( "&Preferences" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
}
