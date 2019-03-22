/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file pcbnew/hotkeys.cpp
 */

#include <fctsys.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <footprint_viewer_frame.h>
#include <footprint_wizard_frame.h>
#include <pcbnew_id.h>

#include <hotkeys.h>

// Remark: the hotkey message info is used as keyword in hotkey config files and
// as comments in help windows, therefore translated only when displayed
// they are marked _HKI to be extracted by translation tools
// See hotkeys_basic.h for more info


/* How to add a new hotkey:
 *  add a new id in the enum hotkey_id_commnand like MY_NEW_ID_FUNCTION.
 *  add a new EDA_HOTKEY entry like:
 *  static EDA_HOTKEY HkMyNewEntry(_HKI("Command Label"), MY_NEW_ID_FUNCTION, default key value);
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
static EDA_HOTKEY HkMouseLeftClick( _HKI( "Mouse Left Click" ),
                                    HK_LEFT_CLICK, WXK_RETURN, 0 );
static EDA_HOTKEY HkMouseLeftDClick( _HKI( "Mouse Left Double Click" ),
                                     HK_LEFT_DCLICK, WXK_END, 0 );

static EDA_HOTKEY HkSwitch2CopperLayer( _HKI( "Switch to Copper (B.Cu) layer" ),
                                        HK_SWITCH_LAYER_TO_COPPER, WXK_PAGEDOWN );

static EDA_HOTKEY HkSwitch2ComponentLayer( _HKI( "Switch to Component (F.Cu) layer" ),
                                           HK_SWITCH_LAYER_TO_COMPONENT, WXK_PAGEUP );

static EDA_HOTKEY HkSwitch2InnerLayer1( _HKI( "Switch to Inner layer 1" ),
                                        HK_SWITCH_LAYER_TO_INNER1, WXK_F5 );
static EDA_HOTKEY HkSwitch2InnerLayer2( _HKI( "Switch to Inner layer 2" ),
                                        HK_SWITCH_LAYER_TO_INNER2, WXK_F6 );
static EDA_HOTKEY HkSwitch2InnerLayer3( _HKI( "Switch to Inner layer 3" ),
                                        HK_SWITCH_LAYER_TO_INNER3, WXK_F7 );
static EDA_HOTKEY HkSwitch2InnerLayer4( _HKI( "Switch to Inner layer 4" ),
                                        HK_SWITCH_LAYER_TO_INNER4, WXK_F8 );
static EDA_HOTKEY HkSwitch2InnerLayer5( _HKI( "Switch to Inner layer 5" ),
                                        HK_SWITCH_LAYER_TO_INNER5, GR_KB_SHIFT+WXK_F5 );
static EDA_HOTKEY HkSwitch2InnerLayer6( _HKI( "Switch to Inner layer 6" ),
                                        HK_SWITCH_LAYER_TO_INNER6, GR_KB_SHIFT+WXK_F6 );

static EDA_HOTKEY HkSwitch2NextCopperLayer( _HKI( "Switch to Next Layer" ),
                                            HK_SWITCH_LAYER_TO_NEXT, '+' );
static EDA_HOTKEY HkSwitch2PreviousCopperLayer( _HKI( "Switch to Previous Layer" ),
                                                HK_SWITCH_LAYER_TO_PREVIOUS, '-' );

static EDA_HOTKEY HkFindItem( _HKI( "Find Item" ), HK_FIND_ITEM, 'F' + GR_KB_CTRL );
static EDA_HOTKEY HkBackspace( _HKI( "Delete Track Segment" ), HK_BACK_SPACE, WXK_BACK );
static EDA_HOTKEY HkAddNewTrack( _HKI( "Add New Track" ), HK_ADD_NEW_TRACK, 'X' );

static EDA_HOTKEY HkRouteDiffPair( _HKI( "Route Differential Pair (Modern Toolset only)" ),
                                   HK_ROUTE_DIFF_PAIR, '6' );
static EDA_HOTKEY HkRouteTuneSingle( _HKI( "Tune Single Track (Modern Toolset only)" ),
                                     HK_ROUTE_TUNE_SINGLE, '7' );
static EDA_HOTKEY HkRouteTuneDiffPair( _HKI( "Tune Differential Pair Length (Modern Toolset only)" ),
                                       HK_ROUTE_TUNE_DIFF_PAIR, '8' );
static EDA_HOTKEY HkRouteTuneSkew( _HKI( "Tune Differential Pair Skew (Modern Toolset only)" ),
                                   HK_ROUTE_TUNE_SKEW, '9' );
static EDA_HOTKEY HkRouteTuneSettings( _HKI( "Length Tuning Settings (Modern Toolset only)" ),
                                       HK_ROUTE_TUNE_SETTINGS, 'L' + GR_KB_CTRL );

static EDA_HOTKEY HkRouteTuneIncreaseSpacing( _HKI( "Increase meander spacing by one step." ),
        HK_ROUTE_TUNE_INCREASE_SPACING, '1' );
static EDA_HOTKEY HkRouteTuneDecreaseSpacing( _HKI( "Decrease meander spacing by one step." ),
        HK_ROUTE_TUNE_DECREASE_SPACING, '2' );
static EDA_HOTKEY HkRouteTuneIncreaseAmp( _HKI( "Increase meander amplitude by one step." ),
        HK_ROUTE_TUNE_INCREASE_AMP, '3' );
static EDA_HOTKEY HkRouteTuneDecreaseAmp( _HKI( "Decrease meander amplitude by one step." ),
        HK_ROUTE_TUNE_DECREASE_AMP, '4' );

static EDA_HOTKEY HkAddThroughVia( _HKI( "Add Through Via" ), HK_ADD_THROUGH_VIA, 'V' );
static EDA_HOTKEY HkSelLayerAndAddThroughVia( _HKI( "Select Layer and Add Through Via" ),
                                              HK_SEL_LAYER_AND_ADD_THROUGH_VIA, '<' );
static EDA_HOTKEY HkAddMicroVia( _HKI( "Add MicroVia" ), HK_ADD_MICROVIA, 'V' + GR_KB_CTRL );
static EDA_HOTKEY HkAddBlindBuriedVia( _HKI( "Add Blind/Buried Via" ), HK_ADD_BLIND_BURIED_VIA,
                                       'V' + GR_KB_ALT + GR_KB_SHIFT );
static EDA_HOTKEY HkSelLayerAndAddBlindBuriedVia( _HKI( "Select Layer and Add Blind/Buried Via" ),
                                                  HK_SEL_LAYER_AND_ADD_BLIND_BURIED_VIA,
                                                  '<' + GR_KB_ALT );
static EDA_HOTKEY HkSwitchTrackPosture( _HKI( "Switch Track Posture" ),
                                        HK_SWITCH_TRACK_POSTURE, '/' );
static EDA_HOTKEY HkDragTrackKeepSlope( _HKI( "Drag Track Keep Slope" ),
                                        HK_DRAG_TRACK_KEEP_SLOPE, 'D' );
static EDA_HOTKEY HkPlaceItem( _HKI( "Place Item" ), HK_PLACE_ITEM, 'P' );
static EDA_HOTKEY HkEditBoardItem( _HKI( "Edit Item" ), HK_EDIT_ITEM, 'E' );
static EDA_HOTKEY HkEditWithModedit( _HKI( "Edit with Footprint Editor" ),
                                     HK_EDIT_MODULE_WITH_MODEDIT, 'E' + GR_KB_CTRL );
static EDA_HOTKEY HkFlipItem( _HKI( "Flip Item" ), HK_FLIP_ITEM, 'F' );
static EDA_HOTKEY HkRotateItem( _HKI( "Rotate Item" ), HK_ROTATE_ITEM, 'R' );
static EDA_HOTKEY HkRotateItemClockwise( _HKI( "Rotate Item Clockwise (Modern Toolset only)" ),
                                         HK_ROTATE_ITEM_CLOCKWISE, GR_KB_SHIFT + 'R' );
static EDA_HOTKEY HkMoveItem( _HKI( "Move Item" ), HK_MOVE_ITEM, 'M' );
static EDA_HOTKEY HkMoveItemExact( _HKI( "Move Item Exactly" ), HK_MOVE_ITEM_EXACT,
                                   'M' + GR_KB_CTRL );
static EDA_HOTKEY HkPositionItemRelative( _HKI( "Position Item Relative" ), HK_POSITION_RELATIVE,
                                          'R' + GR_KB_CTRL );
static EDA_HOTKEY HkDuplicateItem( _HKI( "Duplicate Item" ), HK_DUPLICATE_ITEM, 'D' + GR_KB_CTRL );
static EDA_HOTKEY HkDuplicateItemAndIncrement( _HKI( "Duplicate Item and Increment" ),
                                               HK_DUPLICATE_ITEM_AND_INCREMENT,
                                               'D' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkCreateArray( _HKI( "Create Array" ), HK_CREATE_ARRAY, 'T' + GR_KB_CTRL );
static EDA_HOTKEY HkCopyItem( _HKI( "Copy Item" ), HK_COPY_ITEM, 'C' );
static EDA_HOTKEY HkDragFootprint( _HKI( "Drag Item" ), HK_DRAG_ITEM, 'G' );
static EDA_HOTKEY HkGetAndMoveFootprint( _HKI( "Get and Move Footprint" ),
                                         HK_GET_AND_MOVE_FOOTPRINT, 'T' );
static EDA_HOTKEY HkLock_Unlock_Footprint( _HKI( "Lock/Unlock Footprint" ),
                                           HK_LOCK_UNLOCK_FOOTPRINT, 'L' );
static EDA_HOTKEY HkDelete( _HKI( "Delete Full Track" ), HK_DELETE, WXK_DELETE );
static EDA_HOTKEY HkResetLocalCoord( _HKI( "Reset Local Coordinates" ), HK_RESET_LOCAL_COORD, ' ' );
static EDA_HOTKEY HkSwitchHighContrastMode( _HKI( "Toggle High Contrast Mode" ),
                                            HK_SWITCH_HIGHCONTRAST_MODE, 'H'+GR_KB_CTRL );

static EDA_HOTKEY HkAddLine( _HKI( "Draw Line" ), HK_ADD_LINE, 'L' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddPolygon( _HKI( "Draw Graphic Polygon" ), HK_ADD_POLYGON,
                                'P' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddCircle( _HKI( "Draw Circle" ), HK_ADD_CIRCLE, 'C' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddArc( _HKI( "Draw Arc" ), HK_ADD_ARC, 'A' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddText( _HKI( "Add Text" ), HK_ADD_TEXT, 'T' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddDimension( _HKI( "Add Dimension" ), HK_ADD_DIMENSION,
                                  'H' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddFilledZone( _HKI( "Add Filled Zone" ), HK_ADD_ZONE, 'Z' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddFreeVia( _HKI( "Add Vias" ), HK_ADD_FREE_VIA, 'V' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddKeepout( _HKI( "Add Keepout Area" ), HK_ADD_KEEPOUT, 'K' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddZoneCutout( _HKI( "Add a Zone Cutout" ), HK_ADD_CUTOUT, 'C' + GR_KB_SHIFT );
static EDA_HOTKEY HkAddSimilarZone( _HKI( "Add a Similar Zone" ), HK_ADD_SIMILAR_ZONE,
                                    '.' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddDXF( _HKI( "Place DXF" ), HK_ADD_DXF, 'F' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkAddAnchor( _HKI( "Place the Footprint Anchor" ), HK_ADD_ANCHOR,
                               'N' + GR_KB_SHIFTCTRL );

static EDA_HOTKEY HkIncreaseLineWidth( _HKI( "Increase Line Width" ), HK_INC_LINE_WIDTH,
                                       '+' + GR_KB_CTRL );
static EDA_HOTKEY HkDecreaseLineWidth( _HKI( "Decrease Line Width" ), HK_DEC_LINE_WIDTH,
                                       '-' + GR_KB_CTRL );


static EDA_HOTKEY HkSetGridOrigin( _HKI( "Set Grid Origin" ), HK_SET_GRID_ORIGIN, 'S' );
static EDA_HOTKEY HkResetGridOrigin( _HKI( "Reset Grid Origin" ), HK_RESET_GRID_ORIGIN, 'Z' );

static EDA_HOTKEY HkCanvasDefault( _HKI( "Switch to Legacy Toolset (not all features will be available" ),
                                   HK_CANVAS_LEGACY,
#ifdef __WXMAC__
                                   GR_KB_ALT +
#endif
                                   WXK_F9 );
static EDA_HOTKEY HkCanvasOpenGL( _HKI( "Switch to Modern Toolset with hardware-accelerated graphics (recommended)" ),
                                  HK_CANVAS_OPENGL,
#ifdef __WXMAC__
                                  GR_KB_ALT +
#endif
                                  WXK_F11 );
static EDA_HOTKEY HkCanvasCairo( _HKI( "Switch to Modern Toolset with software graphics (fall-back)" ),
                                 HK_CANVAS_CAIRO,
#ifdef __WXMAC__
                                 GR_KB_ALT +
#endif
                                 WXK_F12 );

static EDA_HOTKEY HkZoneFillOrRefill( _HKI( "Fill or Refill All Zones" ),
                                      HK_ZONE_FILL_OR_REFILL, 'B' );
static EDA_HOTKEY HkZoneRemoveFilled( _HKI( "Remove Filled Areas in All Zones" ),
                                      HK_ZONE_REMOVE_FILLED, 'B' + GR_KB_CTRL );
/* Fit on Screen */
#if !defined( __WXMAC__ )
static EDA_HOTKEY HkZoomAuto( _HKI( "Zoom Auto" ), HK_ZOOM_AUTO, WXK_HOME );
#else
static EDA_HOTKEY HkZoomAuto( _HKI( "Zoom Auto" ), HK_ZOOM_AUTO, GR_KB_CTRL + '0' );
#endif

