/********************/
/* onrightclick.cpp */
/********************/

#include "fctsys.h"
#include "common.h"
#include "eeschema_id.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "bitmaps.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "hotkeys.h"
#include "class_library.h"
#include "sch_marker.h"
#include "sch_text.h"
#include "sch_items.h"
#include "sch_component.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"

#include <iostream>
using namespace std;


static void AddMenusForBlock( wxMenu* PopMenu, WinEDA_SchematicFrame* frame );
static void AddMenusForWire( wxMenu* PopMenu, SCH_LINE* Wire, WinEDA_SchematicFrame* frame );
static void AddMenusForBus( wxMenu* PopMenu, SCH_LINE* Bus, WinEDA_SchematicFrame* frame );
static void AddMenusForHierchicalSheet( wxMenu* PopMenu, SCH_SHEET* Sheet );
static void AddMenusForPinSheet( wxMenu* PopMenu, SCH_SHEET_PIN* PinSheet );
static void AddMenusForText( wxMenu* PopMenu, SCH_TEXT* Text );
static void AddMenusForLabel( wxMenu* PopMenu, SCH_LABEL* Label );
static void AddMenusForGLabel( wxMenu* PopMenu, SCH_GLOBALLABEL* GLabel );
static void AddMenusForHLabel( wxMenu* PopMenu, SCH_HIERLABEL* GLabel );
static void AddMenusForComponent( wxMenu* PopMenu, SCH_COMPONENT* Component );
static void AddMenusForComponentField( wxMenu* PopMenu, SCH_FIELD* Field );
static void AddMenusForJunction( wxMenu* PopMenu, SCH_JUNCTION* Junction,
                                 WinEDA_SchematicFrame* frame );
static void AddMenusForMarkers( wxMenu* aPopMenu, SCH_MARKER* aMarker,
                                WinEDA_SchematicFrame* aFrame );


/* Prepare context menu when a click on the right mouse button occurs.
 *
 * This menu is then added to the list of zoom commands.
 */
