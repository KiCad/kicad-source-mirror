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


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s


TOOL_ACTION EDA_3D_ACTIONS::controlActivate( "3DViewer.Control", AS_GLOBAL,
        0, "", "", "",
        BITMAPS::INVALID_BITMAP, AF_ACTIVATE ); // No description, it is not supposed to be shown anywhere

TOOL_ACTION EDA_3D_ACTIONS::pivotCenter( "3DViewer.Control.pivotCenter",
        AS_ACTIVE,
        ' ', "",
        _( "Center pivot rotation" ), _( "Center pivot rotation (middle mouse click)" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) WXK_SPACE );

TOOL_ACTION EDA_3D_ACTIONS::rotateXCW( "3DViewer.Control.rotateXclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate X Clockwise" ), _( "Rotate X Clockwise" ),
        BITMAPS::rotate_cw_x, AF_NONE, (void*) ROTATION_DIR::X_CW );

TOOL_ACTION EDA_3D_ACTIONS::rotateXCCW( "3DViewer.Control.rotateXcounterclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate X Counterclockwise" ), _( "Rotate X Counterclockwise" ),
        BITMAPS::rotate_ccw_x, AF_NONE, (void*) ROTATION_DIR::X_CCW );

TOOL_ACTION EDA_3D_ACTIONS::rotateYCW( "3DViewer.Control.rotateYclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate Y Clockwise" ), _( "Rotate Y Clockwise" ),
        BITMAPS::rotate_cw_y, AF_NONE, (void*) ROTATION_DIR::Y_CW );

TOOL_ACTION EDA_3D_ACTIONS::rotateYCCW( "3DViewer.Control.rotateYcounterclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate Y Counterclockwise" ), _( "Rotate Y Counterclockwise" ),
        BITMAPS::rotate_ccw_y, AF_NONE, (void*) ROTATION_DIR::Y_CCW );

TOOL_ACTION EDA_3D_ACTIONS::rotateZCW( "3DViewer.Control.rotateZclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate Z Clockwise" ), _( "Rotate Z Clockwise" ),
        BITMAPS::rotate_cw_z, AF_NONE, (void*) ROTATION_DIR::Z_CW );

TOOL_ACTION EDA_3D_ACTIONS::rotateZCCW( "3DViewer.Control.rotateZcounterclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate Z Counterclockwise" ), _( "Rotate Z Counterclockwise" ),
        BITMAPS::rotate_ccw_z, AF_NONE, (void*) ROTATION_DIR::Z_CCW );

TOOL_ACTION EDA_3D_ACTIONS::moveLeft( "3DViewer.Control.moveLeft",
        AS_ACTIVE,
        WXK_LEFT, "",
        _( "Move board Left" ), _( "Move board Left" ),
        BITMAPS::left, AF_NONE, (void*) CURSOR_LEFT );

TOOL_ACTION EDA_3D_ACTIONS::moveRight( "3DViewer.Control.moveRight",
        AS_ACTIVE,
        WXK_RIGHT, "",
        _( "Move board Right" ), _( "Move board Right" ),
        BITMAPS::right, AF_NONE, (void*) CURSOR_RIGHT );

TOOL_ACTION EDA_3D_ACTIONS::moveUp( "3DViewer.Control.moveUp",
        AS_ACTIVE,
        WXK_UP, "",
        _( "Move board Up" ), _( "Move board Up" ),
        BITMAPS::up, AF_NONE, (void*) CURSOR_UP );

TOOL_ACTION EDA_3D_ACTIONS::moveDown( "3DViewer.Control.moveDown",
        AS_ACTIVE,
        WXK_DOWN, "",
        _( "Move board Down" ), _( "Move board Down" ),
        BITMAPS::down, AF_NONE, (void*) CURSOR_DOWN );

TOOL_ACTION EDA_3D_ACTIONS::homeView( "3DViewer.Control.homeView",
        AS_ACTIVE,
        WXK_HOME, "",
        _( "Home view" ), _( "Home view" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) WXK_HOME );

TOOL_ACTION EDA_3D_ACTIONS::resetView( "3DViewer.Control.resetView",
        AS_ACTIVE,
        'R', "",
        _( "Reset view" ), _( "Reset view" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) ID_VIEW3D_RESET );

TOOL_ACTION EDA_3D_ACTIONS::flipView( "3DViewer.Control.flipView",
        AS_ACTIVE,
        0, "",
        _( "Flip Board" ), _( "Flip the board view" ),
        BITMAPS::flip_board, AF_NONE, (void*) ID_VIEW3D_FLIP );

