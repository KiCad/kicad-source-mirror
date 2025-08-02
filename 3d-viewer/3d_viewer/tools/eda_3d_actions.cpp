/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/tool_manager.h>
#include <bitmaps.h>
#include <3d_viewer_id.h>
#include <3d_enums.h>
#include <3d-viewer/3d_viewer/eda_3d_viewer_frame.h>
#include "eda_3d_actions.h"
#include "tool/tool_action.h"


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

// No description, it is not supposed to be shown anywhere
TOOL_ACTION EDA_3D_ACTIONS::controlActivate( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control" )
        .Scope( AS_GLOBAL )
        .Flags( AF_ACTIVATE )
        .ToolbarState( TOOLBAR_STATE::HIDDEN) );

TOOL_ACTION EDA_3D_ACTIONS:: reloadBoard( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.reloadBoard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Reload board" ) )
        .Tooltip( _( "Reload board and refresh 3D view" ) )
        .Icon( BITMAPS::import3d ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleRaytacing( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleRaytacing" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Use raytracing" ) )
        .Tooltip( _( "Render current view using Raytracing" ) )
        .Icon( BITMAPS::ray_tracing )
        .ToolbarState( TOOLBAR_STATE::TOGGLE) );

TOOL_ACTION EDA_3D_ACTIONS::copyToClipboard( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.copyToClipboard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Copy 3D image to clipboard" ) )
        .Tooltip( _( "Copy the current 3D image to the clipboard" ) )
        .Icon( BITMAPS::copy )
        .Parameter<EDA_3D_VIEWER_EXPORT_FORMAT>( EDA_3D_VIEWER_EXPORT_FORMAT::CLIPBOARD ) );

TOOL_ACTION EDA_3D_ACTIONS::exportImage( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.exportImage" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export Image..." ) )
        .Tooltip( _( "Export the Current View as an image file" ) )
        .Icon( BITMAPS::export_file )
        .Parameter<EDA_3D_VIEWER_EXPORT_FORMAT>( EDA_3D_VIEWER_EXPORT_FORMAT::IMAGE ) );

TOOL_ACTION EDA_3D_ACTIONS::pivotCenter( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.pivotCenter" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( ' ' )
        .FriendlyName( _( "Set Pivot" ) )
        .Tooltip( _( "Place point around which the board will be rotated (middle mouse click)" ) )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_PIVOT_CENTER ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateXCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateXclockwise" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rotate X Clockwise" ) )
        .Icon( BITMAPS::rotate_cw_x )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::X_CW ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateXCCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateXcounterclockwise" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rotate X Counterclockwise" ) )
        .Icon( BITMAPS::rotate_ccw_x )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::X_CCW ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateYCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateYclockwise" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rotate Y Clockwise" ) )
        .Icon( BITMAPS::rotate_cw_y )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::Y_CW ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateYCCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateYcounterclockwise" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rotate Y Counterclockwise" ) )
        .Icon( BITMAPS::rotate_ccw_y )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::Y_CCW ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateZCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateZclockwise" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'R' )
        .FriendlyName( _( "Rotate Z Clockwise" ) )
        .Icon( BITMAPS::rotate_cw_z )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::Z_CW ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateZCCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateZcounterclockwise" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'R' )
        .FriendlyName( _( "Rotate Z Counterclockwise" ) )
        .Icon( BITMAPS::rotate_ccw_z )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::Z_CCW ) );

TOOL_ACTION EDA_3D_ACTIONS::moveLeft( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.moveLeft" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( WXK_LEFT )
        .FriendlyName( _( "Move Board Left" ) )
        .Icon( BITMAPS::left )
        .Flags( AF_NONE )
        .Parameter( CURSOR_LEFT ) );

TOOL_ACTION EDA_3D_ACTIONS::moveRight( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.moveRight" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( WXK_RIGHT )
        .FriendlyName( _( "Move Board Right" ) )
        .Icon( BITMAPS::right )
        .Flags( AF_NONE )
        .Parameter( CURSOR_RIGHT ) );

TOOL_ACTION EDA_3D_ACTIONS::moveUp( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.moveUp" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( WXK_UP )
        .FriendlyName( _( "Move Board Up" ) )
        .Icon( BITMAPS::up )
        .Flags( AF_NONE )
        .Parameter( CURSOR_UP ) );

TOOL_ACTION EDA_3D_ACTIONS::moveDown( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.moveDown" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( WXK_DOWN )
        .FriendlyName( _( "Move Board Down" ) )
        .Icon( BITMAPS::down )
        .Flags( AF_NONE )
        .Parameter( CURSOR_DOWN ) );

TOOL_ACTION EDA_3D_ACTIONS::homeView( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.homeView" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( WXK_HOME )
        .FriendlyName ( _( "Home View" ) )
        .Tooltip( _( "Redraw at the home position and zoom" ) )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_FIT_SCREEN ) );

TOOL_ACTION EDA_3D_ACTIONS::flipView( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.flipView" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'F' )
        .FriendlyName( _( "Flip Board" ) )
        .Tooltip( _( "Flip the board view" ) )
        .Icon( BITMAPS::flip_board )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_FLIP ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleOrtho( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleOrtho" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Toggle Orthographic Projection" ) )
        .Tooltip( _( "Enable/disable orthographic projection" ) )
        .Icon( BITMAPS::ortho )
        .ToolbarState( TOOLBAR_STATE::TOGGLE ) );

TOOL_ACTION EDA_3D_ACTIONS::viewFront( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewFront" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'Y' )
        .FriendlyName( _( "View Front" ) )
        .Icon( BITMAPS::axis3d_front )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_FRONT ) );

