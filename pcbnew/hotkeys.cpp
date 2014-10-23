/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew/hotkeys.cpp
 */

#include <fctsys.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <modview_frame.h>
#include <pcbnew_id.h>

#include <hotkeys.h>

/* How to add a new hotkey:
 *  add a new id in the enum hotkey_id_commnand like MY_NEW_ID_FUNCTION.
 *  add a new EDA_HOTKEY entry like:
 *  static EDA_HOTKEY HkMyNewEntry(wxT("Command Label"), MY_NEW_ID_FUNCTION, default key value);
 *      "Command Label" is the name used in hotkey list display, and the identifier in the
 *      hotkey list file MY_NEW_ID_FUNCTION is an equivalent id function used in the switch
 *      in OnHotKey() function.
 *      default key value is the default hotkey for this command. Can be overridden by the user
 *       hotkey list file
 *  add the HkMyNewEntry pointer in the s_board_edit_Hotkey_List list ( or/and the
 *  s_module_edit_Hotkey_List list)
 *  Add the new code in the switch in OnHotKey() function.
 *      Note: when the variable itemCurrentlyEdited is true, an item is currently edited.
 *      This can be useful if the new function cannot be executed while an item is currently
 *      being edited ( For example, one cannot start a new wire when a component is moving.)
 *
 *  Note: If a hotkey is a special key, be sure the corresponding wxWidget keycode (WXK_XXXX)
 *  is handled in the hotkey_name_descr s_Hotkey_Name_List list (see hotkeys_basic.cpp)
 *  and see this list for some ascii keys (space ...)
 */

// Hotkey list:

// mouse click command:
static EDA_HOTKEY HkMouseLeftClick( wxT( "Mouse Left Click" ),
                                    HK_LEFT_CLICK, WXK_RETURN, 0 );
static EDA_HOTKEY HkMouseLeftDClick( wxT( "Mouse Left Double Click" ),
                                     HK_LEFT_DCLICK, WXK_END, 0 );

static EDA_HOTKEY HkSwitch2CopperLayer( wxT( "Switch to Copper (B.Cu) layer" ),
                                        HK_SWITCH_LAYER_TO_COPPER, WXK_PAGEDOWN );

static EDA_HOTKEY HkSwitch2ComponentLayer( wxT( "Switch to Component (F.Cu) layer" ),
                                           HK_SWITCH_LAYER_TO_COMPONENT, WXK_PAGEUP );

static EDA_HOTKEY HkSwitch2InnerLayer1( wxT( "Switch to Inner layer 1" ),
                                        HK_SWITCH_LAYER_TO_INNER1, WXK_F5 );
static EDA_HOTKEY HkSwitch2InnerLayer2( wxT( "Switch to Inner layer 2" ),
                                        HK_SWITCH_LAYER_TO_INNER2, WXK_F6 );
static EDA_HOTKEY HkSwitch2InnerLayer3( wxT( "Switch to Inner layer 3" ),
                                        HK_SWITCH_LAYER_TO_INNER3, WXK_F7 );
static EDA_HOTKEY HkSwitch2InnerLayer4( wxT( "Switch to Inner layer 4" ),
                                        HK_SWITCH_LAYER_TO_INNER4, WXK_F8 );
static EDA_HOTKEY HkSwitch2InnerLayer5( wxT( "Switch to Inner layer 5" ),
                                        HK_SWITCH_LAYER_TO_INNER5, WXK_F9 );
static EDA_HOTKEY HkSwitch2InnerLayer6( wxT( "Switch to Inner layer 6" ),
                                        HK_SWITCH_LAYER_TO_INNER6, WXK_F10 );

static EDA_HOTKEY HkSwitch2NextCopperLayer( wxT( "Switch to Next Layer" ),
                                            HK_SWITCH_LAYER_TO_NEXT, '+' );
static EDA_HOTKEY HkSwitch2PreviousCopperLayer( wxT( "Switch to Previous Layer" ),
                                                HK_SWITCH_LAYER_TO_PREVIOUS, '-' );