bool WinEDA_SchematicFrame::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    SCH_ITEM* DrawStruct  = (SCH_ITEM*) GetScreen()->GetCurItem();
    bool      BlockActive = (GetScreen()->m_BlockLocate.m_Command != BLOCK_IDLE);

    // Do not start a block command  on context menu.
    DrawPanel->m_CanStartBlock = -1;

    if( BlockActive )
    {
        AddMenusForBlock( PopMenu, this );
        PopMenu->AppendSeparator();
        return true;
    }

    // Try to locate items at cursor position.
    if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
    {
        DrawStruct = SchematicGeneralLocateAndDisplay( false );

        if( DrawStruct && (DrawStruct->Type() == DRAW_SHEET_STRUCT_TYPE) )
        {
            SCH_SHEET_PIN* slabel;
            slabel = LocateSheetLabel( (SCH_SHEET*) DrawStruct, GetScreen()->m_Curseur );
            if( slabel )
                DrawStruct = slabel;
        }
    }

    // If Command in progress: add "cancel" and "end tool" menu
    if(  m_ID_current_state )
    {
        if( DrawStruct && DrawStruct->m_Flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel" ), cancel_xpm );
        }
        else
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_CLOSE_CURRENT_TOOL, _( "End Tool" ), cancel_tool_xpm );
        }
        PopMenu->AppendSeparator();
    }
    else
    {
        if( DrawStruct && DrawStruct->m_Flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel" ), cancel_xpm );
            PopMenu->AppendSeparator();
        }
    }

    if( DrawStruct == NULL )
    {
        if( GetSheet()->Last() != g_RootSheet )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_LEAVE_SHEET, _( "Leave Sheet" ), leave_sheet_xpm );
            PopMenu->AppendSeparator();
        }
        return true;
    }

    GetScreen()->SetCurItem( DrawStruct );

    int  flags  = DrawStruct->m_Flags;
    bool is_new = (flags & IS_NEW) ? TRUE : FALSE;

    switch( DrawStruct->Type() )
    {
    case DRAW_NOCONNECT_STRUCT_TYPE:

        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Noconn" ), delete_xpm );
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        AddMenusForJunction( PopMenu, (SCH_JUNCTION*) DrawStruct, this );
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        if( !flags )
        {
            wxString msg = AddHotkeyName( _( "Move Bus Entry" ), s_Schematic_Hokeys_Descr,
                                          HK_MOVE_COMPONENT_OR_ITEM );
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, msg, move_xpm );
        }

        if( GetBusEntryShape( (SCH_BUS_ENTRY*) DrawStruct ) == '\\' )
            PopMenu->Append( ID_POPUP_SCH_ENTRY_SELECT_SLASH, _( "Set Bus Entry /" ) );
        else
            PopMenu->Append( ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH, _( "Set Bus Entry \\" ) );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Bus Entry" ), delete_bus_xpm );
        break;

    case TYPE_SCH_MARKER:
        AddMenusForMarkers( PopMenu, (SCH_MARKER*) DrawStruct, this );
        break;

    case TYPE_SCH_TEXT:
        AddMenusForText( PopMenu, (SCH_TEXT*) DrawStruct );
        break;

    case TYPE_SCH_LABEL:
        AddMenusForLabel( PopMenu, (SCH_LABEL*) DrawStruct );
        break;

    case TYPE_SCH_GLOBALLABEL:
        AddMenusForGLabel( PopMenu, (SCH_GLOBALLABEL*) DrawStruct );
        break;

    case TYPE_SCH_HIERLABEL:
        AddMenusForHLabel( PopMenu, (SCH_HIERLABEL*) DrawStruct );
        break;

    case DRAW_PART_TEXT_STRUCT_TYPE:
    {
        AddMenusForComponentField( PopMenu, (SCH_FIELD*) DrawStruct );
        if( flags )
            break;

        // Many fields are inside a component. If this is the case, add the
        // component menu
        SCH_COMPONENT* Component =
            LocateSmallestComponent( (SCH_SCREEN*) GetScreen() );

        if( Component )
        {
            PopMenu->AppendSeparator();
            AddMenusForComponent( PopMenu, Component );
        }
    }
    break;

    case TYPE_SCH_COMPONENT:
        AddMenusForComponent( PopMenu, (SCH_COMPONENT*) DrawStruct );
        break;

    case DRAW_SEGMENT_STRUCT_TYPE:

//      if( !flags ) PopMenu->Append(ID_POPUP_SCH_MOVE_ITEM_REQUEST, "Move");
        switch( DrawStruct->GetLayer() )
        {
        case LAYER_WIRE:
            AddMenusForWire( PopMenu, (SCH_LINE*) DrawStruct, this );
            break;

        case LAYER_BUS:
            AddMenusForBus( PopMenu, (SCH_LINE*) DrawStruct, this );
            break;

        default:
            if( is_new )
                ADD_MENUITEM( PopMenu, ID_POPUP_END_LINE, _( "End Drawing" ), apply_xpm );
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Drawing" ), delete_xpm );
            break;
        }

        break;

    case DRAW_SHEET_STRUCT_TYPE:
        AddMenusForHierchicalSheet( PopMenu, (SCH_SHEET*) DrawStruct );
        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        AddMenusForPinSheet( PopMenu, (SCH_SHEET_PIN*) DrawStruct );
        break;

    default:
        wxString msg;
        msg.Printf( wxT( "WinEDA_SchematicFrame::OnRightClick Error: unknown DrawType %d" ),
                   DrawStruct->Type() );
        DisplayError( this, msg );
        break;
    }

    PopMenu->AppendSeparator();
    return true;
}


void AddMenusForComponentField( wxMenu* PopMenu, SCH_FIELD* Field )
{
    wxString msg;

    if( !Field->m_Flags )
    {
        msg = AddHotkeyName( _( "Move Field" ), s_Schematic_Hokeys_Descr,
                             HK_MOVE_COMPONENT_OR_ITEM );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, msg, move_text_xpm );
    }

    msg = AddHotkeyName( _( "Rotate Field" ), s_Schematic_Hokeys_Descr, HK_ROTATE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_FIELD, msg, rotate_field_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_FIELD, _( "Edit Field" ), edit_text_xpm );
}


