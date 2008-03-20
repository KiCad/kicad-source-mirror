/***************/
/* hotkeys.cpp */
/***************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "id.h"

#include "hotkeys.h"

#include "protos.h"

#include "schframe.h"

/* How to add a new hotkey:
 *  add a new id in the enum hotkey_id_commnand like MY_NEW_ID_FUNCTION (see hotkeys.h).
 *  add a new Ki_HotkeyInfo entry like:
 *  static Ki_HotkeyInfo HkMyNewEntry(wxT("Command Label"), MY_NEW_ID_FUNCTION, default key value);
 *      "Command Label" is the name used in hotkey list display, and the identifier in the hotkey list file
 *      MY_NEW_ID_FUNCTION is an equivalent id function used in the switch in OnHotKey() function.
 *      default key value is the default hotkey for this command. Can be overrided by the user hotkey list file
 *  add the HkMyNewEntry pointer in the s_Schematic_Hotkey_List list or the s_LibEdit_Hotkey_List list
 *  ( or s_Common_Hotkey_List if the same command is added both in eeschema and libedit)
 *  Add the new code in the switch in OnHotKey() function.
 *  when the variable ItemInEdit is true, an item is currently edited.
 *  This can be usefull if the new function cannot be executed while an item is currently being edited
 *  ( For example, one cannot start a new wire when a component is moving.)
 *
 *  Note: If an hotkey is a special key be sure the corresponding wxWidget keycode (WXK_XXXX)
 *  is handled in the hotkey_name_descr s_Hotkey_Name_List list (see hotkeys_basic.cpp)
 *  and see this list for some ascii keys (space ...)
 *  Key modifier are: GR_KB_CTRL GR_KB_ALT
 */


/* local variables */
/* Hotkey list: */

// Common commands
static Ki_HotkeyInfo    HkZoomCenter( wxT( "Zoom Center" ), HK_ZOOM_CENTER, WXK_F4 );
static Ki_HotkeyInfo    HkZoomRedraw( wxT( "Zoom Redraw" ), HK_ZOOM_REDRAW, WXK_F3 );
static Ki_HotkeyInfo    HkZoomOut( wxT( "Zoom Out" ), HK_ZOOM_OUT, WXK_F2 );
static Ki_HotkeyInfo    HkZoomIn( wxT( "Zoom In" ), HK_ZOOM_IN, WXK_F1 );
static Ki_HotkeyInfo    HkHelp( wxT( "Help: this message" ), HK_HELP, '?' );
static Ki_HotkeyInfo    HkResetLocalCoord( wxT( "Reset local coord." ), HK_RESET_LOCAL_COORD, ' ' );
static Ki_HotkeyInfo    HkUndo( wxT( "Undo" ), HK_UNDO, GR_KB_CTRL + 'Z', (int)ID_SCHEMATIC_UNDO );
static Ki_HotkeyInfo    HkRedo( wxT( "Redo" ), HK_REDO, GR_KB_CTRL + 'Y', (int)ID_SCHEMATIC_REDO );

// Schematic editor
static Ki_HotkeyInfo    HkBeginWire( wxT( "begin Wire" ), HK_BEGIN_WIRE, 'W' );
static Ki_HotkeyInfo    HkAddComponent( wxT( "Add Component" ), HK_ADD_NEW_COMPONENT, 'A' );
static Ki_HotkeyInfo    HkMirrorYComponent( wxT(
                                                "Mirror Y Component" ), HK_MIRROR_Y_COMPONENT, 'Y' );
static Ki_HotkeyInfo    HkMirrorXComponent( wxT(
                                                "Mirror X Component" ), HK_MIRROR_X_COMPONENT, 'X' );
static Ki_HotkeyInfo    HkOrientNormalComponent( wxT(
                                                     "Orient Normal Component" ),
                                                 HK_ORIENT_NORMAL_COMPONENT, 'N' );
