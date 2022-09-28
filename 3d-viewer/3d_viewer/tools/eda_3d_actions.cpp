/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EDA_3D_ACTIONS::pivotCenter( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.pivotCenter" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( ' ' )
        .MenuText( _( "Center pivot rotation" ) )
        .Tooltip( _( "Center pivot rotation (middle mouse click)" ) )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_PIVOT_CENTER ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateXCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateXclockwise" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Rotate X Clockwise" ) )
        .Tooltip( _( "Rotate X Clockwise" ) )
        .Icon( BITMAPS::rotate_cw_x )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::X_CW ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateXCCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateXcounterclockwise" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Rotate X Counterclockwise" ) )
        .Tooltip( _( "Rotate X Counterclockwise" ) )
        .Icon( BITMAPS::rotate_ccw_x )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::X_CCW ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateYCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateYclockwise" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Rotate Y Clockwise" ) )
        .Tooltip( _( "Rotate Y Clockwise" ) )
        .Icon( BITMAPS::rotate_cw_y )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::Y_CW ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateYCCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateYcounterclockwise" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Rotate Y Counterclockwise" ) )
        .Tooltip( _( "Rotate Y Counterclockwise" ) )
        .Icon( BITMAPS::rotate_ccw_y )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::Y_CCW ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateZCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateZclockwise" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Rotate Z Clockwise" ) )
        .Tooltip( _( "Rotate Z Clockwise" ) )
        .Icon( BITMAPS::rotate_cw_z )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::Z_CW ) );

TOOL_ACTION EDA_3D_ACTIONS::rotateZCCW( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.rotateZcounterclockwise" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Rotate Z Counterclockwise" ) )
        .Tooltip( _( "Rotate Z Counterclockwise" ) )
        .Icon( BITMAPS::rotate_ccw_z )
        .Flags( AF_NONE )
        .Parameter( ROTATION_DIR::Z_CCW ) );

TOOL_ACTION EDA_3D_ACTIONS::moveLeft( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.moveLeft" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( WXK_LEFT )
        .MenuText( _( "Move board Left" ) )
        .Tooltip( _( "Move board Left" ) )
        .Icon( BITMAPS::left )
        .Flags( AF_NONE )
        .Parameter( CURSOR_LEFT ) );

TOOL_ACTION EDA_3D_ACTIONS::moveRight( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.moveRight" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( WXK_RIGHT )
        .MenuText( _( "Move board Right" ) )
        .Tooltip( _( "Move board Right" ) )
        .Icon( BITMAPS::right )
        .Flags( AF_NONE )
        .Parameter( CURSOR_RIGHT ) );

TOOL_ACTION EDA_3D_ACTIONS::moveUp( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.moveUp" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( WXK_UP )
        .MenuText( _( "Move board Up" ) )
        .Tooltip( _( "Move board Up" ) )
        .Icon( BITMAPS::up )
        .Flags( AF_NONE )
        .Parameter( CURSOR_UP ) );

TOOL_ACTION EDA_3D_ACTIONS::moveDown( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.moveDown" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( WXK_DOWN )
        .MenuText( _( "Move board Down" ) )
        .Tooltip( _( "Move board Down" ) )
        .Icon( BITMAPS::down )
        .Flags( AF_NONE )
        .Parameter( CURSOR_DOWN ) );

TOOL_ACTION EDA_3D_ACTIONS::homeView( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.homeView" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( WXK_HOME )
        .MenuText ( _( "Home view" ) )
        .Tooltip( _( "Home view" ) )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_FIT_SCREEN ) );

TOOL_ACTION EDA_3D_ACTIONS::resetView( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.resetView" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'R' )
        .MenuText( _( "Reset view" ) )
        .Tooltip( _( "Reset view" ) )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_RESET ) );

TOOL_ACTION EDA_3D_ACTIONS::flipView( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.flipView" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'F' )
        .MenuText( _( "Flip Board" ) )
        .Tooltip( _( "Flip the board view" ) )
        .Icon( BITMAPS::flip_board )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_FLIP ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleOrtho( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleOrtho" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Toggle orthographic projection" ) )
        .Tooltip( _( "Enable/disable orthographic projection" ) )
        .Icon( BITMAPS::ortho ) );