void AddMenusForComponent( wxMenu* PopMenu, SCH_COMPONENT* Component )
{
    if( Component->Type() != TYPE_SCH_COMPONENT )
    {
        wxASSERT( 0 );
        return;
    }

    wxString       msg;
    LIB_ALIAS*     libEntry;
    LIB_COMPONENT* libComponent = NULL;

    libEntry = CMP_LIBRARY::FindLibraryEntry( Component->m_ChipName );

    if( libEntry )
        libComponent = libEntry->GetComponent();

    if( !Component->m_Flags )
    {
        msg = _( "Move Component" );
        msg << wxT( " " ) << Component->GetField( REFERENCE )->m_Text;
        msg = AddHotkeyName( msg, s_Schematic_Hokeys_Descr, HK_MOVE_COMPONENT_OR_ITEM );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_CMP_REQUEST, msg, move_xpm );
        msg = AddHotkeyName( _( "Drag Component" ), s_Schematic_Hokeys_Descr, HK_DRAG );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DRAG_CMP_REQUEST, msg, move_xpm );
    }

    wxMenu* orientmenu = new wxMenu;
    msg = AddHotkeyName( _( "Rotate +" ), s_Schematic_Hokeys_Descr, HK_ROTATE );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE, msg, rotate_pos_xpm );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE, _( "Rotate -" ), rotate_neg_xpm );
    msg = AddHotkeyName( _( "Mirror --" ), s_Schematic_Hokeys_Descr, HK_MIRROR_X_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_MIROR_X_CMP, msg, mirror_V_xpm );
    msg = AddHotkeyName( _( "Mirror ||" ), s_Schematic_Hokeys_Descr, HK_MIRROR_Y_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_MIROR_Y_CMP, msg, mirror_H_xpm );
    msg = AddHotkeyName( _( "Normal" ), s_Schematic_Hokeys_Descr, HK_ORIENT_NORMAL_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_ORIENT_NORMAL_CMP, msg, normal_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, orientmenu, ID_POPUP_SCH_GENERIC_ORIENT_CMP,
                               _( "Orient Component" ), orient_xpm );

    wxMenu* editmenu = new wxMenu;
    msg = AddHotkeyName( _( "Edit" ), s_Schematic_Hokeys_Descr, HK_EDIT );
    ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_CMP, msg, edit_component_xpm );

    if( libComponent && libComponent->IsNormal() )
    {
        msg = AddHotkeyName( _( "Value " ), s_Schematic_Hokeys_Descr, HK_EDIT_COMPONENT_VALUE );
        ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_VALUE_CMP, msg, edit_comp_value_xpm );

        ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_REF_CMP, _( "Reference" ), edit_comp_ref_xpm );

        msg = AddHotkeyName( _( "Footprint " ), s_Schematic_Hokeys_Descr,
                             HK_EDIT_COMPONENT_FOOTPRINT );
        ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_FOOTPRINT_CMP, msg, edit_comp_footprint_xpm );
    }

    if( libComponent && libComponent->HasConversion() )
        ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_CONVERT_CMP, _( "Convert" ),
                      component_select_alternate_shape_xpm );

    if( libComponent && ( libComponent->GetPartCount() >= 2 ) )
    {
        wxMenu* sel_unit_menu = new wxMenu; int ii;
        for( ii = 0; ii < libComponent->GetPartCount(); ii++ )
        {
            wxString num_unit;
            num_unit.Printf( _( "Unit %d %c" ), ii + 1,
                             "?ABCDEFGHIJKLMNOPQRSTUVWXYZ"[ ii + 1 ] );
            sel_unit_menu->Append( ID_POPUP_SCH_SELECT_UNIT1 + ii, num_unit );
        }

        ADD_MENUITEM_WITH_SUBMENU( editmenu, sel_unit_menu, ID_POPUP_SCH_SELECT_UNIT_CMP,
                                   _( "Unit" ), component_select_unit_xpm );
    }

    ADD_MENUITEM_WITH_SUBMENU( PopMenu, editmenu, ID_POPUP_SCH_GENERIC_EDIT_CMP,
                               _( "Edit Component" ), edit_component_xpm );

    if( !Component->m_Flags )
    {
        msg = AddHotkeyName( _( "Copy Component" ), s_Schematic_Hokeys_Descr,
                             HK_COPY_COMPONENT_OR_LABEL );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_COPY_ITEM, msg, copy_button );
        msg = AddHotkeyName( _( "Delete Component" ), s_Schematic_Hokeys_Descr, HK_DELETE );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_CMP, msg, delete_xpm );
    }

    if( libEntry && !libEntry->GetDocFileName().IsEmpty() )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DISPLAYDOC_CMP, _( "Doc" ), datasheet_xpm );
}