static EDA_HOTKEY HkZoomCenter( _HKI( "Zoom Center" ), HK_ZOOM_CENTER, WXK_F4 );

/* Refresh Screen */
#if !defined( __WXMAC__ )
static EDA_HOTKEY HkZoomRedraw( _HKI( "Zoom Redraw" ), HK_ZOOM_REDRAW, WXK_F3 );
#else
static EDA_HOTKEY HkZoomRedraw( _HKI( "Zoom Redraw" ), HK_ZOOM_REDRAW, GR_KB_CTRL + 'R' );
#endif

/* Zoom In */
#if !defined( __WXMAC__ )
static EDA_HOTKEY HkZoomIn( _HKI( "Zoom In" ), HK_ZOOM_IN, WXK_F1 );
#else
static EDA_HOTKEY HkZoomIn( _HKI( "Zoom In" ), HK_ZOOM_IN, GR_KB_CTRL + '+' );
#endif

/* Zoom Out */
#if !defined( __WXMAC__ )
static EDA_HOTKEY HkZoomOut( _HKI( "Zoom Out" ), HK_ZOOM_OUT, WXK_F2 );
#else
static EDA_HOTKEY HkZoomOut( _HKI( "Zoom Out" ), HK_ZOOM_OUT, GR_KB_CTRL + '-' );
#endif

