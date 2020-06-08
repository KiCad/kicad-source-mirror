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
#include "3d_actions.h"


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s


TOOL_ACTION EDA_3D_ACTIONS::controlActivate( "3DViewer.Control", AS_GLOBAL,
        0, "", "", "",
        NULL, AF_ACTIVATE ); // No description, it is not supposed to be shown anywhere

TOOL_ACTION EDA_3D_ACTIONS::pivotCenter( "3DViewer.Control.pivotCenter",
        AS_ACTIVE,
        ' ', "",
        _( "Center pivot rotation" ), _( "Center pivot rotation (middle mouse click)" ),
        nullptr, AF_NONE, (void*) WXK_SPACE );

TOOL_ACTION EDA_3D_ACTIONS::rotateXCW( "3DViewer.Control.rotateXclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate X Clockwise" ), _( "Rotate X Clockwise" ),
        rotate_neg_x_xpm, AF_NONE, (void*) ID_ROTATE3D_X_NEG );

TOOL_ACTION EDA_3D_ACTIONS::rotateXCCW( "3DViewer.Control.rotateXcounterclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate X Counterclockwise" ), _( "Rotate X Counterclockwise" ),
        rotate_pos_x_xpm, AF_NONE, (void*) ID_ROTATE3D_X_POS );

TOOL_ACTION EDA_3D_ACTIONS::rotateYCW( "3DViewer.Control.rotateYclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate Y Clockwise" ), _( "Rotate Y Clockwise" ),
        rotate_neg_y_xpm, AF_NONE, (void*) ID_ROTATE3D_Y_NEG );

TOOL_ACTION EDA_3D_ACTIONS::rotateYCCW( "3DViewer.Control.rotateYcounterclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate Y Counterclockwise" ), _( "Rotate Y Counterclockwise" ),
        rotate_pos_y_xpm, AF_NONE, (void*) ID_ROTATE3D_Y_POS );

TOOL_ACTION EDA_3D_ACTIONS::rotateZCW( "3DViewer.Control.rotateZclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate Z Clockwise" ), _( "Rotate Z Clockwise" ),
        rotate_neg_z_xpm, AF_NONE, (void*) ID_ROTATE3D_Z_NEG );

TOOL_ACTION EDA_3D_ACTIONS::rotateZCCW( "3DViewer.Control.rotateZcounterclockwise",
        AS_GLOBAL, 0, "",
        _( "Rotate Z Counterclockwise" ), _( "Rotate Z Counterclockwise" ),
        rotate_pos_z_xpm, AF_NONE, (void*) ID_ROTATE3D_Z_POS );

TOOL_ACTION EDA_3D_ACTIONS::moveLeft( "3DViewer.Control.moveLeft",
        AS_ACTIVE,
        WXK_LEFT, "",
        _( "Move board Left" ), _( "Move board Left" ),
        left_xpm, AF_NONE, (void*) CURSOR_LEFT );

TOOL_ACTION EDA_3D_ACTIONS::moveRight( "3DViewer.Control.moveRight",
        AS_ACTIVE,
        WXK_RIGHT, "",
        _( "Move board Right" ), _( "Move board Right" ),
        right_xpm, AF_NONE, (void*) CURSOR_RIGHT );

TOOL_ACTION EDA_3D_ACTIONS::moveUp( "3DViewer.Control.moveUp",
        AS_ACTIVE,
        WXK_UP, "",
        _( "Move board Up" ), _( "Move board Up" ),
        up_xpm, AF_NONE, (void*) CURSOR_UP );

TOOL_ACTION EDA_3D_ACTIONS::moveDown( "3DViewer.Control.moveDown",
        AS_ACTIVE,
        WXK_DOWN, "",
        _( "Move board Down" ), _( "Move board Down" ),
        down_xpm, AF_NONE, (void*) CURSOR_DOWN );

TOOL_ACTION EDA_3D_ACTIONS::homeView( "3DViewer.Control.homeView",
        AS_ACTIVE,
        WXK_HOME, "",
        _( "Home view" ), _( "Home view" ),
        nullptr, AF_NONE, (void*) WXK_HOME );

