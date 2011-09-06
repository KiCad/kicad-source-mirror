/***************/
/* hotkeys.cpp */
/***************/

#include "fctsys.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "hotkeys.h"

/* How to add a new hotkey:
 *  add a new id in the enum hotkey_id_commnand like MY_NEW_ID_FUNCTION.
 *  add a new Ki_HotkeyInfo entry like:
 *  static Ki_HotkeyInfo HkMyNewEntry(wxT("Command Label"), MY_NEW_ID_FUNCTION, default key value);
 *      "Command Label" is the name used in hotkey list display, and the identifier in the hotkey list file
 *      MY_NEW_ID_FUNCTION is an equivalent id function used in the switch in OnHotKey() function.
 *      default key value is the default hotkey for this command. Can be overrided by the user hotkey list file
 *  add the HkMyNewEntry pointer in the s_board_edit_Hotkey_List list ( or/and the s_module_edit_Hotkey_List list)
 *  Add the new code in the switch in OnHotKey() function.
 *      Note: when the variable itemCurrentlyEdited is true, an item is currently edited.
 *      This can be useful if the new function cannot be executed while an item is currently being edited
 *      ( For example, one cannot start a new wire when a component is moving.)
 *
 *  Note: If an hotkey is a special key, be sure the corresponding wxWidget keycode (WXK_XXXX)
 *  is handled in the hotkey_name_descr s_Hotkey_Name_List list (see hotkeys_basic.cpp)
 *  and see this list for some ascii keys (space ...)
 */

/* local variables */
/* Hotkey list: */
static Ki_HotkeyInfo HkSwitch2CopperLayer( wxT( "Switch to Copper layer" ),
                                           HK_SWITCH_LAYER_TO_COPPER, WXK_PAGEDOWN );
static Ki_HotkeyInfo HkSwitch2ComponentLayer( wxT( "Switch to Component layer" ),
                                              HK_SWITCH_LAYER_TO_COMPONENT, WXK_PAGEUP );
static Ki_HotkeyInfo HkSwitch2InnerLayer1( wxT( "Switch to Inner layer 1" ),
                                           HK_SWITCH_LAYER_TO_INNER1, WXK_F5 );
static Ki_HotkeyInfo HkSwitch2InnerLayer2( wxT( "Switch to Inner layer 2" ),
                                           HK_SWITCH_LAYER_TO_INNER2, WXK_F6 );
static Ki_HotkeyInfo HkSwitch2InnerLayer3( wxT( "Switch to Inner layer 3" ),
                                           HK_SWITCH_LAYER_TO_INNER3, WXK_F7 );
static Ki_HotkeyInfo HkSwitch2InnerLayer4( wxT( "Switch to Inner layer 4" ),
                                           HK_SWITCH_LAYER_TO_INNER4, WXK_F8 );
static Ki_HotkeyInfo HkSwitch2InnerLayer5( wxT( "Switch to Inner layer 5" ),
                                           HK_SWITCH_LAYER_TO_INNER5, WXK_F9 );
static Ki_HotkeyInfo HkSwitch2InnerLayer6( wxT( "Switch to Inner layer 6" ),
                                           HK_SWITCH_LAYER_TO_INNER6, WXK_F10 );

static Ki_HotkeyInfo HkSwitch2NextCopperLayer( wxT( "Switch to Next Layer" ),
                                               HK_SWITCH_LAYER_TO_NEXT, '+' );
static Ki_HotkeyInfo HkSwitch2PreviousCopperLayer( wxT(
                                                       "Switch to Previous Layer" ),
                                                   HK_SWITCH_LAYER_TO_PREVIOUS, '-' );

static Ki_HotkeyInfo HkSavefile( wxT( "Save board" ), HK_SAVE_BOARD, 'S'
                                 + GR_KB_CTRL );
static Ki_HotkeyInfo HkLoadfile( wxT( "Load board" ), HK_LOAD_BOARD, 'L'
                                 + GR_KB_CTRL );
static Ki_HotkeyInfo HkFindItem( wxT( "Find Item" ), HK_FIND_ITEM, 'F'
                                 + GR_KB_CTRL );
