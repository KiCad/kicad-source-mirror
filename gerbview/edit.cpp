/******************************************************/
/* edit.cpp: fonctions generales de l'edition du PCB */
/******************************************************/

#include "fctsys.h"
#include "id.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"

/************************************************************************/
void WinEDA_GerberFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
/************************************************************************/

/* Traite les commandes declench�e par le bouton gauche de la souris,
 *  quand un outil est deja selectionn�
 */
{
    BOARD_ITEM*     DrawStruct = GetScreen()->GetCurItem();
    wxString        msg;

    if( m_ID_current_state == 0 )
    {
        if( DrawStruct && DrawStruct->m_Flags ) // Commande "POPUP" en cours
        {
            msg.Printf( wxT( "WinEDA_GerberFrame::ProcessCommand err: Struct %d, m_Flags = %X" ),
                        (unsigned) DrawStruct->Type(),
                        (unsigned) DrawStruct->m_Flags );
            DisplayError( this, msg );
        }
        else
        {
            DrawStruct = GerberGeneralLocateAndDisplay();
            GetScreen()->SetCurItem( DrawStruct );
        }
    }

    switch( m_ID_current_state )
    {
    case 0:
        break;

    case ID_NO_SELECT_BUTT:
        break;


    case ID_PCB_DELETE_ITEM_BUTT:
        DrawStruct = GerberGeneralLocateAndDisplay();
        if( DrawStruct == NULL )
            break;
        if( DrawStruct->Type() == TYPE_TRACK )
        {
            Delete_Segment( DC, (TRACK*) DrawStruct );
            GetScreen()->SetCurItem( NULL );
            GetScreen()->SetModify();
        }
        break;

    default:
        DisplayError( this, wxT( "WinEDA_GerberFrame::ProcessCommand error" ) );
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;
    }
}


/********************************************************************************/
void WinEDA_GerberFrame::Process_Special_Functions( wxCommandEvent& event )
/********************************************************************************/

/* Traite les selections d'outils et les commandes appelees du menu POPUP
 */
{
    int           id    = event.GetId();
    int           layer = GetScreen()->m_Active_Layer;
    GERBER*       gerber_layer = g_GERBER_List[layer];
    wxPoint       pos;
    wxClientDC    dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );

    wxGetMousePosition( &pos.x, &pos.y );

    pos.y += 20;

    switch( id )   // Arret eventuel de la commande de d�placement en cours
    {
    case wxID_CUT:
    case wxID_COPY:
    case ID_POPUP_MIRROR_X_BLOCK:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_INVERT_BLOCK:
    case ID_POPUP_ROTATE_BLOCK:
    case ID_POPUP_COPY_BLOCK:
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( DrawPanel->ManageCurseur
            && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
        /* ne devrait pas etre execute, sauf bug */
        if( GetScreen()->m_BlockLocate.m_Command != BLOCK_IDLE )
        {
            GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
            GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
            GetScreen()->m_BlockLocate.ClearItemsList();
        }
        if( m_ID_current_state == 0 )
            SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        else
            SetCursor( DrawPanel->m_PanelCursor = DrawPanel->m_PanelDefaultCursor );
        break;

    default:        // Arret dea commande de d�placement en cours
        if( DrawPanel->ManageCurseur
            && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;
    }

    switch( id )   // Traitement des commandes
    {
    case ID_EXIT:
        Close( TRUE );
        break;

    case ID_NEW_PROJECT:
    case ID_LOAD_PROJECT:
        Files_io( event );
        break;

    case ID_PCB_GLOBAL_DELETE:
        Erase_Current_Layer( TRUE );
        break;

    case wxID_CUT:
        break;

    case wxID_COPY:
        break;

    case wxID_PASTE:

//			HandleBlockBegin(&dc, BLOCK_PASTE);
        break;

    case ID_UNDO_BUTT:
        UnDeleteItem( &dc );
        break;

    case ID_GET_TOOLS:

//			InstallToolsFrame(this, wxPoint(-1,-1) );
        break;

    case ID_FIND_ITEMS:

//			InstallFindFrame(this, pos);
        break;


    case ID_BUS_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, wxT( "Add Tracks" ) );
        break;

    case ID_LINE_COMMENT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, wxT( "Add Drawing" ) );
        break;

    case ID_TEXT_COMMENT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, wxT( "Add Text" ) );
        break;

    case ID_NO_SELECT_BUTT:
        SetToolID( 0, 0, wxEmptyString );
        break;

    case ID_POPUP_CLOSE_CURRENT_TOOL:
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        break;

    case ID_POPUP_END_LINE:
        DrawPanel->MouseToCursorSchema();

