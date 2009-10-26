/******************************************************************/
/* onrightclick.cpp - creation du menu popup appele par le bouton */
/*	droit de la souris											  */
/******************************************************************/

#include "fctsys.h"
#include "common.h"
#include "eeschema_id.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "bitmaps.h"

#include "program.h"
#include "general.h"
#include "class_marker_sch.h"
#include "protos.h"
#include "hotkeys.h"
#include "class_library.h"


/* functions to add commands and submenus depending on the item */
static void AddMenusForBlock( wxMenu* PopMenu, WinEDA_SchematicFrame* frame );
static void AddMenusForWire( wxMenu* PopMenu, EDA_DrawLineStruct* Wire,
                             WinEDA_SchematicFrame* frame );
static void AddMenusForBus( wxMenu* PopMenu, EDA_DrawLineStruct* Bus,
                            WinEDA_SchematicFrame* frame );
static void AddMenusForHierchicalSheet( wxMenu*          PopMenu,
                                        DrawSheetStruct* Sheet );
static void AddMenusForPinSheet( wxMenu*                        PopMenu,
                                 Hierarchical_PIN_Sheet_Struct* PinSheet );
static void AddMenusForText( wxMenu* PopMenu, SCH_TEXT* Text );
static void AddMenusForLabel( wxMenu* PopMenu, SCH_LABEL* Label );
static void AddMenusForGLabel( wxMenu* PopMenu, SCH_GLOBALLABEL* GLabel );
static void AddMenusForHLabel( wxMenu* PopMenu, SCH_HIERLABEL* GLabel );
static void AddMenusForComponent( wxMenu* PopMenu, SCH_COMPONENT* Component );
static void AddMenusForComponentField( wxMenu* PopMenu, SCH_CMP_FIELD* Field );
static void AddMenusForJunction( wxMenu* PopMenu, DrawJunctionStruct* Junction,
                                 WinEDA_SchematicFrame* frame );
static void AddMenusForMarkers( wxMenu* aPopMenu, MARKER_SCH* aMarker,
                                WinEDA_SchematicFrame* aFrame );


/*****************************************************************/
bool WinEDA_SchematicFrame::OnRightClick( const wxPoint& MousePos,
                                          wxMenu*        PopMenu )
{
/*****************************************************************/

/* Prepare le menu PullUp affich� par un click sur le bouton droit
 *  de la souris.
 *  Ce menu est ensuite compl�t� par la liste des commandes de ZOOM
 */
    SCH_ITEM* DrawStruct  = (SCH_ITEM*) GetScreen()->GetCurItem();
    bool      BlockActive =
        (GetScreen()->m_BlockLocate.m_Command != BLOCK_IDLE);


    DrawPanel->m_CanStartBlock = -1;    // Ne pas engager un debut de bloc sur validation menu

    if( BlockActive )
    {
        AddMenusForBlock( PopMenu, this );
        PopMenu->AppendSeparator();
        return true;
    }

    if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) ) // Just try to locate items at cursor position
    {
        DrawStruct = SchematicGeneralLocateAndDisplay( false );
        if( DrawStruct && (DrawStruct->Type() == DRAW_SHEET_STRUCT_TYPE) )
        {
            Hierarchical_PIN_Sheet_Struct* slabel;
            slabel = LocateSheetLabel( (DrawSheetStruct*) DrawStruct,
                                      GetScreen()->m_Curseur );
            if( slabel )
                DrawStruct = slabel;
        }
    }

    // If Command in progress: add "cancel" and "end tool" menu
    if(  m_ID_current_state )
    {
        if( DrawStruct && DrawStruct->m_Flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                          _( "Cancel" ), cancel_xpm );
        }
        else
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_CLOSE_CURRENT_TOOL,
                          _( "End Tool" ), cancel_tool_xpm );
        }
        PopMenu->AppendSeparator();
    }
    else
    {
        if( DrawStruct && DrawStruct->m_Flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                          _( "Cancel" ), cancel_xpm );
            PopMenu->AppendSeparator();
        }
    }

    if( DrawStruct == NULL )
    {
        if( GetSheet()->Last() != g_RootSheet )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_LEAVE_SHEET,
                          _( "Leave Sheet" ), leave_sheet_xpm );
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

        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Noconn" ),
                      delete_xpm );
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        AddMenusForJunction( PopMenu, (DrawJunctionStruct*) DrawStruct, this );
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        if( !flags )
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST,
                          _( "Move Bus Entry" ), move_xpm );
        if( GetBusEntryShape( (DrawBusEntryStruct*) DrawStruct ) == '\\' )
            PopMenu->Append( ID_POPUP_SCH_ENTRY_SELECT_SLASH,
                            _( "Set Bus Entry /" ) );
        else
            PopMenu->Append( ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH,
                            _( "Set Bus Entry \\" ) );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE,
                      _( "Delete Bus Entry" ), delete_bus_xpm );
        break;

    case TYPE_MARKER_SCH:
        AddMenusForMarkers( PopMenu, (MARKER_SCH*) DrawStruct, this );
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
        AddMenusForComponentField( PopMenu, (SCH_CMP_FIELD*) DrawStruct );
        if( flags )
            break;

        // Many fields are inside a component. If this is the case, add the component menu
        SCH_COMPONENT* Component = LocateSmallestComponent(
            (SCH_SCREEN*) GetScreen() );
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