void AddMenusForGLabel( wxMenu* PopMenu, SCH_GLOBALLABEL* GLabel )
{
    wxMenu*  menu_change_type = new wxMenu;
    wxString msg;

    if( !GLabel->m_Flags )
    {
        msg = AddHotkeyName( _( "Move Global Label" ), s_Schematic_Hokeys_Descr,
                             HK_MOVE_COMPONENT_OR_ITEM );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, msg, move_text_xpm );
        msg = AddHotkeyName( _( "Drag Global Label" ), s_Schematic_Hokeys_Descr,
                             HK_DRAG );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DRAG_CMP_REQUEST, msg, move_text_xpm );
        msg = AddHotkeyName( _( "Copy Global Label" ), s_Schematic_Hokeys_Descr,
                             HK_COPY_COMPONENT_OR_LABEL );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_COPY_ITEM, msg, copy_button );
    }

    msg = AddHotkeyName( _( "Rotate Global Label" ), s_Schematic_Hokeys_Descr, HK_ROTATE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT, msg, rotate_glabel_xpm );
    msg = AddHotkeyName( _( "Edit Global Label" ), s_Schematic_Hokeys_Descr, HK_EDIT );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT, msg, edit_text_xpm );
    msg = AddHotkeyName( _( "Delete Global Label" ), s_Schematic_Hokeys_Descr, HK_DELETE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, msg, delete_text_xpm );

    // add menu change type text (to label, glabel, text):
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_HLABEL,
                  _( "Change to Hierarchical Label" ), label2glabel_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL,
                  _( "Change to Label" ), glabel2label_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT,
                  _( "Change to Text" ), glabel2text_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT,
                               _( "Change Type" ), gl_change_xpm );
}


void AddMenusForHLabel( wxMenu* PopMenu, SCH_HIERLABEL* HLabel )
{
    wxMenu*  menu_change_type = new wxMenu;
    wxString msg;

    if( !HLabel->m_Flags )
    {
        msg = AddHotkeyName( _( "Move Hierarchical Label" ), s_Schematic_Hokeys_Descr,
                             HK_MOVE_COMPONENT_OR_ITEM );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, msg, move_text_xpm );
        msg = AddHotkeyName( _( "Drag Hierarchical Label" ), s_Schematic_Hokeys_Descr,
                             HK_DRAG );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DRAG_CMP_REQUEST, msg, move_text_xpm );
        msg = AddHotkeyName( _( "Copy Hierarchical Label" ), s_Schematic_Hokeys_Descr,
                             HK_COPY_COMPONENT_OR_LABEL );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_COPY_ITEM, msg, copy_button );
    }
    msg = AddHotkeyName( _( "Rotate Hierarchical Label" ), s_Schematic_Hokeys_Descr, HK_ROTATE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT, msg, rotate_glabel_xpm );
    msg = AddHotkeyName( _( "Edit Hierarchical Label" ), s_Schematic_Hokeys_Descr, HK_EDIT );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT, msg, edit_text_xpm );
    msg = AddHotkeyName( _( "Delete Hierarchical Label" ), s_Schematic_Hokeys_Descr, HK_DELETE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, msg, delete_text_xpm );

    // add menu change type text (to label, glabel, text):
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL,
                  _( "Change to Label" ), glabel2label_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT,
                  _( "Change to Text" ), glabel2text_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL,
                  _( "Change to Global Label" ), label2glabel_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT,
                               _( "Change Type" ), gl_change_xpm );
}