//			EndSegment(&dc);
        break;

    case ID_POPUP_PCB_DELETE_TRACKSEG:
        DrawPanel->MouseToCursorSchema();
        if( GetScreen()->GetCurItem() == NULL )
            break;
        Delete_Segment( &dc, (TRACK*) GetScreen()->GetCurItem() );
        GetScreen()->SetCurItem( NULL );
        GetScreen()->SetModify();
        break;

    case ID_PCB_DELETE_ITEM_BUTT:
        SetToolID( id, wxCURSOR_BULLSEYE, wxT( "Delete item" ) );
        break;

    case ID_TOOLBARH_PCB_SELECT_LAYER:
        ((PCB_SCREEN*)GetScreen())->m_Active_Layer = m_SelLayerBox->GetChoice();
        DrawPanel->Refresh( TRUE );
        break;

    case ID_TOOLBARH_GERBER_SELECT_TOOL:
        if( gerber_layer )
        {
            int tool = m_SelLayerTool->GetChoice();
            if( tool > 0 )
                tool = tool - 1 + FIRST_DCODE;
            else
                tool = 0;
            gerber_layer->m_Selected_Tool = tool;
            DrawPanel->Refresh( TRUE );
        }
        else
            DisplayError( this, _( "No layer selected" ) );
        break;

    case ID_GERBVIEW_SHOW_LIST_DCODES:
        Liste_D_Codes( &dc );
        break;

    case ID_GERBVIEW_SHOW_SOURCE:
        if( gerber_layer )
        {
            wxString editorname = wxGetApp().GetEditorName();
            if( !editorname.IsEmpty() )
            {
                wxFileName fn( gerber_layer->m_FileName );
                ExecuteFile( this, editorname, QuoteFullPath( fn ) );
            }
        }
        break;

    case ID_POPUP_PLACE_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_MOVE;
        DrawPanel->m_AutoPAN_Request = FALSE;
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_COPY_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_COPY;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        DrawPanel->m_AutoPAN_Request = FALSE;
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_ZOOM_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_ZOOM;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_DELETE;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_MIRROR_X_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_MIRROR_X;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_GERBVIEW_POPUP_DELETE_DCODE_ITEMS:
        if( gerber_layer )
            Delete_DCode_Items( &dc, gerber_layer->m_Selected_Tool, ((PCB_SCREEN*)GetScreen())->m_Active_Layer );
        break;

    default:
        wxMessageBox( wxT( "WinEDA_GerberFrame::Process_Special_Functions error" ) );
        break;
    }

    SetToolbars();
}


/**************************************************************************/
void WinEDA_GerberFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
/**************************************************************************/

/* Appel� sur un double click:
 *  pour un �l�ment editable (textes, composant):
 *      appel de l'editeur correspondant.
 *  pour une connexion en cours:
 *      termine la connexion
 */
{
    EDA_BaseStruct* DrawStruct = GetScreen()->GetCurItem();
    wxClientDC      dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );

    switch( m_ID_current_state )
    {
    case 0:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = GerberGeneralLocateAndDisplay();
        }

        break;      // end case 0

    case ID_BUS_BUTT:
    case ID_WIRE_BUTT:

//			if ( DrawStruct && (DrawStruct->m_Flags & IS_NEW) )
//				EndSegment(DC);
        break;
    }
}