static EDA_HOTKEY HkSaveModule( wxT( "Save Module" ), HK_SAVE_MODULE, 'S' + GR_KB_CTRL );
static EDA_HOTKEY HkSavefile( wxT( "Save Board" ), HK_SAVE_BOARD, 'S' + GR_KB_CTRL );
static EDA_HOTKEY HkSavefileAs( wxT( "Save Board As" ), HK_SAVE_BOARD_AS, 'S' + GR_KB_CTRL + GR_KB_SHIFT );
static EDA_HOTKEY HkLoadfile( wxT( "Load Board" ), HK_LOAD_BOARD, 'L' + GR_KB_CTRL );
static EDA_HOTKEY HkFindItem( wxT( "Find Item" ), HK_FIND_ITEM, 'F' + GR_KB_CTRL );
static EDA_HOTKEY HkBackspace( wxT( "Delete Track Segment" ), HK_BACK_SPACE, WXK_BACK );
static EDA_HOTKEY HkAddNewTrack( wxT( "Add New Track" ), HK_ADD_NEW_TRACK, 'X' );
static EDA_HOTKEY HkAddThroughVia( wxT( "Add Through Via" ), HK_ADD_THROUGH_VIA, 'V' );
static EDA_HOTKEY HkSelLayerAndAddThroughVia( wxT( "Select Layer and Add Through Via" ),
                                              HK_SEL_LAYER_AND_ADD_THROUGH_VIA, '<' );
static EDA_HOTKEY HkAddMicroVia( wxT( "Add MicroVia" ), HK_ADD_MICROVIA, 'V' + GR_KB_CTRL );
static EDA_HOTKEY HkAddBlindBuriedVia( wxT( "Add Blind/Buried Via" ), HK_ADD_BLIND_BURIED_VIA, 'V' + GR_KB_ALT );
static EDA_HOTKEY HkSelLayerAndAddBlindBuriedVia( wxT( "Select Layer and Add Blind/Buried Via" ),
                                                  HK_SEL_LAYER_AND_ADD_BLIND_BURIED_VIA, '<' + GR_KB_ALT );
static EDA_HOTKEY HkSwitchTrackPosture( wxT( "Switch Track Posture" ),  HK_SWITCH_TRACK_POSTURE, '/' );
static EDA_HOTKEY HkDragTrackKeepSlope( wxT( "Drag Track Keep Slope" ), HK_DRAG_TRACK_KEEP_SLOPE, 'D' );
static EDA_HOTKEY HkPlaceItem( wxT( "Place Item" ), HK_PLACE_ITEM, 'P' );
static EDA_HOTKEY HkEditBoardItem( wxT( "Edit Item" ), HK_EDIT_ITEM, 'E' );
static EDA_HOTKEY HkFlipItem( wxT( "Flip Item" ), HK_FLIP_ITEM, 'F' );
static EDA_HOTKEY HkRotateItem( wxT( "Rotate Item" ), HK_ROTATE_ITEM, 'R' );
static EDA_HOTKEY HkMoveItem( wxT( "Move Item" ), HK_MOVE_ITEM, 'M' );
static EDA_HOTKEY HkCopyItem( wxT( "Copy Item" ), HK_COPY_ITEM, 'C' );
static EDA_HOTKEY HkDragFootprint( wxT( "Drag Footprint" ), HK_DRAG_ITEM, 'G' );
static EDA_HOTKEY HkGetAndMoveFootprint( wxT( "Get and Move Footprint" ), HK_GET_AND_MOVE_FOOTPRINT, 'T' );
static EDA_HOTKEY HkLock_Unlock_Footprint( wxT( "Lock/Unlock Footprint" ), HK_LOCK_UNLOCK_FOOTPRINT, 'L' );
static EDA_HOTKEY HkDelete( wxT( "Delete Track or Footprint" ), HK_DELETE, WXK_DELETE );
static EDA_HOTKEY HkResetLocalCoord( wxT( "Reset Local Coordinates" ), HK_RESET_LOCAL_COORD, ' ' );
static EDA_HOTKEY HkSwitchHighContrastMode( wxT( "Toggle High Contrast Mode" ), HK_SWITCH_HIGHCONTRAST_MODE,'H');

