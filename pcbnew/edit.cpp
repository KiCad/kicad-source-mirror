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



static void Process_Move_Item( WinEDA_PcbFrame* frame,
                               EDA_BaseStruct* DrawStruct, wxDC* DC );

// see wxstruct.h
void WinEDA_PcbFrame::SendMessageToEESCHEMA( BOARD_ITEM* objectToSync )
{
    char    cmd[1024];
    MODULE* module = NULL;
    
	if ( objectToSync == NULL ) 
        return;

    if( objectToSync->Type() == TYPEMODULE )
        module = (MODULE*) objectToSync;
    else if( objectToSync->Type() == TYPEPAD )
        module = (MODULE*) objectToSync->GetParent();
    else if( objectToSync->Type() == TYPETEXTEMODULE )
        module = (MODULE*) objectToSync->GetParent();

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
    
    wxGetMousePosition( &pos.x, &pos.y );

    pos.y += 20;

    switch( id )   // Some (not all ) edit commands must be finished or aborted
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
        /* Should not be executed, just in case */
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

    default:        // Finish (abort ) the command
        if( DrawPanel->ManageCurseur
            && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;
    }

    switch( id )   // Execute command
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
        if( GetCurItem() == NULL )
            break;
        Edit_Track_Width( &dc, (TRACK*) GetCurItem() );
        DrawPanel->MouseToCursorSchema();
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_EDIT_TRACKSEG:
        if( GetCurItem() == NULL )
            break;
        Edit_TrackSegm_Width( &dc, (TRACK*) GetCurItem() );
        DrawPanel->MouseToCursorSchema();
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_EDIT_NET:
        if( GetCurItem() == NULL )
            break;
        Edit_Net_Width( &dc, ( (TRACK*) GetCurItem() )->m_NetCode );
        DrawPanel->MouseToCursorSchema();
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_EDIT_ALL_VIAS_AND_TRACK_SIZE:
    case ID_POPUP_PCB_EDIT_ALL_VIAS_SIZE:
    case ID_POPUP_PCB_EDIT_ALL_TRACK_SIZE:
        if( GetCurItem() == NULL )
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
        End_Route( (TRACK*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_PLACE_MOVED_TRACK_NODE:
        DrawPanel->MouseToCursorSchema();
        if( GetCurItem()->m_Flags & IS_DRAGGED )
        {
            PlaceDraggedTrackSegment( (TRACK*) GetCurItem(), &dc );
        }
        break;

    case ID_POPUP_PCB_PLACE_VIA:
        DrawPanel->MouseToCursorSchema();
        if( GetCurItem()->m_Flags & IS_DRAGGED )
        {
            PlaceDraggedTrackSegment( (TRACK*) GetCurItem(), &dc );
        }
        else
        {
            Other_Layer_Route( (TRACK*) GetCurItem(), &dc );
            if( DisplayOpt.ContrastModeDisplay )
                GetScreen()->SetRefreshReq();
        }
        break;

    case ID_POPUP_PCB_DELETE_TRACKSEG:
        if( GetCurItem() == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        SetCurItem( Delete_Segment( &dc, (TRACK*) GetCurItem() ) );
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_DELETE_TRACK:
        if( GetCurItem() == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        Delete_Track( &dc, (TRACK*) GetCurItem() );
        SetCurItem( NULL );
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_DELETE_TRACKNET:
        DrawPanel->MouseToCursorSchema();
        Delete_net( &dc, (TRACK*) GetCurItem() );
        SetCurItem( NULL );
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_LOCK_ON_TRACKSEG:
        Attribut_Segment( (TRACK*) GetCurItem(), &dc, TRUE );
        break;

    case ID_POPUP_PCB_LOCK_OFF_TRACKSEG:
        Attribut_Segment( (TRACK*) GetCurItem(), &dc, FALSE );
        break;

    case ID_POPUP_PCB_LOCK_ON_TRACK:
        Attribut_Track( (TRACK*) GetCurItem(), &dc, TRUE );
        break;

    case ID_POPUP_PCB_LOCK_OFF_TRACK:
        Attribut_Track( (TRACK*) GetCurItem(), &dc, FALSE );
        break;

    case ID_POPUP_PCB_LOCK_ON_NET:
        Attribut_net( &dc, ( (TRACK*) GetCurItem() )->m_NetCode, TRUE );
        break;

    case ID_POPUP_PCB_LOCK_OFF_NET:
        Attribut_net( &dc, ( (TRACK*) GetCurItem() )->m_NetCode, FALSE );
        break;

    case ID_POPUP_PCB_SETFLAGS_TRACK_MNU:
        break;

    case ID_POPUP_PCB_DELETE_ZONE:
        DrawPanel->MouseToCursorSchema();
        if( GetCurItem() == NULL )
            break;
        Delete_Zone( &dc, (SEGZONE*) GetCurItem() );
        SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_EDIT_ZONE:
        DrawPanel->MouseToCursorSchema();
        if( GetCurItem() == NULL )
            break;
        Edit_Zone_Width( &dc, (SEGZONE*) GetCurItem() );
        break;

    case ID_POPUP_PCB_DELETE_ZONE_LIMIT:
        DrawPanel->MouseToCursorSchema();
        DelLimitesZone( &dc, TRUE );
        break;

    case ID_PCB_DELETE_ITEM_BUTT:
        SetToolID( id, wxCURSOR_BULLSEYE, _( "Delete item" ) );
        break;

    case ID_POPUP_PCB_MOVE_TEXTEPCB_REQUEST:
        Process_Move_Item( this, GetCurItem(), &dc );
        DrawPanel->m_AutoPAN_Request = TRUE;
        break;

    case ID_POPUP_PCB_DRAG_MODULE_REQUEST:
        g_Drag_Pistes_On = TRUE;

    case ID_POPUP_PCB_MOVE_MODULE_REQUEST:

        // If the current Item is a pad, text module ...: Get the parent
        if( GetCurItem()->Type() != TYPEMODULE )
            SetCurItem( GetCurItem()->GetParent() );
        if( !GetCurItem() || GetCurItem()->Type() != TYPEMODULE )
        {
            g_Drag_Pistes_On = FALSE;
            break;
        }
        DrawPanel->MouseToCursorSchema();
        StartMove_Module( (MODULE*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST:      /* get module by name and move it */
        SetCurItem( GetModuleByName() );
        if( GetCurItem() )
        {
            DrawPanel->MouseToCursorSchema();
            StartMove_Module( (MODULE*) GetCurItem(), &dc );
        }
        break;

    case ID_POPUP_PCB_DELETE_MODULE:
        DrawPanel->MouseToCursorSchema();

        // If the current Item is a pad, text module ...: Get the parent
        if( GetCurItem()->Type() != TYPEMODULE )
            SetCurItem( GetCurItem()->GetParent() );
        
        if( !GetCurItem() || GetCurItem()->Type() != TYPEMODULE )
            break;
        if( Delete_Module( (MODULE*) GetCurItem(), &dc ) )
        {
            SetCurItem( NULL );
        }
        break;

    case ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE:
        DrawPanel->MouseToCursorSchema();

        // If the current Item is a pad, text module ...: Get the parent
        if( GetCurItem()->Type() != TYPEMODULE )
            SetCurItem( GetCurItem()->GetParent() );
        
        if( !GetCurItem() || GetCurItem()->Type() != TYPEMODULE )
            break;
        Rotate_Module( &dc, (MODULE*) GetCurItem(), -900, TRUE );
        break;

    case ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE:
        DrawPanel->MouseToCursorSchema();

        // If the current Item is a pad, text module ...: Get the parent
        if( GetCurItem()->Type() != TYPEMODULE )
            SetCurItem( GetCurItem()->GetParent() );

        if( !GetCurItem() || GetCurItem()->Type() != TYPEMODULE )
            break;
        Rotate_Module( &dc, (MODULE*) GetCurItem(), 900, TRUE );
        break;

    case ID_POPUP_PCB_CHANGE_SIDE_MODULE:
        DrawPanel->MouseToCursorSchema();

        // If the current Item is a pad, text module ...: Get the parent
        if( GetCurItem()->Type() != TYPEMODULE )
            SetCurItem( GetCurItem()->GetParent() );
        if( !GetCurItem() || GetCurItem()->Type() != TYPEMODULE )
            break;
        Change_Side_Module( (MODULE*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_EDIT_MODULE:
        // If the current Item is a pad, text module ...: Get the parent
        if( GetCurItem()->Type() != TYPEMODULE )
            SetCurItem( GetCurItem()->GetParent() );
        if( !GetCurItem() || GetCurItem()->Type() != TYPEMODULE )
            break;
        InstallModuleOptionsFrame( (MODULE*) GetCurItem(), &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DRAG_PAD_REQUEST:
        g_Drag_Pistes_On = TRUE;

    case ID_POPUP_PCB_MOVE_PAD_REQUEST:
        DrawPanel->MouseToCursorSchema();
        StartMovePad( (D_PAD*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_EDIT_PAD:
        InstallPadOptionsFrame( (D_PAD*) GetCurItem(), &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_IMPORT_PAD_SETTINGS:
        DrawPanel->MouseToCursorSchema();
        Import_Pad_Settings( (D_PAD*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS:
        DrawPanel->MouseToCursorSchema();
        Global_Import_Pad_Settings( (D_PAD*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_EXPORT_PAD_SETTINGS:
        DrawPanel->MouseToCursorSchema();
        Export_Pad_Settings( (D_PAD*) GetCurItem() );
        break;

    case ID_POPUP_PCB_DELETE_PAD:
        DeletePad( (D_PAD*) GetCurItem(), &dc );
        SetCurItem( NULL );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_EDIT_TEXTMODULE:
        InstallTextModOptionsFrame( (TEXTE_MODULE*) GetCurItem(), &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST:
        DrawPanel->MouseToCursorSchema();
        StartMoveTexteModule( (TEXTE_MODULE*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_ROTATE_TEXTMODULE:
        RotateTextModule( (TEXTE_MODULE*) GetCurItem(),
                         &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DELETE_TEXTMODULE:
        DeleteTextModule( (TEXTE_MODULE*) GetCurItem(),  &dc );
        SetCurItem( NULL );
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
        InstallTextPCBOptionsFrame( (TEXTE_PCB*) GetCurItem(),
                                   &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_ROTATE_TEXTEPCB:
        Rotate_Texte_Pcb( (TEXTE_PCB*) GetCurItem(), &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DELETE_TEXTEPCB:
        Delete_Texte_Pcb( (TEXTE_PCB*) GetCurItem(), &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_MOVE_MIRE_REQUEST:
        StartMove_Mire( (MIREPCB*) GetCurItem(), &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_EDIT_MIRE:
        InstallMireOptionsFrame( (MIREPCB*) GetCurItem(), &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DELETE_MIRE:
        DrawPanel->MouseToCursorSchema();
        Delete_Mire( (MIREPCB*) GetCurItem(), &dc );
        SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_DELETE_COTATION:
        DrawPanel->MouseToCursorSchema();
        Delete_Cotation( (COTATION*) GetCurItem(), &dc );
        SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_EDIT_COTATION:
        Install_Edit_Cotation( (COTATION*) GetCurItem(), &dc, pos );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DELETE_DRAWING:
        Delete_Segment_Edge( (DRAWSEGMENT*) GetCurItem(), &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_DELETE_DRAWING_LAYER:
        Delete_Drawings_All_Layer( (DRAWSEGMENT*) GetCurItem(), &dc );
        SetCurItem( NULL );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_EDIT_DRAWING:
        Drawing_SetNewWidth( (DRAWSEGMENT*) GetCurItem(), &dc );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_PCB_MOVE_DRAWING_REQUEST:
        DrawPanel->MouseToCursorSchema();
        Start_Move_DrawItem( (DRAWSEGMENT*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_STOP_CURRENT_DRAWING:
        DrawPanel->MouseToCursorSchema();
        if( GetCurItem() && (GetCurItem()->m_Flags & IS_NEW) )
        {
            End_Edge( (DRAWSEGMENT*) GetCurItem(), &dc );
            SetCurItem( NULL );
        }
        break;

    case ID_POPUP_PCB_STOP_CURRENT_EDGE_ZONE:
        DrawPanel->MouseToCursorSchema();
        if( GetCurItem() && (GetCurItem()->m_Flags & IS_NEW) )
        {
            End_Zone( &dc );
            SetCurItem( NULL );
        }
        break;

    case ID_POPUP_PCB_DELETE_EDGE_ZONE:
        DrawPanel->MouseToCursorSchema();
        if( GetCurItem() && (GetCurItem()->m_Flags & IS_NEW) )
        {
            SetCurItem( Del_SegmEdgeZone( &dc,
                                                           (EDGE_ZONE*) GetCurItem() ) );
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