static Ki_HotkeyInfo    HkRotateComponent( wxT( "Rotate Component" ), HK_ROTATE_COMPONENT, 'R' );
static Ki_HotkeyInfo    HkEditComponentValue( wxT( "Edit Component Value" ), HK_EDIT_COMPONENT_VALUE, 'V' );
static Ki_HotkeyInfo    HkEditComponentFootprint( wxT( "Edit Component Footprint" ), HK_EDIT_COMPONENT_FOOTPRINT, 'F' );
static Ki_HotkeyInfo    HkMoveComponent( wxT( "Move Component" ), HK_MOVE_COMPONENT, 'M', ID_POPUP_SCH_MOVE_CMP_REQUEST );
static Ki_HotkeyInfo    HkDragComponent( wxT( "Drag Component" ), HK_DRAG_COMPONENT, 'G', ID_POPUP_SCH_DRAG_CMP_REQUEST );
static Ki_HotkeyInfo    HkMove2Drag( wxT(
                                         "Switch move block to drag block" ),
                                     HK_MOVEBLOCK_TO_DRAGBLOCK, '\t' );
static Ki_HotkeyInfo    HkInsert( wxT( "Repeat Last Item" ), HK_REPEAT_LAST, WXK_INSERT );
static Ki_HotkeyInfo    HkDelete( wxT( "Delete Item" ), HK_DELETE, WXK_DELETE );
static Ki_HotkeyInfo    HkNextSearch( wxT( "Next Search" ), HK_NEXT_SEARCH, WXK_F5 );

// Library editor:
static Ki_HotkeyInfo    HkInsertPin( wxT( "Repeat Pin" ), HK_REPEAT_LAST, WXK_INSERT );
static Ki_HotkeyInfo    HkEditPin( wxT( "Edit Pin" ), HK_EDIT_PIN, 'E' );
static Ki_HotkeyInfo    HkMovePin( wxT( "Move Pin" ), HK_MOVE_PIN, 'M' );
static Ki_HotkeyInfo    HkDeletePin( wxT( "Delete Pin" ), HK_DELETE_PIN, WXK_DELETE );


// List of common hotkey descriptors
Ki_HotkeyInfo* s_Common_Hotkey_List[] =
{
    &HkHelp,
    &HkZoomIn,          &HkZoomOut, &HkZoomRedraw, &HkZoomCenter,
    &HkResetLocalCoord,
    &HkUndo,            &HkRedo,
    NULL
};

// List of hotkey descriptors for schematic
Ki_HotkeyInfo* s_Schematic_Hotkey_List[] = {
    &HkNextSearch,
    &HkDelete,          &HkInsert,           &HkMove2Drag,
    &HkMoveComponent,   &HkDragComponent,    &HkAddComponent,
    &HkRotateComponent, &HkMirrorXComponent, &HkMirrorYComponent, &HkOrientNormalComponent,
    &HkEditComponentValue, &HkEditComponentFootprint,
    &HkBeginWire,
    NULL
};

// List of hotkey descriptors for libray editor
Ki_HotkeyInfo* s_LibEdit_Hotkey_List[] =
{
    &HkInsertPin,
    &HkEditPin,
    &HkMovePin,
    &HkDeletePin,
    NULL
};

// list of sections and corresponding hotkey list for eeschema (used to create an hotkey config file)
struct Ki_HotkeyInfoSectionDescriptor s_Eeschema_Hokeys_Descr[] =
{
    { &g_CommonSectionTag,    s_Common_Hotkey_List,    "Common keys"           },
    { &g_SchematicSectionTag, s_Schematic_Hotkey_List, "Schematic editor keys" },
    { &g_LibEditSectionTag,   s_LibEdit_Hotkey_List,   "library editor keys"   },
    { NULL,                   NULL }
};

// list of sections and corresponding hotkey list for the schematic editor (used to list current hotkeys)
struct Ki_HotkeyInfoSectionDescriptor s_Schematic_Hokeys_Descr[] =
{
    { &g_CommonSectionTag,    s_Common_Hotkey_List,    NULL },
    { &g_SchematicSectionTag, s_Schematic_Hotkey_List, NULL },
    { NULL,                   NULL,                    NULL }
};

// list of sections and corresponding hotkey list for the component editor (used to list current hotkeys)
struct Ki_HotkeyInfoSectionDescriptor s_Libedit_Hokeys_Descr[] =
{
    { &g_CommonSectionTag,  s_Common_Hotkey_List,  NULL },
    { &g_LibEditSectionTag, s_LibEdit_Hotkey_List, NULL },
    { NULL,                 NULL,                  NULL }
};