static EDA_HOTKEY HkZoomSelection( _HKI( "Zoom to Selection" ), HK_ZOOM_SELECTION,
                                   GR_KB_CTRL + WXK_F5, ID_ZOOM_SELECTION );

static EDA_HOTKEY Hk3DViewer( _HKI( "3D Viewer" ), HK_3D_VIEWER, GR_KB_ALT + '3' );

static EDA_HOTKEY HkHelp( _HKI( "List Hotkeys" ), HK_HELP, GR_KB_CTRL + WXK_F1 );

static EDA_HOTKEY HkSwitchTrackWidthToNext( _HKI( "Switch Track Width To Next" ),
                                            HK_SWITCH_TRACK_WIDTH_TO_NEXT, 'W' );

static EDA_HOTKEY HkSwitchTrackWidthToPrevious( _HKI( "Switch Track Width To Previous" ),
                                                HK_SWITCH_TRACK_WIDTH_TO_PREVIOUS,
                                                'W' + GR_KB_SHIFT );

static EDA_HOTKEY HkSwitchGridToFastGrid1( _HKI( "Switch Grid To Fast Grid1" ),
                                           HK_SWITCH_GRID_TO_FASTGRID1, GR_KB_ALT + '1' );

static EDA_HOTKEY HkSwitchGridToFastGrid2( _HKI( "Switch Grid To Fast Grid2" ),
                                           HK_SWITCH_GRID_TO_FASTGRID2, GR_KB_ALT + '2' );

