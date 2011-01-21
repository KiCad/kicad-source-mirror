/***************/
/* hotkeys.cpp */
/***************/

#include "fctsys.h"
#include "common.h"
#include "eeschema_id.h"
#include "hotkeys.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_libentry.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_component.h"
#include "sch_sheet.h"

#include "dialogs/dialog_schematic_find.h"


/* How to add a new hotkey:
 * add a new id in the enum hotkey_id_command like MY_NEW_ID_FUNCTION (see
 * hotkeys.h).
 * add a new Ki_HotkeyInfo entry like:
 *  static Ki_HotkeyInfo HkMyNewEntry(wxT("Command Label"), MY_NEW_ID_FUNCTION,
 *                                    default key value);
 * wxT("Command Label") is the name used in hotkey list display, and the
 * identifier in the hotkey list file
 * MY_NEW_ID_FUNCTION is an equivalent id function used in the switch in
 * OnHotKey() function.
 * default key value is the default hotkey for this command. Can be overridden
 * by the user hotkey list file
 * add the HkMyNewEntry pointer in the s_Schematic_Hotkey_List list or the
 * s_LibEdit_Hotkey_List list or s_Common_Hotkey_List if the same command is
 * added both in eeschema and libedit)
 * Add the new code in the switch in OnHotKey() function.
 * when the variable itemInEdit is true, an item is currently edited.
 * This can be useful if the new function cannot be executed while an item is
 * currently being edited
 * ( For example, one cannot start a new wire when a component is moving.)
 *
 * Note: If an hotkey is a special key be sure the corresponding wxWidget
 *       keycode (WXK_XXXX) is handled in the hotkey_name_descr
 *       s_Hotkey_Name_List list (see hotkeys_basic.cpp) and see this list
 *       for some ascii keys (space ...)
 *
 *  Key modifier are: GR_KB_CTRL GR_KB_ALT
 */


/* local variables */
/* Hotkey list: */

/**
 * Common commands
 */

/* Fit on Screen */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkZoomAuto( wxT( "Fit on Screen" ), HK_ZOOM_AUTO, WXK_HOME );
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

static Ki_HotkeyInfo HkHelp( wxT( "Help: this message" ), HK_HELP, '?' );
static Ki_HotkeyInfo HkResetLocalCoord( wxT( "Reset local coord." ),
                                        HK_RESET_LOCAL_COORD, ' ' );

/* Undo */
static Ki_HotkeyInfo HkUndo( wxT( "Undo" ), HK_UNDO, GR_KB_CTRL + 'Z', (int) wxID_UNDO );

/* Redo */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkRedo( wxT( "Redo" ), HK_REDO, GR_KB_CTRL + 'Y', (int) wxID_REDO );
#else
static Ki_HotkeyInfo HkRedo( wxT( "Redo" ), HK_REDO, GR_KB_SHIFT + GR_KB_CTRL + 'Z',
                             (int) wxID_REDO );
#endif

// Schematic editor
static Ki_HotkeyInfo HkAddLabel( wxT( "add Label" ), HK_ADD_LABEL, 'L' );
static Ki_HotkeyInfo HkAddHierarchicalLabel( wxT( "add Hierarchical Label" ), HK_ADD_HLABEL, 'H' );
static Ki_HotkeyInfo HkAddGlobalLabel( wxT( "add Global Label" ), HK_ADD_GLABEL, GR_KB_CTRL + 'L' );
static Ki_HotkeyInfo HkAddJunction( wxT( "add Junction" ), HK_ADD_JUNCTION, 'J' );
static Ki_HotkeyInfo HkBeginWire( wxT( "begin Wire" ), HK_BEGIN_WIRE, 'W' );
static Ki_HotkeyInfo HkBeginBus( wxT( "begin Bus" ), HK_BEGIN_BUS, 'B' );
static Ki_HotkeyInfo HkAddComponent( wxT( "Add Component" ), HK_ADD_NEW_COMPONENT, 'A' );
static Ki_HotkeyInfo HkAddPower( wxT( "Add Power" ), HK_ADD_NEW_POWER, 'P' );
static Ki_HotkeyInfo HkAddNoConn( wxT( "Add NoConnected Flag" ), HK_ADD_NOCONN_FLAG, 'Q' );
static Ki_HotkeyInfo HkAddHierSheet( wxT( "Add Sheet" ), HK_ADD_HIER_SHEET, 'S' );
static Ki_HotkeyInfo HkAddBusEntry( wxT( "Add Bus Entry" ), HK_ADD_BUS_ENTRY, '/' );
static Ki_HotkeyInfo HkAddWireEntry( wxT( "Add Wire Entry" ), HK_ADD_WIRE_ENTRY, 'Z' );
static Ki_HotkeyInfo HkAddGraphicPolyLine( wxT( "Add Graphic PolyLine" ), HK_ADD_GRAPHIC_POLYLINE, 'I' );
static Ki_HotkeyInfo HkAddGraphicText( wxT( "Add Graphic Text" ), HK_ADD_GRAPHIC_TEXT, 'T' );
static Ki_HotkeyInfo HkMirrorYComponent( wxT( "Mirror Y Component" ), HK_MIRROR_Y_COMPONENT, 'Y' );
static Ki_HotkeyInfo HkMirrorXComponent( wxT( "Mirror X Component" ), HK_MIRROR_X_COMPONENT, 'X' );
static Ki_HotkeyInfo HkOrientNormalComponent( wxT( "Orient Normal Component" ),
                                              HK_ORIENT_NORMAL_COMPONENT, 'N' );