static Ki_HotkeyInfo HkBackspace( wxT( "Delete track segment" ), HK_BACK_SPACE,
                                  WXK_BACK );
static Ki_HotkeyInfo HkAddNewTrack( wxT( "Add new track" ), HK_ADD_NEW_TRACK, 'X' );
static Ki_HotkeyInfo HkAddVia( wxT( "Add Via" ), HK_ADD_VIA, 'V' );
static Ki_HotkeyInfo HkSwitchTrackPosture( wxT( "Switch Track Posture" ),
                                           HK_SWITCH_TRACK_POSTURE, '/' );
static Ki_HotkeyInfo HkDragTrackKeepSlope( wxT( "Drag track keep slope" ),
                                           HK_DRAG_TRACK_KEEP_SLOPE, 'D' );
static Ki_HotkeyInfo HkAddMicroVia( wxT( "Add MicroVia" ), HK_ADD_MICROVIA, 'V'
                                    + GR_KB_CTRL );
static Ki_HotkeyInfo HkEndTrack( wxT( "End Track" ), HK_END_TRACK, WXK_END );
static Ki_HotkeyInfo HkEditBoardItem( wxT( "Edit Item" ), HK_EDIT_ITEM, 'E' );
static Ki_HotkeyInfo HkFlipFootprint( wxT( "Flip Footprint" ), HK_FLIP_FOOTPRINT,
                                      'F' );
static Ki_HotkeyInfo HkRotateItem( wxT( "Rotate Item" ), HK_ROTATE_ITEM, 'R' );
static Ki_HotkeyInfo HkMoveItem( wxT( "Move Item" ), HK_MOVE_ITEM, 'M' );
static Ki_HotkeyInfo HkDragFootprint( wxT( "Drag Footprint" ), HK_DRAG_ITEM,
                                      'G' );
static Ki_HotkeyInfo HkGetAndMoveFootprint( wxT( "Get and Move Footprint" ),
                                            HK_GET_AND_MOVE_FOOTPRINT, 'T' );
static Ki_HotkeyInfo HkLock_Unlock_Footprint( wxT( "Lock/Unlock Footprint" ),
                                              HK_LOCK_UNLOCK_FOOTPRINT, 'L' );
static Ki_HotkeyInfo HkDelete( wxT( "Delete Track or Footprint" ), HK_DELETE, WXK_DELETE );
static Ki_HotkeyInfo HkResetLocalCoord( wxT( "Reset Local Coordinates" ),
                                        HK_RESET_LOCAL_COORD, ' ' );

/* Fit on Screen */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkZoomAuto( wxT( "Zoom Auto" ), HK_ZOOM_AUTO, WXK_HOME );
#else
static Ki_HotkeyInfo HkZoomAuto( wxT( "Zoom Auto" ), HK_ZOOM_AUTO, GR_KB_CTRL + '0' );
#endif

static Ki_HotkeyInfo HkZoomCenter( wxT( "Zoom Center" ), HK_ZOOM_CENTER, WXK_F4 );

/* Refresh Screen */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkZoomRedraw( wxT( "Zoom Redraw" ), HK_ZOOM_REDRAW, WXK_F3 );
#else
static Ki_HotkeyInfo HkZoomRedraw( wxT( "Zoom Redraw" ), HK_ZOOM_REDRAW, GR_KB_CTRL + 'R' );
#endif

/* Zoom In */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkZoomIn( wxT( "Zoom In" ), HK_ZOOM_IN, WXK_F1 );
#else
static Ki_HotkeyInfo HkZoomIn( wxT( "Zoom In" ), HK_ZOOM_IN, GR_KB_CTRL + '+' );
#endif

/* Zoom Out */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkZoomOut( wxT( "Zoom Out" ), HK_ZOOM_OUT, WXK_F2 );
#else
static Ki_HotkeyInfo HkZoomOut( wxT( "Zoom Out" ), HK_ZOOM_OUT, GR_KB_CTRL + '-' );
#endif

static Ki_HotkeyInfo HkHelp( wxT( "Help (this window)" ), HK_HELP, '?' );


/* Undo */
static Ki_HotkeyInfo HkUndo( wxT( "Undo" ), HK_UNDO, GR_KB_CTRL + 'Z',
                             (int) wxID_UNDO );