static EDA_HOTKEY HkSwitchGridToNext( _HKI( "Switch Grid To Next" ),
                                      HK_SWITCH_GRID_TO_NEXT, 'N' );

static EDA_HOTKEY HkSwitchGridToPrevious( _HKI( "Switch Grid To Previous" ),
                                          HK_SWITCH_GRID_TO_PREVIOUS, 'N' + GR_KB_SHIFT );

static EDA_HOTKEY HkSwitchUnits( _HKI( "Switch Units" ), HK_SWITCH_UNITS, 'U' + GR_KB_CTRL );

static EDA_HOTKEY HkTrackDisplayMode( _HKI( "Track Display Mode" ),
                                      HK_SWITCH_TRACK_DISPLAY_MODE, 'K' );

static EDA_HOTKEY HkAddModule( _HKI( "Add Footprint" ), HK_ADD_MODULE, 'O' );

// These hotkeys work only in GAL canvas, because the legacy canvas using wxDC does not know
// the transparency (alpha channel)
static EDA_HOTKEY HkIncLayerAlpha( _HKI( "Increment Layer Transparency (Modern Toolset only)" ),
                                   HK_INC_LAYER_ALPHA, '}' );

static EDA_HOTKEY HkDecLayerAlpha( _HKI( "Decrement Layer Transparency (Modern Toolset only)" ),
                                   HK_DEC_LAYER_ALPHA, '{' );