TOOL_ACTION EDA_3D_ACTIONS::viewBack( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewBack" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( MD_SHIFT + 'Y' )
        .FriendlyName( _( "View Back" ) )
        .Icon( BITMAPS::axis3d_back )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_BACK ) );

TOOL_ACTION EDA_3D_ACTIONS::viewLeft( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewLeft" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( MD_SHIFT + 'X' )
        .FriendlyName( _( "View Left" ) )
        .Icon( BITMAPS::axis3d_left )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_LEFT ) );

TOOL_ACTION EDA_3D_ACTIONS::viewRight( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewRight" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'X' )
        .FriendlyName( _( "View Right" ) )
        .Icon( BITMAPS::axis3d_right )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_RIGHT ) );

TOOL_ACTION EDA_3D_ACTIONS::viewTop( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewTop" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'Z' )
        .FriendlyName( _( "View Top" ) )
        .Icon( BITMAPS::axis3d_top )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_TOP ) );

TOOL_ACTION EDA_3D_ACTIONS::viewBottom( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewBottom" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( MD_SHIFT + 'Z' )
        .FriendlyName( _( "View Bottom" ) )
        .Icon( BITMAPS::axis3d_bottom )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_BOTTOM ) );

TOOL_ACTION EDA_3D_ACTIONS::noGrid( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.noGrid" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "No 3D Grid" ) )
        .Flags( AF_NONE )
        .Parameter( GRID3D_TYPE::NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::show10mmGrid( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.show10mmGrid" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "3D Grid 10mm" ) )
        .Flags( AF_NONE )
        .Parameter( GRID3D_TYPE::GRID_10MM ) );

TOOL_ACTION EDA_3D_ACTIONS::show5mmGrid( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.show5mmGrid" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "3D Grid 5mm" ) )
        .Flags( AF_NONE )
        .Parameter( GRID3D_TYPE::GRID_5MM ) );

TOOL_ACTION EDA_3D_ACTIONS::show2_5mmGrid( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.show2_5mmGrid" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "3D Grid 2.5mm" ) )
        .Flags( AF_NONE )
        .Parameter( GRID3D_TYPE::GRID_2P5MM ) );

TOOL_ACTION EDA_3D_ACTIONS::show1mmGrid( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.show1mmGrid" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "3D Grid 1mm" ) )
        .Flags( AF_NONE )
        .Parameter( GRID3D_TYPE::GRID_1MM ) );

TOOL_ACTION EDA_3D_ACTIONS::materialNormal( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.materialNormal" )
        .Scope( AS_ACTIVE )
        .FriendlyName( _( "Render Realistic Materials" ) )
        .Tooltip( _( "Use all material properties from each 3D model file" ) )
        .Flags( AF_NONE )
        .Parameter( MATERIAL_MODE::NORMAL ) );

TOOL_ACTION EDA_3D_ACTIONS::materialDiffuse( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.materialDiffuse" )
        .Scope( AS_ACTIVE )
        .FriendlyName( _( "Render Solid Colors" ) )
        .Tooltip( _( "Use only the diffuse color property from 3D model file" ) )
        .Flags( AF_NONE )
        .Parameter( MATERIAL_MODE::DIFFUSE_ONLY ) );

TOOL_ACTION EDA_3D_ACTIONS::materialCAD( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.materialCAD" )
        .Scope( AS_ACTIVE )
        .FriendlyName( _( "Render CAD Colors" ) )
        .Tooltip( _( "Use a CAD color style based on the diffuse color of the material" ) )
        .Flags( AF_NONE )
        .Parameter( MATERIAL_MODE::CAD_MODE ) );

TOOL_ACTION EDA_3D_ACTIONS::showTHT( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.attributesTHT" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'T' )
        .FriendlyName( _( "Show Through Hole 3D Models" ) )
        .Tooltip( _( "Show 3D models for 'Through hole' type footprints" ) )
        .Icon( BITMAPS::show_tht )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showSMD( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.attributesSMD" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'S' )
        .FriendlyName( _( "Show SMD 3D Models" ) )
        .Tooltip( _( "Show 3D models for 'Surface mount' type footprints" ) )
        .Icon( BITMAPS::show_smt )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showVirtual( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.attributesOther" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'V' )
        .FriendlyName( _( "Show Unspecified 3D Models" ) )
        .Tooltip( _( "Show 3D models for 'unspecified' type footprints" ) )
        .Icon( BITMAPS::show_other )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showNotInPosFile( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.attribute_not_in_posfile" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'P' )
        .FriendlyName( _( "Show 3D Models not in POS File" ) )
        .Tooltip( _( "Show 3D models even if not found in .pos file" ) )
        .Icon( BITMAPS::show_not_in_posfile )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showDNP( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.attribute_dnp" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'D' )
        .FriendlyName( _( "Show 3D Models marked DNP" ) )
        .Tooltip( _( "Show 3D models even if marked 'Do Not Place'" ) )
        .Icon( BITMAPS::show_dnp )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showBBoxes( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.showBoundingBoxes" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Model Bounding Boxes" ) )
        .Tooltip( _( "Show 3D model bounding boxes in realtime renderer" ) )
        .Icon( BITMAPS::ortho )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showNavigator( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.showNavigator" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show 3D Navigator" ) )
        .Icon( BITMAPS::axis3d_front )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showLayersManager( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.showLayersManager" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Appearance Manager" ) )
        .Tooltip( _( "Show/hide the appearance manager" ) )
        .Icon( BITMAPS::layers_manager )
        .ToolbarState( TOOLBAR_STATE::TOGGLE ) );