TOOL_ACTION EDA_3D_ACTIONS::resetView( "3DViewer.Control.resetView",
        AS_ACTIVE,
        'R', "",
        _( "Reset view" ), _( "Reset view" ),
        nullptr, AF_NONE, (void*) ID_VIEW3D_RESET );

TOOL_ACTION EDA_3D_ACTIONS::flipView( "3DViewer.Control.flipView",
        AS_ACTIVE,
        0, "",
        _( "Flip Board" ), _( "Flip the board view" ),
        reload2_xpm, AF_NONE, (void*) ID_VIEW3D_FLIP );

TOOL_ACTION EDA_3D_ACTIONS::toggleOrtho( "3DViewer.Control.toggleOrtho",
        AS_GLOBAL, 0, "",
        _( "Toggle orthographic projection" ), _( "Enable/disable orthographic projection" ),
        ortho_xpm );

TOOL_ACTION EDA_3D_ACTIONS::viewFront( "3DViewer.Control.viewFront",
        AS_ACTIVE,
        'Y', "",
        _( "View Front" ), _( "View Front" ),
        axis3d_front_xpm, AF_NONE, (void*) ID_VIEW3D_FRONT );

TOOL_ACTION EDA_3D_ACTIONS::viewBack( "3DViewer.Control.viewBack",
        AS_ACTIVE,
        MD_SHIFT + 'Y', "",
        _( "View Back" ), _( "View Back" ),
        axis3d_back_xpm, AF_NONE, (void*) ID_VIEW3D_BACK );

TOOL_ACTION EDA_3D_ACTIONS::viewLeft( "3DViewer.Control.viewLeft",
        AS_ACTIVE,
        MD_SHIFT + 'X', "",
        _( "View Left" ), _( "View Left" ),
        axis3d_left_xpm, AF_NONE, (void*) ID_VIEW3D_LEFT );

TOOL_ACTION EDA_3D_ACTIONS::viewRight( "3DViewer.Control.viewRight",
        AS_ACTIVE,
        'X', "",
        _( "View Right" ), _( "View Right" ),
        axis3d_right_xpm, AF_NONE, (void*) ID_VIEW3D_RIGHT );

TOOL_ACTION EDA_3D_ACTIONS::viewTop( "3DViewer.Control.viewTop",
        AS_ACTIVE,
        'Z', "",
        _( "View Top" ), _( "View Top" ),
        axis3d_top_xpm, AF_NONE, (void*) ID_VIEW3D_TOP );

TOOL_ACTION EDA_3D_ACTIONS::viewBottom( "3DViewer.Control.viewBottom",
        AS_ACTIVE,
        MD_SHIFT + 'Z', "",
        _( "View Bottom" ), _( "View Bottom" ),
        axis3d_bottom_xpm, AF_NONE, (void*) ID_VIEW3D_BOTTOM );

TOOL_ACTION EDA_3D_ACTIONS::noGrid( "3DViewer.Control.noGrid",
        AS_GLOBAL, 0, "",
        _( "No 3D Grid" ), _( "No 3D Grid" ),
        nullptr, AF_NONE, (void*) GRID3D_TYPE::NONE );

TOOL_ACTION EDA_3D_ACTIONS::show10mmGrid( "3DViewer.Control.show10mmGrid",
        AS_GLOBAL, 0, "",
        _( "3D Grid 10mm" ), _( "3D Grid 10mm" ),
        nullptr, AF_NONE, (void*) GRID3D_TYPE::GRID_10MM );

TOOL_ACTION EDA_3D_ACTIONS::show5mmGrid( "3DViewer.Control.show5mmGrid",
        AS_GLOBAL, 0, "",
        _( "3D Grid 5mm" ), _( "3D Grid 5mm" ),
        nullptr, AF_NONE, (void*) GRID3D_TYPE::GRID_5MM );

