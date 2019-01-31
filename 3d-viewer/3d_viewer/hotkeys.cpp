/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  hotkeys.cpp
 * @brief list of hotkeys used in 3D viewer
 */

#include <hotkeys_basic.h>
#include "eda_3d_viewer.h"
#include "../3d_viewer_id.h"

// Define 3D Viewer Hotkeys
static EDA_HOTKEY HkHotkeysHelp( _HKI( "List Hotkeys" ), HK_HELP, GR_KB_CTRL + WXK_F1 );
static EDA_HOTKEY Hk3D_PivotCenter( _HKI( "Center pivot rotation (Middle mouse click)" ), 0, WXK_SPACE );
static EDA_HOTKEY Hk3D_MoveLeft( _HKI( "Move board Left" ), ID_POPUP_MOVE3D_LEFT, WXK_LEFT );
static EDA_HOTKEY Hk3D_MoveRight( _HKI( "Move board Right" ), ID_POPUP_MOVE3D_RIGHT, WXK_RIGHT );
static EDA_HOTKEY Hk3D_MoveUp( _HKI( "Move board Up" ), ID_POPUP_MOVE3D_UP, WXK_UP );
static EDA_HOTKEY Hk3D_MoveDown( _HKI( "Move board Down" ), ID_POPUP_MOVE3D_DOWN, WXK_DOWN );
static EDA_HOTKEY Hk3D_HomeView( _HKI( "Home view" ), 0, WXK_HOME );
static EDA_HOTKEY Hk3D_ResetView( _HKI( "Reset view" ), 0, 'R' );

static EDA_HOTKEY Hk3D_ViewFront( _HKI( "View Front" ), ID_POPUP_VIEW_YPOS, 'Y' );
static EDA_HOTKEY Hk3D_ViewBack( _HKI( "View Back" ), ID_POPUP_VIEW_YNEG, GR_KB_SHIFT + 'Y' );
static EDA_HOTKEY Hk3D_ViewLeft( _HKI( "View Left" ), ID_POPUP_VIEW_XNEG, GR_KB_SHIFT + 'X' );
static EDA_HOTKEY Hk3D_ViewRight( _HKI( "View Right" ), ID_POPUP_VIEW_XPOS, 'X' );
static EDA_HOTKEY Hk3D_ViewTop( _HKI( "View Top" ), ID_POPUP_VIEW_ZPOS, 'Z' );
static EDA_HOTKEY Hk3D_ViewBot( _HKI( "View Bot" ), ID_POPUP_VIEW_ZNEG, GR_KB_SHIFT + 'Z' );

static EDA_HOTKEY Hk3D_Rotate45axisZ( _HKI( "Rotate 45 degrees over Z axis" ), 0, WXK_TAB );
static EDA_HOTKEY Hk3D_ZoomIn( _HKI( "Zoom in " ), ID_POPUP_ZOOMIN, WXK_F1 );
static EDA_HOTKEY Hk3D_ZoomOut( _HKI( "Zoom out" ), ID_POPUP_ZOOMOUT, WXK_F2 );
static EDA_HOTKEY Hk3D_AttributesTHT( _HKI( "Toggle 3D models with type Through Hole" ), 0, 'T' );
static EDA_HOTKEY Hk3D_AttributesSMD( _HKI( "Toggle 3D models with type Surface Mount" ), 0, 'S' );
static EDA_HOTKEY Hk3D_AttributesVirtual( _HKI( "Toggle 3D models with type Virtual" ), 0, 'V' );

static wxString viewer3DSectionTitle( _HKI( "Viewer 3D" ) );

// List of hotkey descriptors for the 3D Viewer only
// !TODO: this is used just for help menu, the structured are not used yet in the viewer
static EDA_HOTKEY* viewer3d_Hotkey_List[] =
{
    &HkHotkeysHelp,
    &Hk3D_PivotCenter,
    &Hk3D_MoveLeft,
    &Hk3D_MoveRight,
    &Hk3D_MoveUp,
    &Hk3D_MoveDown,
    &Hk3D_HomeView,
    &Hk3D_ResetView,
    &Hk3D_ViewFront,
    &Hk3D_ViewBack,
    &Hk3D_ViewLeft,
    &Hk3D_ViewRight,
    &Hk3D_ViewTop,
    &Hk3D_ViewBot,
    &Hk3D_Rotate45axisZ,
    &Hk3D_ZoomIn,
    &Hk3D_ZoomOut,
    &Hk3D_AttributesTHT,
    &Hk3D_AttributesSMD,
    &Hk3D_AttributesVirtual,
    NULL
};


// list of sections and corresponding hotkey list for the 3D Viewer
// (used to list current hotkeys)
static struct EDA_HOTKEY_CONFIG s_3DViewer_Hotkeys_Descr[] =
{
    { &g_CommonSectionTag, viewer3d_Hotkey_List, &viewer3DSectionTitle },
    { NULL,                NULL,                 NULL }
};


EDA_HOTKEY_CONFIG* EDA_3D_VIEWER::GetHotkeyConfig() const
{
    return s_3DViewer_Hotkeys_Descr;
}


EDA_HOTKEY_CONFIG* EDA_3D_CANVAS::GetHotkeyConfig() const
{
    return s_3DViewer_Hotkeys_Descr;
}