TOOL_ACTION EDA_3D_ACTIONS::toggleOrtho( "3DViewer.Control.toggleOrtho",
        AS_GLOBAL, 0, "",
        _( "Toggle orthographic projection" ), _( "Enable/disable orthographic projection" ),
        BITMAPS::ortho );

TOOL_ACTION EDA_3D_ACTIONS::viewFront( "3DViewer.Control.viewFront",
        AS_ACTIVE,
        'Y', "",
        _( "View Front" ), _( "View Front" ),
        BITMAPS::axis3d_front, AF_NONE, (void*) ID_VIEW3D_FRONT );

TOOL_ACTION EDA_3D_ACTIONS::viewBack( "3DViewer.Control.viewBack",
        AS_ACTIVE,
        MD_SHIFT + 'Y', "",
        _( "View Back" ), _( "View Back" ),
        BITMAPS::axis3d_back, AF_NONE, (void*) ID_VIEW3D_BACK );

TOOL_ACTION EDA_3D_ACTIONS::viewLeft( "3DViewer.Control.viewLeft",
        AS_ACTIVE,
        MD_SHIFT + 'X', "",
        _( "View Left" ), _( "View Left" ),
        BITMAPS::axis3d_left, AF_NONE, (void*) ID_VIEW3D_LEFT );

TOOL_ACTION EDA_3D_ACTIONS::viewRight( "3DViewer.Control.viewRight",
        AS_ACTIVE,
        'X', "",
        _( "View Right" ), _( "View Right" ),
        BITMAPS::axis3d_right, AF_NONE, (void*) ID_VIEW3D_RIGHT );

TOOL_ACTION EDA_3D_ACTIONS::viewTop( "3DViewer.Control.viewTop",
        AS_ACTIVE,
        'Z', "",
        _( "View Top" ), _( "View Top" ),
        BITMAPS::axis3d_top, AF_NONE, (void*) ID_VIEW3D_TOP );

TOOL_ACTION EDA_3D_ACTIONS::viewBottom( "3DViewer.Control.viewBottom",
        AS_ACTIVE,
        MD_SHIFT + 'Z', "",
        _( "View Bottom" ), _( "View Bottom" ),
        BITMAPS::axis3d_bottom, AF_NONE, (void*) ID_VIEW3D_BOTTOM );

TOOL_ACTION EDA_3D_ACTIONS::noGrid( "3DViewer.Control.noGrid",
        AS_GLOBAL, 0, "",
        _( "No 3D Grid" ), _( "No 3D Grid" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) GRID3D_TYPE::NONE );

TOOL_ACTION EDA_3D_ACTIONS::show10mmGrid( "3DViewer.Control.show10mmGrid",
        AS_GLOBAL, 0, "",
        _( "3D Grid 10mm" ), _( "3D Grid 10mm" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) GRID3D_TYPE::GRID_10MM );

TOOL_ACTION EDA_3D_ACTIONS::show5mmGrid( "3DViewer.Control.show5mmGrid",
        AS_GLOBAL, 0, "",
        _( "3D Grid 5mm" ), _( "3D Grid 5mm" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) GRID3D_TYPE::GRID_5MM );

TOOL_ACTION EDA_3D_ACTIONS::show2_5mmGrid( "3DViewer.Control.show2_5mmGrid",
        AS_GLOBAL, 0, "",
        _( "3D Grid 2.5mm" ), _( "3D Grid 2.5mm" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) GRID3D_TYPE::GRID_2P5MM );

TOOL_ACTION EDA_3D_ACTIONS::show1mmGrid( "3DViewer.Control.show1mmGrid",
        AS_GLOBAL, 0, "",
        _( "3D Grid 1mm" ), _( "3D Grid 1mm" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) GRID3D_TYPE::GRID_1MM );

TOOL_ACTION EDA_3D_ACTIONS::materialNormal( "3DViewer.Control.materialNormal",
        AS_ACTIVE,
        0, "",
        _( "Render Realistic Materials" ),
        _( "Use all material properties from each 3D model file" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) MATERIAL_MODE::NORMAL );

TOOL_ACTION EDA_3D_ACTIONS::materialDiffuse( "3DViewer.Control.materialDiffuse",
        AS_ACTIVE,
        0, "",
        _( "Render Solid Colors" ),
        _( "Use only the diffuse color property from model 3D model file" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) MATERIAL_MODE::DIFFUSE_ONLY );

TOOL_ACTION EDA_3D_ACTIONS::materialCAD( "3DViewer.Control.materialCAD",
        AS_ACTIVE,
        0, "",
        _( "Render CAD Colors" ),
        _( "Use a CAD color style based on the diffuse color of the material" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) MATERIAL_MODE::CAD_MODE );