TOOL_ACTION EDA_3D_ACTIONS::show2_5mmGrid( "3DViewer.Control.show2_5mmGrid",
        AS_GLOBAL, 0, "",
        _( "3D Grid 2.5mm" ), _( "3D Grid 2.5mm" ),
        nullptr, AF_NONE, (void*) GRID3D_TYPE::GRID_2P5MM );

TOOL_ACTION EDA_3D_ACTIONS::show1mmGrid( "3DViewer.Control.show1mmGrid",
        AS_GLOBAL, 0, "",
        _( "3D Grid 1mm" ), _( "3D Grid 1mm" ),
        nullptr, AF_NONE, (void*) GRID3D_TYPE::GRID_1MM );

TOOL_ACTION EDA_3D_ACTIONS::rotate45axisZ( "3DViewer.Control.rotate45axisZ",
        AS_ACTIVE,
        WXK_TAB, "",
        "Rotate 45 degrees over Z axis" );

TOOL_ACTION EDA_3D_ACTIONS::attributesTHT( "3DViewer.Control.attributesTHT",
        AS_ACTIVE,
        'T', "",
        _( "Toggle Through Hole 3D models" ),  _( "Toggle 3D models with 'Through hole' attribute" ),
        nullptr, AF_NONE, (void*) FL_MODULE_ATTRIBUTES_NORMAL );

TOOL_ACTION EDA_3D_ACTIONS::attributesSMD( "3DViewer.Control.attributesSMD",
        AS_ACTIVE,
        'S', "",
        _( "Toggle SMD 3D models" ), _( "Toggle 3D models with 'Surface mount' attribute" ),
        nullptr, AF_NONE, (void*) FL_MODULE_ATTRIBUTES_NORMAL_INSERT );

TOOL_ACTION EDA_3D_ACTIONS::attributesVirtual( "3DViewer.Control.attributesVirtual",
        AS_ACTIVE,
        'V', "",
        _( "Toggle Virtual 3D models" ), _( "Toggle 3D models with 'Virtual' attribute" ),
        nullptr, AF_NONE, (void*) FL_MODULE_ATTRIBUTES_VIRTUAL );

TOOL_ACTION EDA_3D_ACTIONS::showBoundingBoxes( "3DViewer.Control.showBoundingBoxes",
         AS_GLOBAL, 0, "",
         _( "Show Model Bounding Boxes" ), _( "Show Model Bounding Boxes" ),
         ortho_xpm, AF_NONE, (void*) FL_RENDER_OPENGL_SHOW_MODEL_BBOX );

TOOL_ACTION EDA_3D_ACTIONS::renderShadows( "3DViewer.Control.renderShadows",
         AS_GLOBAL, 0, "",
         _( "Render Shadows" ), _( "Render Shadows" ),
         nullptr, AF_NONE, (void*) FL_RENDER_RAYTRACING_SHADOWS );

TOOL_ACTION EDA_3D_ACTIONS::proceduralTextures( "3DViewer.Control.proceduralTextures",
        AS_GLOBAL, 0, "",
        _( "Procedural Textures" ), _( "Apply procedural textures to materials (slow)" ),
        nullptr, AF_NONE, (void*) FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES );

TOOL_ACTION EDA_3D_ACTIONS::addFloor( "3DViewer.Control.addFloor",
        AS_GLOBAL, 0, "",
        _( "Add Floor" ), _( "Adds a floor plane below the board (slow)"),
        nullptr, AF_NONE, (void*) FL_RENDER_RAYTRACING_BACKFLOOR );

TOOL_ACTION EDA_3D_ACTIONS::showRefractions( "3DViewer.Control.showRefractions",
         AS_GLOBAL, 0, "",
         _( "Refractions" ), _( "Render materials with refractive properties on final render (slow)" ),
         nullptr, AF_NONE, (void*) FL_RENDER_RAYTRACING_REFRACTIONS );

TOOL_ACTION EDA_3D_ACTIONS::showReflections( "3DViewer.Control.showReflections",
         AS_GLOBAL, 0, "",
         _( "Reflections" ), _( "Render materials with reflective properties on final render (slow)" ),
         nullptr, AF_NONE, (void*) FL_RENDER_RAYTRACING_REFLECTIONS );

