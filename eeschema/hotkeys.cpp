/***************/
/* hotkeys.cpp */
/***************/

#include "fctsys.h"
#include "common.h"
#include "eeschema_id.h"
#include "hotkeys.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditfrm.h"
#include "class_libentry.h"


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
 * when the variable ItemInEdit is true, an item is currently edited.
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
static Ki_HotkeyInfo HkZoomAuto( wxT( "Fit on Screen" ), HK_ZOOM_AUTO,
                                 WXK_HOME );

static Ki_HotkeyInfo HkZoomCenter( wxT( "Zoom Center" ), HK_ZOOM_CENTER,
                                   WXK_F4 );
static Ki_HotkeyInfo HkZoomRedraw( wxT( "Zoom Redraw" ), HK_ZOOM_REDRAW,
                                   WXK_F3 );

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

// Schematic editor
static Ki_HotkeyInfo HkBeginWire( wxT( "begin Wire" ), HK_BEGIN_WIRE, 'W' );
static Ki_HotkeyInfo HkAddComponent( wxT( "Add Component" ),
                                     HK_ADD_NEW_COMPONENT, 'A' );
static Ki_HotkeyInfo HkMirrorYComponent( wxT( "Mirror Y Component" ),
                                         HK_MIRROR_Y_COMPONENT, 'Y' );
static Ki_HotkeyInfo HkMirrorXComponent( wxT( "Mirror X Component" ),
                                         HK_MIRROR_X_COMPONENT, 'X' );
static Ki_HotkeyInfo HkOrientNormalComponent( wxT( "Orient Normal Component" ),
                                              HK_ORIENT_NORMAL_COMPONENT, 'N' );
static Ki_HotkeyInfo HkRotate( wxT( "Rotate Schematic Item" ),
                               HK_ROTATE, 'R' );
static Ki_HotkeyInfo HkEdit( wxT( "Edit Schematic Item" ),
                             HK_EDIT, 'E' );
static Ki_HotkeyInfo HkEditComponentValue( wxT( "Edit Component Value" ),
                                           HK_EDIT_COMPONENT_VALUE, 'V' );
static Ki_HotkeyInfo HkEditComponentFootprint( wxT( "Edit Component Footprint" ),
                                               HK_EDIT_COMPONENT_FOOTPRINT,
                                               'F' );
static Ki_HotkeyInfo HkMove( wxT( "Move Schematic Item" ),
                             HK_MOVE_COMPONENT_OR_ITEM, 'M',
                             ID_POPUP_SCH_MOVE_CMP_REQUEST );

static Ki_HotkeyInfo HkCopyComponentOrText( wxT( "Copy Component or Label" ),
                                            HK_COPY_COMPONENT_OR_LABEL, 'C',
                                            ID_POPUP_SCH_COPY_ITEM );

static Ki_HotkeyInfo HkDrag( wxT( "Drag Schematic Item" ),
                             HK_DRAG, 'G',
                             ID_POPUP_SCH_DRAG_CMP_REQUEST );
static Ki_HotkeyInfo HkMove2Drag( wxT( "Switch move block to drag block" ),
                                  HK_MOVEBLOCK_TO_DRAGBLOCK, '\t' );
static Ki_HotkeyInfo HkInsert( wxT( "Repeat Last Item" ), HK_REPEAT_LAST,
                               WXK_INSERT );
static Ki_HotkeyInfo HkDelete( wxT( "Delete Item" ), HK_DELETE, WXK_DELETE );
static Ki_HotkeyInfo HkNextSearch( wxT( "Next Search" ), HK_NEXT_SEARCH,
                                   WXK_F5 );

// Special keys for library editor:
static Ki_HotkeyInfo HkInsertPin( wxT( "Repeat Pin" ), HK_REPEAT_LAST,
                                  WXK_INSERT );
static Ki_HotkeyInfo HkMovePin( wxT( "Move Pin" ), HK_LIBEDIT_MOVE_GRAPHIC_ITEM, 'M' );
static Ki_HotkeyInfo HkDeletePin( wxT( "Delete Pin" ), HK_DELETE_PIN,
                                  WXK_DELETE );


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
    &HkNextSearch,
    &HkDelete,
    &HkInsert,
    &HkMove2Drag,
    &HkMove,
    &HkCopyComponentOrText,
    &HkDrag,
    &HkAddComponent,
    &HkRotate,
    &HkMirrorXComponent,
    &HkMirrorYComponent,
    &HkOrientNormalComponent,
    &HkEdit,
    &HkEditComponentValue,
    &HkEditComponentFootprint,
    &HkBeginWire,
    NULL
};