// These two are currently unused, and are intentionally not added to a list below.
static EDA_HOTKEY HkIncHighContrast( _HKI( "Increment High Contrast" ), HK_HIGHCONTRAST_INC, '>' );
static EDA_HOTKEY HkDecHighContrast( _HKI( "Decrement High Contrast" ), HK_HIGHCONTRAST_DEC, '<' );

static EDA_HOTKEY HkSelectConnection( _HKI( "Select Single Track" ), HK_SEL_TRIVIAL_CONNECTION,
                                      'U' );

static EDA_HOTKEY HkSelectCopper( _HKI( "Select Connected Tracks" ), HK_SEL_COPPER_CONNECTION,
                                  'I' );

static EDA_HOTKEY HkRoutingOptions( _HKI( "Routing Options" ), HK_ROUTING_OPTIONS,
                                    ',' + GR_KB_CTRL + GR_KB_SHIFT );

static EDA_HOTKEY HkCustomTrackWidth( _HKI( "Custom Track/Via Size" ), HK_CUSTOM_TRACK_WIDTH, 'Q' );

static EDA_HOTKEY HkDpDimension( _HKI( "Differential Pair Dimensions" ), HK_DP_DIMENSIONS,
                                 'R' + GR_KB_SHIFTCTRL );

static EDA_HOTKEY HkViaSizeInc( _HKI( "Increase Via Size" ), HK_VIA_SIZE_INC, '\'' );

static EDA_HOTKEY HkViaSizeDec( _HKI( "Decrease Via Size" ), HK_VIA_SIZE_DEC, '\\' );

static EDA_HOTKEY HkHighlightNetSelection( _HKI( "Toggle Highlight of Selected Net (Modern Toolset only)" ),
                                           HK_HIGHLIGHT_NET_SELECTION, '`' );

// Common: hotkeys_basic.h
static EDA_HOTKEY HkNew( _HKI( "New" ), HK_NEW, GR_KB_CTRL + 'N', (int) wxID_NEW );
static EDA_HOTKEY HkOpen( _HKI( "Open" ), HK_OPEN, GR_KB_CTRL + 'O', (int) wxID_OPEN );
static EDA_HOTKEY HkSave( _HKI( "Save" ), HK_SAVE, GR_KB_CTRL + 'S', (int) wxID_SAVE );
static EDA_HOTKEY HkSaveAs( _HKI( "Save As" ), HK_SAVEAS, GR_KB_SHIFT + GR_KB_CTRL + 'S',
                            (int) wxID_SAVEAS );