TOOL_ACTION EDA_3D_ACTIONS::antiAliasing( "3DViewer.Control.antiAliasing",
         AS_GLOBAL, 0, "",
         _( "Anti-aliasing" ), _( "Render with improved quality on final render (slow)" ),
         nullptr, AF_NONE, (void*) FL_RENDER_RAYTRACING_ANTI_ALIASING );

TOOL_ACTION EDA_3D_ACTIONS::postProcessing( "3DViewer.Control.postProcessing",
        AS_GLOBAL, 0, "",
        _( "Post-processing" ),
        _( "Apply Screen Space Ambient Occlusion and Global Illumination reflections on final render (slow)"),
        nullptr, AF_NONE, (void*) FL_RENDER_RAYTRACING_POST_PROCESSING );

TOOL_ACTION EDA_3D_ACTIONS::toggleRealisticMode( "3DViewer.Control.toggleRealisticMode",
        AS_GLOBAL, 0, "",
        _( "Toggle realistic mode" ), _( "Toggle realistic mode" ),
        nullptr, AF_NONE, (void*) FL_USE_REALISTIC_MODE );

TOOL_ACTION EDA_3D_ACTIONS::toggleBoardBody( "3DViewer.Control.toggleBoardBody",
        AS_GLOBAL, 0, "",
        _( "Toggle board body display" ), _( "Toggle board body display" ),
        nullptr, AF_NONE, (void*) FL_SHOW_BOARD_BODY );

TOOL_ACTION EDA_3D_ACTIONS::showAxis( "3DViewer.Control.showAxis",
        AS_GLOBAL, 0, "",
        _( "Show 3D Axis" ), _( "Show 3D Axis" ),
        axis3d_front_xpm, AF_NONE, (void*) FL_AXIS );

TOOL_ACTION EDA_3D_ACTIONS::toggleZones( "3DViewer.Control.toggleZones",
        AS_GLOBAL, 0, "",
        _( "Toggle zone display" ), _( "Toggle zone display" ),
        nullptr, AF_NONE, (void*) FL_ZONE );

TOOL_ACTION EDA_3D_ACTIONS::toggleAdhesive( "3DViewer.Control.toggleAdhesive",
        AS_GLOBAL, 0, "",
        _( "Toggle adhesive display" ), _( "Toggle display of adhesive layers" ),
        nullptr, AF_NONE, (void*) FL_ADHESIVE );

TOOL_ACTION EDA_3D_ACTIONS::toggleSilk( "3DViewer.Control.toggleSilk",
        AS_GLOBAL, 0, "",
        _( "Toggle silkscreen display" ), _( "Toggle display of silkscreen layers" ),
        nullptr, AF_NONE, (void*) FL_SILKSCREEN );

TOOL_ACTION EDA_3D_ACTIONS::toggleSolderMask( "3DViewer.Control.toggleSolderMask",
        AS_GLOBAL, 0, "",
        _( "Toggle solder mask display" ), _( "Toggle display of solder mask layers" ),
        nullptr, AF_NONE, (void*) FL_SOLDERMASK );

TOOL_ACTION EDA_3D_ACTIONS::toggleSolderPaste( "3DViewer.Control.toggleSolderPaste",
        AS_GLOBAL, 0, "",
        _( "Toggle solder paste display" ), _( "Toggle display of solder paste layers" ),
        nullptr, AF_NONE, (void*) FL_SOLDERPASTE );

TOOL_ACTION EDA_3D_ACTIONS::toggleComments( "3DViewer.Control.toggleComments",
        AS_GLOBAL, 0, "",
        _( "Toggle comments display" ), _( "Toggle display of comments and drawings layers" ),
        nullptr, AF_NONE, (void*) FL_COMMENTS );

TOOL_ACTION EDA_3D_ACTIONS::toggleECO( "3DViewer.Control.toggleECO",
        AS_GLOBAL, 0, "",
        _( "Toggle ECO display" ), _( "Toggle display of ECO layers" ),
        nullptr, AF_NONE, (void*) FL_ECO );

