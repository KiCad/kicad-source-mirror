/*******************/
/* onleftclick.cpp */
/*******************/

#include "fctsys.h"
#include "common.h"
#include "eeschema_id.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_bus_entry.h"
#include "sch_text.h"
#include "sch_marker.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_component.h"
#include "sch_sheet.h"


static wxArrayString s_CmpNameList;
static wxArrayString s_PowerNameList;


void SCH_EDIT_FRAME::OnLeftClick( wxDC* aDC, const wxPoint& aPosition )
{
    SCH_ITEM* item = GetScreen()->GetCurItem();
    wxPoint gridPosition = GetGridPosition( aPosition );

    if( ( m_ID_current_state == 0 ) || ( item && item->m_Flags ) )
    {
        DrawPanel->m_AutoPAN_Request = false;
        m_itemToRepeat = NULL;

        if( item && item->m_Flags )
        {
            switch( item->Type() )
            {
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIERARCHICAL_LABEL_T:
            case SCH_TEXT_T:
            case SCH_SHEET_LABEL_T:
            case SCH_SHEET_T:
            case SCH_BUS_ENTRY_T:
            case SCH_JUNCTION_T:
            case SCH_COMPONENT_T:
            case SCH_FIELD_T:
                item->Place( this, aDC );
                GetScreen()->SetCurItem( NULL );
                GetScreen()->TestDanglingEnds();
                DrawPanel->Refresh( true );
                return;

            case SCH_SCREEN_T:
                DisplayError( this, wxT( "OnLeftClick err: unexpected type for Place" ) );
                item->m_Flags = 0;
                break;

            case SCH_LINE_T: // May already be drawing segment.
                break;

            default:
            {
                wxString msg;
                msg.Printf( wxT( "SCH_EDIT_FRAME::OnLeftClick err: m_Flags != 0, itmetype %d" ),
                            item->Type());
                DisplayError( this, msg );
                item->m_Flags = 0;
                break;
            }
            }
        }
        else
        {
            item = LocateAndShowItem( aPosition );
        }
    }

    switch( m_ID_current_state )
    {
    case 0:
        break;

    case ID_NO_SELECT_BUTT:
        break;

    case ID_HIERARCHY_PUSH_POP_BUTT:
        if( item && item->m_Flags )
            break;

        item = LocateAndShowItem( aPosition );

        if( item && ( item->Type() == SCH_SHEET_T ) )
        {
            m_CurrentSheet->Push( (SCH_SHEET*) item );
            DisplayCurrentSheet();
        }
        else
        {
            wxCHECK_RET( m_CurrentSheet->Last() != g_RootSheet,
                         wxT( "Cannot leave root sheet.  Bad Programmer!" ) );

            m_CurrentSheet->Pop();
            DisplayCurrentSheet();
        }

        break;

    case ID_NOCONN_BUTT:
        if( ( item == NULL ) || ( item->m_Flags == 0 ) )
        {
            m_itemToRepeat = AddNoConnect( aDC, gridPosition );
            GetScreen()->SetCurItem( m_itemToRepeat );
            DrawPanel->m_AutoPAN_Request = true;
        }
        else
        {
            item->Place( this, aDC );
            DrawPanel->m_AutoPAN_Request = false;
        }

        GetScreen()->TestDanglingEnds();
        DrawPanel->Refresh( true );
        break;

    case ID_JUNCTION_BUTT:
        if( ( item == NULL ) || ( item->m_Flags == 0 ) )
        {
            m_itemToRepeat = AddJunction( aDC, gridPosition, true );
            GetScreen()->SetCurItem( m_itemToRepeat );
            DrawPanel->m_AutoPAN_Request = true;
        }
        else
        {
            item->Place( this, aDC );
            DrawPanel->m_AutoPAN_Request = false;
        }

        GetScreen()->TestDanglingEnds();
        DrawPanel->Refresh( true );
        break;

    case ID_WIRETOBUS_ENTRY_BUTT:
    case ID_BUSTOBUS_ENTRY_BUTT:
        if( ( item == NULL ) || ( item->m_Flags == 0 ) )
        {
            item = CreateBusEntry( aDC, ( m_ID_current_state == ID_WIRETOBUS_ENTRY_BUTT ) ?
                                   WIRE_TO_BUS : BUS_TO_BUS );
            GetScreen()->SetCurItem( item );
            DrawPanel->m_AutoPAN_Request = true;
        }
        else
        {
            item->Place( this, aDC );
            GetScreen()->SetCurItem( NULL );
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( true );
            DrawPanel->m_AutoPAN_Request = false;
        }
        break;

    case ID_SCHEMATIC_DELETE_ITEM_BUTT:
        LocateAndDeleteItem( this, aDC );
        OnModify();
        GetScreen()->SetCurItem( NULL );
        GetScreen()->TestDanglingEnds();
        DrawPanel->Refresh( true );
        break;

    case ID_WIRE_BUTT:
        BeginSegment( aDC, LAYER_WIRE );
        DrawPanel->m_AutoPAN_Request = true;
        break;

    case ID_BUS_BUTT:
        BeginSegment( aDC, LAYER_BUS );
        DrawPanel->m_AutoPAN_Request = true;
        break;

    case ID_LINE_COMMENT_BUTT:
        BeginSegment( aDC, LAYER_NOTES );
        DrawPanel->m_AutoPAN_Request = true;
        break;

    case ID_TEXT_COMMENT_BUTT:
        if( ( item == NULL ) || ( item->m_Flags == 0 ) )
        {
            GetScreen()->SetCurItem( CreateNewText( aDC, LAYER_NOTES ) );
            DrawPanel->m_AutoPAN_Request = true;
        }
        else
        {
            item->Place( this, aDC );
            DrawPanel->m_AutoPAN_Request = false;
        }
        break;

    case ID_LABEL_BUTT:
        if( ( item == NULL ) || ( item->m_Flags == 0 ) )
        {
            GetScreen()->SetCurItem( CreateNewText( aDC, LAYER_LOCLABEL ) );
            DrawPanel->m_AutoPAN_Request = true;
        }
        else
        {
            item->Place( this, aDC );
            DrawPanel->m_AutoPAN_Request = false;
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( true );
        }
        break;

    case ID_GLABEL_BUTT:
    case ID_HIERLABEL_BUTT:
        if( (item == NULL) || (item->m_Flags == 0) )
        {
            if(m_ID_current_state == ID_GLABEL_BUTT)
                GetScreen()->SetCurItem( CreateNewText( aDC, LAYER_GLOBLABEL ) );

            if(m_ID_current_state == ID_HIERLABEL_BUTT)
                GetScreen()->SetCurItem( CreateNewText( aDC, LAYER_HIERLABEL ) );

            DrawPanel->m_AutoPAN_Request = true;
        }
        else
        {
            item->Place( this, aDC );
            DrawPanel->m_AutoPAN_Request = false;
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( true );
        }
        break;

    case ID_SHEET_SYMBOL_BUTT:
        if( ( item == NULL ) || ( item->m_Flags == 0 ) )
        {
            GetScreen()->SetCurItem( CreateSheet( aDC ) );
            DrawPanel->m_AutoPAN_Request = true;
        }
        else
        {
            item->Place( this, aDC );
            DrawPanel->m_AutoPAN_Request = false;
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( true );
        }
        break;

    case ID_IMPORT_HLABEL_BUTT:
    case ID_SHEET_LABEL_BUTT:
        if( ( item == NULL ) || ( item->m_Flags == 0 ) )
            item = LocateAndShowItem( aPosition );

        if( item == NULL )
            break;

        if( (item->Type() == SCH_SHEET_T) && (item->m_Flags == 0) )
        {
            if( m_ID_current_state == ID_IMPORT_HLABEL_BUTT )
                GetScreen()->SetCurItem( Import_PinSheet( (SCH_SHEET*) item, aDC ) );
            else
                GetScreen()->SetCurItem( Create_PinSheet( (SCH_SHEET*) item, aDC ) );
        }
        else if( (item->Type() == SCH_SHEET_LABEL_T) && (item->m_Flags != 0) )
        {
            item->Place( this, aDC );
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( true );
        }
        break;

    case ID_COMPONENT_BUTT:
        if( (item == NULL) || (item->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( Load_Component( aDC, wxEmptyString, s_CmpNameList, true ) );
            DrawPanel->m_AutoPAN_Request = true;
        }
        else
        {
            item->Place( this, aDC );
            DrawPanel->m_AutoPAN_Request = false;
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( true );
        }
        break;

    case ID_PLACE_POWER_BUTT:
        if( ( item == NULL ) || ( item->m_Flags == 0 ) )
        {
            GetScreen()->SetCurItem( Load_Component( aDC, wxT( "power" ),
                                                     s_PowerNameList, false ) );
            DrawPanel->m_AutoPAN_Request = true;
        }
        else
        {
            item->Place( this, aDC );
            DrawPanel->m_AutoPAN_Request = false;
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( true );
        }
        break;

    default:
    {
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        wxString msg( wxT( "SCH_EDIT_FRAME::OnLeftClick error state " ) );

        msg << m_ID_current_state;
        DisplayError( this, msg );
        break;
    }
    }
}


/**
 * Function OnLeftDClick
 * called on a double click event from the drawpanel mouse handler
 *  if an editable item is found (text, component)
 *      Call the suitable dialog editor.
 *  Id a create command is in progress:
 *      validate and finish the command
 */
void SCH_EDIT_FRAME::OnLeftDClick( wxDC* aDC, const wxPoint& aPosition )

{
    EDA_ITEM* item = GetScreen()->GetCurItem();
    wxPoint   pos = aPosition;

    switch( m_ID_current_state )
    {
    case 0:
        if( ( item == NULL ) || ( item->m_Flags == 0 ) )
        {
            item = LocateAndShowItem( aPosition );
        }

        if( ( item == NULL ) || ( item->m_Flags != 0 ) )
            break;

        switch( item->Type() )
        {
        case SCH_SHEET_T:
            m_CurrentSheet->Push( (SCH_SHEET*) item );
            DisplayCurrentSheet();
            break;

        case SCH_COMPONENT_T:
            InstallCmpeditFrame( this, (SCH_COMPONENT*) item );
            DrawPanel->MouseToCursorSchema();
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
            EditSchematicText( (SCH_TEXT*) item );
            break;

        case SCH_FIELD_T:
            EditCmpFieldText( (SCH_FIELD*) item, aDC );
            DrawPanel->MouseToCursorSchema();
            break;

        case SCH_MARKER_T:
            ( (SCH_MARKER*) item )->DisplayMarkerInfo( this );
            break;

        default:
            break;
        }

        break;

    case ID_BUS_BUTT:
    case ID_WIRE_BUTT:
    case ID_LINE_COMMENT_BUTT:
        if( item && ( item->m_Flags & IS_NEW ) )
            EndSegment( aDC );

        break;
    }
}
