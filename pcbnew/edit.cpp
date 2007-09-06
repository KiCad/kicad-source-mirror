/******************************************************/
/* edit.cpp: fonctions generales de l'edition du PCB */
/******************************************************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "id.h"
#include "protos.h"
#include "eda_dde.h"


#define CURRENT_ITEM (GetScreen()->GetCurItem())


static void Process_Move_Item( WinEDA_PcbFrame* frame,
                               EDA_BaseStruct* DrawStruct, wxDC* DC );

/********************************************************************/
void WinEDA_PcbFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
/********************************************************************/

/* Traite les commandes declench�e par le bouton gauche de la souris,
 *  quand un outil est deja selectionn�
 */
{
    EDA_BaseStruct* DrawStruct = CURRENT_ITEM;

    DrawPanel->m_IgnoreMouseEvents = TRUE;
    DrawPanel->CursorOff( DC );

    if( (m_ID_current_state == 0) || ( DrawStruct && DrawStruct->m_Flags ) )
    {
        DrawPanel->m_AutoPAN_Request = FALSE;
        if( DrawStruct && DrawStruct->m_Flags ) // Commande "POPUP" en cours
        {
            switch( DrawStruct->Type() )
            {
            case TYPETRACK:
            case TYPEVIA:
                if( DrawStruct->m_Flags & IS_DRAGGED )
                {
                    PlaceDraggedTrackSegment( (TRACK*) DrawStruct, DC );
                    goto out;
                }
                break;

            case TYPETEXTE:
                Place_Texte_Pcb( (TEXTE_PCB*) DrawStruct, DC );
                goto out;
                break;

            case TYPETEXTEMODULE:
                PlaceTexteModule( (TEXTE_MODULE*) DrawStruct, DC );
                goto out;
                break;

            case TYPEPAD:
                PlacePad( (D_PAD*) DrawStruct, DC );
                goto out;
                break;

            case TYPEMODULE:
                Place_Module( (MODULE*) DrawStruct, DC );
                goto out;
                break;

            case TYPEMIRE:
                Place_Mire( (MIREPCB*) DrawStruct, DC );
                goto out;
                break;

            case TYPEDRAWSEGMENT:
                if( m_ID_current_state == 0 )
                {
                    Place_DrawItem( (DRAWSEGMENT*) DrawStruct, DC );
                    goto out;
                }
                break;

            default:
                if( m_ID_current_state == 0 )
                {
                    DisplayError( this,
                                 wxT( "WinEDA_PcbFrame::OnLeftClick() err: m_Flags != 0" ) );
                    goto out;
                }
            }
        }
        else
        {
            DrawStruct = PcbGeneralLocateAndDisplay();
            if( DrawStruct )
                SendMessageToEESCHEMA( DrawStruct );
        }
    }

    switch( m_ID_current_state )
    {
    case ID_MAIN_MENUBAR:
    case 0:
        break;

    case ID_NO_SELECT_BUTT:
        break;

    case ID_PCB_MUWAVE_TOOL_SELF_CMD:
    case ID_PCB_MUWAVE_TOOL_GAP_CMD:
    case ID_PCB_MUWAVE_TOOL_STUB_CMD:
    case ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD:
    case ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD:
        MuWaveCommand( DC, MousePos );
        break;

    case ID_PCB_HIGHLIGHT_BUTT:
    {
        int netcode = Select_High_Light( DC );
        if( netcode < 0 )
            m_Pcb->Display_Infos( this );
        else
            Affiche_Infos_Equipot( netcode, this );
    }
        break;

    case ID_PCB_SHOW_1_RATSNEST_BUTT:
        DrawStruct = m_Pcb->FindPadOrModule( GetScreen()->RefPos(true), 
                            GetScreen()->m_Active_Layer );
        Show_1_Ratsnest( DrawStruct, DC );
        
        if( DrawStruct )
            SendMessageToEESCHEMA( DrawStruct );
        break;

    case ID_PCB_MIRE_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( Create_Mire( DC ) );
            DrawPanel->MouseToCursorSchema();
        }
        else if( DrawStruct->Type() == TYPEMIRE )
        {
            Place_Mire( (MIREPCB*) DrawStruct, DC );
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not TYPEMIRE" ) );
        break;

    case ID_PCB_CIRCLE_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_LINE_COMMENT_BUTT:
    {
        int shape = S_SEGMENT;
        if( m_ID_current_state == ID_PCB_CIRCLE_BUTT )
            shape = S_CIRCLE;
        if( m_ID_current_state == ID_PCB_ARC_BUTT )
            shape = S_ARC;

        if( GetScreen()->m_Active_Layer <= CMP_N )
        {
            DisplayError( this, _( "Graphic not autorized on Copper layers" ) );
            break;
        }
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = Begin_DrawSegment( NULL, shape, DC ); 
            GetScreen()->SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else if( DrawStruct
                && (DrawStruct->Type() == TYPEDRAWSEGMENT)
                && (DrawStruct->m_Flags & IS_NEW) )
        {
            DrawStruct = Begin_DrawSegment( (DRAWSEGMENT*) DrawStruct, shape, DC );
            GetScreen()->SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        break;
    }

    case ID_TRACK_BUTT:
        if( GetScreen()->m_Active_Layer > CMP_N )
        {
            DisplayError( this, _( "Tracks on Copper layers only " ) );
            break;
        }

        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = Begin_Route( NULL, DC );
            GetScreen()->SetCurItem( DrawStruct );
            if( DrawStruct )
                DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else if( DrawStruct &&

//					(DrawStruct->Type() == TYPETRACK) &&
                (DrawStruct->m_Flags & IS_NEW) )
        {
            TRACK* track = Begin_Route( (TRACK*) DrawStruct, DC );
            if( track )  // c'est a dire si OK
                GetScreen()->SetCurItem( DrawStruct = track );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        break;


    case ID_PCB_ZONES_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( DrawStruct = Begin_Zone() );
        }
        else if( DrawStruct
                && (DrawStruct->Type() == TYPEEDGEZONE)
                && (DrawStruct->m_Flags & IS_NEW) )
        {
            GetScreen()->SetCurItem( DrawStruct = Begin_Zone() );
        }
        else
            DisplayError( this, wxT( "Edit: zone internal error" ) );
        break;

    case ID_TEXT_COMMENT_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( Create_Texte_Pcb( DC ) );
            DrawPanel->MouseToCursorSchema();
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else if( DrawStruct->Type() == TYPETEXTE )
        {
            Place_Texte_Pcb( (TEXTE_PCB*) DrawStruct, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not TYPETEXTE" ) );
        break;

    case ID_COMPONENT_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawPanel->MouseToCursorSchema();
            DrawStruct = Load_Module_From_Library( wxEmptyString, DC );             
            GetScreen()->SetCurItem( DrawStruct );
            if( DrawStruct )
                StartMove_Module( (MODULE*) DrawStruct, DC );
        }
        else if( DrawStruct->Type() == TYPEMODULE )
        {
            Place_Module( (MODULE*) DrawStruct, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not TYPEMODULE" ) );
        break;

    case ID_PCB_COTATION_BUTT:
        if( GetScreen()->m_Active_Layer <= CMP_N )
        {
            DisplayError( this, _( "Cotation not autorized on Copper layers" ) );
            break;
        }
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = Begin_Cotation( NULL, DC );
            GetScreen()->SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else if( DrawStruct
                && (DrawStruct->Type() == TYPECOTATION)
                && (DrawStruct->m_Flags & IS_NEW) )
        {
            DrawStruct = Begin_Cotation( (COTATION*) DrawStruct, DC ); 
            GetScreen()->SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not COTATION" ) );
        break;

    case ID_PCB_DELETE_ITEM_BUTT:
        if( !DrawStruct || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();
            if( DrawStruct && (DrawStruct->m_Flags == 0) )
            {
                RemoveStruct( DrawStruct, DC );
                GetScreen()->SetCurItem( DrawStruct = NULL );
            }
        }
        break;

    case ID_PCB_PLACE_OFFSET_COORD_BUTT:
        DrawPanel->m_Draw_Auxiliary_Axis( DC, GR_XOR );
        m_Auxiliary_Axis_Position = GetScreen()->m_Curseur;
        DrawPanel->m_Draw_Auxiliary_Axis( DC, GR_COPY );
        GetScreen()->SetModify();
        break;

    default:
        DrawPanel->SetCursor( wxCURSOR_ARROW );
        DisplayError( this, wxT( "WinEDA_PcbFrame::OnLeftClick() id error" ) );
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;
    }

out:
    DrawPanel->m_IgnoreMouseEvents = FALSE;
    DrawPanel->CursorOn( DC );
}


// see wxstruct.h
void WinEDA_PcbFrame::SendMessageToEESCHEMA( EDA_BaseStruct* objectToSync )
{
    char    cmd[1024];
    MODULE* module = NULL;
    
    if( objectToSync->Type() == TYPEMODULE )
        module = (MODULE*) objectToSync;
    else if( objectToSync->Type() == TYPEPAD )
        module = (MODULE*) objectToSync->m_Parent;
    else if( objectToSync->Type() == TYPETEXTEMODULE )
        module = (MODULE*) objectToSync->m_Parent;

    // ask only for the reference for now, maybe pins later.            
    if( module )
    {
        sprintf( cmd, "$PART: %s", CONV_TO_UTF8(module->m_Reference->m_Text) );
        SendCommand( MSG_TO_SCH, cmd );
    }
}



/*********************************************************************/
void WinEDA_PcbFrame::Process_Special_Functions( wxCommandEvent& event )
/*********************************************************************/

/* Traite les selections d'outils et les commandes appelees du menu POPUP
 */
{
    int        id = event.GetId();
    wxPoint    pos;
    int        itmp;
    wxClientDC dc( DrawPanel );

    DrawPanel->CursorOff( &dc );
    DrawPanel->PrepareGraphicContext( &dc );
    
#if defined(DEBUG)
    std::cout << "GetString=" << event.GetString().mb_str() << '\n';
#endif

    wxGetMousePosition( &pos.x, &pos.y );

    pos.y += 20;

    switch( id )   // Arret eventuel de la commande de d�placement en cours
    {
    case wxID_CUT:
    case wxID_COPY:
    case ID_AUX_TOOLBAR_PCB_TRACK_WIDTH:
    case ID_AUX_TOOLBAR_PCB_VIA_SIZE:
    case ID_ON_GRID_SELECT:
    case ID_ON_ZOOM_SELECT:
    case ID_PCB_USER_GRID_SETUP:
    case ID_TOOLBARH_PCB_SELECT_LAYER:
    case ID_POPUP_PCB_ROTATE_TEXTEPCB:
    case ID_POPUP_PCB_EDIT_TEXTEPCB:
    case ID_POPUP_PCB_EDIT_MIRE:
    case ID_POPUP_PCB_ROTATE_TEXTMODULE:
    case ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE:
    case ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE:
    case ID_POPUP_PCB_CHANGE_SIDE_MODULE:
    case ID_POPUP_PCB_EDIT_MODULE:
    case ID_POPUP_PCB_EDIT_TEXTMODULE:
    case ID_POPUP_PCB_STOP_CURRENT_DRAWING:
    case ID_POPUP_PCB_END_TRACK:
    case ID_POPUP_PCB_PLACE_VIA:
    case ID_POPUP_PCB_IMPORT_PAD_SETTINGS:
    case ID_POPUP_PCB_EXPORT_PAD_SETTINGS:
    case ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS:
    case ID_POPUP_PCB_STOP_CURRENT_EDGE_ZONE:
    case ID_POPUP_PCB_DELETE_EDGE_ZONE:
    case ID_POPUP_PCB_DELETE_ZONE_LIMIT:
    case ID_POPUP_PCB_EDIT_ZONE:
    case ID_POPUP_PCB_DELETE_ZONE:
    case ID_POPUP_PCB_DELETE_TRACKSEG:
    case ID_POPUP_PCB_DELETE_TRACK:
    case ID_POPUP_PCB_DELETE_TRACKNET:
    case ID_POPUP_PCB_FILL_ZONE:
    case ID_POPUP_PCB_SELECT_NET_ZONE:
    case ID_POPUP_PCB_SELECT_LAYER:
    case ID_POPUP_PCB_SELECT_CU_LAYER:
    case ID_POPUP_PCB_SELECT_LAYER_PAIR:
    case ID_POPUP_PCB_SELECT_NO_CU_LAYER:
    case ID_POPUP_PCB_SELECT_WIDTH:
    case ID_POPUP_PCB_SELECT_WIDTH1:
    case ID_POPUP_PCB_SELECT_WIDTH2:
    case ID_POPUP_PCB_SELECT_WIDTH3:
    case ID_POPUP_PCB_SELECT_WIDTH4:
    case ID_POPUP_PCB_SELECT_WIDTH5:
    case ID_POPUP_PCB_SELECT_WIDTH6:
    case ID_POPUP_PCB_SELECT_WIDTH7:
    case ID_POPUP_PCB_SELECT_WIDTH8:
    case ID_POPUP_PCB_SELECT_VIASIZE:
    case ID_POPUP_PCB_SELECT_VIASIZE1:
    case ID_POPUP_PCB_SELECT_VIASIZE2:
    case ID_POPUP_PCB_SELECT_VIASIZE3:
    case ID_POPUP_PCB_SELECT_VIASIZE4:
    case ID_POPUP_PCB_SELECT_VIASIZE5:
    case ID_POPUP_PCB_SELECT_VIASIZE6:
    case ID_POPUP_PCB_SELECT_VIASIZE7:
    case ID_POPUP_PCB_SELECT_VIASIZE8:
    case ID_POPUP_PCB_MOVE_TRACK_NODE:
    case ID_POPUP_PCB_DRAG_TRACK_SEGMENT_KEEP_SLOPE:
    case ID_POPUP_PCB_DRAG_TRACK_SEGMENT:
    case ID_POPUP_PCB_MOVE_TRACK_SEGMENT:
    case ID_POPUP_PCB_PLACE_MOVED_TRACK_NODE:
    case ID_POPUP_PCB_BREAK_TRACK:
    case ID_POPUP_PCB_EDIT_NET:
    case ID_POPUP_PCB_EDIT_TRACK:
    case ID_POPUP_PCB_EDIT_TRACKSEG:
    case ID_POPUP_PCB_LOCK_ON_TRACKSEG:
    case ID_POPUP_PCB_LOCK_OFF_TRACKSEG:
    case ID_POPUP_PCB_LOCK_ON_TRACK:
    case ID_POPUP_PCB_LOCK_OFF_TRACK:
    case ID_POPUP_PCB_LOCK_ON_NET:
    case ID_POPUP_PCB_LOCK_OFF_NET:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_INVERT_BLOCK:
    case ID_POPUP_ROTATE_BLOCK:
    case ID_POPUP_COPY_BLOCK:
    case ID_POPUP_PCB_VIA_EDITING:
    case ID_POPUP_PCB_VIA_HOLE_TO_DEFAULT:
    case ID_POPUP_PCB_VIA_HOLE_TO_VALUE:
    case ID_POPUP_PCB_VIA_HOLE_ENTER_VALUE:
    case ID_POPUP_PCB_VIA_HOLE_EXPORT:
    case ID_POPUP_PCB_VIA_HOLE_RESET_TO_DEFAULT:
    case ID_POPUP_PCB_VIA_HOLE_EXPORT_TO_OTHERS:
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( DrawPanel->ManageCurseur
            && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
        /* ne devrait pas etre execute, sauf bug */
        if( m_CurrentScreen->BlockLocate.m_Command != BLOCK_IDLE )
        {
            m_CurrentScreen->BlockLocate.m_Command = BLOCK_IDLE;
            m_CurrentScreen->BlockLocate.m_State   = STATE_NO_BLOCK;
            m_CurrentScreen->BlockLocate.m_BlockDrawStruct = NULL;
        }
        if( m_ID_current_state == 0 )
            SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        else
            SetCursor( DrawPanel->m_PanelCursor = DrawPanel->m_PanelDefaultCursor );
        break;

    default:        // Arret de la commande de d�placement en cours
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

    case ID_OPEN_MODULE_EDITOR:
        if( m_Parent->m_ModuleEditFrame == NULL )
        {
            m_Parent->m_ModuleEditFrame = 
                new WinEDA_ModuleEditFrame( this,
                                             m_Parent, _( "Module Editor" ),
                                             wxPoint( -1,
                                                      -1 ),
                                             wxSize( 600, 400 ) );
            m_Parent->m_ModuleEditFrame->Show( TRUE );
            m_Parent->m_ModuleEditFrame->Zoom_Automatique( TRUE );
        }
        else
            m_Parent->m_ModuleEditFrame->Iconize( FALSE );
        break;

    case ID_NEW_PROJECT:
    case ID_LOAD_PROJECT:
        Files_io( event );
        break;

    case ID_PCB_GLOBAL_DELETE:
        InstallPcbGlobalDeleteFrame( pos );
        break;

    case ID_POPUP_PLACE_BLOCK:
        GetScreen()->BlockLocate.m_Command = BLOCK_MOVE;
        DrawPanel->m_AutoPAN_Request = FALSE;
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_COPY_BLOCK:
        GetScreen()->BlockLocate.m_Command = BLOCK_COPY;
        m_CurrentScreen->BlockLocate.SetMessageBlock( this );
        DrawPanel->m_AutoPAN_Request = FALSE;
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_ZOOM_BLOCK:
        GetScreen()->BlockLocate.m_Command = BLOCK_ZOOM;
        m_CurrentScreen->BlockLocate.SetMessageBlock( this );
        m_CurrentScreen->BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        GetScreen()->BlockLocate.m_Command = BLOCK_DELETE;
        m_CurrentScreen->BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_ROTATE_BLOCK:
        GetScreen()->BlockLocate.m_Command = BLOCK_ROTATE;
        m_CurrentScreen->BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_INVERT_BLOCK:
        GetScreen()->BlockLocate.m_Command = BLOCK_INVERT;
        m_CurrentScreen->BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_UNDO_BUTT:
        UnDeleteItem( &dc );
        break;

    case ID_DRC_CONTROL:
        Install_Test_DRC_Frame( &dc );
        break;

    case ID_GET_NETLIST:
        InstallNetlistFrame( &dc, wxPoint( -1, -1 ) );
        break;

    case ID_GET_TOOLS:
        // InstalloolsFrame(this, wxPoint(-1,-1) );
        break;

    case ID_FIND_ITEMS:
        InstallFindFrame( pos, &dc );
        break;

    case ID_TRACK_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Tracks" ) );
        DisplayTrackSettings();
        if( (m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 )
        {
            Compile_Ratsnest( &dc, TRUE );
        }
        break;

    case ID_PCB_ZONES_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Zones" ) );
        if( !DisplayOpt.DisplayZones )
            DisplayInfo( this, _( "Warning: Display Zone is OFF!!!" ) );
        DelLimitesZone( &dc, TRUE );
        if( !g_HightLigt_Status && (g_HightLigth_NetCode > 0 ) )
            Hight_Light( &dc );
        break;

    case ID_PCB_MIRE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Mire" ) );
        break;

    case ID_PCB_PLACE_OFFSET_COORD_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Adjust Zero" ) );
        break;

    case ID_LINE_COMMENT_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_PCB_CIRCLE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Graphic" ) );
        break;

    case ID_TEXT_COMMENT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Text" ) );
        break;

    case ID_COMPONENT_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Add Modules" ) );
        break;

    case ID_PCB_COTATION_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Cotation" ) );
        break;

    case ID_NO_SELECT_BUTT:
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_PCB_HIGHLIGHT_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Net Highlight" ) );
        break;

    case ID_PCB_SHOW_1_RATSNEST_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Local Ratsnest" ) );
        if( (m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 )
            Compile_Ratsnest( &dc, TRUE );
        break;

    case ID_POPUP_CLOSE_CURRENT_TOOL:
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        break;

    case ID_POPUP_END_LINE:
        DrawPanel->MouseToCursorSchema();
        //	EndSegment(&dc);
        break;

    case ID_POPUP_PCB_EDIT_TRACK:
        if( CURRENT_ITEM == NULL )
            break;
        Edit_Track_Width( &dc, (TRACK*) CURRENT_ITEM );
        DrawPanel->MouseToCursorSchema();
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_EDIT_TRACKSEG:
        if( CURRENT_ITEM == NULL )
            break;
        Edit_TrackSegm_Width( &dc, (TRACK*) CURRENT_ITEM );
        DrawPanel->MouseToCursorSchema();
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_EDIT_NET:
        if( CURRENT_ITEM == NULL )
            break;
        Edit_Net_Width( &dc, ( (TRACK*) CURRENT_ITEM )->m_NetCode );
        DrawPanel->MouseToCursorSchema();
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_EDIT_ALL_VIAS_AND_TRACK_SIZE:
    case ID_POPUP_PCB_EDIT_ALL_VIAS_SIZE:
    case ID_POPUP_PCB_EDIT_ALL_TRACK_SIZE:
        if( CURRENT_ITEM == NULL )
            break;
        {
            bool resize_vias = TRUE, resize_track = TRUE;
            if( id == ID_POPUP_PCB_EDIT_ALL_VIAS_SIZE )
                resize_track = FALSE;
            if( id == ID_POPUP_PCB_EDIT_ALL_TRACK_SIZE )
                resize_vias = FALSE;
            if( Resize_Pistes_Vias( &dc, resize_track, resize_vias ) )
                GetScreen()->SetModify();
        }
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_END_TRACK:
        DrawPanel->MouseToCursorSchema();
        End_Route( (TRACK*) CURRENT_ITEM, &dc );
        break;

    case ID_POPUP_PCB_PLACE_MOVED_TRACK_NODE:
        DrawPanel->MouseToCursorSchema();
        if( CURRENT_ITEM->m_Flags & IS_DRAGGED )
        {
            PlaceDraggedTrackSegment( (TRACK*) CURRENT_ITEM, &dc );
        }
        break;

    case ID_POPUP_PCB_PLACE_VIA:
        DrawPanel->MouseToCursorSchema();
        if( CURRENT_ITEM->m_Flags & IS_DRAGGED )
        {
            PlaceDraggedTrackSegment( (TRACK*) CURRENT_ITEM, &dc );
        }
        else
        {
            Other_Layer_Route( (TRACK*) CURRENT_ITEM, &dc );
            if( DisplayOpt.ContrastModeDisplay )
                GetScreen()->SetRefreshReq();
        }
        break;

    case ID_POPUP_PCB_DELETE_TRACKSEG:
        if( CURRENT_ITEM == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        GetScreen()->SetCurItem( Delete_Segment( &dc, (TRACK*) CURRENT_ITEM ) );
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_DELETE_TRACK:
        if( CURRENT_ITEM == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        Delete_Track( &dc, (TRACK*) CURRENT_ITEM );
        GetScreen()->SetCurItem( NULL );
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_DELETE_TRACKNET:
        DrawPanel->MouseToCursorSchema();
        Delete_net( &dc, (TRACK*) CURRENT_ITEM );
        GetScreen()->SetCurItem( NULL );
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_LOCK_ON_TRACKSEG:
        Attribut_Segment( (TRACK*) CURRENT_ITEM, &dc, TRUE );
        break;

    case ID_POPUP_PCB_LOCK_OFF_TRACKSEG:
        Attribut_Segment( (TRACK*) CURRENT_ITEM, &dc, FALSE );
        break;

    case ID_POPUP_PCB_LOCK_ON_TRACK:
        Attribut_Track( (TRACK*) CURRENT_ITEM, &dc, TRUE );
        break;

    case ID_POPUP_PCB_LOCK_OFF_TRACK:
        Attribut_Track( (TRACK*) CURRENT_ITEM, &dc, FALSE );
        break;

    case ID_POPUP_PCB_LOCK_ON_NET:
        Attribut_net( &dc, ( (TRACK*) CURRENT_ITEM )->m_NetCode, TRUE );
        break;

    case ID_POPUP_PCB_LOCK_OFF_NET:
        Attribut_net( &dc, ( (TRACK*) CURRENT_ITEM )->m_NetCode, FALSE );
        break;

    case ID_POPUP_PCB_SETFLAGS_TRACK_MNU:
        break;

    case ID_POPUP_PCB_DELETE_ZONE:
        DrawPanel->MouseToCursorSchema();
        if( CURRENT_ITEM == NULL )
            break;
        Delete_Zone( &dc, (SEGZONE*) CURRENT_ITEM );
        GetScreen()->SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_EDIT_ZONE:
        DrawPanel->MouseToCursorSchema();
        if( CURRENT_ITEM == NULL )
            break;
        Edit_Zone_Width( &dc, (SEGZONE*) CURRENT_ITEM );
        break;

    case ID_POPUP_PCB_DELETE_ZONE_LIMIT:
        DrawPanel->MouseToCursorSchema();
        DelLimitesZone( &dc, TRUE );
        break;

    case ID_PCB_DELETE_ITEM_BUTT:
        SetToolID( id, wxCURSOR_BULLSEYE, _( "Delete item" ) );
        break;

    case ID_POPUP_PCB_MOVE_TEXTEPCB_REQUEST:
        Process_Move_Item( this, CURRENT_ITEM, &dc );
        DrawPanel->m_AutoPAN_Request = TRUE;
        break;

    case ID_POPUP_PCB_DRAG_MODULE_REQUEST:
        g_Drag_Pistes_On = TRUE;

    case ID_POPUP_PCB_MOVE_MODULE_REQUEST:

        // If the current Item is a pad, text module ...: Get the parent
        if( CURRENT_ITEM->Type() != TYPEMODULE )
            GetScreen()->SetCurItem( CURRENT_ITEM->m_Parent );
        if( !CURRENT_ITEM || CURRENT_ITEM->Type() != TYPEMODULE )
        {
            g_Drag_Pistes_On = FALSE;
            break;
        }
        DrawPanel->MouseToCursorSchema();
        StartMove_Module( (MODULE*) CURRENT_ITEM, &dc );
        break;

    case ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST:      /* get module by name and move it */
        GetScreen()->SetCurItem( GetModuleByName() );
        if( CURRENT_ITEM )
        {
            DrawPanel->MouseToCursorSchema();
            StartMove_Module( (MODULE*) CURRENT_ITEM, &dc );
        }
        break;

    case ID_POPUP_PCB_DELETE_MODULE:
        DrawPanel->MouseToCursorSchema();

        // If the current Item is a pad, text module ...: Get the parent
        if( CURRENT_ITEM->Type() != TYPEMODULE )
            GetScreen()->SetCurItem( CURRENT_ITEM->m_Parent );
        
        if( !CURRENT_ITEM || CURRENT_ITEM->Type() != TYPEMODULE )
            break;
        if( Delete_Module( (MODULE*) CURRENT_ITEM, &dc ) )
        {
            GetScreen()->SetCurItem( NULL );
        }
        break;

    case ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE:
        DrawPanel->MouseToCursorSchema();

        // If the current Item is a pad, text module ...: Get the parent
        if( CURRENT_ITEM->Type() != TYPEMODULE )
            GetScreen()->SetCurItem( CURRENT_ITEM->m_Parent );
        
        if( !CURRENT_ITEM || CURRENT_ITEM->Type() != TYPEMODULE )
            break;
        Rotate_Module( &dc, (MODULE*) CURRENT_ITEM, -900, TRUE );
        break;

    case ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE:
        DrawPanel->MouseToCursorSchema();

        // If the current Item is a pad, text module ...: Get the parent
        if( CURRENT_ITEM->Type() != TYPEMODULE )
            GetScreen()->SetCurItem( CURRENT_ITEM->m_Parent );

        if( !CURRENT_ITEM || CURRENT_ITEM->Type() != TYPEMODULE )
            break;
        Rotate_Module( &dc, (MODULE*) CURRENT_ITEM, 900, TRUE );
        break;

    case ID_POPUP_PCB_CHANGE_SIDE_MODULE:
        DrawPanel->MouseToCursorSchema();

        // If the current Item is a pad, text module ...: Get the parent
        if( CURRENT_ITEM->Type() != TYPEMODULE )
            GetScreen()->SetCurItem( CURRENT_ITEM->m_Parent );
        if( !CURRENT_ITEM || CURRENT_ITEM->Type() != TYPEMODULE )
            break;
        Change_Side_Module( (MODULE*) CURRENT_ITEM, &dc );
        break;

    case ID_POPUP_PCB_EDIT_MODULE:
        // If the current Item is a pad, text module ...: Get the parent
        if( CURRENT_ITEM->Type() != TYPEMODULE )
            GetScreen()->SetCurItem( CURRENT_ITEM->m_Parent );
        if( !CURRENT_ITEM || CURRENT_ITEM->Type() != TYPEMODULE )
            break;
        InstallModuleOptionsFrame( (MODULE*) CURRENT_ITEM, &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DRAG_PAD_REQUEST:
        g_Drag_Pistes_On = TRUE;

    case ID_POPUP_PCB_MOVE_PAD_REQUEST:
        DrawPanel->MouseToCursorSchema();
        StartMovePad( (D_PAD*) CURRENT_ITEM, &dc );
        break;

    case ID_POPUP_PCB_EDIT_PAD:
        InstallPadOptionsFrame( (D_PAD*) CURRENT_ITEM, &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_IMPORT_PAD_SETTINGS:
        DrawPanel->MouseToCursorSchema();
        Import_Pad_Settings( (D_PAD*) CURRENT_ITEM, &dc );
        break;

    case ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS:
        DrawPanel->MouseToCursorSchema();
        Global_Import_Pad_Settings( (D_PAD*) CURRENT_ITEM, &dc );
        break;

    case ID_POPUP_PCB_EXPORT_PAD_SETTINGS:
        DrawPanel->MouseToCursorSchema();
        Export_Pad_Settings( (D_PAD*) CURRENT_ITEM );
        break;

    case ID_POPUP_PCB_DELETE_PAD:
        DeletePad( (D_PAD*) CURRENT_ITEM, &dc );
        GetScreen()->SetCurItem( NULL );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_EDIT_TEXTMODULE:
        InstallTextModOptionsFrame( (TEXTE_MODULE*) CURRENT_ITEM, &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST:
        DrawPanel->MouseToCursorSchema();
        StartMoveTexteModule( (TEXTE_MODULE*) CURRENT_ITEM, &dc );
        break;

    case ID_POPUP_PCB_ROTATE_TEXTMODULE:
        RotateTextModule( (TEXTE_MODULE*) CURRENT_ITEM,
                         &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DELETE_TEXTMODULE:
        DeleteTextModule( (TEXTE_MODULE*) CURRENT_ITEM,  &dc );
        GetScreen()->SetCurItem( NULL );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_SELECT_LAYER:
        itmp = SelectLayer( GetScreen()->m_Active_Layer, -1, -1 );
        if( itmp >= 0 )
            GetScreen()->m_Active_Layer = itmp;
        DrawPanel->MouseToCursorSchema();
        break;

    case  ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR:
        SelectLayerPair();
        break;

    case ID_POPUP_PCB_SELECT_NO_CU_LAYER:
        itmp = SelectLayer( GetScreen()->m_Active_Layer, CMP_N + 1, -1 );
        if( itmp >= 0 )
            GetScreen()->m_Active_Layer = itmp;
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_SELECT_CU_LAYER:
        itmp = SelectLayer( GetScreen()->m_Active_Layer, -1, CMP_N );
        if( itmp >= 0 )
            GetScreen()->m_Active_Layer = itmp;
        break;

    case ID_POPUP_PCB_SELECT_LAYER_PAIR:
        SelectLayerPair();
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_TOOLBARH_PCB_SELECT_LAYER:
        itmp = m_SelLayerBox->GetChoice();
        GetScreen()->m_Active_Layer = (int) ( (size_t) m_SelLayerBox->GetClientData( itmp ) );
        if( DisplayOpt.ContrastModeDisplay )
            DrawPanel->Refresh( TRUE );
        break;

    case ID_POPUP_PCB_EDIT_TEXTEPCB:
        InstallTextPCBOptionsFrame( (TEXTE_PCB*) CURRENT_ITEM,
                                   &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_ROTATE_TEXTEPCB:
        Rotate_Texte_Pcb( (TEXTE_PCB*) CURRENT_ITEM, &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DELETE_TEXTEPCB:
        Delete_Texte_Pcb( (TEXTE_PCB*) CURRENT_ITEM, &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_MOVE_MIRE_REQUEST:
        StartMove_Mire( (MIREPCB*) CURRENT_ITEM, &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_EDIT_MIRE:
        InstallMireOptionsFrame( (MIREPCB*) CURRENT_ITEM, &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DELETE_MIRE:
        DrawPanel->MouseToCursorSchema();
        Delete_Mire( (MIREPCB*) CURRENT_ITEM, &dc );
        GetScreen()->SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_DELETE_COTATION:
        DrawPanel->MouseToCursorSchema();
        Delete_Cotation( (COTATION*) CURRENT_ITEM, &dc );
        GetScreen()->SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_EDIT_COTATION:
        Install_Edit_Cotation( (COTATION*) CURRENT_ITEM, &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DELETE_DRAWING:
        Delete_Segment_Edge( (DRAWSEGMENT*) CURRENT_ITEM, &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DELETE_DRAWING_LAYER:
        Delete_Drawings_All_Layer( (DRAWSEGMENT*) CURRENT_ITEM, &dc );
        GetScreen()->SetCurItem( NULL );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_EDIT_DRAWING:
        Drawing_SetNewWidth( (DRAWSEGMENT*) CURRENT_ITEM, &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_MOVE_DRAWING_REQUEST:
        DrawPanel->MouseToCursorSchema();
        Start_Move_DrawItem( (DRAWSEGMENT*) CURRENT_ITEM, &dc );
        break;

    case ID_POPUP_PCB_STOP_CURRENT_DRAWING:
        DrawPanel->MouseToCursorSchema();
        if( CURRENT_ITEM && (CURRENT_ITEM->m_Flags & IS_NEW) )
        {
            End_Edge( (DRAWSEGMENT*) CURRENT_ITEM, &dc );
            GetScreen()->SetCurItem( NULL );
        }
        break;

    case ID_POPUP_PCB_STOP_CURRENT_EDGE_ZONE:
        DrawPanel->MouseToCursorSchema();
        if( CURRENT_ITEM && (CURRENT_ITEM->m_Flags & IS_NEW) )
        {
            End_Zone( &dc );
            GetScreen()->SetCurItem( NULL );
        }
        break;

    case ID_POPUP_PCB_DELETE_EDGE_ZONE:
        DrawPanel->MouseToCursorSchema();
        if( CURRENT_ITEM && (CURRENT_ITEM->m_Flags & IS_NEW) )
        {
            GetScreen()->SetCurItem( Del_SegmEdgeZone( &dc,
                                                           (EDGE_ZONE*) CURRENT_ITEM ) );
        }
        break;

    case ID_POPUP_PCB_FILL_ZONE:
        DrawPanel->MouseToCursorSchema();
        Fill_Zone( &dc );
        break;

    case ID_POPUP_PCB_SELECT_NET_ZONE:
        DrawPanel->MouseToCursorSchema();
        CaptureNetName( &dc );
        break;

    case ID_POPUP_PCB_SELECT_WIDTH:
        break;

    case ID_AUX_TOOLBAR_PCB_TRACK_WIDTH:
    {
        int ii = m_SelTrackWidthBox->GetChoice();
        g_DesignSettings.m_CurrentTrackWidth = g_DesignSettings.m_TrackWidhtHistory[ii];
        DisplayTrackSettings();
        m_SelTrackWidthBox_Changed = FALSE;
        m_SelViaSizeBox_Changed    = FALSE;
    }
        break;

    case ID_POPUP_PCB_SELECT_WIDTH1:
    case ID_POPUP_PCB_SELECT_WIDTH2:
    case ID_POPUP_PCB_SELECT_WIDTH3:
    case ID_POPUP_PCB_SELECT_WIDTH4:
    case ID_POPUP_PCB_SELECT_WIDTH5:
    case ID_POPUP_PCB_SELECT_WIDTH6:
    case ID_POPUP_PCB_SELECT_WIDTH7:
    case ID_POPUP_PCB_SELECT_WIDTH8:
        DrawPanel->MouseToCursorSchema();
        {
            int ii = id - ID_POPUP_PCB_SELECT_WIDTH1;
            g_DesignSettings.m_CurrentTrackWidth = g_DesignSettings.m_TrackWidhtHistory[ii];
            DisplayTrackSettings();
        }
        break;

    case ID_POPUP_PCB_SELECT_VIASIZE:
        break;

    case ID_AUX_TOOLBAR_PCB_VIA_SIZE:
    {
        int ii = m_SelViaSizeBox->GetChoice();
        g_DesignSettings.m_CurrentViaSize = g_DesignSettings.m_ViaSizeHistory[ii];
        DisplayTrackSettings();
        m_SelTrackWidthBox_Changed = FALSE;
        m_SelViaSizeBox_Changed    = FALSE;
    }
        break;

    case ID_POPUP_PCB_SELECT_VIASIZE1:
    case ID_POPUP_PCB_SELECT_VIASIZE2:
    case ID_POPUP_PCB_SELECT_VIASIZE3:
    case ID_POPUP_PCB_SELECT_VIASIZE4:
    case ID_POPUP_PCB_SELECT_VIASIZE5:
    case ID_POPUP_PCB_SELECT_VIASIZE6:
    case ID_POPUP_PCB_SELECT_VIASIZE7:
    case ID_POPUP_PCB_SELECT_VIASIZE8:
    case ID_POPUP_PCB_VIA_EDITING:
    case ID_POPUP_PCB_VIA_HOLE_TO_DEFAULT:
    case ID_POPUP_PCB_VIA_HOLE_TO_VALUE:
    case ID_POPUP_PCB_VIA_HOLE_ENTER_VALUE:
    case ID_POPUP_PCB_VIA_HOLE_EXPORT:
    case ID_POPUP_PCB_VIA_HOLE_RESET_TO_DEFAULT:
    case ID_POPUP_PCB_VIA_HOLE_EXPORT_TO_OTHERS:
        Via_Edit_Control( &dc, id, (SEGVIA*) GetScreen()->GetCurItem() );
        break;

    case ID_POPUP_PCB_MOVE_TRACK_SEGMENT:
        DrawPanel->MouseToCursorSchema();
        Start_MoveOneNodeOrSegment( (TRACK*) GetScreen()->GetCurItem(),
                                   &dc, id );
        break;

    case ID_POPUP_PCB_DRAG_TRACK_SEGMENT:
    case ID_POPUP_PCB_MOVE_TRACK_NODE:
        DrawPanel->MouseToCursorSchema();
        Start_MoveOneNodeOrSegment( (TRACK*) GetScreen()->GetCurItem(),
                                   &dc, id );
        break;

    case ID_POPUP_PCB_DRAG_TRACK_SEGMENT_KEEP_SLOPE:
        DrawPanel->MouseToCursorSchema();
        Start_DragTrackSegmentAndKeepSlope( (TRACK*) GetScreen()->GetCurItem(),
                                           &dc );
        break;

    case ID_POPUP_PCB_BREAK_TRACK:
        DrawPanel->MouseToCursorSchema();
        {
            TRACK*  track = (TRACK*) GetScreen()->GetCurItem();
            wxPoint pos   = GetScreen()->m_Curseur;
            track->Draw( DrawPanel, &dc, GR_XOR );
            TRACK*  newtrack = CreateLockPoint( &pos.x, &pos.y, track, NULL );
            track->Draw( DrawPanel, &dc, GR_XOR );
            newtrack->Draw( DrawPanel, &dc, GR_XOR );
        }
        break;

    case ID_MENU_PCB_CLEAN:
        Clean_Pcb( &dc );
        break;

    case ID_MENU_PCB_SWAP_LAYERS:
        Swap_Layers( event );
        break;

    case ID_POPUP_PCB_AUTOROUTE_GET_AUTOROUTER:
        GlobalRoute( &dc );
        break;

    case ID_POPUP_PCB_AUTOROUTE_GET_AUTOROUTER_DATA:
        ReadAutoroutedTracks( &dc );
        break;

    case ID_PCB_USER_GRID_SETUP:
        InstallGridFrame( pos );
        break;

    case ID_POPUP_PCB_DISPLAY_FOOTPRINT_DOC:
    {
        wxString msg = FindKicadHelpPath();
        msg += EDA_Appl->m_EDA_CommonConfig->Read( wxT( "module_doc_file" ),
                                                  wxT( "pcbnew/footprints.pdf" ) );
        GetAssociatedDocument( this, wxEmptyString, msg );
    }
        break;

    case ID_MENU_ARCHIVE_NEW_MODULES:
        Archive_Modules( wxEmptyString, TRUE );
        break;

    case ID_MENU_ARCHIVE_ALL_MODULES:
        Archive_Modules( wxEmptyString, FALSE );
        break;

    default:
        DisplayError( this, wxT( "WinEDA_PcbFrame::Process_Special_Functions() id error" ) );
        break;
    }

    SetToolbars();
    DrawPanel->CursorOn( &dc );
    DrawPanel->m_IgnoreMouseEvents = FALSE;
}


/****************************************************************/
static void Process_Move_Item( WinEDA_PcbFrame* frame,
                               EDA_BaseStruct* DrawStruct, wxDC* DC )
/****************************************************************/
{
    if( DrawStruct == NULL )
        return;

    frame->DrawPanel->MouseToCursorSchema();

    switch( DrawStruct->Type() )
    {
    case  TYPETEXTE:
        frame->StartMoveTextePcb( (TEXTE_PCB*) DrawStruct, DC );
        break;

    default:
        wxString msg;
        msg.Printf(
            wxT( "WinEDA_PcbFrame::Move_Item Error: Bad DrawType %d" ),
            DrawStruct->Type() );
        DisplayError( frame, msg );
        break;
    }
}


/********************************************************************************/
void WinEDA_PcbFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
/********************************************************************************/

/* Appel� sur un double click:
 *  pour un �l�ment editable (textes, composant):
 *      appel de l'editeur correspondant.
 *  pour une connexion en cours:
 *      termine la connexion
 */
{
    EDA_BaseStruct* DrawStruct = CURRENT_ITEM;
    wxPoint         pos = GetPosition();
    wxClientDC      dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );

    switch( m_ID_current_state )
    {
    case 0:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();
        }

        if( (DrawStruct == NULL) || (DrawStruct->m_Flags != 0) )
            break;

        // Element localis�
        GetScreen()->SetCurItem( DrawStruct );

        switch( DrawStruct->Type() )
        {
        case TYPETRACK:
        case TYPEVIA:
            if( DrawStruct->m_Flags & IS_NEW )
            {
                End_Route( (TRACK*) DrawStruct, DC );
                DrawPanel->m_AutoPAN_Request = FALSE;
            }
            else if( DrawStruct->m_Flags == 0 )
            {
                Edit_TrackSegm_Width( DC,
                                      (TRACK*) DrawStruct );
            }
            break;

        case TYPETEXTE:
            InstallTextPCBOptionsFrame( (TEXTE_PCB*) DrawStruct,
                                       DC, ( (TEXTE_PCB*) DrawStruct )->m_Pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPEPAD:
            InstallPadOptionsFrame(
                (D_PAD*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPEMODULE:
            InstallModuleOptionsFrame( (MODULE*) DrawStruct,
                                      &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPEMIRE:
            InstallMireOptionsFrame( (MIREPCB*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPETEXTEMODULE:
            InstallTextModOptionsFrame( (TEXTE_MODULE*) DrawStruct,
                                       &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPEDRAWSEGMENT:
            break;

        default:
            break;
        }

        break;      // end case 0

    case ID_TRACK_BUTT:
        if( DrawStruct && (DrawStruct->m_Flags & IS_NEW) )
        {
            End_Route( (TRACK*) DrawStruct, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        break;

    case ID_PCB_ZONES_BUTT:
        End_Zone( DC );
        DrawPanel->m_AutoPAN_Request = FALSE;
        GetScreen()->SetCurItem( NULL );
        break;

    case ID_LINE_COMMENT_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_PCB_CIRCLE_BUTT:
        if( DrawStruct == NULL )
            break;
        if( DrawStruct->Type() != TYPEDRAWSEGMENT )
        {
            DisplayError( this, wxT( "DrawStruct Type error" ) );
            DrawPanel->m_AutoPAN_Request = FALSE;
            break;
        }
        if( (DrawStruct->m_Flags & IS_NEW) )
        {
            End_Edge( (DRAWSEGMENT*) DrawStruct, &dc );
            DrawPanel->m_AutoPAN_Request = FALSE;
            GetScreen()->SetCurItem( NULL );
        }
        break;
    }
}


/***************************************************************/
void WinEDA_PcbFrame::RemoveStruct( EDA_BaseStruct* Item, wxDC* DC )
/***************************************************************/
{
    if( Item == NULL )
        return;

    switch( Item->Type() )
    {
    case TYPEMODULE:
        Delete_Module( (MODULE*) Item, DC );
        break;

    case TYPECOTATION:
        Delete_Cotation( (COTATION*) Item, DC );
        break;

    case TYPEMIRE:
        Delete_Mire( (MIREPCB*) Item, DC );
        break;

    case TYPEDRAWSEGMENT:
        Delete_Segment_Edge( (DRAWSEGMENT*) Item, DC );
        break;

    case TYPETEXTE:
        Delete_Texte_Pcb( (TEXTE_PCB*) Item, DC );
        break;

    case TYPETRACK:
        Delete_Track( DC, (TRACK*) Item );
        break;

    case TYPEVIA:
        Delete_Segment( DC, (TRACK*) Item );
        break;

    case TYPEZONE:
        Delete_Zone( DC, (SEGZONE*) Item );
        break;

    case TYPEMARQUEUR:
        break;

    case TYPEPAD:
    case TYPETEXTEMODULE:
    case TYPEEDGEMODULE:
        break;

    case TYPE_NOT_INIT:
    case PCB_EQUIPOT_STRUCT_TYPE:
    case TYPEPCB:
    default:
    {
        wxString Line;
        Line.Printf( wxT( "Remove: StructType %d Inattendu" ),
                     Item->Type() );
        DisplayError( this, Line );
    }
        break;
    }
}


/****************************************************************/
void WinEDA_PcbFrame::SwitchLayer( wxDC* DC, int layer )
/*****************************************************************/
{
    int preslayer = GetScreen()->m_Active_Layer;

    //if there is only one layer, don't switch.
    if( m_Pcb->m_BoardSettings->m_CopperLayerCount <= 1 )
        return;

    //otherwise, must be at least 2 layers..see if it is possible.
    if( layer == LAYER_CUIVRE_N || layer == LAYER_CMP_N
        || layer < m_Pcb->m_BoardSettings->m_CopperLayerCount - 1 )
    {
        if( preslayer == layer )
            return;
        EDA_BaseStruct* current = GetScreen()->GetCurItem();

        //see if we are drawing a segment; if so, add a via?
        if( m_ID_current_state == ID_TRACK_BUTT && current != NULL )
        {
            if( current->Type() == TYPETRACK && (current->m_Flags & IS_NEW) )
            {
                //want to set the routing layers so that it switches properly -
                //see the implementation of Other_Layer_Route - the working
                //layer is used to 'start' the via and set the layer masks appropriately.
                GetScreen()->m_Route_Layer_TOP    = preslayer;
                GetScreen()->m_Route_Layer_BOTTOM = layer;
                GetScreen()->m_Active_Layer = preslayer;
                Other_Layer_Route( (TRACK*) GetScreen()->GetCurItem(), DC );
            }
        }

        GetScreen()->m_Active_Layer = layer;

        if( DisplayOpt.ContrastModeDisplay )
            GetScreen()->SetRefreshReq();
    }
}