static EDA_HOTKEY HkSetGridOrigin( wxT( "Set Grid Origin" ), HK_SET_GRID_ORIGIN, 'S' );
static EDA_HOTKEY HkResetGridOrigin( wxT( "Reset Grid Origin" ), HK_RESET_GRID_ORIGIN, 'Z' );

static EDA_HOTKEY HkCanvasDefault( wxT( "Switch to Default Canvas" ),
                                   HK_CANVAS_DEFAULT, WXK_F9 );
static EDA_HOTKEY HkCanvasOpenGL( wxT( "Switch to OpenGL Canvas" ),
                                  HK_CANVAS_OPENGL, WXK_F11 );
static EDA_HOTKEY HkCanvasCairo( wxT( "Switch to Cairo Canvas" ),
                                 HK_CANVAS_CAIRO, WXK_F12 );

/* Fit on Screen */
#if !defined( __WXMAC__ )
static EDA_HOTKEY HkZoomAuto( wxT( "Zoom Auto" ), HK_ZOOM_AUTO, WXK_HOME );
#else
static EDA_HOTKEY HkZoomAuto( wxT( "Zoom Auto" ), HK_ZOOM_AUTO, GR_KB_CTRL + '0' );
#endif

static EDA_HOTKEY HkZoomCenter( wxT( "Zoom Center" ), HK_ZOOM_CENTER, WXK_F4 );

/* Refresh Screen */
#if !defined( __WXMAC__ )
static EDA_HOTKEY HkZoomRedraw( wxT( "Zoom Redraw" ), HK_ZOOM_REDRAW, WXK_F3 );
#else
static EDA_HOTKEY HkZoomRedraw( wxT( "Zoom Redraw" ), HK_ZOOM_REDRAW, GR_KB_CTRL + 'R' );
#endif

/* Zoom In */
#if !defined( __WXMAC__ )
static EDA_HOTKEY HkZoomIn( wxT( "Zoom In" ), HK_ZOOM_IN, WXK_F1 );
#else
static EDA_HOTKEY HkZoomIn( wxT( "Zoom In" ), HK_ZOOM_IN, GR_KB_CTRL + '+' );
#endif

/* Zoom Out */
#if !defined( __WXMAC__ )
static EDA_HOTKEY HkZoomOut( wxT( "Zoom Out" ), HK_ZOOM_OUT, WXK_F2 );
#else
static EDA_HOTKEY HkZoomOut( wxT( "Zoom Out" ), HK_ZOOM_OUT, GR_KB_CTRL + '-' );
#endif

static EDA_HOTKEY HkHelp( wxT( "Help (this window)" ), HK_HELP, '?' );


/* Undo */
static EDA_HOTKEY HkUndo( wxT( "Undo" ), HK_UNDO, GR_KB_CTRL + 'Z', (int) wxID_UNDO );

/* Redo */
#if !defined( __WXMAC__ )
static EDA_HOTKEY HkRedo( wxT( "Redo" ), HK_REDO, GR_KB_CTRL + 'Y', (int) wxID_REDO );
#else
static EDA_HOTKEY HkRedo( wxT( "Redo" ), HK_REDO,
                          GR_KB_SHIFT + GR_KB_CTRL + 'Z',
                          (int) wxID_REDO );
#endif

static EDA_HOTKEY HkSwitchTrackWidthToNext( wxT( "Switch Track Width To Next" ),
                                            HK_SWITCH_TRACK_WIDTH_TO_NEXT, 'W' );

static EDA_HOTKEY HkSwitchTrackWidthToPrevious( wxT( "Switch Track Width To Previous" ),
                                                HK_SWITCH_TRACK_WIDTH_TO_PREVIOUS, 'W'
                                                + GR_KB_CTRL );

