/*******************/
/* onleftclick.cpp */
/*******************/

#include "fctsys.h"
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
#include "sch_bitmap.h"


static wxArrayString s_CmpNameList;
static wxArrayString s_PowerNameList;


void SCH_EDIT_FRAME::OnLeftClick( wxDC* aDC, const wxPoint& aPosition )
{
    SCH_ITEM* item = GetScreen()->GetCurItem();
    wxPoint gridPosition = GetGridPosition( aPosition );

    if( ( GetToolId() == ID_NO_TOOL_SELECTED ) || ( item && item->GetFlags() ) )
    {
        DrawPanel->m_AutoPAN_Request = false;
        m_itemToRepeat = NULL;

        if( item && item->GetFlags() )
        {
            switch( item->Type() )
            {
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIERARCHICAL_LABEL_T:
            case SCH_TEXT_T:
            case SCH_SHEET_PIN_T:
            case SCH_SHEET_T:
            case SCH_BUS_ENTRY_T:
            case SCH_JUNCTION_T:
            case SCH_COMPONENT_T:
            case SCH_FIELD_T:
            case SCH_BITMAP_T:
                item->Place( this, aDC );
                GetScreen()->SetCurItem( NULL );
                GetScreen()->TestDanglingEnds();
                DrawPanel->Refresh( true );
                return;

            case SCH_LINE_T:    // May already be drawing segment.
                break;

            default:
                wxFAIL_MSG( wxT( "SCH_EDIT_FRAME::OnLeftClick error.  Item type <" ) +
                            item->GetClass() + wxT( "> is already being edited." ) );
                item->ClearFlags();
            }
        }
        else
        {
            item = LocateAndShowItem( aPosition );
        }
    }

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        break;

    case ID_HIERARCHY_PUSH_POP_BUTT:
        if( ( item && item->GetFlags() ) || ( g_RootSheet->CountSheets() == 0 ) )
            break;

        item = LocateAndShowItem( aPosition, SCH_COLLECTOR::SheetsOnly );

        if( item )
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
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
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
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
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
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            item = CreateBusEntry( aDC, ( GetToolId() == ID_WIRETOBUS_ENTRY_BUTT ) ?
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
        DeleteItemAtCrossHair( aDC );
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
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
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

    case ID_ADD_IMAGE_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            GetScreen()->SetCurItem( CreateNewImage( aDC ) );
            DrawPanel->m_AutoPAN_Request = true;
        }
        else
        {
            item->Place( this, aDC );
            DrawPanel->m_AutoPAN_Request = false;
        }
        break;

    case ID_LABEL_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
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
        if( (item == NULL) || (item->GetFlags() == 0) )
        {
            if( GetToolId() == ID_GLABEL_BUTT )
                GetScreen()->SetCurItem( CreateNewText( aDC, LAYER_GLOBLABEL ) );

            if( GetToolId() == ID_HIERLABEL_BUTT )
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
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
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
    case ID_SHEET_PIN_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
            item = LocateAndShowItem( aPosition, SCH_COLLECTOR::SheetsAndSheetLabels );

        if( item == NULL )
            break;

        if( (item->Type() == SCH_SHEET_T) && (item->GetFlags() == 0) )
        {
            if( GetToolId() == ID_IMPORT_HLABEL_BUTT )
                GetScreen()->SetCurItem( ImportSheetPin( (SCH_SHEET*) item, aDC ) );
            else
                GetScreen()->SetCurItem( CreateSheetPin( (SCH_SHEET*) item, aDC ) );
        }
        else if( (item->Type() == SCH_SHEET_PIN_T) && (item->GetFlags() != 0) )
        {
            item->Place( this, aDC );
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( true );
        }
        break;

    case ID_SCH_PLACE_COMPONENT:
        if( (item == NULL) || (item->GetFlags() == 0) )
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
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
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
        SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );
        wxFAIL_MSG( wxT( "SCH_EDIT_FRAME::OnLeftClick invalid tool ID <" ) +
                    wxString::Format( wxT( "%d> selected." ), GetToolId() ) );
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

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            item = LocateAndShowItem( aPosition );
        }

        if( ( item == NULL ) || ( item->GetFlags() != 0 ) )
            break;

        switch( item->Type() )
        {
        case SCH_SHEET_T:
            m_CurrentSheet->Push( (SCH_SHEET*) item );
            DisplayCurrentSheet();
            break;

        case SCH_COMPONENT_T:
            EditComponent( (SCH_COMPONENT*) item );
            DrawPanel->MoveCursorToCrossHair();
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
            EditSchematicText( (SCH_TEXT*) item );
            break;

        case SCH_BITMAP_T:
            EditImage( (SCH_BITMAP*) item );
            break;

        case SCH_FIELD_T:
            EditComponentFieldText( (SCH_FIELD*) item, aDC );
            DrawPanel->MoveCursorToCrossHair();
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
        if( item && item->IsNew() )
            EndSegment( aDC );

        break;
    }
}