void AddMenusForLabel( wxMenu* PopMenu, SCH_LABEL* Label )
{
    wxMenu*  menu_change_type = new wxMenu;
    wxString msg;

    if( !Label->m_Flags )
    {
        msg = AddHotkeyName( _( "Move Label" ), s_Schematic_Hokeys_Descr,
                             HK_MOVE_COMPONENT_OR_ITEM );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, msg, move_text_xpm );
        msg = AddHotkeyName( _( "Drag Label" ), s_Schematic_Hokeys_Descr,
                             HK_DRAG );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DRAG_CMP_REQUEST, msg, move_text_xpm );
        msg = AddHotkeyName( _( "Copy Label" ), s_Schematic_Hokeys_Descr,
                             HK_COPY_COMPONENT_OR_LABEL );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_COPY_ITEM, msg, copy_button );
    }
    msg = AddHotkeyName( _( "Rotate Label" ), s_Schematic_Hokeys_Descr, HK_ROTATE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT, msg, rotate_pos_xpm );
    msg = AddHotkeyName( _( "Edit Label" ), s_Schematic_Hokeys_Descr, HK_EDIT );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT, msg, edit_text_xpm );
    msg = AddHotkeyName( _( "Delete Label" ), s_Schematic_Hokeys_Descr, HK_DELETE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, msg, delete_text_xpm );

    // add menu change type text (to label, glabel, text):
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_HLABEL,
                  _( "Change to Hierarchical Label" ), label2glabel_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT,
                  _( "Change to Text" ), label2text_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL,
                  _( "Change to Global Label" ), label2glabel_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT,
                               _( "Change Type" ), gl_change_xpm );
}


void AddMenusForText( wxMenu* PopMenu, SCH_TEXT* Text )
{
    wxString msg;
    wxMenu*  menu_change_type = new wxMenu;

    if( !Text->m_Flags )
    {
        msg = AddHotkeyName( _( "Move Text" ), s_Schematic_Hokeys_Descr,
                             HK_MOVE_COMPONENT_OR_ITEM );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, msg, move_text_xpm );
        msg = AddHotkeyName( _( "Copy Text" ), s_Schematic_Hokeys_Descr,
                             HK_COPY_COMPONENT_OR_LABEL );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_COPY_ITEM, msg, copy_button );
    }

    msg = AddHotkeyName( _( "Rotate Text" ), s_Schematic_Hokeys_Descr, HK_ROTATE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT, msg, rotate_pos_xpm );
    msg = AddHotkeyName( _( "Edit Text" ), s_Schematic_Hokeys_Descr, HK_EDIT );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT, msg, edit_text_xpm );
    msg = AddHotkeyName( _( "Delete Text" ), s_Schematic_Hokeys_Descr, HK_DELETE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, msg, delete_text_xpm );

    /* add menu change type text (to label, glabel, text),
     * but only if this is a single line text
     */
    if( Text->m_Text.Find( wxT( "\n" ) ) ==  wxNOT_FOUND )
    {
        ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL,
                      _( "Change to Label" ), label2text_xpm );
        ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_HLABEL,
                      _( "Change to Hierarchical Label" ), label2glabel_xpm );
        ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL,
                      _( "Change to Glabel" ), label2glabel_xpm );
        ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT,
                                   _( "Change Type" ), gl_change_xpm );
    }
}


void AddMenusForJunction( wxMenu* PopMenu, SCH_JUNCTION* Junction, WinEDA_SchematicFrame* frame )
{
    bool     is_new = (Junction->m_Flags & IS_NEW) ? TRUE : FALSE;
    wxString msg;

    if( !is_new )
    {
        if( PickStruct( frame->GetScreen()->m_Curseur, frame->GetScreen(),
                        WIREITEM | BUSITEM | EXCLUDE_WIRE_BUS_ENDPOINTS ) )
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_BREAK_WIRE, _( "Break Wire" ), break_line_xpm );
    }

    msg = AddHotkeyName( _( "Delete Junction" ), s_Schematic_Hokeys_Descr, HK_DELETE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, msg, delete_xpm );

    if( PickStruct( frame->GetScreen()->m_Curseur, frame->GetScreen(), WIREITEM | BUSITEM ) )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_NODE, _( "Delete Node" ), delete_node_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_CONNECTION, _( "Delete Connection" ),
                      delete_connection_xpm );
    }
}