static EDA_HOTKEY HkSwitchGridToFastGrid1( wxT( "Switch Grid To Fast Grid1" ),
                                           HK_SWITCH_GRID_TO_FASTGRID1, GR_KB_ALT + '1' );

static EDA_HOTKEY HkSwitchGridToFastGrid2( wxT( "Switch Grid To Fast Grid2" ),
                                           HK_SWITCH_GRID_TO_FASTGRID2, GR_KB_ALT + '2' );

static EDA_HOTKEY HkSwitchGridToNext( wxT( "Switch Grid To Next" ),
                                      HK_SWITCH_GRID_TO_NEXT, '`' );

static EDA_HOTKEY HkSwitchGridToPrevious( wxT( "Switch Grid To Previous" ),
                                          HK_SWITCH_GRID_TO_PREVIOUS, '`' + GR_KB_CTRL );

static EDA_HOTKEY HkSwitchUnits( wxT( "Switch Units" ), HK_SWITCH_UNITS, 'U' + GR_KB_CTRL );
static EDA_HOTKEY HkTrackDisplayMode( wxT( "Track Display Mode" ),
                                      HK_SWITCH_TRACK_DISPLAY_MODE, 'K' );
static EDA_HOTKEY HkAddModule( wxT( "Add Module" ), HK_ADD_MODULE, 'O' );

/* Record and play macros */
static EDA_HOTKEY HkRecordMacros0( wxT( "Record Macro 0" ), HK_RECORD_MACROS_0, GR_KB_CTRL+'0' );

static EDA_HOTKEY HkCallMacros0( wxT( "Call Macro 0" ), HK_CALL_MACROS_0, '0' );

static EDA_HOTKEY HkRecordMacros1( wxT( "Record Macro 1" ), HK_RECORD_MACROS_1, GR_KB_CTRL+'1' );

static EDA_HOTKEY HkCallMacros1( wxT( "Call Macro 1" ), HK_CALL_MACROS_1, '1' );

static EDA_HOTKEY HkRecordMacros2( wxT( "Record Macro 2" ), HK_RECORD_MACROS_2, GR_KB_CTRL+'2' );

static EDA_HOTKEY HkCallMacros2( wxT( "Call Macro 2" ), HK_CALL_MACROS_2, '2' );

static EDA_HOTKEY HkRecordMacros3( wxT( "Record Macro 3" ), HK_RECORD_MACROS_3, GR_KB_CTRL+'3' );

static EDA_HOTKEY HkCallMacros3( wxT( "Call Macro 3" ), HK_CALL_MACROS_3, '3' );

static EDA_HOTKEY HkRecordMacros4( wxT( "Record Macro 4" ), HK_RECORD_MACROS_4, GR_KB_CTRL+'4' );

static EDA_HOTKEY HkCallMacros4( wxT( "Call Macro 4" ), HK_CALL_MACROS_4, '4' );

static EDA_HOTKEY HkRecordMacros5( wxT( "Record Macro 5" ), HK_RECORD_MACROS_5, GR_KB_CTRL+'5' );

static EDA_HOTKEY HkCallMacros5( wxT( "Call Macro 5" ), HK_CALL_MACROS_5, '5' );

static EDA_HOTKEY HkRecordMacros6( wxT( "Record Macro 6" ), HK_RECORD_MACROS_6, GR_KB_CTRL+'6' );

static EDA_HOTKEY HkCallMacros6( wxT( "Call Macro 6" ), HK_CALL_MACROS_6, '6' );

static EDA_HOTKEY HkRecordMacros7( wxT( "Record Macro 7" ), HK_RECORD_MACROS_7, GR_KB_CTRL+'7' );

static EDA_HOTKEY HkCallMacros7( wxT( "Call Macro 7" ), HK_CALL_MACROS_7, '7' );

static EDA_HOTKEY HkRecordMacros8( wxT( "Record Macro 8" ), HK_RECORD_MACROS_8, GR_KB_CTRL+'8' );

static EDA_HOTKEY HkCallMacros8( wxT( "Call Macro 8" ), HK_CALL_MACROS_8, '8' );