/* Redo */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkRedo( wxT( "Redo" ), HK_REDO, GR_KB_CTRL + 'Y',
                             (int) wxID_REDO );
#else
static Ki_HotkeyInfo HkRedo( wxT( "Redo" ), HK_REDO,
                             GR_KB_SHIFT + GR_KB_CTRL + 'Z',
                             (int) wxID_REDO );
#endif


static Ki_HotkeyInfo HkSwitchUnits( wxT( "Switch Units" ), HK_SWITCH_UNITS, 'U'
                                    + GR_KB_CTRL );
static Ki_HotkeyInfo HkTrackDisplayMode( wxT( "Track Display Mode" ),
                                         HK_SWITCH_TRACK_DISPLAY_MODE, 'K' );
static Ki_HotkeyInfo HkAddModule( wxT( "Add Module" ), HK_ADD_MODULE, 'O' );

// List of common hotkey descriptors
Ki_HotkeyInfo* common_Hotkey_List[] =
{
    &HkHelp,        &HkZoomIn,          &HkZoomOut,
    &HkZoomRedraw,  &HkZoomCenter,      &HkZoomAuto,
    &HkSwitchUnits, &HkResetLocalCoord,
    &HkUndo,        &HkRedo,
    NULL
};

// List of hotkey descriptors for pcbnew
Ki_HotkeyInfo* board_edit_Hotkey_List[] =
{
    &HkTrackDisplayMode,       &HkDelete,
    &HkBackspace,
    &HkAddNewTrack,            &HkAddVia,                    &HkAddMicroVia,
    &HkSwitchTrackPosture,
    &HkDragTrackKeepSlope,
    &HkEndTrack,               &HkMoveItem,
    &HkFlipFootprint,          &HkRotateItem,                &HkDragFootprint,
    &HkGetAndMoveFootprint,    &HkLock_Unlock_Footprint,     &HkSavefile,
    &HkLoadfile,               &HkFindItem,                  &HkEditBoardItem,
    &HkSwitch2CopperLayer,     &HkSwitch2InnerLayer1,
    &HkSwitch2InnerLayer2,     &HkSwitch2InnerLayer3,        &HkSwitch2InnerLayer4,
    &HkSwitch2InnerLayer5,     &HkSwitch2InnerLayer6,        &HkSwitch2ComponentLayer,
    &HkSwitch2NextCopperLayer, &HkSwitch2PreviousCopperLayer,&HkAddModule,
    NULL
};

// List of hotkey descriptors for the module editor
Ki_HotkeyInfo* module_edit_Hotkey_List[] = {
    &HkMoveItem,               &HkRotateItem,                &HkEditBoardItem,
    &HkDelete,
    NULL
 };

// list of sections and corresponding hotkey list for pcbnew
 // (used to create an hotkey config file, and edit hotkeys )
struct Ki_HotkeyInfoSectionDescriptor g_Pcbnew_Editor_Hokeys_Descr[] =
{ {
      &g_CommonSectionTag, common_Hotkey_List, L"Common keys"
  },
  {
      &g_BoardEditorSectionTag, board_edit_Hotkey_List, L"Board editor keys"
  },{
      &g_ModuleEditSectionTag, module_edit_Hotkey_List, L"Footprint editor keys"
  },{
      NULL, NULL, NULL
  } };

// list of sections and corresponding hotkey list for the board editor
// (used to list current hotkeys in the board editor)
struct Ki_HotkeyInfoSectionDescriptor g_Board_Editor_Hokeys_Descr[] =
{ {
      &g_CommonSectionTag, common_Hotkey_List,
      NULL
  },{
      &g_BoardEditorSectionTag, board_edit_Hotkey_List, NULL
  },{
      NULL, NULL, NULL
  } };

// list of sections and corresponding hotkey list for the footprint editor
// (used to list current hotkeys in the module editor)
struct Ki_HotkeyInfoSectionDescriptor g_Module_Editor_Hokeys_Descr[] =
{ {
      &g_CommonSectionTag, common_Hotkey_List, NULL
  },{
      &g_ModuleEditSectionTag, module_edit_Hotkey_List, NULL
  },{
      NULL, NULL, NULL
  } };

