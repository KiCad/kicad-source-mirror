/******************************************************/
/* schedit.cpp: fonctions generales de la schematique */
/******************************************************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "id.h"

#include "protos.h"

static wxArrayString s_CmpNameList;
static wxArrayString s_PowerNameList;

/**********************************************************************************/
void WinEDA_SchematicFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
/**********************************************************************************/

/* Traite les commandes declenchée par le bouton gauche de la souris,
 *  quand un outil est deja selectionné
 */
{
    EDA_BaseStruct* DrawStruct = m_CurrentScreen->GetCurItem();

    if( (m_ID_current_state == 0) || ( DrawStruct && DrawStruct->m_Flags ) )
    {
        DrawPanel->m_AutoPAN_Request = FALSE;
        g_ItemToRepeat = NULL;

        if( DrawStruct && DrawStruct->m_Flags ) // Commande "POPUP" en cours
        {
            switch( DrawStruct->Type() )
            {
            case DRAW_LABEL_STRUCT_TYPE:
            case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
            case DRAW_TEXT_STRUCT_TYPE:
            case DRAW_SHEETLABEL_STRUCT_TYPE:
            case DRAW_SHEET_STRUCT_TYPE:
            case DRAW_BUSENTRY_STRUCT_TYPE:
            case DRAW_JUNCTION_STRUCT_TYPE:
            case DRAW_LIB_ITEM_STRUCT_TYPE:
            case DRAW_PART_TEXT_STRUCT_TYPE:
                DrawStruct->Place( this, DC );
                m_CurrentScreen->SetCurItem( NULL );
                TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
                return;

            case SCREEN_STRUCT_TYPE:
            case DRAW_PICK_ITEM_STRUCT_TYPE:
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
            DrawStruct = SchematicGeneralLocateAndDisplay();
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
            InstallPreviousScreen();
        break;

    case ID_NOCONN_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            g_ItemToRepeat = CreateNewNoConnectStruct( DC );
            m_CurrentScreen->SetCurItem( g_ItemToRepeat );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
        break;

    case ID_JUNCTION_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            g_ItemToRepeat = CreateNewJunctionStruct( DC, m_CurrentScreen->m_Curseur, TRUE );
            m_CurrentScreen->SetCurItem( g_ItemToRepeat );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
        break;

    case ID_WIRETOBUS_ENTRY_BUTT:
    case ID_BUSTOBUS_ENTRY_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct =
                CreateBusEntry( DC,
                                (m_ID_current_state == ID_WIRETOBUS_ENTRY_BUTT) ?
                                WIRE_TO_BUS : BUS_TO_BUS );
            m_CurrentScreen->SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            m_CurrentScreen->SetCurItem( NULL );
            TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        break;

    case ID_SCHEMATIC_DELETE_ITEM_BUTT:
        LocateAndDeleteItem( this, DC );
        m_CurrentScreen->SetModify();
        m_CurrentScreen->SetCurItem( NULL );
        TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
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
            m_CurrentScreen->SetCurItem( CreateNewText( DC, LAYER_NOTES ) );
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
            m_CurrentScreen->SetCurItem( CreateNewText( DC, LAYER_LOCLABEL ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
        }
        break;

    case ID_GLABEL_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            m_CurrentScreen->SetCurItem( CreateNewText( DC, LAYER_GLOBLABEL ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
        }
        break;

    case ID_SHEET_SYMBOL_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            m_CurrentScreen->SetCurItem( CreateSheet( DC ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
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
                m_CurrentScreen->SetCurItem(
                         Import_PinSheet( (DrawSheetStruct*) DrawStruct, DC ) );
            else
                m_CurrentScreen->SetCurItem(
                    Create_PinSheet( (DrawSheetStruct*) DrawStruct, DC ) );
        }
        else if( (DrawStruct->Type() == DRAW_SHEETLABEL_STRUCT_TYPE)
                && (DrawStruct->m_Flags != 0) )
        {
            DrawStruct->Place( this, DC );
            TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
        }
        break;

    case ID_COMPONENT_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            m_CurrentScreen->SetCurItem( Load_Component( DC, wxEmptyString,
                                                         s_CmpNameList, TRUE ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
        }
        break;

    case ID_PLACE_POWER_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            m_CurrentScreen->SetCurItem(
                Load_Component( DC, wxT( "power" ), s_PowerNameList, FALSE ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            TestDanglingEnds( m_CurrentScreen->EEDrawList, DC );
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

/* Appelé sur un double click:
 *  pour un élément editable (textes, composant):
 *      appel de l'editeur correspondant.
 *  pour une connexion en cours:
 *      termine la connexion
 */
{
    EDA_BaseStruct* DrawStruct = m_CurrentScreen->GetCurItem();
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

        // Element localisé
        switch( DrawStruct->Type() )
        {
        case DRAW_SHEET_STRUCT_TYPE:
            InstallNextScreen( (DrawSheetStruct*) DrawStruct );
            break;

        case DRAW_LIB_ITEM_STRUCT_TYPE:
            InstallCmpeditFrame( this, pos, (EDA_SchComponentStruct*) DrawStruct );
            DrawPanel->MouseToCursorSchema();
            break;

        case DRAW_TEXT_STRUCT_TYPE:
        case DRAW_LABEL_STRUCT_TYPE:
        case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
            EditSchematicText( (DrawTextStruct*) DrawStruct, DC );
            break;

        case DRAW_PART_TEXT_STRUCT_TYPE:
            EditCmpFieldText( (PartTextStruct*) DrawStruct, DC );
            DrawPanel->MouseToCursorSchema();
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