//			if( !flags ) PopMenu->Append(ID_POPUP_SCH_MOVE_ITEM_REQUEST, "Move");
        switch( DrawStruct->GetLayer() )
        {
        case LAYER_WIRE:
            AddMenusForWire( PopMenu, (EDA_DrawLineStruct*) DrawStruct, this );
            break;

        case LAYER_BUS:
            AddMenusForBus( PopMenu, (EDA_DrawLineStruct*) DrawStruct, this );
            break;

        default:
            if( is_new )
                ADD_MENUITEM( PopMenu, ID_POPUP_END_LINE, _( "End Drawing" ),
                              apply_xpm );
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE,
                          _( "Delete Drawing" ), delete_xpm );
            break;
        }

        break;

    case DRAW_SHEET_STRUCT_TYPE:
        AddMenusForHierchicalSheet( PopMenu, (DrawSheetStruct*) DrawStruct );
        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        AddMenusForPinSheet( PopMenu,
                             (Hierarchical_PIN_Sheet_Struct*) DrawStruct );
        break;

    default:
        wxString msg;
        msg.Printf( wxT( "WinEDA_SchematicFrame::OnRightClick Error: unknown \
DrawType %d" ),
                   DrawStruct->Type() );
        DisplayError( this, msg );
        break;
    }

    PopMenu->AppendSeparator();
    return true;
}


/*************************************************************************/
void AddMenusForComponentField( wxMenu* PopMenu, SCH_CMP_FIELD* Field )
{
/*************************************************************************/

/* Add menu commands for a component field (like value, reference)
 */
    if( !Field->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST,
                      _( "Move Field" ), move_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_FIELD,
                  _( "Rotate Field" ), rotate_field_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_FIELD,
                  _( "Edit Field" ), edit_text_xpm );
}