static EDA_HOTKEY HkRecordMacros9( wxT( "Record Macro 9" ), HK_RECORD_MACROS_9, GR_KB_CTRL+'9' );

static EDA_HOTKEY HkCallMacros9( wxT( "Call Macro 9" ), HK_CALL_MACROS_9, '9' );


// List of common hotkey descriptors
EDA_HOTKEY* common_Hotkey_List[] =
{
    &HkHelp,        &HkZoomIn,          &HkZoomOut,
    &HkZoomRedraw,  &HkZoomCenter,      &HkZoomAuto,
    &HkSwitchUnits, &HkResetLocalCoord, &HkSetGridOrigin, &HkResetGridOrigin,
    &HkUndo,        &HkRedo,
    &HkMouseLeftClick,
    &HkMouseLeftDClick,
    NULL
};

// common hotkey descriptors only useful in footprint viewer
EDA_HOTKEY* common_basic_Hotkey_List[] =
{
    &HkHelp,        &HkZoomIn,          &HkZoomOut,
    &HkZoomRedraw,  &HkZoomCenter,      &HkZoomAuto,
    &HkSwitchUnits, &HkResetLocalCoord,
    &HkMouseLeftClick,
    &HkMouseLeftDClick,
    NULL
};

// List of hotkey descriptors for Pcbnew
EDA_HOTKEY* board_edit_Hotkey_List[] =
{
    &HkTrackDisplayMode,       &HkDelete,
    &HkBackspace,
    &HkAddNewTrack,            &HkAddThroughVia, &HkAddBlindBuriedVia,
    &HkAddMicroVia,
    &HkSelLayerAndAddThroughVia, &HkSelLayerAndAddBlindBuriedVia,
    &HkSwitchTrackPosture,
    &HkDragTrackKeepSlope,
    &HkPlaceItem,              &HkCopyItem,
    &HkMoveItem,
    &HkFlipItem,
    &HkRotateItem,             &HkDragFootprint,
    &HkGetAndMoveFootprint,    &HkLock_Unlock_Footprint,     &HkSavefile, &HkSavefileAs,
    &HkLoadfile,               &HkFindItem,                  &HkEditBoardItem,
    &HkSwitch2CopperLayer,     &HkSwitch2InnerLayer1,
    &HkSwitch2InnerLayer2,     &HkSwitch2InnerLayer3,        &HkSwitch2InnerLayer4,
    &HkSwitch2InnerLayer5,     &HkSwitch2InnerLayer6,        &HkSwitch2ComponentLayer,
    &HkSwitch2NextCopperLayer, &HkSwitch2PreviousCopperLayer,&HkAddModule,
    &HkSwitchTrackWidthToNext, &HkSwitchTrackWidthToPrevious,&HkSwitchGridToFastGrid1,
    &HkSwitchGridToFastGrid2,  &HkSwitchGridToNext,          &HkSwitchGridToPrevious,
    &HkRecordMacros0,          &HkCallMacros0,    &HkRecordMacros1,          &HkCallMacros1,
    &HkRecordMacros2,          &HkCallMacros2,    &HkRecordMacros3,          &HkCallMacros3,
    &HkRecordMacros4,          &HkCallMacros4,    &HkRecordMacros5,          &HkCallMacros5,
    &HkRecordMacros6,          &HkCallMacros6,    &HkRecordMacros7,          &HkCallMacros7,
    &HkRecordMacros8,          &HkCallMacros8,    &HkRecordMacros9,          &HkCallMacros9,
    &HkSwitchHighContrastMode,
    &HkCanvasDefault,          &HkCanvasCairo,               &HkCanvasOpenGL,
    NULL
};

// List of hotkey descriptors for the module editor
EDA_HOTKEY* module_edit_Hotkey_List[] = {
    &HkMoveItem,               &HkRotateItem,                &HkEditBoardItem,
    &HkDelete,
    &HkSaveModule,
    NULL
 };