// List of hotkey descriptors for library editor
Ki_HotkeyInfo* s_LibEdit_Hotkey_List[] =
{
    &HkInsertPin,
    &HkEdit,
    &HkMovePin,
    &HkDeletePin,
    &HkRotate,
    &HkDrag,
    NULL
};

// list of sections and corresponding hotkey list for eeschema (used to create
// an hotkey config file)
struct Ki_HotkeyInfoSectionDescriptor s_Eeschema_Hokeys_Descr[] =
{
    { &g_CommonSectionTag,    s_Common_Hotkey_List,    "Common keys"           },
    { &g_SchematicSectionTag, s_Schematic_Hotkey_List, "Schematic editor keys" },
    { &g_LibEditSectionTag,   s_LibEdit_Hotkey_List,   "library editor keys"   },
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
void WinEDA_SchematicFrame::OnHotKey( wxDC* DC, int hotkey,
                                      EDA_BaseStruct* DrawStruct )
{
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );

    cmd.SetEventObject( this );

    bool        ItemInEdit = GetScreen()->GetCurItem()
                             && GetScreen()->GetCurItem()->m_Flags;
    bool        RefreshToolBar = FALSE;
    SCH_SCREEN* screen = GetScreen();

    if( hotkey == 0 )
        return;

    wxPoint MousePos = GetScreen()->m_MousePosition;

    /* Convert lower to upper case (the usual toupper function has problem
     * with non ascii codes like function keys */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';