/**************************************************************************/
void AddMenusForComponent( wxMenu* PopMenu, SCH_COMPONENT* Component )
{
/**************************************************************************/

/* Add menu commands for a component
 */
    if( Component->Type() != TYPE_SCH_COMPONENT )
    {
        wxASSERT( 0 );
        return;
    }

    wxString       msg;
    CMP_LIB_ENTRY* libEntry;
    LIB_COMPONENT* libComponent = NULL;

    libEntry = CMP_LIBRARY::FindLibraryEntry( Component->m_ChipName );

    if( libEntry )
    {
        if( libEntry->Type == ALIAS )
            libComponent = ( (LIB_ALIAS*) libEntry )->GetComponent();
        else
            libComponent = (LIB_COMPONENT*) libEntry;
    }

    if( !Component->m_Flags )
    {
        msg = _( "Move Component" );
        msg << wxT( " " ) << Component->GetField( REFERENCE )->m_Text;
        msg = AddHotkeyName( msg, s_Schematic_Hokeys_Descr, HK_MOVE_COMPONENT );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_CMP_REQUEST,
                      msg, move_xpm );
        msg = AddHotkeyName( _( "Drag Component" ), s_Schematic_Hokeys_Descr,
                             HK_DRAG_COMPONENT );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DRAG_CMP_REQUEST,
                      msg, move_xpm );
    }

    // add menu orient et sous menu:
    wxMenu* orientmenu = new wxMenu;
    msg = AddHotkeyName( _( "Rotate +" ), s_Schematic_Hokeys_Descr,
                         HK_ROTATE_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE,
                  msg, rotate_pos_xpm );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE,
                  _( "Rotate -" ), rotate_neg_xpm );
    msg = AddHotkeyName( _( "Mirror --" ), s_Schematic_Hokeys_Descr,
                         HK_MIRROR_X_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_MIROR_X_CMP, msg, mirror_V_xpm );
    msg = AddHotkeyName( _( "Mirror ||" ), s_Schematic_Hokeys_Descr,
                         HK_MIRROR_Y_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_MIROR_Y_CMP, msg, mirror_H_xpm );
    msg = AddHotkeyName( _( "Normal" ), s_Schematic_Hokeys_Descr,
                         HK_ORIENT_NORMAL_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_ORIENT_NORMAL_CMP, msg, normal_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, orientmenu,
                               ID_POPUP_SCH_GENERIC_ORIENT_CMP,
                               _( "Orient Component" ), orient_xpm );

    wxMenu* editmenu = new wxMenu;
    msg = AddHotkeyName( _( "Edit" ), s_Schematic_Hokeys_Descr, HK_EDIT_COMPONENT );
    ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_CMP, msg,
                  edit_component_xpm );

    if( libEntry && libEntry->m_Options != ENTRY_POWER )
    {
        msg = AddHotkeyName( _( "Value " ), s_Schematic_Hokeys_Descr,
                             HK_EDIT_COMPONENT_VALUE );
        ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_VALUE_CMP, msg,
                      edit_comp_value_xpm );

        ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_REF_CMP,
                      _( "Reference" ), edit_comp_ref_xpm );

        msg = AddHotkeyName( _( "Footprint " ), s_Schematic_Hokeys_Descr,
                             HK_EDIT_COMPONENT_FOOTPRINT );
        ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_FOOTPRINT_CMP, msg,
                      edit_comp_footprint_xpm );
    }

    if( libComponent && libComponent->HasConversion() )
        ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_CONVERT_CMP,
                      _( "Convert" ), component_select_alternate_shape_xpm );

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

        ADD_MENUITEM_WITH_SUBMENU( editmenu, sel_unit_menu,
                                   ID_POPUP_SCH_SELECT_UNIT_CMP,
                                   _( "Unit" ), component_select_unit_xpm );
    }

    ADD_MENUITEM_WITH_SUBMENU( PopMenu, editmenu,
                               ID_POPUP_SCH_GENERIC_EDIT_CMP,
                               _( "Edit Component" ), edit_component_xpm );

    if( !Component->m_Flags )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_COPY_COMPONENT_CMP,
                      _( "Copy Component" ), import_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_CMP,
                      _( "Delete Component" ), delete_xpm );
    }

    if( libEntry && !libEntry->m_DocFile.IsEmpty() )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DISPLAYDOC_CMP, _( "Doc" ),
                      datasheet_xpm );
}