// List of hotkey descriptors for the module viewer
// Currently empty
EDA_HOTKEY* module_viewer_Hotkey_List[] = {
    NULL
 };

// list of sections and corresponding hotkey list for Pcbnew
 // (used to create an hotkey config file, and edit hotkeys )
struct EDA_HOTKEY_CONFIG g_Pcbnew_Editor_Hokeys_Descr[] = {
    { &g_CommonSectionTag,      common_Hotkey_List,         &g_CommonSectionTitle      },
    { &g_BoardEditorSectionTag, board_edit_Hotkey_List,     &g_BoardEditorSectionTitle },
    { &g_ModuleEditSectionTag,  module_edit_Hotkey_List,    &g_ModuleEditSectionTitle  },
    { NULL,                     NULL,                       NULL                       }
};

// list of sections and corresponding hotkey list for the board editor
// (used to list current hotkeys in the board editor)
struct EDA_HOTKEY_CONFIG g_Board_Editor_Hokeys_Descr[] = {
    { &g_CommonSectionTag,      common_Hotkey_List,      &g_CommonSectionTitle },
    { &g_BoardEditorSectionTag, board_edit_Hotkey_List,  &g_BoardEditorSectionTitle },
    { NULL, NULL, NULL }
};

// list of sections and corresponding hotkey list for the footprint editor
// (used to list current hotkeys in the module editor)
struct EDA_HOTKEY_CONFIG g_Module_Editor_Hokeys_Descr[] = {
    { &g_CommonSectionTag,     common_Hotkey_List,      &g_CommonSectionTitle },
    { &g_ModuleEditSectionTag, module_edit_Hotkey_List, &g_ModuleEditSectionTitle },
    { NULL,                    NULL,                    NULL }
};

// list of sections and corresponding hotkey list for the footprint viewer
// (used to list current hotkeys in the module viewer)
struct EDA_HOTKEY_CONFIG g_Module_Viewer_Hokeys_Descr[] = {
    { &g_CommonSectionTag, common_basic_Hotkey_List, &g_CommonSectionTitle },
    { NULL,                NULL,                     NULL }
};


bool FOOTPRINT_VIEWER_FRAME::OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition,
                                     EDA_ITEM* aItem )
{
    if( aHotKey == 0 )
        return false;

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    /* Convert lower to upper case (the usual toupper function has problem with non ascii
     * codes like function keys */
    if( (aHotKey >= 'a') && (aHotKey <= 'z') )
        aHotKey += 'A' - 'a';

    EDA_HOTKEY* HK_Descr = GetDescriptorFromHotkey( aHotKey, common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( aHotKey, module_viewer_Hotkey_List );

    if( HK_Descr == NULL )
        return false;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return false;

    case HK_HELP:                   // Display Current hotkey list
        DisplayHotkeyList( this, g_Module_Viewer_Hokeys_Descr );
        break;

    case HK_RESET_LOCAL_COORD:      // set local (relative) coordinate origin
        GetScreen()->m_O_Curseur = GetCrossHairPosition();
        break;

    case HK_LEFT_CLICK:
        OnLeftClick( aDC, aPosition );
        break;

    case HK_LEFT_DCLICK:    // Simulate a double left click: generate 2 events
        OnLeftClick( aDC, aPosition );
        OnLeftDClick( aDC, aPosition );
        break;

    case HK_SWITCH_UNITS:
        cmd.SetId( (g_UserUnit == INCHES) ?
                    ID_TB_OPTIONS_SELECT_UNIT_MM : ID_TB_OPTIONS_SELECT_UNIT_INCH );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_IN:
        cmd.SetId( ID_POPUP_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_OUT:
        cmd.SetId( ID_POPUP_ZOOM_OUT );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_REDRAW:
        cmd.SetId( ID_ZOOM_REDRAW );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_CENTER:
        cmd.SetId( ID_POPUP_ZOOM_CENTER );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_AUTO:
        cmd.SetId( ID_ZOOM_PAGE );
        GetEventHandler()->ProcessEvent( cmd );
        break;
    }

    return true;
}