TOOL_ACTION EDA_3D_ACTIONS::viewFront( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewFront" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'Y' )
        .MenuText( _( "View Front" ) )
        .Tooltip( _( "View Front" ) )
        .Icon( BITMAPS::axis3d_front )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_FRONT ) );

TOOL_ACTION EDA_3D_ACTIONS::viewBack( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewBack" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( MD_SHIFT + 'Y' )
        .MenuText( _( "View Back" ) )
        .Tooltip( _( "View Back" ) )
        .Icon( BITMAPS::axis3d_back )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_BACK ) );

TOOL_ACTION EDA_3D_ACTIONS::viewLeft( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewLeft" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( MD_SHIFT + 'X' )
        .MenuText( _( "View Left" ) )
        .Tooltip( _( "View Left" ) )
        .Icon( BITMAPS::axis3d_left )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_LEFT ) );

TOOL_ACTION EDA_3D_ACTIONS::viewRight( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewRight" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'X' )
        .MenuText( _( "View Right" ) )
        .Tooltip( _( "View Right" ) )
        .Icon( BITMAPS::axis3d_right )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_RIGHT ) );

TOOL_ACTION EDA_3D_ACTIONS::viewTop( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewTop" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'Z' )
        .MenuText( _( "View Top" ) )
        .Tooltip( _( "View Top" ) )
        .Icon( BITMAPS::axis3d_top )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_TOP ) );

TOOL_ACTION EDA_3D_ACTIONS::viewBottom( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.viewBottom" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( MD_SHIFT + 'Z' )
        .MenuText( _( "View Bottom" ) )
        .Tooltip( _( "View Bottom" ) )
        .Icon( BITMAPS::axis3d_bottom )
        .Flags( AF_NONE )
        .Parameter( VIEW3D_TYPE::VIEW3D_BOTTOM ) );