static Ki_HotkeyInfo HkRotate( wxT( "Rotate Item" ), HK_ROTATE, 'R' );
static Ki_HotkeyInfo HkEdit( wxT( "Edit Schematic Item" ), HK_EDIT, 'E' );
static Ki_HotkeyInfo HkEditComponentValue( wxT( "Edit Component Value" ),
                                           HK_EDIT_COMPONENT_VALUE, 'V' );
static Ki_HotkeyInfo HkEditComponentFootprint( wxT( "Edit Component Footprint" ),
                                               HK_EDIT_COMPONENT_FOOTPRINT, 'F' );
static Ki_HotkeyInfo HkMove( wxT( "Move Schematic Item" ),
                             HK_MOVE_COMPONENT_OR_ITEM, 'M',
                             ID_POPUP_SCH_MOVE_CMP_REQUEST );

static Ki_HotkeyInfo HkCopyComponentOrText( wxT( "Copy Component or Label" ),
                                            HK_COPY_COMPONENT_OR_LABEL, 'C',
                                            ID_POPUP_SCH_COPY_ITEM );

static Ki_HotkeyInfo HkDrag( wxT( "Drag Schematic Item" ), HK_DRAG, 'G',
                             ID_POPUP_SCH_DRAG_CMP_REQUEST );
static Ki_HotkeyInfo HkMove2Drag( wxT( "Switch move block to drag block" ),
                                  HK_MOVEBLOCK_TO_DRAGBLOCK, '\t' );
static Ki_HotkeyInfo HkInsert( wxT( "Repeat Last Item" ), HK_REPEAT_LAST, WXK_INSERT );
static Ki_HotkeyInfo HkDelete( wxT( "Delete Item" ), HK_DELETE, WXK_DELETE );

static Ki_HotkeyInfo HkFindItem( wxT( "Find Item" ), HK_FIND_ITEM, 'F' + GR_KB_CTRL );
static Ki_HotkeyInfo HkFindNextItem( wxT( "Find Next Item" ), HK_FIND_NEXT_ITEM, WXK_F5 );
static Ki_HotkeyInfo HkFindNextDrcMarker( wxT( "Find next DRC marker" ), HK_FIND_NEXT_DRC_MARKER,
                                          WXK_F5 + GR_KB_SHIFT );

// Special keys for library editor:
static Ki_HotkeyInfo HkCreatePin( wxT( "Create Pin" ), HK_LIBEDIT_CREATE_PIN, 'P' );
static Ki_HotkeyInfo HkInsertPin( wxT( "Repeat Pin" ), HK_REPEAT_LAST, WXK_INSERT );
static Ki_HotkeyInfo HkMoveLibItem( wxT( "Move Lib Item" ), HK_LIBEDIT_MOVE_GRAPHIC_ITEM, 'M' );


// List of common hotkey descriptors
Ki_HotkeyInfo* s_Common_Hotkey_List[] =
{
    &HkHelp,
    &HkZoomIn,
    &HkZoomOut,
    &HkZoomRedraw,
    &HkZoomCenter,
    &HkZoomAuto,
    &HkResetLocalCoord,
    &HkUndo,
    &HkRedo,
    NULL
};