void AddMenusForWire( wxMenu* PopMenu, SCH_LINE* Wire, WinEDA_SchematicFrame* frame )
{
    bool     is_new = (Wire->m_Flags & IS_NEW) ? TRUE : FALSE;
    wxPoint  pos    = frame->GetScreen()->m_Curseur;
    wxString msg;

    if( is_new )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_END_LINE, _( "Wire End" ), apply_xpm );
        return;
    }

    msg = AddHotkeyName( _( "Drag Wire" ), s_Schematic_Hokeys_Descr, HK_DRAG );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DRAG_WIRE_REQUEST, msg, move_track_xpm );
    PopMenu->AppendSeparator();
    msg = AddHotkeyName( _( "Delete Wire" ), s_Schematic_Hokeys_Descr, HK_DELETE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, msg, delete_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_NODE, _( "Delete Node" ), delete_node_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_CONNECTION, _( "Delete Connection" ),
                  delete_connection_xpm );

    if( PickStruct( frame->GetScreen()->m_Curseur, frame->GetScreen(),
                    WIREITEM | BUSITEM | EXCLUDE_WIRE_BUS_ENDPOINTS ) )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_BREAK_WIRE, _( "Break Wire" ), break_line_xpm );

    PopMenu->AppendSeparator();

    msg = AddHotkeyName( _( "Add Junction" ), s_Schematic_Hokeys_Descr, HK_ADD_JUNCTION );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_JUNCTION, msg, add_junction_xpm );
    msg = AddHotkeyName( _( "Add Label" ), s_Schematic_Hokeys_Descr, HK_ADD_LABEL );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_LABEL, msg, add_line_label_xpm );

    // Add global label command only if the cursor is over one end of the wire.
    if( ( pos == Wire->m_Start ) || ( pos == Wire->m_End ) )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_GLABEL, _( "Add Global Label" ), add_glabel_xpm );
}


void AddMenusForBus( wxMenu* PopMenu, SCH_LINE* Bus, WinEDA_SchematicFrame* frame )
{
    bool    is_new = (Bus->m_Flags & IS_NEW) ? TRUE : FALSE;
    wxPoint pos    = frame->GetScreen()->m_Curseur;
    wxString msg;
    if( is_new )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_END_LINE, _( "Bus End" ), apply_xpm );
        return;
    }

    msg = AddHotkeyName( _( "Delete Bus" ), s_Schematic_Hokeys_Descr, HK_DELETE );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, msg, delete_bus_xpm );

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_BREAK_WIRE, _( "Break Bus" ), break_bus_xpm );

    PopMenu->AppendSeparator();
    msg = AddHotkeyName( _( "Add Junction" ), s_Schematic_Hokeys_Descr, HK_ADD_JUNCTION );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_JUNCTION, msg, add_junction_xpm );
    msg = AddHotkeyName( _( "Add Label" ), s_Schematic_Hokeys_Descr, HK_ADD_LABEL );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_LABEL, msg, add_line_label_xpm );

    // Add global label command only if the cursor is over one end of the bus.
    if( ( pos == Bus->m_Start ) || ( pos == Bus->m_End ) )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_GLABEL, _( "Add Global Label" ), add_glabel_xpm );
}