TOOL_ACTION EDA_3D_ACTIONS::noGrid( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.noGrid" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "No 3D Grid" ) )
        .Tooltip( _( "No 3D Grid" ) )
        .Flags( AF_NONE )
        .Parameter( GRID3D_TYPE::NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::show10mmGrid( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.show10mmGrid" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "3D Grid 10mm" ) )
        .Tooltip( _( "3D Grid 10mm" ) )
        .Flags( AF_NONE )
        .Parameter( GRID3D_TYPE::GRID_10MM ) );

TOOL_ACTION EDA_3D_ACTIONS::show5mmGrid( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.show5mmGrid" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "3D Grid 5mm" ) )
        .Tooltip( _( "3D Grid 5mm" ) )
        .Flags( AF_NONE )
        .Parameter( GRID3D_TYPE::GRID_5MM ) );

TOOL_ACTION EDA_3D_ACTIONS::show2_5mmGrid( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.show2_5mmGrid" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "3D Grid 2.5mm" ) )
        .Tooltip( _( "3D Grid 2.5mm" ) )
        .Flags( AF_NONE )
        .Parameter( GRID3D_TYPE::GRID_2P5MM ) );

TOOL_ACTION EDA_3D_ACTIONS::show1mmGrid( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.show1mmGrid" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "3D Grid 1mm" ) )
        .Tooltip( _( "3D Grid 1mm" ) )
        .Flags( AF_NONE )
        .Parameter( GRID3D_TYPE::GRID_1MM ) );

TOOL_ACTION EDA_3D_ACTIONS::materialNormal( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.materialNormal" )
        .Scope( AS_ACTIVE )
        .MenuText( _( "Render Realistic Materials" ) )
        .Tooltip( _( "Use all material properties from each 3D model file" ) )
        .Flags( AF_NONE )
        .Parameter( MATERIAL_MODE::NORMAL ) );

TOOL_ACTION EDA_3D_ACTIONS::materialDiffuse( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.materialDiffuse" )
        .Scope( AS_ACTIVE )
        .MenuText( _( "Render Solid Colors" ) )
        .Tooltip( _( "Use only the diffuse color property from model 3D model file" ) )
        .Flags( AF_NONE )
        .Parameter( MATERIAL_MODE::DIFFUSE_ONLY ) );

TOOL_ACTION EDA_3D_ACTIONS::materialCAD( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.materialCAD" )
        .Scope( AS_ACTIVE )
        .MenuText( _( "Render CAD Colors" ) )
        .Tooltip( _( "Use a CAD color style based on the diffuse color of the material" ) )
        .Flags( AF_NONE )
        .Parameter( MATERIAL_MODE::CAD_MODE ) );

TOOL_ACTION EDA_3D_ACTIONS::showTHT( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.attributesTHT" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'T' )
        .MenuText( _( "Toggle Through Hole 3D models" ) )
        .Tooltip( _( "Toggle 3D models for 'Through hole' type components" ) )
        .Icon( BITMAPS::show_tht )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showSMD( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.attributesSMD" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'S' )
        .MenuText( _( "Toggle SMD 3D models" ) )
        .Tooltip( _( "Toggle 3D models for 'Surface mount' type components" ) )
        .Icon( BITMAPS::show_smt )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showVirtual( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.attributesOther" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'V' )
        .MenuText( _( "Toggle unspecified 3D models" ) )
        .Tooltip( _( "Toggle 3D models for 'unspecified' type components" ) )
        .Icon( BITMAPS::show_other )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showNotInPosFile( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.attribute_not_in_posfile" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'P' )
        .MenuText( _( "Toggle 3D models not in pos file" ) )
        .Tooltip( _( "Toggle 3D models not in pos file" ) )
        .Icon( BITMAPS::show_not_in_posfile )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showDNP( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.attribute_dnp" )
        .Scope( AS_ACTIVE )
        .DefaultHotkey( 'D' )
        .MenuText( _( "Toggle 3D models marked DNP" ) )
        .Tooltip( _( "Toggle 3D models for components marked 'Do Not Place'" ) )
        .Icon( BITMAPS::show_dnp )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::showBBoxes( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.showBoundingBoxes" )
         .Scope( AS_GLOBAL )
         .MenuText( _( "Show Model Bounding Boxes" ) )
         .Tooltip( _( "Show Model Bounding Boxes" ) )
         .Icon( BITMAPS::ortho )
         .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleRealisticMode( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleRealisticMode" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Toggle realistic mode" ) )
        .Tooltip( _( "Toggle realistic mode" ) ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleBoardBody( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleBoardBody" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Toggle board body display" ) )
        .Tooltip( _( "Toggle board body display" ) ) );

TOOL_ACTION EDA_3D_ACTIONS::showAxis( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.showAxis" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Show 3D Axis" ) )
        .Tooltip( _( "Show 3D Axis" ) )
        .Icon( BITMAPS::axis3d_front )
        .Flags( AF_NONE ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleZones( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleZones" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Toggle zone display" ) )
        .Tooltip( _( "Toggle zone display" ) ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleAdhesive( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleAdhesive" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Toggle adhesive display" ) )
        .Tooltip( _( "Toggle display of adhesive layers" ) ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleSilk( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleSilk" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Toggle silkscreen display" ) )
        .Tooltip( _( "Toggle display of silkscreen layers" ) ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleSolderMask( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleSolderMask" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Toggle solder mask display" ) )
        .Tooltip( _( "Toggle display of solder mask layers" ) ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleSolderPaste( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleSolderPaste" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Toggle solder paste display" ) )
        .Tooltip( _( "Toggle display of solder paste layers" ) ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleComments( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleComments" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Toggle comments display" ) )
        .Tooltip( _( "Toggle display of comments and drawings layers" ) ) );

TOOL_ACTION EDA_3D_ACTIONS::toggleECO( TOOL_ACTION_ARGS()
        .Name( "3DViewer.Control.toggleECO" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Toggle ECO display" ) )
        .Tooltip( _( "Toggle display of ECO layers" ) ) );