    // Search command from key :
    Ki_HotkeyInfo* HK_Descr = GetDescriptorFromHotkey( hotkey,
                                                       s_Common_Hotkey_List );
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
        if( !ItemInEdit )
        {
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED,
                                  HK_Descr->m_IdMenuEvent );
            wxPostEvent( this, event );
        }
        break;

    case HK_MOVEBLOCK_TO_DRAGBLOCK:   // Switch to drag mode, when block moving
        HandleBlockEndByPopUp( BLOCK_DRAG, DC );
        break;

    case HK_DELETE:
        if( !ItemInEdit && screen->m_BlockLocate.m_State == STATE_NO_BLOCK )
        {
            RefreshToolBar = LocateAndDeleteItem( this, DC );
            GetScreen()->SetModify();
            GetScreen()->SetCurItem( NULL );
            TestDanglingEnds( GetScreen()->EEDrawList, DC );
        }
        break;

    case HK_REPEAT_LAST:
        if( !ItemInEdit && g_ItemToRepeat && ( g_ItemToRepeat->m_Flags == 0 ) )
            RepeatDrawItem( DC );
        break;

    case HK_NEXT_SEARCH:
        if( !ItemInEdit )
        {
            if( g_LastSearchIsMarker )
                WinEDA_SchematicFrame::FindMarker( 1 );
            else
                FindSchematicItem( wxEmptyString, 2 );
        }
        break;

    case HK_ADD_NEW_COMPONENT:      // Add component
        if( !ItemInEdit )
        {
            // switch to m_ID_current_state = ID_COMPONENT_BUTT;
            if( m_ID_current_state != ID_COMPONENT_BUTT )
                SetToolID( ID_COMPONENT_BUTT, wxCURSOR_PENCIL,
                          _( "Add Component" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_BEGIN_WIRE:

        /* An item is selected. If edited and not a wire, a new command is not
         * possible */
        if( !ItemInEdit && screen->m_BlockLocate.m_State == STATE_NO_BLOCK )
        {
            if( DrawStruct && DrawStruct->m_Flags )
            {
                if( DrawStruct->Type() == DRAW_SEGMENT_STRUCT_TYPE )
                {
                    SCH_LINE* segment = (SCH_LINE*) DrawStruct;
                    if( segment->GetLayer() != LAYER_WIRE )
                        break;
                }
                else
                    break;
            }

            // switch to m_ID_current_state = ID_WIRE_BUTT;
            if( m_ID_current_state != ID_WIRE_BUTT )
                SetToolID( ID_WIRE_BUTT, wxCURSOR_PENCIL, _( "Add Wire" ) );
            OnLeftClick( DC, MousePos );
        }
        break;

    case HK_ROTATE:       // Component or other schematic item rotation

        if( DrawStruct == NULL )
        {
            // Find the schematic object to rotate under the cursor
            DrawStruct = SchematicGeneralLocateAndDisplay( false );

            if( DrawStruct == NULL )
                break;

            if( DrawStruct->Type() == TYPE_SCH_COMPONENT )
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
            case TYPE_SCH_COMPONENT:
                wxPostEvent( this, eventRotateComponent );
                break;

            case TYPE_SCH_TEXT:
            case TYPE_SCH_LABEL:
            case TYPE_SCH_GLOBALLABEL:
            case TYPE_SCH_HIERLABEL:
                wxPostEvent( this, eventRotateText );
                break;

            case DRAW_PART_TEXT_STRUCT_TYPE:
                wxPostEvent( this, eventRotateField );

            default:
                ;
            }
        }

        break;

    case HK_MIRROR_Y_COMPONENT:     // Mirror Y (Component)
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( (SCH_SCREEN*) GetScreen() );
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
            TestDanglingEnds( GetScreen()->EEDrawList, DC );
        }
        break;

    case HK_DRAG:                           // Start drag
    case HK_MOVE_COMPONENT_OR_ITEM:         // Start move component or other schematic item
    case HK_COPY_COMPONENT_OR_LABEL:        // Duplicate component or text/label
        if( ItemInEdit )
            break;

        if( DrawStruct == NULL )
        {
            // Find the schematic object to move under the cursor
            DrawStruct = SchematicGeneralLocateAndDisplay( false );

            if( DrawStruct == NULL )
                break;
            if( DrawStruct->Type() == TYPE_SCH_COMPONENT )
                DrawStruct = LocateSmallestComponent( GetScreen() );
            if( DrawStruct == NULL )
                break;
            if( DrawStruct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                // If it's a sheet, then check if a pinsheet is under the cursor
                SCH_SHEET_PIN* slabel = LocateSheetLabel( (SCH_SHEET*) DrawStruct,
                                                         GetScreen()->m_Curseur );
                if( slabel )
                    DrawStruct = slabel;
            }
            if( DrawStruct->Type() == DRAW_JUNCTION_STRUCT_TYPE )
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
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED,
                                  HK_Descr->m_IdMenuEvent );
            wxPostEvent( this, event );
            break;
        }

        if( DrawStruct && (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( (SCH_ITEM*) DrawStruct );

            // Create the events for moving a component or other schematic item
            wxCommandEvent eventMoveComponent( wxEVT_COMMAND_TOOL_CLICKED,
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
            case TYPE_SCH_COMPONENT:
                wxPostEvent( this, eventMoveComponent );
                break;

            case TYPE_SCH_TEXT:
            case TYPE_SCH_LABEL:
            case TYPE_SCH_GLOBALLABEL:
            case TYPE_SCH_HIERLABEL:
            case DRAW_SHEET_STRUCT_TYPE:
            case DRAW_PART_TEXT_STRUCT_TYPE:
            case DRAW_BUSENTRY_STRUCT_TYPE:
                wxPostEvent( this, eventMoveItem );
                break;

            case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
                wxPostEvent( this, eventMovePinsheet );
                break;

            case DRAW_SEGMENT_STRUCT_TYPE:
                if( ( (SCH_ITEM*) DrawStruct )->GetLayer() == LAYER_WIRE )
                    wxPostEvent( this, eventDragWire );
                break;

            default:
                ;
            }
        }

        break;

    case HK_EDIT:

        if( ItemInEdit )
            break;
        if( DrawStruct == NULL )
        {
            DrawStruct = PickStruct( GetScreen()->m_Curseur,
                                     GetScreen(), LIBITEM | TEXTITEM |
                                     LABELITEM | SHEETITEM );
            if( DrawStruct == NULL )
                break;
            if( DrawStruct->Type() == TYPE_SCH_COMPONENT )
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
            case TYPE_SCH_COMPONENT:
                InstallCmpeditFrame( this, MousePos, (SCH_COMPONENT*) DrawStruct );
                break;

            case DRAW_SHEET_STRUCT_TYPE:
                GetScreen()->SetCurItem( (SCH_ITEM*) DrawStruct );
                wxPostEvent( this, eventEditPinsheet );
                break;

            case TYPE_SCH_TEXT:
            case TYPE_SCH_LABEL:
            case TYPE_SCH_GLOBALLABEL:
            case TYPE_SCH_HIERLABEL:
                EditSchematicText( (SCH_TEXT*) DrawStruct );
                break;

            default:
                ;
            }
        }
        break;

    case HK_EDIT_COMPONENT_VALUE:
        if( ItemInEdit )
            break;
        if( DrawStruct == NULL )
            DrawStruct = LocateSmallestComponent( GetScreen() );
        if( DrawStruct )
        {
            EditComponentValue( (SCH_COMPONENT*) DrawStruct, DC );
        }
        break;

    case HK_EDIT_COMPONENT_FOOTPRINT:
        if( ItemInEdit )
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
void WinEDA_LibeditFrame::OnHotKey( wxDC* DC, int hotkey,
                                    EDA_BaseStruct* DrawStruct )
{
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    wxCommandEvent toolCmd( wxEVT_COMMAND_TOOL_CLICKED );

    cmd.SetEventObject( this );

    wxPoint MousePos   = GetScreen()->m_MousePosition;
    bool    ItemInEdit = GetScreen()->GetCurItem()
                         && GetScreen()->GetCurItem()->m_Flags;

    if( hotkey == 0 )
        return;

    /* Convert lower to upper case (the usual toupper function has problem
     * with non ascii codes like function keys */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';
    Ki_HotkeyInfo* HK_Descr = GetDescriptorFromHotkey( hotkey,
                                                       s_Common_Hotkey_List );
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
        if( !ItemInEdit )
        {
            toolCmd.SetId( wxID_UNDO );
            GetEventHandler()->ProcessEvent( toolCmd );
        }
        break;

    case HK_REDO:
        if( !ItemInEdit )
        {
            toolCmd.SetId( wxID_REDO );
            GetEventHandler()->ProcessEvent( toolCmd );
        }
        break;

    case HK_REPEAT_LAST:
        if( m_lastDrawItem && (m_lastDrawItem->m_Flags == 0)
           && ( m_lastDrawItem->Type() == COMPONENT_PIN_DRAW_TYPE ) )
        {
            RepeatPinItem( DC, (LIB_PIN*) m_lastDrawItem );
        }
        else
            wxBell();
        break;

    case HK_EDIT:
        m_drawItem = LocateItemUsingCursor();

        if( m_drawItem )
        {
            switch( m_drawItem->Type() )
            {
            case COMPONENT_PIN_DRAW_TYPE:
                cmd.SetId( ID_LIBEDIT_EDIT_PIN );
                GetEventHandler()->ProcessEvent( cmd );
                break;

            case COMPONENT_ARC_DRAW_TYPE:
            case COMPONENT_CIRCLE_DRAW_TYPE:
            case COMPONENT_RECT_DRAW_TYPE:
            case COMPONENT_POLYLINE_DRAW_TYPE:
            case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
                cmd.SetId( ID_POPUP_LIBEDIT_BODY_EDIT_ITEM );
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
            case COMPONENT_PIN_DRAW_TYPE:
                cmd.SetId( ID_LIBEDIT_ROTATE_PIN );
                GetEventHandler()->ProcessEvent( cmd );
                break;

            case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
                cmd.SetId( ID_POPUP_LIBEDIT_ROTATE_GRAPHIC_TEXT );
                GetEventHandler()->ProcessEvent( cmd );
                break;
            
            default:
                break;
            }
        }
        break;


    case HK_DELETE_PIN:
        m_drawItem = LocateItemUsingCursor();

        if( m_drawItem )
        {
            wxCommandEvent evt;
            evt.SetId( ID_POPUP_LIBEDIT_DELETE_ITEM );
            Process_Special_Functions( evt );
        }
        break;

    case HK_LIBEDIT_MOVE_GRAPHIC_ITEM:
        m_drawItem = LocateItemUsingCursor();

        if( m_drawItem )
        {
            wxCommandEvent evt;
            evt.SetId( ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST );
            Process_Special_Functions( evt );
        }
        break;

    case HK_DRAG:
        m_drawItem = LocateItemUsingCursor();

        if( m_drawItem )
        {
            wxCommandEvent evt;
            evt.SetId( ID_POPUP_LIBEDIT_MODIFY_ITEM );
            Process_Special_Functions( evt );
        }
        break;
    }
}