static EDA_HOTKEY HkPrint( _HKI( "Print" ), HK_PRINT, GR_KB_CTRL + 'P', (int) wxID_PRINT );

static EDA_HOTKEY HkUndo( _HKI( "Undo" ), HK_UNDO, GR_KB_CTRL + 'Z', (int) wxID_UNDO );

#if !defined( __WXMAC__ )
static EDA_HOTKEY HkRedo( _HKI( "Redo" ), HK_REDO, GR_KB_CTRL + 'Y', (int) wxID_REDO );
#else
static EDA_HOTKEY HkRedo( _HKI( "Redo" ), HK_REDO,
                          GR_KB_SHIFT + GR_KB_CTRL + 'Z',
                          (int) wxID_REDO );
#endif

static EDA_HOTKEY HkEditCut( _HKI( "Cut" ), HK_EDIT_CUT, GR_KB_CTRL + 'X', (int) wxID_CUT );
static EDA_HOTKEY HkEditCopy( _HKI( "Copy" ), HK_EDIT_COPY, GR_KB_CTRL + 'C', (int) wxID_COPY );
static EDA_HOTKEY HkEditPaste( _HKI( "Paste" ), HK_EDIT_PASTE, GR_KB_CTRL + 'V', (int) wxID_PASTE );
static EDA_HOTKEY HkPreferences( _HKI( "Preferences" ),
                                 HK_PREFERENCES, GR_KB_CTRL + ',', (int) wxID_PREFERENCES );

static EDA_HOTKEY HkToggleCursor( _HKI( "Toggle Cursor Display (Modern Toolset only)" ),
                                  HK_TOGGLE_CURSOR, 'X' + GR_KB_SHIFTCTRL );
static EDA_HOTKEY HkMeasureTool( _HKI( "Measure Distance (Modern Toolset only)" ),
                                 HK_MEASURE_TOOL, 'M' + GR_KB_SHIFTCTRL );

static EDA_HOTKEY HkInsertCorner( _HKI( "Insert Corner (Modern Toolset only)" ),
                                 HK_INSERT_CORNER, WXK_INSERT );

// List of common hotkey descriptors
EDA_HOTKEY* common_Hotkey_List[] =
{
    &HkNew,         &HkOpen,            &HkSave,          &HkSaveAs,        &HkPrint,
    &HkUndo,        &HkRedo,
    &HkEditCut,     &HkEditCopy,        &HkEditPaste,
    &HkHelp,        &HkPreferences,
    &HkZoomIn,      &HkZoomOut,
    &HkZoomRedraw,  &HkZoomCenter,      &HkZoomAuto,      &HkZoomSelection,
    &Hk3DViewer,
    &HkSwitchUnits, &HkResetLocalCoord, &HkSetGridOrigin, &HkResetGridOrigin,
    &HkMouseLeftClick,
    &HkMouseLeftDClick,
    &HkIncLayerAlpha, &HkDecLayerAlpha,
    &HkToggleCursor,
    &HkMeasureTool,
    NULL
};

// common hotkey descriptors only useful in footprint viewer
EDA_HOTKEY* common_basic_Hotkey_List[] =
{
    &HkHelp,        &HkZoomIn,          &HkZoomOut,
    &HkZoomRedraw,  &HkZoomCenter,      &HkZoomAuto,   &Hk3DViewer,
    &HkSwitchUnits, &HkResetLocalCoord,
    &HkMouseLeftClick,
    &HkMouseLeftDClick,
    NULL
};