/*******************************************************************/
void AddMenusForGLabel( wxMenu* PopMenu, SCH_GLOBALLABEL* GLabel )
{
/*******************************************************************/

/* Add menu commands for a Global Label
 */
    wxMenu*  menu_change_type = new wxMenu;
    wxString msg;

    if( !GLabel->m_Flags )
    {
        msg = AddHotkeyName( _( "Move Global Label" ), s_Schematic_Hokeys_Descr, HK_MOVE_COMPONENT );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST,
                      msg, move_text_xpm );
    }
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT,
                  _( "Rotate Global Label" ), rotate_glabel_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT,
                  _( "Edit Global Label" ), edit_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE,
                  _( "Delete Global Label" ), delete_text_xpm );

    // add menu change type text (to label, glabel, text):
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_HLABEL,
                  _( "Change to Hierarchical Label" ), label2glabel_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL,
                  _( "Change to Label" ), glabel2label_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT,
                  _( "Change to Text" ), glabel2text_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type,
                               ID_POPUP_SCH_CHANGE_TYPE_TEXT,
                               _( "Change Type" ), gl_change_xpm );
}


/*******************************************************************/
void AddMenusForHLabel( wxMenu* PopMenu, SCH_HIERLABEL* HLabel )
{
/*******************************************************************/

/* Add menu commands for a hierarchical Label
 */
    wxMenu*  menu_change_type = new wxMenu;
    wxString msg;

    if( !HLabel->m_Flags )
    {
        msg = AddHotkeyName( _(
                                 "Move Hierarchical Label" ), s_Schematic_Hokeys_Descr,
                             HK_MOVE_COMPONENT );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST,
                      msg, move_text_xpm );
    }
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT,
                  _( "Rotate Hierarchical Label" ), rotate_glabel_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT,
                  _( "Edit Hierarchical Label" ), edit_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE,
                  _( "Delete Hierarchical label" ), delete_text_xpm );

    // add menu change type text (to label, glabel, text):
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL,
                  _( "Change to Label" ), glabel2label_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT,
                  _( "Change to Text" ), glabel2text_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL,
                  _( "Change to Global Label" ), label2glabel_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type,
                               ID_POPUP_SCH_CHANGE_TYPE_TEXT,
                               _( "Change Type" ), gl_change_xpm );
}


/*****************************************************************/
void AddMenusForLabel( wxMenu* PopMenu, SCH_LABEL* Label )
{
/*****************************************************************/

/* Add menu commands for a Label
 */
    wxMenu*  menu_change_type = new wxMenu;
    wxString msg;

    if( !Label->m_Flags )
    {
        msg = AddHotkeyName( _( "Move Label" ), s_Schematic_Hokeys_Descr, HK_MOVE_COMPONENT );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST,
                      msg, move_text_xpm );
    }
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT,
                  _( "Rotate Label" ), rotate_pos_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT,
                  _( "Edit Label" ), edit_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE,
                  _( "Delete Label" ), delete_text_xpm );

    // add menu change type text (to label, glabel, text):
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_HLABEL,
                  _( "Change to Hierarchical Label" ), label2glabel_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT,
                  _( "Change to Text" ), label2text_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL,
                  _( "Change to Global Label" ), label2glabel_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type,
                               ID_POPUP_SCH_CHANGE_TYPE_TEXT,
                               _( "Change Type" ), gl_change_xpm );
}


/*****************************************************************/
void AddMenusForText( wxMenu* PopMenu, SCH_TEXT* Text )
{
/*****************************************************************/

/* Add menu commands for a Text (a comment)
 */
    wxMenu* menu_change_type = new wxMenu;

    if( !Text->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST,
                      _( "Move Text" ), move_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT, _( "Rotate Text" ),
                  rotate_pos_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT, _( "Edit Text" ),
                  edit_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Text" ),
                  delete_text_xpm );

    /* add menu change type text (to label, glabel, text),
     * but only if this is a single line text
     */
    if( Text->m_Text.Find( wxT( "\n" ) ) ==  wxNOT_FOUND )
    {
        ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL,
                      _( "Change to Label" ), label2text_xpm );
        ADD_MENUITEM( menu_change_type,
                      ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_HLABEL,
                      _( "Change to Hierarchical Label" ),
                      label2glabel_xpm );
        ADD_MENUITEM( menu_change_type,
                      ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL,
                      _( "Change to Glabel" ), label2glabel_xpm );
        ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type,
                                   ID_POPUP_SCH_CHANGE_TYPE_TEXT,
                                   _( "Change Type" ), gl_change_xpm );
    }
}


