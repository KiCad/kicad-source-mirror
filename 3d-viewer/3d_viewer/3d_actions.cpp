/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "3d_actions.h"


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s


TOOL_ACTION EDA_3D_ACTIONS::pivotCenter( "3DViewer.Control.pivotCenter",
        AS_GLOBAL,
        ' ', "",
        "Center pivot rotation (Middle mouse click)" );

TOOL_ACTION EDA_3D_ACTIONS::moveLeft( "3DViewer.Control.moveLeft",
        AS_GLOBAL,
        WXK_LEFT, "",
        "Move board Left" );

TOOL_ACTION EDA_3D_ACTIONS::moveRight( "3DViewer.Control.moveRight",
        AS_GLOBAL,
        WXK_RIGHT, "",
        "Move board Right" );

TOOL_ACTION EDA_3D_ACTIONS::moveUp( "3DViewer.Control.moveUp",
        AS_GLOBAL,
        WXK_UP, "",
        "Move board Up" );

TOOL_ACTION EDA_3D_ACTIONS::moveDown( "3DViewer.Control.moveDown",
        AS_GLOBAL,
        WXK_DOWN, "",
        "Move board Down" );

TOOL_ACTION EDA_3D_ACTIONS::homeView( "3DViewer.Control.homeView",
        AS_GLOBAL,
        WXK_HOME, "",
        "Home view" );

TOOL_ACTION EDA_3D_ACTIONS::resetView( "3DViewer.Control.resetView",
        AS_GLOBAL,
        'R', "",
        "Reset view" );

TOOL_ACTION EDA_3D_ACTIONS::viewFront( "3DViewer.Control.viewFront",
        AS_GLOBAL,
        'Y', "",
        "View Front" );

TOOL_ACTION EDA_3D_ACTIONS::viewBack( "3DViewer.Control.viewBack",
        AS_GLOBAL,
        MD_SHIFT + 'Y', "",
        "View Back" );

TOOL_ACTION EDA_3D_ACTIONS::viewLeft( "3DViewer.Control.viewLeft",
        AS_GLOBAL,
        MD_SHIFT + 'X', "",
        "View Left" );

TOOL_ACTION EDA_3D_ACTIONS::viewRight( "3DViewer.Control.viewRight",
        AS_GLOBAL,
        'X', "",
        "View Right" );

TOOL_ACTION EDA_3D_ACTIONS::viewTop( "3DViewer.Control.viewTop",
        AS_GLOBAL,
        'Z', "",
        "View Top" );

TOOL_ACTION EDA_3D_ACTIONS::viewBottom( "3DViewer.Control.viewBottom",
        AS_GLOBAL,
        MD_SHIFT + 'Z', "",
        "View Bottom" );

TOOL_ACTION EDA_3D_ACTIONS::rotate45axisZ( "3DViewer.Control.rotate45axisZ",
        AS_GLOBAL,
        WXK_TAB, "",
        "Rotate 45 degrees over Z axis" );

TOOL_ACTION EDA_3D_ACTIONS::zoomIn( "3DViewer.Control.zoomIn",
        AS_GLOBAL,
        WXK_F1, "",
        "Zoom in " );

TOOL_ACTION EDA_3D_ACTIONS::zoomOut( "3DViewer.Control.zoomOut",
        AS_GLOBAL,
        WXK_F2, "",
        "Zoom out" );

TOOL_ACTION EDA_3D_ACTIONS::attributesTHT( "3DViewer.Control.attributesTHT",
        AS_GLOBAL,
        'T', "",
        "Toggle 3D models with type Through Hole" );

TOOL_ACTION EDA_3D_ACTIONS::attributesSMD( "3DViewer.Control.attributesSMD",
        AS_GLOBAL,
        'S', "",
        "Toggle 3D models with type Surface Mount" );

TOOL_ACTION EDA_3D_ACTIONS::attributesVirtual( "3DViewer.Control.attributesVirtual",
        AS_GLOBAL,
        'V', "",
        "Toggle 3D models with type Virtual" );