void AddMenusForHierchicalSheet( wxMenu* PopMenu, SCH_SHEET* Sheet )
{
    wxString msg;

    if( !Sheet->m_Flags )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ENTER_SHEET, _( "Enter Sheet" ), enter_sheet_xpm );
        PopMenu->AppendSeparator();
        msg = AddHotkeyName( _( "Move Sheet" ), s_Schematic_Hokeys_Descr,
                             HK_MOVE_COMPONENT_OR_ITEM );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, msg, move_sheet_xpm );

        msg = AddHotkeyName( _( "Drag Sheet" ), s_Schematic_Hokeys_Descr, HK_DRAG );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DRAG_CMP_REQUEST, msg, move_sheet_xpm );
    }

    if( Sheet->m_Flags )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_END_SHEET, _( "Place Sheet" ), apply_xpm );
    }
    else
    {
        msg = AddHotkeyName( _( "Edit Sheet" ), s_Schematic_Hokeys_Descr, HK_EDIT );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_SHEET, msg, edit_sheet_xpm );

        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_RESIZE_SHEET, _( "Resize Sheet" ),
                      resize_sheet_xpm );
        PopMenu->AppendSeparator();
        ADD_MENUITEM( PopMenu, ID_POPUP_IMPORT_GLABEL, _( "Import PinSheets" ),
                      import_hierarchical_label_xpm );

        if( Sheet->HasUndefinedLabels() )  // Sheet has pin labels, and can be cleaned
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_CLEANUP_SHEET, _( "Cleanup PinSheets" ),
                          options_pinsheet_xpm );
        PopMenu->AppendSeparator();
        msg = AddHotkeyName( _( "Delete Sheet" ), s_Schematic_Hokeys_Descr, HK_DELETE );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, msg, delete_sheet_xpm );
    }
}


void AddMenusForPinSheet( wxMenu* PopMenu, SCH_SHEET_PIN* PinSheet )
{
    wxString msg;

    if( !PinSheet->m_Flags )
    {
        msg = AddHotkeyName( _( "Move PinSheet" ), s_Schematic_Hokeys_Descr,
                             HK_MOVE_COMPONENT_OR_ITEM );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_PINSHEET, msg, move_xpm );
    }

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_PINSHEET, _( "Edit PinSheet" ), edit_xpm );

    if( !PinSheet->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete PinSheet" ), delete_pinsheet_xpm );
}


void AddMenusForBlock( wxMenu* PopMenu, WinEDA_SchematicFrame* frame )
{
    wxString msg;

    ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel Block" ), cancel_xpm );

    PopMenu->AppendSeparator();

    if( frame->GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE )
        ADD_MENUITEM( PopMenu, ID_POPUP_ZOOM_BLOCK, _( "Window Zoom" ), zoom_selected_xpm );

    ADD_MENUITEM( PopMenu, ID_POPUP_PLACE_BLOCK, _( "Place Block" ), apply_xpm );

    // After a block move (that is also a block selection) one can reselect
    // a block function.
    if( frame->GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE )
    {
        ADD_MENUITEM( PopMenu, wxID_COPY, _( "Save Block" ), copy_button );
        ADD_MENUITEM( PopMenu, ID_POPUP_COPY_BLOCK, _( "Copy Block" ), copyblock_xpm );
        msg = AddHotkeyName( _( "Drag Block" ), s_Schematic_Hokeys_Descr,
                             HK_MOVEBLOCK_TO_DRAGBLOCK );
        ADD_MENUITEM( PopMenu, ID_POPUP_DRAG_BLOCK, msg, move_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_DELETE_BLOCK, _( "Delete Block" ), delete_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_MIRROR_Y_BLOCK, _( "Mirror Block ||" ), mirror_H_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_MIRROR_X_BLOCK, _( "Mirror Block --" ), mirror_V_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_ROTATE_BLOCK, _( "Rotate Block ccw" ), rotate_pos_xpm );

#if 0
  #ifdef __WINDOWS__
        ADD_MENUITEM( menu_other_block_commands, ID_GEN_COPY_BLOCK_TO_CLIPBOARD,
                      _( "Copy to Clipboard" ), copy_button );
  #endif
#endif
    }
}


void AddMenusForMarkers( wxMenu* aPopMenu, SCH_MARKER* aMarker, WinEDA_SchematicFrame* aFrame )
{
    ADD_MENUITEM( aPopMenu, ID_POPUP_SCH_DELETE, _( "Delete Marker" ), delete_xpm );
    ADD_MENUITEM( aPopMenu, ID_POPUP_SCH_GETINFO_MARKER, _( "Marker Error Info" ), info_xpm );
}