// List of hotkey descriptors for Pcbnew
EDA_HOTKEY* board_edit_Hotkey_List[] =
{
    // Adding Items
    &HkPlaceItem,
    &HkAddModule,
    &HkAddFreeVia,
    &HkAddFilledZone,
    &HkAddKeepout,
    &HkAddZoneCutout,
    &HkAddSimilarZone,
    &HkAddLine,
    &HkAddCircle,
    &HkAddArc,
    &HkAddPolygon,
    &HkAddText,
    &HkAddDimension,
    &HkAddDXF,

    // Routing and options during routing
    &HkAddNewTrack,
    &HkSwitchTrackPosture,
    &HkDragTrackKeepSlope,
    &HkAddThroughVia,
    &HkAddBlindBuriedVia,
    &HkAddMicroVia,

    // Routing
    &HkRouteDiffPair,
    &HkRouteTuneSingle,
    &HkRouteTuneDiffPair,
    &HkRouteTuneSkew,

    // Routing Options
    &HkRoutingOptions,
    &HkRouteTuneSettings,
    &HkRouteTuneIncreaseSpacing,
    &HkRouteTuneDecreaseSpacing,
    &HkRouteTuneIncreaseAmp,
    &HkRouteTuneDecreaseAmp,
    &HkDpDimension,

    // Zones
    &HkZoneFillOrRefill,
    &HkZoneRemoveFilled,

    // Point editor (zones and segments)
    &HkInsertCorner,

    // Highlight and display
    &HkSelectConnection,
    &HkSelectCopper,
    &HkHighlightNetSelection,
    &HkTrackDisplayMode,

    // Adjust track/via sizes
    &HkCustomTrackWidth,
    &HkSwitchTrackWidthToNext,
    &HkSwitchTrackWidthToPrevious,
    &HkViaSizeInc,
    &HkViaSizeDec,

    // Remove items
    &HkDelete,
    &HkBackspace,

    // Edit placement
    &HkDragFootprint,
    &HkCopyItem,
    &HkMoveItem,
    &HkGetAndMoveFootprint,
    &HkMoveItemExact,
    &HkPositionItemRelative,
    &HkFlipItem,
    &HkRotateItem,
    &HkRotateItemClockwise,

    &HkSelLayerAndAddThroughVia,
    &HkSelLayerAndAddBlindBuriedVia,

    &HkDuplicateItem,
    &HkDuplicateItemAndIncrement,
    &HkCreateArray,
    &HkLock_Unlock_Footprint,

    &HkFindItem,
    &HkEditBoardItem,
    &HkEditWithModedit,

    // Active Layer
    &HkSwitch2ComponentLayer,
    &HkSwitch2CopperLayer,
    &HkSwitch2InnerLayer1,
    &HkSwitch2InnerLayer2,
    &HkSwitch2InnerLayer3,
    &HkSwitch2InnerLayer4,
    &HkSwitch2InnerLayer5,
    &HkSwitch2InnerLayer6,
    &HkSwitch2NextCopperLayer,
    &HkSwitch2PreviousCopperLayer,

    // Grids
    &HkSwitchGridToFastGrid1,
    &HkSwitchGridToFastGrid2,
    &HkSwitchGridToNext,
    &HkSwitchGridToPrevious,

    // Display
    &HkSwitchHighContrastMode,
    &HkCanvasDefault,
    &HkCanvasCairo,
    &HkCanvasOpenGL,
    NULL
};

// List of hotkey descriptors for the module editor
EDA_HOTKEY* module_edit_Hotkey_List[] = {
    // Add Items
    &HkAddLine,
    &HkAddCircle,
    &HkAddArc,
    &HkAddPolygon,
    &HkAddText,
    &HkAddAnchor,

    // Delete/Move
    &HkDelete,
    &HkMoveItem,
    &HkMoveItemExact,
    &HkRotateItem,
    &HkSwitchTrackPosture,

    // Edit/Duplicate
    &HkEditBoardItem,
    &HkDuplicateItem,
    &HkDuplicateItemAndIncrement,
    &HkCreateArray,

    // Grids
    &HkSwitchGridToFastGrid1,
    &HkSwitchGridToFastGrid2,
    &HkSwitchGridToNext,
    &HkSwitchGridToPrevious,

    // Display
    &HkSwitchHighContrastMode,
    &HkCanvasDefault,
    &HkCanvasCairo,
    &HkCanvasOpenGL,
    NULL
 };

// List of hotkey descriptors for the module viewer
// Currently empty
EDA_HOTKEY* module_viewer_Hotkey_List[] = {
    NULL
 };