/*****************************************************************/
void AddMenusForJunction( wxMenu* PopMenu, DrawJunctionStruct* Junction,
                          WinEDA_SchematicFrame* frame )
{
/*****************************************************************/

/* Add menu commands for a junction
 */
    bool is_new = (Junction->m_Flags & IS_NEW) ? TRUE : FALSE;

    if( !is_new )
    {
        if( PickStruct( frame->GetScreen()->m_Curseur, frame->GetScreen(),
                        WIREITEM | BUSITEM | EXCLUDE_WIRE_BUS_ENDPOINTS ) )
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_BREAK_WIRE,
                          _( "Break Wire" ), break_line_xpm );
    }

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Junction" ),
                  delete_xpm );

    if( PickStruct( frame->GetScreen()->m_Curseur, frame->GetScreen(),
                    WIREITEM | BUSITEM ) )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_NODE,
                      _( "Delete Node" ), delete_node_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_CONNECTION,
                      _( "Delete Connection" ), delete_connection_xpm );
    }
}


/*****************************************************************/
void AddMenusForWire( wxMenu* PopMenu, EDA_DrawLineStruct* Wire,
                      WinEDA_SchematicFrame* frame )
{
/*****************************************************************/

/* Add menu commands for a wire
 */
    bool    is_new = (Wire->m_Flags & IS_NEW) ? TRUE : FALSE;
    wxPoint pos    = frame->GetScreen()->m_Curseur;

    if( is_new )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_END_LINE, _( "Wire End" ), apply_xpm );
        return;
    }

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DRAG_WIRE_REQUEST, _( "Drag Wire" ),
                  move_track_xpm );
    PopMenu->AppendSeparator();
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Wire" ), delete_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_NODE, _( "Delete Node" ),
                  delete_node_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_CONNECTION,
                  _( "Delete Connection" ), delete_connection_xpm );

    if( PickStruct( frame->GetScreen()->m_Curseur, frame->GetScreen(),
                    WIREITEM | BUSITEM | EXCLUDE_WIRE_BUS_ENDPOINTS ) )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_BREAK_WIRE, _( "Break Wire" ),
                      break_line_xpm );

    PopMenu->AppendSeparator();

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_JUNCTION, _( "Add Junction" ),
                  add_junction_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_LABEL, _( "Add Label" ),
                  add_line_label_xpm );

    // Place Global label command only if the cursor is over one end of the Wire:
    if( ( pos.x == Wire->m_Start.x && pos.y == Wire->m_Start.y)
       || ( pos.x == Wire->m_End.x && pos.y == Wire->m_End.y ) )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_GLABEL,
                      _( "Add Global Label" ), add_glabel_xpm );
}


/*****************************************************************/
void AddMenusForBus( wxMenu* PopMenu, EDA_DrawLineStruct* Bus,
                     WinEDA_SchematicFrame* frame )
{
/*****************************************************************/

/* Add menu commands for a Bus
 */
    bool    is_new = (Bus->m_Flags & IS_NEW) ? TRUE : FALSE;
    wxPoint pos    = frame->GetScreen()->m_Curseur;

    if( is_new )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_END_LINE, _( "Bus End" ), apply_xpm );
        return;
    }

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Bus" ),
                  delete_bus_xpm );

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_BREAK_WIRE, _( "Break Bus" ),
                  break_bus_xpm );

    PopMenu->AppendSeparator();
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_JUNCTION, _( "Add Junction" ),
                  add_junction_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_LABEL, _( "Add Label" ),
                  add_line_label_xpm );

    // Place Global label command only if the cursor is over one end of the Bus:
    if( ( pos.x == Bus->m_Start.x && pos.y == Bus->m_Start.y)
       || ( pos.x == Bus->m_End.x && pos.y == Bus->m_End.y ) )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_GLABEL,
                      _( "Add Global Label" ), add_glabel_xpm );
}