// List of hotkey descriptors for schematic
Ki_HotkeyInfo* s_Schematic_Hotkey_List[] =
{
    &HkFindItem,
    &HkFindNextItem,
    &HkFindNextDrcMarker,
    &HkDelete,
    &HkInsert,
    &HkMove2Drag,
    &HkMove,
    &HkCopyComponentOrText,
    &HkDrag,
    &HkAddComponent,
    &HkAddPower,
    &HkRotate,
    &HkMirrorXComponent,
    &HkMirrorYComponent,
    &HkOrientNormalComponent,
    &HkEdit,
    &HkEditComponentValue,
    &HkEditComponentFootprint,
    &HkBeginWire,
    &HkBeginBus,
    &HkAddLabel,
    &HkAddHierarchicalLabel,
    &HkAddGlobalLabel,
    &HkAddJunction,
    &HkAddNoConn,
    &HkAddHierSheet,
    &HkAddWireEntry,
    &HkAddBusEntry,
    &HkAddGraphicPolyLine,
    &HkAddGraphicText,
    NULL
};

// List of hotkey descriptors for library editor
Ki_HotkeyInfo* s_LibEdit_Hotkey_List[] =
{
    &HkCreatePin,
    &HkInsertPin,
    &HkEdit,
    &HkMoveLibItem,
    &HkDelete,
    &HkRotate,
    &HkDrag,
    NULL
};

// list of sections and corresponding hotkey list for eeschema (used to create
// an hotkey config file)
struct Ki_HotkeyInfoSectionDescriptor s_Eeschema_Hokeys_Descr[] =
{
    { &g_CommonSectionTag,    s_Common_Hotkey_List,    L"Common keys"           },
    { &g_SchematicSectionTag, s_Schematic_Hotkey_List, L"Schematic editor keys" },
    { &g_LibEditSectionTag,   s_LibEdit_Hotkey_List,   L"library editor keys"   },
    { NULL,                   NULL,                    NULL                    }
};

// list of sections and corresponding hotkey list for the schematic editor
// (used to list current hotkeys)
struct Ki_HotkeyInfoSectionDescriptor s_Schematic_Hokeys_Descr[] =
{
    { &g_CommonSectionTag,    s_Common_Hotkey_List,    NULL },
    { &g_SchematicSectionTag, s_Schematic_Hotkey_List, NULL },
    { NULL,                   NULL,                    NULL }
};

// list of sections and corresponding hotkey list for the component editor
// (used to list current hotkeys)
struct Ki_HotkeyInfoSectionDescriptor s_Libedit_Hokeys_Descr[] =
{
    { &g_CommonSectionTag,  s_Common_Hotkey_List,  NULL },
    { &g_LibEditSectionTag, s_LibEdit_Hotkey_List, NULL },
    { NULL,                 NULL,                  NULL }
};

// list of sections and corresponding hotkey list for the component browser
// (used to list current hotkeys)
struct Ki_HotkeyInfoSectionDescriptor s_Viewlib_Hokeys_Descr[] =
{
    { &g_CommonSectionTag, s_Common_Hotkey_List, NULL },
    { NULL,                NULL,                 NULL }
};

/*
 * Hot keys. Some commands are relative to the item under the mouse cursor
 * Commands are case insensitive
 */
