/***************************************/
/* edit.cpp: Gerbview events functions */
/***************************************/

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "common.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"
#include "kicad_device_context.h"
#include "gerbview_id.h"


/* Process the command triggered by the left button of the mouse when a tool
 * is already selected.
 */
void WinEDA_GerberFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
    BOARD_ITEM* DrawStruct = GetScreen()->GetCurItem();
    wxString    msg;

    if( m_ID_current_state == 0 )
    {
        if( DrawStruct && DrawStruct->m_Flags )
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


    case ID_GERBVIEW_DELETE_ITEM_BUTT:
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
        break;
    }
}


/* Handles the selection of tools, menu, and popup menu commands.
 */
void WinEDA_GerberFrame::Process_Special_Functions( wxCommandEvent& event )
{
    int        id    = event.GetId();
    int        layer = GetScreen()->m_Active_Layer;
    GERBER*    gerber_layer = g_GERBER_List[layer];
    wxPoint    pos;

    wxGetMousePosition( &pos.x, &pos.y );

    pos.y += 20;

    switch( id )
    {
    case wxID_CUT:
    case wxID_COPY:
    case ID_POPUP_MIRROR_X_BLOCK:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_FLIP_BLOCK:
    case ID_POPUP_ROTATE_BLOCK:
    case ID_POPUP_COPY_BLOCK:
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        DrawPanel->UnManageCursor( );
        /* Should not be executed, except bug */
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

    default:
        DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW, wxEmptyString );
        break;
    }

    INSTALL_DC( dc, DrawPanel );
    switch( id )
    {
    case ID_EXIT:
        Close( TRUE );
        break;

    case ID_NEW_PROJECT:
    case ID_LOAD_PROJECT:
        Files_io( event );
        break;

    case ID_GERBVIEW_GLOBAL_DELETE:
        Erase_Current_Layer( TRUE );
        break;

    case ID_GET_TOOLS:

//			InstallToolsFrame(this, wxPoint(-1,-1) );
        break;

    case ID_FIND_ITEMS:

//			InstallFindFrame(this, pos);
        break;


    case ID_NO_SELECT_BUTT:
        SetToolID( 0, 0, wxEmptyString );
        break;

    case ID_POPUP_CLOSE_CURRENT_TOOL:
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        break;

    case ID_GERBVIEW_DELETE_ITEM_BUTT:
        SetToolID( id, wxCURSOR_BULLSEYE, wxT( "Delete item" ) );
        break;

    case ID_TOOLBARH_GERBVIEW_SELECT_LAYER:
        setActiveLayer(m_SelLayerBox->GetChoice());
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
        Liste_D_Codes( );
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
            Delete_DCode_Items( &dc, gerber_layer->m_Selected_Tool,
                                ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer );
        break;

    default:
        wxMessageBox( wxT( "WinEDA_GerberFrame::Process_Special_Functions error" ) );
        break;
    }

    SetToolbars();
}


/* Called on a double click of left mouse button.
 */
void WinEDA_GerberFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
    EDA_BaseStruct* DrawStruct = GetScreen()->GetCurItem();

    switch( m_ID_current_state )
    {
    case 0:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = GerberGeneralLocateAndDisplay();
        }

        break;

    default:
        break;
    }
}