// Keyword Identifiers (tags) in key code configuration file (section names)
// (.m_SectionTag member of a EDA_HOTKEY_CONFIG)
static wxString boardEditorSectionTag( wxT( "[pcbnew]" ) );
static wxString moduleEditSectionTag( wxT( "[footprinteditor]" ) );

// Titles for hotkey editor and hotkey display
static wxString commonSectionTitle( _HKI( "Common" ) );
static wxString boardEditorSectionTitle( _HKI( "Board Editor" ) );
static wxString moduleEditSectionTitle( _HKI( "Footprint Editor" ) );

// list of sections and corresponding hotkey list for Pcbnew
// (used to create an hotkey config file, and edit hotkeys )
struct EDA_HOTKEY_CONFIG g_Pcbnew_Editor_Hotkeys_Descr[] = {
    { &g_CommonSectionTag,      common_Hotkey_List,         &commonSectionTitle      },
    { &boardEditorSectionTag,   board_edit_Hotkey_List,     &boardEditorSectionTitle },
    { &moduleEditSectionTag,  module_edit_Hotkey_List,    &moduleEditSectionTitle  },
    { NULL,                     NULL,                       NULL                       }
};

// list of sections and corresponding hotkey list for the board editor
// (used to list current hotkeys in the board editor)
struct EDA_HOTKEY_CONFIG g_Board_Editor_Hotkeys_Descr[] = {
    { &g_CommonSectionTag,      common_Hotkey_List,      &commonSectionTitle },
    { &boardEditorSectionTag,   board_edit_Hotkey_List,  &boardEditorSectionTitle },
    { NULL, NULL, NULL }
};

// list of sections and corresponding hotkey list for the footprint editor
// (used to list current hotkeys in the module editor)
struct EDA_HOTKEY_CONFIG g_Module_Editor_Hotkeys_Descr[] = {
    { &g_CommonSectionTag,     common_Hotkey_List,      &commonSectionTitle },
    { &moduleEditSectionTag, module_edit_Hotkey_List, &moduleEditSectionTitle },
    { NULL,                    NULL,                    NULL }
};

// list of sections and corresponding hotkey list for the footprint viewer
// (used to list current hotkeys in the module viewer)
struct EDA_HOTKEY_CONFIG g_Module_Viewer_Hotkeys_Descr[] = {
    { &g_CommonSectionTag, common_basic_Hotkey_List, &commonSectionTitle },
    { NULL,                NULL,                     NULL }
};


EDA_HOTKEY* FOOTPRINT_VIEWER_FRAME::GetHotKeyDescription( int aCommand ) const
{
    EDA_HOTKEY* HK_Descr = GetDescriptorFromCommand( aCommand, common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromCommand( aCommand, module_viewer_Hotkey_List );

    return HK_Descr;
}


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
        DisplayHotkeyList( this, g_Module_Viewer_Hotkeys_Descr );
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
        cmd.SetId( (GetUserUnits() == INCHES) ?
                    ID_TB_OPTIONS_SELECT_UNIT_MM : ID_TB_OPTIONS_SELECT_UNIT_INCH );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_IN:
        cmd.SetId( ID_KEY_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_OUT:
        cmd.SetId( ID_KEY_ZOOM_OUT );
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



EDA_HOTKEY* FOOTPRINT_WIZARD_FRAME::GetHotKeyDescription( int aCommand ) const
{
    EDA_HOTKEY* HK_Descr = GetDescriptorFromCommand( aCommand, common_Hotkey_List );

    return HK_Descr;
}


bool FOOTPRINT_WIZARD_FRAME::OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition,
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
        return false;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return false;

    case HK_HELP:                   // Display Current hotkey list
        DisplayHotkeyList( this, g_Module_Viewer_Hotkeys_Descr );
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
        cmd.SetId( (GetUserUnits() == INCHES) ?
                    ID_TB_OPTIONS_SELECT_UNIT_MM : ID_TB_OPTIONS_SELECT_UNIT_INCH );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_IN:
        cmd.SetId( ID_KEY_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_OUT:
        cmd.SetId( ID_KEY_ZOOM_OUT );
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
