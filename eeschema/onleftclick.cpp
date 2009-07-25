/******************************************************/
/* schedit.cpp: fonctions generales de la schematique */
/******************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "id.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "class_marker_sch.h"

#include "protos.h"

static wxArrayString s_CmpNameList;
static wxArrayString s_PowerNameList;

/**********************************************************************************/
void WinEDA_SchematicFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
/**********************************************************************************/

/* Traite les commandes declench�e par le bouton gauche de la souris,
 *  quand un outil est deja selectionn�
 */
{
    SCH_ITEM* DrawStruct = (SCH_ITEM*) GetScreen()->GetCurItem();

    if( (m_ID_current_state == 0) || ( DrawStruct && DrawStruct->m_Flags ) )
    {
        DrawPanel->m_AutoPAN_Request = FALSE;
        g_ItemToRepeat = NULL;

        if( DrawStruct && DrawStruct->m_Flags ) // Commande "POPUP" en cours
        {
            switch( DrawStruct->Type() )
            {
            case TYPE_SCH_LABEL:
            case TYPE_SCH_GLOBALLABEL:
            case TYPE_SCH_HIERLABEL:
            case TYPE_SCH_TEXT:
            case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
            case DRAW_SHEET_STRUCT_TYPE:
            case DRAW_BUSENTRY_STRUCT_TYPE:
            case DRAW_JUNCTION_STRUCT_TYPE:
            case TYPE_SCH_COMPONENT:
            case DRAW_PART_TEXT_STRUCT_TYPE:
                DrawStruct->Place( this, DC );
                GetScreen()->SetCurItem( NULL );
                TestDanglingEnds( GetScreen()->EEDrawList, NULL );    // don't draw here
                DrawPanel->Refresh( TRUE );
                return;

            case SCREEN_STRUCT_TYPE:
                DisplayError( this,
                             wxT( "OnLeftClick err: unexpected type for Place" ) );
                DrawStruct->m_Flags = 0;
                break;

            case DRAW_SEGMENT_STRUCT_TYPE:      // Segment peut-etre en cours de trace
                break;

            default:
                DisplayError( this,
                             wxT( "WinEDA_SchematicFrame::OnLeftClick err: m_Flags != 0" ) );
                DrawStruct->m_Flags = 0;
                break;
            }
        }
        else
        {
            DrawStruct = SchematicGeneralLocateAndDisplay(true);
        }
    }

    switch( m_ID_current_state )
    {
    case 0:
        break;

    case ID_NO_SELECT_BUTT:
        break;

    case ID_HIERARCHY_PUSH_POP_BUTT:
        if( DrawStruct && DrawStruct->m_Flags )
            break;
        DrawStruct = SchematicGeneralLocateAndDisplay();
        if( DrawStruct && (DrawStruct->Type() == DRAW_SHEET_STRUCT_TYPE) )
        {
            InstallNextScreen( (DrawSheetStruct*) DrawStruct );
        }
        else
            InstallPreviousSheet();
        break;

    case ID_NOCONN_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            g_ItemToRepeat = CreateNewNoConnectStruct( DC );
            GetScreen()->SetCurItem( g_ItemToRepeat );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        TestDanglingEnds( GetScreen()->EEDrawList, NULL );
        DrawPanel->Refresh( TRUE );
        break;

    case ID_JUNCTION_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            g_ItemToRepeat = CreateNewJunctionStruct( DC, GetScreen()->m_Curseur, TRUE );
            GetScreen()->SetCurItem( g_ItemToRepeat );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        TestDanglingEnds( GetScreen()->EEDrawList, NULL );
        DrawPanel->Refresh( TRUE );
        break;

    case ID_WIRETOBUS_ENTRY_BUTT:
    case ID_BUSTOBUS_ENTRY_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct =
                CreateBusEntry( DC,
                                (m_ID_current_state == ID_WIRETOBUS_ENTRY_BUTT) ?
                                WIRE_TO_BUS : BUS_TO_BUS );
            GetScreen()->SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            GetScreen()->SetCurItem( NULL );
            TestDanglingEnds( GetScreen()->EEDrawList, NULL );
            DrawPanel->Refresh( TRUE );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        break;

    case ID_SCHEMATIC_DELETE_ITEM_BUTT:
        LocateAndDeleteItem( this, DC );
        GetScreen()->SetModify();
        GetScreen()->SetCurItem( NULL );
        TestDanglingEnds( GetScreen()->EEDrawList, NULL );
        DrawPanel->Refresh( TRUE );
        break;

    case ID_WIRE_BUTT:
        BeginSegment( DC, LAYER_WIRE );
        DrawPanel->m_AutoPAN_Request = TRUE;
        break;

    case ID_BUS_BUTT:
        BeginSegment( DC, LAYER_BUS );
        DrawPanel->m_AutoPAN_Request = TRUE;
        break;

    case ID_LINE_COMMENT_BUTT:
        BeginSegment( DC, LAYER_NOTES );
        DrawPanel->m_AutoPAN_Request = TRUE;
        break;

    case ID_TEXT_COMMENT_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( CreateNewText( DC, LAYER_NOTES ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        break;

    case ID_LABEL_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( CreateNewText( DC, LAYER_LOCLABEL ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( GetScreen()->EEDrawList, NULL );
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_GLABEL_BUTT:
    case ID_HIERLABEL_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            if(m_ID_current_state == ID_GLABEL_BUTT)
                GetScreen()->SetCurItem( CreateNewText( DC, LAYER_GLOBLABEL ) );
            if(m_ID_current_state == ID_HIERLABEL_BUTT)
                GetScreen()->SetCurItem( CreateNewText( DC, LAYER_HIERLABEL ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( GetScreen()->EEDrawList, NULL );
            DrawPanel->Refresh( TRUE );
        }
        break;

        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( CreateNewText( DC, LAYER_HIERLABEL ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( GetScreen()->EEDrawList, NULL );
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_SHEET_SYMBOL_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( CreateSheet( DC ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( GetScreen()->EEDrawList, NULL );
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_IMPORT_GLABEL_BUTT:
    case ID_SHEET_LABEL_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
            DrawStruct = SchematicGeneralLocateAndDisplay();

        if( DrawStruct == NULL )
            break;

        if( (DrawStruct->Type() == DRAW_SHEET_STRUCT_TYPE)
           && (DrawStruct->m_Flags == 0) )
        {
            if( m_ID_current_state == ID_IMPORT_GLABEL_BUTT )
                GetScreen()->SetCurItem(
                         Import_PinSheet( (DrawSheetStruct*) DrawStruct, DC ) );
            else
                GetScreen()->SetCurItem(
                    Create_PinSheet( (DrawSheetStruct*) DrawStruct, DC ) );
        }
        else if( (DrawStruct->Type() == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE)
                && (DrawStruct->m_Flags != 0) )
        {
            DrawStruct->Place( this, DC );
            TestDanglingEnds( GetScreen()->EEDrawList, NULL );
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_COMPONENT_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( Load_Component( DC, wxEmptyString,
                                                         s_CmpNameList, TRUE ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( GetScreen()->EEDrawList, NULL );
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_PLACE_POWER_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem(
                Load_Component( DC, wxT( "power" ), s_PowerNameList, FALSE ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( GetScreen()->EEDrawList, NULL );
            DrawPanel->Refresh( TRUE );
        }
        break;

    default:
    {
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        wxString msg( wxT( "WinEDA_SchematicFrame::OnLeftClick error state " ) );

        msg << m_ID_current_state;
        DisplayError( this, msg );
        break;
    }
    }
}


/***************************************************************************/
void WinEDA_SchematicFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
/***************************************************************************/

/** Function OnLeftDClick
 * called on a double click event from the drawpanel mouse handler
 *  if an editable item is found (text, component)
 *      Call the suitable dialog editor.
 *  Id a creat command is in progress:
 *      validate and finish the command
 */
{
    EDA_BaseStruct* DrawStruct = GetScreen()->GetCurItem();
    wxPoint         pos = GetPosition();

    switch( m_ID_current_state )
    {
    case 0:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = SchematicGeneralLocateAndDisplay();
        }

        if( (DrawStruct == NULL) || (DrawStruct->m_Flags != 0) )
            break;

        // Element localis�
        switch( DrawStruct->Type() )
        {
        case DRAW_SHEET_STRUCT_TYPE:
            InstallNextScreen( (DrawSheetStruct*) DrawStruct );
            break;

        case TYPE_SCH_COMPONENT:
            InstallCmpeditFrame( this, pos, (SCH_COMPONENT*) DrawStruct );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPE_SCH_TEXT:
        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
            EditSchematicText( (SCH_TEXT*) DrawStruct, DC );
            break;

        case DRAW_PART_TEXT_STRUCT_TYPE:
            EditCmpFieldText( (SCH_CMP_FIELD*) DrawStruct, DC );
            DrawPanel->MouseToCursorSchema();
            break;

        case DRAW_MARKER_STRUCT_TYPE:
            ((MARKER_SCH*)DrawStruct)->DisplayMarkerInfo( this);
            break;

        default:
            break;
        }

        break;      // end case 0

    case ID_BUS_BUTT:
    case ID_WIRE_BUTT:
    case ID_LINE_COMMENT_BUTT:
        if( DrawStruct && (DrawStruct->m_Flags & IS_NEW) )
            EndSegment( DC );
        break;
    }
}