/************************************************************************/
void AddMenusForHierchicalSheet( wxMenu* PopMenu, DrawSheetStruct* Sheet )
{
/************************************************************************/

/* Add menu commands for a Sheet
 */
    if( !Sheet->m_Flags )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ENTER_SHEET,
                      _( "Enter Sheet" ), enter_sheet_xpm );
        PopMenu->AppendSeparator();
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST,
                      _( "Move Sheet" ), move_sheet_xpm );
    }

    if( Sheet->m_Flags )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_END_SHEET, _( "Place Sheet" ),
                      apply_xpm );
    }
    else
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_SHEET, _( "Edit Sheet" ),
                      edit_sheet_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_RESIZE_SHEET, _( "Resize Sheet" ),
                      resize_sheet_xpm );
        PopMenu->AppendSeparator();
        ADD_MENUITEM( PopMenu, ID_POPUP_IMPORT_GLABEL, _( "Import PinSheets" ),
                      import_hierarchical_label_xpm );
        if( Sheet->m_Label )  // Sheet has pin labels, and can be cleaned
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_CLEANUP_SHEET,
                          _( "Cleanup PinSheets" ), options_pinsheet_xpm );
        PopMenu->AppendSeparator();
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Sheet" ),
                      delete_sheet_xpm );
    }
}


/************************************************************************/
void AddMenusForPinSheet( wxMenu*                        PopMenu,
                          Hierarchical_PIN_Sheet_Struct* PinSheet )
{
/************************************************************************/

/* Add menu commands for a Pin Sheet (or Sheet label)
 */
    if( !PinSheet->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_PINSHEET,
                      _( "Move PinSheet" ), move_xpm );

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_PINSHEET, _( "Edit PinSheet" ),
                  edit_xpm );

    if( !PinSheet->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete PinSheet" ),
                      delete_pinsheet_xpm );
}


/**********************************************************************/
void AddMenusForBlock( wxMenu* PopMenu, WinEDA_SchematicFrame* frame )
{
/**********************************************************************/

/* Add menu commands for block
 */
    ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                  _( "Cancel Block" ), cancel_xpm );

    PopMenu->AppendSeparator();

    if( frame->GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE )
        ADD_MENUITEM( PopMenu, ID_POPUP_ZOOM_BLOCK, _( "Window Zoom" ),
                      zoom_selected_xpm );

    ADD_MENUITEM( PopMenu, ID_POPUP_PLACE_BLOCK, _( "Place Block" ), apply_xpm );

    if( frame->GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE ) // After a block move (that is also a block selection) one can reselect a block function:
    {
        ADD_MENUITEM( PopMenu, wxID_COPY, _( "Save Block" ), copy_button );
        ADD_MENUITEM( PopMenu, ID_POPUP_COPY_BLOCK, _( "Copy Block" ),
                      copyblock_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_DRAG_BLOCK, _( "Drag Block" ),
                      move_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_DELETE_BLOCK, _( "Delete Block" ),
                      delete_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_MIRROR_Y_BLOCK,
                      _( "Mirror Block ||" ), mirror_H_xpm );
#if 0
  #ifdef __WINDOWS__
        ADD_MENUITEM( menu_other_block_commands,
                      ID_GEN_COPY_BLOCK_TO_CLIPBOARD,
                      _( "Copy to Clipboard" ), copy_button );
  #endif
#endif
    }
}


/**********************************************************************/
void AddMenusForMarkers( wxMenu* aPopMenu, MARKER_SCH* aMarker,
                         WinEDA_SchematicFrame* aFrame )
{
/**********************************************************************/
    ADD_MENUITEM( aPopMenu, ID_POPUP_SCH_DELETE, _( "Delete Marker" ),
                  delete_xpm );
    ADD_MENUITEM( aPopMenu, ID_POPUP_SCH_GETINFO_MARKER,
                  _( "Marker Error Info" ), info_xpm );
}