TOOL_ACTION EDA_3D_ACTIONS::rotate45axisZ( "3DViewer.Control.rotate45axisZ",
        AS_ACTIVE,
        WXK_TAB, "",
        "Rotate 45 degrees over Z axis" );

TOOL_ACTION EDA_3D_ACTIONS::showTHT( "3DViewer.Control.attributesTHT",
        AS_ACTIVE,
        'T', "",
        _( "Toggle Through Hole 3D models" ), _( "Toggle 3D models with 'Through hole' attribute" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_FP_ATTRIBUTES_NORMAL );

TOOL_ACTION EDA_3D_ACTIONS::showSMD( "3DViewer.Control.attributesSMD",
        AS_ACTIVE,
        'S', "",
        _( "Toggle SMD 3D models" ), _( "Toggle 3D models with 'Surface mount' attribute" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_FP_ATTRIBUTES_NORMAL_INSERT );

TOOL_ACTION EDA_3D_ACTIONS::showVirtual( "3DViewer.Control.attributesVirtual",
        AS_ACTIVE,
        'V', "",
        _( "Toggle Virtual 3D models" ), _( "Toggle 3D models with 'Virtual' attribute" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_FP_ATTRIBUTES_VIRTUAL );

TOOL_ACTION EDA_3D_ACTIONS::showBBoxes( "3DViewer.Control.showBoundingBoxes",
         AS_GLOBAL, 0, "",
         _( "Show Model Bounding Boxes" ), _( "Show Model Bounding Boxes" ),
         BITMAPS::ortho, AF_NONE, (void*) FL_RENDER_OPENGL_SHOW_MODEL_BBOX );

TOOL_ACTION EDA_3D_ACTIONS::toggleRealisticMode( "3DViewer.Control.toggleRealisticMode",
        AS_GLOBAL, 0, "",
        _( "Toggle realistic mode" ), _( "Toggle realistic mode" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_USE_REALISTIC_MODE );

TOOL_ACTION EDA_3D_ACTIONS::toggleBoardBody( "3DViewer.Control.toggleBoardBody",
        AS_GLOBAL, 0, "",
        _( "Toggle board body display" ), _( "Toggle board body display" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_SHOW_BOARD_BODY );

TOOL_ACTION EDA_3D_ACTIONS::showAxis( "3DViewer.Control.showAxis",
        AS_GLOBAL, 0, "",
        _( "Show 3D Axis" ), _( "Show 3D Axis" ),
        BITMAPS::axis3d_front, AF_NONE, (void*) FL_AXIS );

TOOL_ACTION EDA_3D_ACTIONS::toggleZones( "3DViewer.Control.toggleZones",
        AS_GLOBAL, 0, "",
        _( "Toggle zone display" ), _( "Toggle zone display" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_ZONE );

TOOL_ACTION EDA_3D_ACTIONS::toggleAdhesive( "3DViewer.Control.toggleAdhesive",
        AS_GLOBAL, 0, "",
        _( "Toggle adhesive display" ), _( "Toggle display of adhesive layers" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_ADHESIVE );

TOOL_ACTION EDA_3D_ACTIONS::toggleSilk( "3DViewer.Control.toggleSilk",
        AS_GLOBAL, 0, "",
        _( "Toggle silkscreen display" ), _( "Toggle display of silkscreen layers" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_SILKSCREEN );

TOOL_ACTION EDA_3D_ACTIONS::toggleSolderMask( "3DViewer.Control.toggleSolderMask",
        AS_GLOBAL, 0, "",
        _( "Toggle solder mask display" ), _( "Toggle display of solder mask layers" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_SOLDERMASK );

TOOL_ACTION EDA_3D_ACTIONS::toggleSolderPaste( "3DViewer.Control.toggleSolderPaste",
        AS_GLOBAL, 0, "",
        _( "Toggle solder paste display" ), _( "Toggle display of solder paste layers" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_SOLDERPASTE );

TOOL_ACTION EDA_3D_ACTIONS::toggleComments( "3DViewer.Control.toggleComments",
        AS_GLOBAL, 0, "",
        _( "Toggle comments display" ), _( "Toggle display of comments and drawings layers" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_COMMENTS );

TOOL_ACTION EDA_3D_ACTIONS::toggleECO( "3DViewer.Control.toggleECO",
        AS_GLOBAL, 0, "",
        _( "Toggle ECO display" ), _( "Toggle display of ECO layers" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) FL_ECO );