/***********************************************************/
void WinEDA_SchematicFrame::OnHotKey( wxDC* DC, int hotkey,
                                      EDA_BaseStruct* DrawStruct )
/***********************************************************/

/* Hot keys. Some commands are relative to the item under the mouse cursor
 *  Commands are case insensitive
 */
{
    bool ItemInEdit = GetScreen()->GetCurItem()
            && GetScreen()->GetCurItem()->m_Flags;
    bool RefreshToolBar = FALSE; // We must refresh tool bar when the undo/redo tool state is modified

    if( hotkey == 0 )
        return;

    wxPoint MousePos = GetScreen()->m_MousePosition;

    // Remap the control key Ctrl A (0x01) to GR_KB_CTRL + 'A' (easier to handle...)
    if( (hotkey & GR_KB_CTRL) != 0 )
        hotkey += 'A' - 1;
    /* Convert lower to upper case (the usual toupper function has problem with non ascii codes like function keys */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';

    // Search command from key :
    Ki_HotkeyInfo * HK_Descr = GetDescriptorFromHotkey( hotkey, s_Common_Hotkey_List );
    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( hotkey, s_Schematic_Hotkey_List );
    if( HK_Descr == NULL ) return;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;
        break;

    case HK_HELP:       // Display Current hotkey list
        DisplayHotkeyList( this, s_Schematic_Hokeys_Descr );
        break;

    case HK_RESET_LOCAL_COORD:         /* Reset the relative coord */
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
        break;

    case HK_ZOOM_IN:
        OnZoom( ID_ZOOM_IN_KEY );
        break;

    case HK_ZOOM_OUT:
        OnZoom( ID_ZOOM_OUT_KEY );
        break;

    case HK_ZOOM_REDRAW:
        OnZoom( ID_ZOOM_REDRAW_KEY );
        break;

    case HK_ZOOM_CENTER:
        OnZoom( ID_ZOOM_CENTER_KEY );
        break;

    case HK_UNDO:
    case HK_REDO:
        if( ItemInEdit )
            break;
    {
        wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED, HK_Descr->m_IdMenuEvent );

        wxPostEvent( this, event );
    }
        break;

    case HK_MOVEBLOCK_TO_DRAGBLOCK:        // Switch to drag mode, when block moving
        HandleBlockEndByPopUp( BLOCK_DRAG, DC );
        break;

    case HK_DELETE:
        if( ItemInEdit )
            break;
        RefreshToolBar = LocateAndDeleteItem( this, DC );
        GetScreen()->SetModify();
        GetScreen()->SetCurItem( NULL );
        TestDanglingEnds( GetScreen()->EEDrawList, DC );
        break;

    case HK_REPEAT_LAST:
        if( ItemInEdit )
            break;
        if( g_ItemToRepeat && (g_ItemToRepeat->m_Flags == 0) )
        {
            RepeatDrawItem( DC );
        }
        break;

    case HK_NEXT_SEARCH:
        if( ItemInEdit )
            break;
        if( g_LastSearchIsMarker )
            WinEDA_SchematicFrame::FindMarker( 1 );
        else
            FindSchematicItem( wxEmptyString, 2 );
        break;

    case HK_ADD_NEW_COMPONENT:      // Add component
        if( ItemInEdit )
            break;

        // switch to m_ID_current_state = ID_COMPONENT_BUTT;
        if( m_ID_current_state != ID_COMPONENT_BUTT )
            SetToolID( ID_COMPONENT_BUTT, wxCURSOR_PENCIL, _( "Add Component" ) );
        OnLeftClick( DC, MousePos );
        break;

    case HK_BEGIN_WIRE:                     // Add wire
        if( DrawStruct )                    // An item is selected. If edited and not a wire, a new command is not possible
        {
            if( DrawStruct->m_Flags )       // Item selected and edition in progress
            {
                if( DrawStruct->Type() == DRAW_SEGMENT_STRUCT_TYPE )
                {
                    EDA_DrawLineStruct* segment = (EDA_DrawLineStruct*) DrawStruct;
                    if( segment->m_Layer != LAYER_WIRE )
                        break;
                }
                else
                    break;
            }
        }

        // switch to m_ID_current_state = ID_WIRE_BUTT;
        if( m_ID_current_state != ID_WIRE_BUTT )
            SetToolID( ID_WIRE_BUTT, wxCURSOR_PENCIL, _( "Add Wire" ) );
        OnLeftClick( DC, MousePos );
        break;

    case HK_ROTATE_COMPONENT:       // Component Rotation
        if( DrawStruct == NULL )
        {
            DrawStruct = PickStruct( GetScreen()->m_Curseur,
                                     GetScreen(), LIBITEM | TEXTITEM | LABELITEM );
            if( DrawStruct == NULL )
                break;
            if( DrawStruct->Type() == TYPE_SCH_COMPONENT )
                DrawStruct = LocateSmallestComponent( (SCH_SCREEN*)GetScreen() );
            if( DrawStruct == NULL )
                break;
        }

        switch( DrawStruct->Type() )
        {
        case TYPE_SCH_COMPONENT:
            if( DrawStruct->m_Flags == 0 )
            {
                SaveCopyInUndoList( DrawStruct, IS_CHANGED );
                RefreshToolBar = TRUE;
            }

            CmpRotationMiroir(
                (SCH_COMPONENT*) DrawStruct, DC, CMP_ROTATE_COUNTERCLOCKWISE );
            break;

        case TYPE_SCH_TEXT:
        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
            if( DrawStruct->m_Flags == 0 )
            {
                SaveCopyInUndoList( DrawStruct, IS_CHANGED );
                RefreshToolBar = TRUE;
            }
            ChangeTextOrient( (SCH_TEXT*) DrawStruct, DC );
            break;

        default:
            ;
        }

        break;

    case HK_MIRROR_Y_COMPONENT:     // Mirror Y (Component)
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( (SCH_SCREEN*)GetScreen() );
        if( DrawStruct )
        {
            if( DrawStruct->m_Flags == 0 )
            {
                SaveCopyInUndoList( DrawStruct, IS_CHANGED );
                RefreshToolBar = TRUE;
            }
            CmpRotationMiroir(
                (SCH_COMPONENT*) DrawStruct, DC, CMP_MIROIR_Y );
        }
        break;

    case HK_MIRROR_X_COMPONENT:     // Mirror X (Component)
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( (SCH_SCREEN*)GetScreen() );
        if( DrawStruct )
        {
            if( DrawStruct->m_Flags == 0 )
            {
                SaveCopyInUndoList( DrawStruct, IS_CHANGED );
                RefreshToolBar = TRUE;
            }
            CmpRotationMiroir(
                (SCH_COMPONENT*) DrawStruct, DC, CMP_MIROIR_X );
        }
        break;

    case HK_ORIENT_NORMAL_COMPONENT:        // Orient 0, no mirror (Component)
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( (SCH_SCREEN*)GetScreen() );
        if( DrawStruct )
        {
            if( DrawStruct->m_Flags == 0 )
            {
                SaveCopyInUndoList( DrawStruct, IS_CHANGED );
                RefreshToolBar = TRUE;
            }
            CmpRotationMiroir(
                (SCH_COMPONENT*) DrawStruct, DC, CMP_NORMAL );
            TestDanglingEnds( (SCH_SCREEN*)GetScreen()->EEDrawList, DC );
        }
        break;

    case HK_DRAG_COMPONENT:         // Start drag Component
    case HK_MOVE_COMPONENT:         // Start move Component
        if( ItemInEdit )
            break;
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( (SCH_SCREEN*)GetScreen() );
        if( DrawStruct && (DrawStruct->m_Flags ==0) )
        {
            ((SCH_SCREEN*)GetScreen())->SetCurItem( DrawStruct );
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED, HK_Descr->m_IdMenuEvent );

            wxPostEvent( this, event );
        }
        break;
    case HK_EDIT_COMPONENT_VALUE:
        if( ItemInEdit )
            break;
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( (SCH_SCREEN*)GetScreen() );
        if(DrawStruct)
        {
            EditComponentValue(
                (SCH_COMPONENT*) DrawStruct, DC );
        }
        break;

    case HK_EDIT_COMPONENT_FOOTPRINT:
        if( ItemInEdit )
            break;
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( (SCH_SCREEN*)GetScreen() );
        if(DrawStruct)
        {
            EditComponentFootprint(
                (SCH_COMPONENT*) DrawStruct, DC );
        }
        break;
    }

    if( RefreshToolBar )
        SetToolbars();
}