void SCH_EDIT_FRAME::OnHotKey( wxDC* DC, int hotkey, EDA_ITEM* DrawStruct )
{
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );

    cmd.SetEventObject( this );

    SCH_SCREEN* screen = GetScreen();
    // itemInEdit == false means no item currently edited. We can ask for editing a new item
    bool        itemInEdit = screen->GetCurItem() && screen->GetCurItem()->m_Flags;
    // notBusy == true means no item currently edited and no other command in progress
    // We can change active tool and ask for editing a new item
    bool        notBusy = (!itemInEdit) && (screen->m_BlockLocate.m_State == STATE_NO_BLOCK);
    bool        RefreshToolBar = FALSE;

    if( hotkey == 0 )
        return;

    wxPoint MousePos = GetScreen()->m_MousePosition;

    /* Convert lower to upper case (the usual toupper function has problem
     * with non ascii codes like function keys */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';

    // Search command from key :
    Ki_HotkeyInfo* HK_Descr = GetDescriptorFromHotkey( hotkey, s_Common_Hotkey_List );
    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( hotkey, s_Schematic_Hotkey_List );
    if( HK_Descr == NULL )
        return;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;

    case HK_HELP:       // Display Current hotkey list
        DisplayHotkeyList( this, s_Schematic_Hokeys_Descr );
        break;

    case HK_RESET_LOCAL_COORD:         /* Reset the relative coord */
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
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

    case HK_UNDO:
    case HK_REDO:
        if( notBusy )
        {
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED, HK_Descr->m_IdMenuEvent );
            wxPostEvent( this, event );
        }
        break;

    case HK_MOVEBLOCK_TO_DRAGBLOCK:   // Switch to drag mode, when block moving
        HandleBlockEndByPopUp( BLOCK_DRAG, DC );
        break;

    case HK_DELETE:
        if( notBusy)
        {
            RefreshToolBar = LocateAndDeleteItem( this, DC );
            OnModify();
            GetScreen()->SetCurItem( NULL );
            GetScreen()->TestDanglingEnds( DrawPanel, DC );
        }
        break;

    case HK_REPEAT_LAST:
        if( notBusy && m_itemToRepeat && ( m_itemToRepeat->m_Flags == 0 ) )
            RepeatDrawItem( DC );
        break;

    case HK_FIND_ITEM:
        if( notBusy )
        {
            wxCommandEvent evt;
            evt.SetId( ID_FIND_ITEMS );
            Process_Special_Functions( evt );
        }
        break;

    case HK_FIND_NEXT_ITEM:
        if( notBusy )
        {
            wxFindDialogEvent event( wxEVT_COMMAND_FIND, GetId() );
            event.SetEventObject( this );
            event.SetFlags( m_findReplaceData->GetFlags() );
            event.SetFindString( m_findReplaceData->GetFindString() );
            GetEventHandler()->ProcessEvent( event );
        }
        break;

    case HK_FIND_NEXT_DRC_MARKER:
        if( notBusy )
        {
            wxFindDialogEvent event( EVT_COMMAND_FIND_DRC_MARKER, GetId() );
            event.SetEventObject( this );
            event.SetFlags( m_findReplaceData->GetFlags() );
            event.SetFindString( m_findReplaceData->GetFindString() );
            GetEventHandler()->ProcessEvent( event );
        }
        break;

    case HK_ADD_NEW_COMPONENT:      // Add component
        if( !itemInEdit )
        {
            // switch to m_ID_current_state = ID_COMPONENT_BUTT;
            if( m_ID_current_state != ID_COMPONENT_BUTT )
                SetToolID( ID_COMPONENT_BUTT, wxCURSOR_PENCIL, _( "Add Component" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ADD_NEW_POWER:      // Add power component
        if( !itemInEdit )
        {
            // switch to m_ID_current_state = ID_PLACE_POWER_BUTT;
            if( m_ID_current_state != ID_PLACE_POWER_BUTT )
                SetToolID( ID_PLACE_POWER_BUTT, wxCURSOR_PENCIL, _( "Add Power" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ADD_LABEL:
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_LABEL_BUTT;
            if( m_ID_current_state != ID_LABEL_BUTT )
                SetToolID( ID_LABEL_BUTT, wxCURSOR_PENCIL, _( "Add Label" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ADD_HLABEL:
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_HIERLABEL_BUTT;
            if( m_ID_current_state != ID_HIERLABEL_BUTT )
                SetToolID( ID_HIERLABEL_BUTT, wxCURSOR_PENCIL, _( "Add Hierarchical Label" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ADD_GLABEL:
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_GLABEL_BUTT;
            if( m_ID_current_state != ID_GLABEL_BUTT )
                SetToolID( ID_GLABEL_BUTT, wxCURSOR_PENCIL, _( "Add Global Label" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ADD_JUNCTION:
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_JUNCTION_BUTT;
            if( m_ID_current_state != ID_JUNCTION_BUTT )
                SetToolID( ID_JUNCTION_BUTT, wxCURSOR_PENCIL, _( "Add Junction" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ADD_WIRE_ENTRY:
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_WIRETOBUS_ENTRY_BUTT;
            if( m_ID_current_state != ID_WIRETOBUS_ENTRY_BUTT )
                SetToolID( ID_WIRETOBUS_ENTRY_BUTT, wxCURSOR_PENCIL, _( "Add Wire to Bus entry" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ADD_BUS_ENTRY:
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_BUSTOBUS_ENTRY_BUTT;
            if( m_ID_current_state != ID_BUSTOBUS_ENTRY_BUTT )
                SetToolID( ID_BUSTOBUS_ENTRY_BUTT, wxCURSOR_PENCIL, _( "Add Bus to Bus entry" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ADD_HIER_SHEET:
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_SHEET_SYMBOL_BUTT;
            if( m_ID_current_state != ID_SHEET_SYMBOL_BUTT )
                SetToolID( ID_SHEET_SYMBOL_BUTT, wxCURSOR_PENCIL, _( "Add Sheet" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ADD_GRAPHIC_TEXT:
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_TEXT_COMMENT_BUTT;
            if( m_ID_current_state != ID_TEXT_COMMENT_BUTT )
                SetToolID( ID_TEXT_COMMENT_BUTT, wxCURSOR_PENCIL, _( "Add Text" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ADD_GRAPHIC_POLYLINE:
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_LINE_COMMENT_BUTT;
            if( m_ID_current_state != ID_LINE_COMMENT_BUTT )
                SetToolID( ID_LINE_COMMENT_BUTT, wxCURSOR_PENCIL, _( "Add Lines" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_BEGIN_BUS:
        // An item can be selected. If not a Bus, a begin command is not possible
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_WIRE_BUTT;
            if( m_ID_current_state != ID_BUS_BUTT )
                SetToolID( ID_BUS_BUTT, wxCURSOR_PENCIL, _( "Add Bus" ) );
            OnLeftClick( DC, MousePos );
            break;
        }
        if( DrawStruct && DrawStruct->IsNew() && ( m_ID_current_state == ID_BUS_BUTT ) )
        {
            if( DrawStruct->Type() == SCH_LINE_T )
            {
                SCH_LINE* segment = (SCH_LINE*) DrawStruct;

                if( segment->GetLayer() != LAYER_BUS )
                    break;

            // Bus in progress:
            OnLeftClick( DC, MousePos );
            }
        }
        break;

    case HK_BEGIN_WIRE:
        // An item can be selected. If not a wire, a begin command is not possible
        if( notBusy )
        {
            // switch to m_ID_current_state = ID_WIRE_BUTT;
            if( m_ID_current_state != ID_WIRE_BUTT )
                SetToolID( ID_WIRE_BUTT, wxCURSOR_PENCIL, _( "Add Wire" ) );
            OnLeftClick( DC, MousePos );
            break;
        }
        if( DrawStruct && DrawStruct->IsNew() && ( m_ID_current_state == ID_WIRE_BUTT ) )
        {
            if( DrawStruct->Type() == SCH_LINE_T )
            {
                SCH_LINE* segment = (SCH_LINE*) DrawStruct;
                if( segment->GetLayer() != LAYER_WIRE )
                    break;
            // Wire in progress:
            OnLeftClick( DC, MousePos );
            }
        }
        break;

    case HK_ADD_NOCONN_FLAG:      // Add a no connected flag
        if( notBusy )
        {
            if( m_ID_current_state != ID_NOCONN_BUTT )
                SetToolID( ID_NOCONN_BUTT, wxCURSOR_PENCIL, _( "Add \"NoNonnect\" Flags" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ROTATE:       // Component or other schematic item rotation
        if ( screen->m_BlockLocate.m_State != STATE_NO_BLOCK)//allows bloc operation on hotkey
        {
            HandleBlockEndByPopUp(BLOCK_ROTATE, DC );
            break;
        }
        if( DrawStruct == NULL )
        {
            // Find the schematic object to rotate under the cursor
            DrawStruct = SchematicGeneralLocateAndDisplay( false );

            if( DrawStruct == NULL )
                break;

            if( DrawStruct->Type() == SCH_COMPONENT_T )
                DrawStruct = LocateSmallestComponent( GetScreen() );

            if( DrawStruct == NULL )
                break;
        }

        if( DrawStruct )
        {
            GetScreen()->SetCurItem( (SCH_ITEM*) DrawStruct );

            // Create the events for rotating a component or other schematic item
            wxCommandEvent eventRotateComponent( wxEVT_COMMAND_TOOL_CLICKED,
                                                 ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE );
            wxCommandEvent eventRotateText( wxEVT_COMMAND_TOOL_CLICKED,
                                            ID_POPUP_SCH_ROTATE_TEXT );
            wxCommandEvent eventRotateField( wxEVT_COMMAND_TOOL_CLICKED,
                                             ID_POPUP_SCH_ROTATE_FIELD );

            switch( DrawStruct->Type() )
            {
            case SCH_SHEET_T: //TODO allow sheet rotate on hotkey
                //wxPostEvent( this, eventRotateSheet );
                break;
            case SCH_COMPONENT_T:
                wxPostEvent( this, eventRotateComponent );
                break;

            case SCH_TEXT_T:
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIERARCHICAL_LABEL_T:
                wxPostEvent( this, eventRotateText );
                break;

            case SCH_FIELD_T:
                wxPostEvent( this, eventRotateField );

            default:
                ;
            }
        }

        break;

    case HK_MIRROR_Y_COMPONENT:     // Mirror Y (Component)
        if ( screen->m_BlockLocate.m_State != STATE_NO_BLOCK )
        {
            HandleBlockEndByPopUp(BLOCK_MIRROR_Y, DC );
            break;
        }
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( GetScreen() );
        if( DrawStruct )
        {
            if( DrawStruct->m_Flags == 0 )
            {
                SaveCopyInUndoList( (SCH_ITEM*) DrawStruct, UR_CHANGED );
                RefreshToolBar = TRUE;
            }
            CmpRotationMiroir( (SCH_COMPONENT*) DrawStruct, DC, CMP_MIRROR_Y );
        }
        break;

    case HK_MIRROR_X_COMPONENT:     // Mirror X (Component)
        if ( screen->m_BlockLocate.m_State != STATE_NO_BLOCK ) //allows bloc operation on hotkey
		{
            HandleBlockEndByPopUp(BLOCK_MIRROR_X, DC );
            break;
		}
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( GetScreen() );
        if( DrawStruct )
        {
            if( DrawStruct->m_Flags == 0 )
            {
                SaveCopyInUndoList( (SCH_ITEM*) DrawStruct, UR_CHANGED );
                RefreshToolBar = TRUE;
            }
            CmpRotationMiroir( (SCH_COMPONENT*) DrawStruct, DC, CMP_MIRROR_X );
        }
        break;

    case HK_ORIENT_NORMAL_COMPONENT:        // Orient 0, no mirror (Component)
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( GetScreen() );
        if( DrawStruct )
        {
            if( DrawStruct->m_Flags == 0 )
            {
                SaveCopyInUndoList( (SCH_ITEM*) DrawStruct, UR_CHANGED );
                RefreshToolBar = TRUE;
            }
            CmpRotationMiroir( (SCH_COMPONENT*) DrawStruct, DC, CMP_NORMAL );
            GetScreen()->TestDanglingEnds( DrawPanel, DC );
        }
        break;

    case HK_DRAG:                           // Start drag
    case HK_MOVE_COMPONENT_OR_ITEM:         // Start move component or other schematic item
    case HK_COPY_COMPONENT_OR_LABEL:        // Duplicate component or text/label
        if( itemInEdit )
            break;

        if( DrawStruct == NULL )
        {
            // For a drag or copy command, try to find first a component:
            if( DrawStruct == NULL && HK_Descr->m_Idcommand != HK_MOVE_COMPONENT_OR_ITEM )
                DrawStruct = LocateSmallestComponent( GetScreen() );

            // If no component, find the schematic object to move/drag or copy under the cursor
            if( DrawStruct == NULL )
                DrawStruct = SchematicGeneralLocateAndDisplay( false );

            if( DrawStruct == NULL )
                break;
            if( DrawStruct->Type() == SCH_COMPONENT_T )
                DrawStruct = LocateSmallestComponent( GetScreen() );
            if( DrawStruct == NULL )
                break;
            if( DrawStruct->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sheet = (SCH_SHEET*) DrawStruct;
                // If it's a sheet, then check if a pinsheet is under the cursor
                SCH_SHEET_PIN* slabel = sheet->GetLabel( GetScreen()->m_Curseur );

                if( slabel )
                    DrawStruct = slabel;
            }
            if( DrawStruct->Type() == SCH_JUNCTION_T )
            {
                // If it's a junction, pick the underlying wire instead
                DrawStruct = PickStruct( GetScreen()->m_Curseur, GetScreen(), WIREITEM );
            }
            if( DrawStruct == NULL )
                break;
        }

        if( HK_Descr->m_Idcommand == HK_COPY_COMPONENT_OR_LABEL )
        {
            GetScreen()->SetCurItem( (SCH_ITEM*) DrawStruct );
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED, HK_Descr->m_IdMenuEvent );
            wxPostEvent( this, event );
            break;
        }

        if( DrawStruct && (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( (SCH_ITEM*) DrawStruct );

            // Create the events for moving a component or other schematic item
            wxCommandEvent eventMoveOrDragComponent( wxEVT_COMMAND_TOOL_CLICKED,
                                                     HK_Descr->m_IdMenuEvent );
            wxCommandEvent eventMoveItem( wxEVT_COMMAND_TOOL_CLICKED,
                                          ID_POPUP_SCH_MOVE_ITEM_REQUEST );
            wxCommandEvent eventMovePinsheet( wxEVT_COMMAND_TOOL_CLICKED,
                                              ID_POPUP_SCH_MOVE_PINSHEET );
            wxCommandEvent eventDragWire( wxEVT_COMMAND_TOOL_CLICKED,
                                          ID_POPUP_SCH_DRAG_WIRE_REQUEST );

            switch( DrawStruct->Type() )
            {
            // select the correct event for moving an schematic object
            // and add it to the event queue
            case SCH_SHEET_T:
            case SCH_COMPONENT_T:
                wxPostEvent( this, eventMoveOrDragComponent );
                break;

            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIERARCHICAL_LABEL_T:
                wxPostEvent( this, eventMoveOrDragComponent );
                break;

            case SCH_TEXT_T:
            case SCH_FIELD_T:
            case SCH_BUS_ENTRY_T:
                if( HK_Descr->m_Idcommand != HK_DRAG )
                    wxPostEvent( this, eventMoveItem );
                break;

            case SCH_SHEET_LABEL_T:
                if( HK_Descr->m_Idcommand != HK_DRAG )
                    wxPostEvent( this, eventMovePinsheet );
                break;

            case SCH_LINE_T:
                if( ( (SCH_ITEM*) DrawStruct )->GetLayer() == LAYER_WIRE )
                {
                    if( HK_Descr->m_Idcommand == HK_DRAG )
                        wxPostEvent( this, eventDragWire );
                    else
                        wxPostEvent( this, eventMoveItem );
                }
                break;

            default:
                ;
            }
        }

        break;

    case HK_EDIT:

        if( itemInEdit )
            break;
        if( DrawStruct == NULL )
        {
            DrawStruct = PickStruct( GetScreen()->m_Curseur, GetScreen(),
                                     LIBITEM | TEXTITEM | LABELITEM | SHEETITEM );
            if( DrawStruct == NULL )
                break;
            if( DrawStruct->Type() == SCH_COMPONENT_T )
                DrawStruct = LocateSmallestComponent( GetScreen() );
            if( DrawStruct == NULL )
                break;
        }

        if( DrawStruct )
        {
            wxCommandEvent eventEditPinsheet( wxEVT_COMMAND_TOOL_CLICKED,
                                              ID_POPUP_SCH_EDIT_SHEET );

            switch( DrawStruct->Type() )
            {
            case SCH_COMPONENT_T:
                InstallCmpeditFrame( this, MousePos, (SCH_COMPONENT*) DrawStruct );
                break;

            case SCH_SHEET_T:
                GetScreen()->SetCurItem( (SCH_ITEM*) DrawStruct );
                wxPostEvent( this, eventEditPinsheet );
                break;

            case SCH_TEXT_T:
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIERARCHICAL_LABEL_T:
                EditSchematicText( (SCH_TEXT*) DrawStruct );
                break;

            default:
                ;
            }
        }
        break;

    case HK_EDIT_COMPONENT_VALUE:
        if( itemInEdit )
            break;
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( GetScreen() );
        if( DrawStruct )
        {
            EditComponentValue( (SCH_COMPONENT*) DrawStruct, DC );
        }
        break;

    case HK_EDIT_COMPONENT_FOOTPRINT:
        if( itemInEdit )
            break;
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( GetScreen() );
        if( DrawStruct )
        {
            EditComponentFootprint( (SCH_COMPONENT*) DrawStruct, DC );
        }
        break;
    }

    if( RefreshToolBar )
        SetToolbars();
}


/*
 * Hot keys for the component editor. Some commands are relatives to the item
 * under the mouse cursor
 * Commands are case insensitive
 */
void LIB_EDIT_FRAME::OnHotKey( wxDC* DC, int hotkey, EDA_ITEM* DrawStruct )
{
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    wxCommandEvent toolCmd( wxEVT_COMMAND_TOOL_CLICKED );

    cmd.SetEventObject( this );

    wxPoint MousePos   = GetScreen()->m_MousePosition;
    bool    itemInEdit = GetScreen()->GetCurItem()&& GetScreen()->GetCurItem()->m_Flags;

    if( hotkey == 0 )
        return;

    /* Convert lower to upper case (the usual toupper function has problem
     * with non ascii codes like function keys */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';
    Ki_HotkeyInfo* HK_Descr = GetDescriptorFromHotkey( hotkey, s_Common_Hotkey_List );
    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( hotkey, s_LibEdit_Hotkey_List );
    if( HK_Descr == NULL )
        return;

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

    case HK_UNDO:
        if( !itemInEdit )
        {
            toolCmd.SetId( wxID_UNDO );
            GetEventHandler()->ProcessEvent( toolCmd );
        }
        break;

    case HK_REDO:
        if( !itemInEdit )
        {
            toolCmd.SetId( wxID_REDO );
            GetEventHandler()->ProcessEvent( toolCmd );
        }
        break;

    case HK_REPEAT_LAST:
        if( m_lastDrawItem && (m_lastDrawItem->m_Flags == 0)
           && ( m_lastDrawItem->Type() == LIB_PIN_T ) )
            RepeatPinItem( DC, (LIB_PIN*) m_lastDrawItem );
         break;

    case HK_EDIT:
        m_drawItem = LocateItemUsingCursor();

        if( m_drawItem )
        {
            switch( m_drawItem->Type() )
            {
            case LIB_PIN_T:
                cmd.SetId( ID_LIBEDIT_EDIT_PIN );
                GetEventHandler()->ProcessEvent( cmd );
                break;

            case LIB_ARC_T:
            case LIB_CIRCLE_T:
            case LIB_RECTANGLE_T:
            case LIB_POLYLINE_T:
            case LIB_TEXT_T:
                cmd.SetId( ID_POPUP_LIBEDIT_BODY_EDIT_ITEM );
                GetEventHandler()->ProcessEvent( cmd );
                break;

            case LIB_FIELD_T:
                cmd.SetId( ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM );
                GetEventHandler()->ProcessEvent( cmd );
                break;

            default:
                break;
            }
        }
        break;

    case HK_ROTATE:
        m_drawItem = LocateItemUsingCursor();

        if( m_drawItem )
        {
            switch( m_drawItem->Type() )
            {
            case LIB_PIN_T:
                cmd.SetId( ID_LIBEDIT_ROTATE_PIN );
                GetEventHandler()->ProcessEvent( cmd );
                break;

            case LIB_TEXT_T:
                cmd.SetId( ID_POPUP_LIBEDIT_ROTATE_GRAPHIC_TEXT );
                GetEventHandler()->ProcessEvent( cmd );
                break;

            case LIB_FIELD_T:
                cmd.SetId( ID_POPUP_LIBEDIT_FIELD_ROTATE_ITEM );
                GetEventHandler()->ProcessEvent( cmd );
                break;

            default:
                break;
            }
        }
        break;

    case HK_LIBEDIT_CREATE_PIN:
    {
        wxCommandEvent evt;
        evt.SetId( ID_LIBEDIT_PIN_BUTT );
        Process_Special_Functions( evt );
        break;
    }


    case HK_DELETE:
        m_drawItem = LocateItemUsingCursor();

        if( m_drawItem && !m_drawItem->InEditMode() )
        {
            wxCommandEvent evt;
            evt.SetId( ID_POPUP_LIBEDIT_DELETE_ITEM );
            Process_Special_Functions( evt );
        }
        break;

    case HK_LIBEDIT_MOVE_GRAPHIC_ITEM:
        m_drawItem = LocateItemUsingCursor();

        if( m_drawItem && !m_drawItem->InEditMode() )
        {
            wxCommandEvent evt;
            evt.SetId( ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST );
            Process_Special_Functions( evt );
        }
        break;

    case HK_DRAG:
        m_drawItem = LocateItemUsingCursor();

        if( m_drawItem && !m_drawItem->InEditMode() )
        {
            wxCommandEvent evt;
            evt.SetId( ID_POPUP_LIBEDIT_MODIFY_ITEM );
            Process_Special_Functions( evt );
        }
        break;
    }
}