/***********************************************************/
void WinEDA_LibeditFrame::OnHotKey( wxDC* DC, int hotkey,
                                    EDA_BaseStruct* DrawStruct )
/***********************************************************/

/* Hot keys for the component editor. Some commands are relatives to the item under the mouse cursor
 *  Commands are case insensitive
 */
{
    bool ItemInEdit = GetScreen()->GetCurItem()
            && GetScreen()->GetCurItem()->m_Flags;
    bool RefreshToolBar = FALSE; // We must refresh tool bar when the undo/redo tool state is modified

    if( hotkey == 0 )
        return;

    wxPoint MousePos = GetScreen()->m_MousePosition;

    LibEDA_BaseStruct* DrawEntry = LocateItemUsingCursor();

    // Remap the control key Ctrl A (0x01) to GR_KB_CTRL + 'A' (easier to handle...)
    if( (hotkey & GR_KB_CTRL) != 0 )
        hotkey += 'A' - 1;
    /* Convert lower to upper case (the usual toupper function has problem with non ascii codes like function keys */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';
    Ki_HotkeyInfo * HK_Descr = GetDescriptorFromHotkey( hotkey, s_Common_Hotkey_List );
    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( hotkey, s_LibEdit_Hotkey_List );
    if( HK_Descr == NULL ) return;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;
        break;

    case HK_HELP:       // Display Current hotkey list
        DisplayHotkeyList( this, s_Libedit_Hokeys_Descr );
        break;

    case HK_RESET_LOCAL_COORD:         /* Reset the relative coord */
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
        break;

    case HK_ZOOM_IN:
        OnZoom( ID_ZOOM_IN_KEY );
        break;

    case HK_ZOOM_OUT:
        OnZoom( ID_ZOOM_OUT_KEY );
        break;

    case HK_ZOOM_REDRAW:
        OnZoom( ID_ZOOM_REDRAW_KEY );
        break;

    case HK_ZOOM_CENTER:
        OnZoom( ID_ZOOM_CENTER_KEY );
        break;

    case HK_UNDO:
    case HK_REDO:
        if( ItemInEdit )
            break;
    {
        wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED, HK_Descr->m_IdMenuEvent );

        wxPostEvent( this, event );
    }
        break;

    case HK_REPEAT_LAST:
        if( LibItemToRepeat && (LibItemToRepeat->m_Flags == 0)
           && (LibItemToRepeat->Type() == COMPONENT_PIN_DRAW_TYPE) )
        {
            RepeatPinItem( DC, (LibDrawPin*) LibItemToRepeat );
        }
        else
            wxBell();
        break;
    case HK_EDIT_PIN:
        if(DrawEntry)
            CurrentDrawItem = DrawEntry;
        if(CurrentDrawItem)
        {
            if(CurrentDrawItem->Type() == COMPONENT_PIN_DRAW_TYPE)
                InstallPineditFrame( this, DC, MousePos );
        }
        break;
    case HK_DELETE_PIN:
        if(DrawEntry)
            CurrentDrawItem = DrawEntry;
        if(CurrentDrawItem)
        {
            wxCommandEvent evt;
            evt.SetId(ID_POPUP_LIBEDIT_DELETE_ITEM);
            Process_Special_Functions(evt);
        }
        break;
    case HK_MOVE_PIN:
        if(DrawEntry)
            CurrentDrawItem = DrawEntry;
        if(CurrentDrawItem)
        {
            wxCommandEvent evt;
            evt.SetId(ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST);
            Process_Special_Functions(evt);
        }
        break;
    }
    if( RefreshToolBar )
        SetToolbars();
}
