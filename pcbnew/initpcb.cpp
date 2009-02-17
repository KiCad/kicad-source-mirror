/*********************************************/
/******* file initpcb.cpp ********************/
/*********************************************/


#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "autorout.h"

#include "protos.h"

/**************************************/
/* dialog WinEDA_PcbGlobalDeleteFrame */
/**************************************/
#include "dialog_initpcb.cpp"


/********************************************************************/
void WinEDA_PcbFrame::InstallPcbGlobalDeleteFrame( const wxPoint& pos )
/********************************************************************/
{
    WinEDA_PcbGlobalDeleteFrame* frame =
        new WinEDA_PcbGlobalDeleteFrame( this );

    frame->ShowModal(); frame->Destroy();
}


/***********************************************************************/
void WinEDA_PcbGlobalDeleteFrame::AcceptPcbDelete( wxCommandEvent& event )
/***********************************************************************/
{
    int        track_mask;
    bool       redraw = FALSE;
    wxClientDC dc( m_Parent->DrawPanel );

    m_Parent->DrawPanel->PrepareGraphicContext( &dc );

    if( m_DelAlls->GetValue() )
    {
        m_Parent->Clear_Pcb( TRUE );
        redraw = TRUE;
    }
    else
    {
        if( m_DelZones->GetValue() )
        {
            m_Parent->Erase_Zones( TRUE );
            redraw = TRUE;
        }

        if( m_DelTexts->GetValue() )
        {
            m_Parent->Erase_Textes_Pcb( TRUE );
            redraw = TRUE;
        }

        if( m_DelEdges->GetValue() )
        {
            m_Parent->Erase_Segments_Pcb( TRUE, TRUE );
            redraw = TRUE;
        }

        if( m_DelDrawings->GetValue() )
        {
            m_Parent->Erase_Segments_Pcb( FALSE, TRUE );
            redraw = TRUE;
        }

        if( m_DelModules->GetValue() )
        {
            m_Parent->Erase_Modules( TRUE );
            redraw = TRUE;
        }

        if( m_DelTracks->GetValue() )
        {
            {
                track_mask = 0;
                if( !m_TrackFilterLocked->GetValue() )
                    track_mask |= SEGM_FIXE;
                if( !m_TrackFilterAR->GetValue() )
                    track_mask |= SEGM_AR;

                m_Parent->Erase_Pistes( &dc, track_mask, TRUE );
                redraw = TRUE;
            }
        }

        if( m_DelMarkers->GetValue() )
        {
            m_Parent->Erase_Marqueurs();
            redraw = TRUE;
        }
    }

    if( redraw )
    {
        m_Parent->SetCurItem( NULL );
        m_Parent->DrawPanel->Refresh();
    }

    EndModal( 1 );
}


/*********************************************************/
bool WinEDA_BasePcbFrame::Clear_Pcb( bool query )
/*********************************************************/

/* Realise les init des pointeurs et variables
 *  Si query == FALSE, il n'y aura pas de confirmation
 */
{
    if( GetBoard() == NULL )
        return FALSE;

    if( query && GetScreen()->IsModify() )
    {
        if( GetBoard()->m_Drawings || GetBoard()->m_Modules
            || GetBoard()->m_Track || GetBoard()->m_Zone )
        {
            if( !IsOK( this, _( "Current Board will be lost ?" ) ) )
                return FALSE;
        }
    }

    // delete the old BOARD and create a new BOARD so that the default
    // layer names are put into the BOARD.
    SetBoard( new BOARD( NULL, this ) );

    while( g_UnDeleteStackPtr > 0 )
    {
        g_UnDeleteStackPtr--;

        delete g_UnDeleteStack[g_UnDeleteStackPtr];
    }

    /* init pointeurs  et variables */
    GetScreen()->m_FileName.Empty();

    memset( buf_work, 0, BUFMEMSIZE );
    adr_lowmem = adr_max = buf_work;

    SetCurItem( NULL );

    /* Init parametres de gestion */
    wxRealPoint gridsize = GetScreen()->GetGrid();
    ((PCB_SCREEN*)GetScreen())->Init();
    GetScreen()->SetGrid( gridsize );

    g_HightLigt_Status = 0;

    for( int ii = 1; ii < HISTORY_NUMBER; ii++ )
    {
        g_DesignSettings.m_ViaSizeHistory[ii] =
            g_DesignSettings.m_TrackWidthHistory[ii] = 0;
    }

    g_DesignSettings.m_TrackWidthHistory[0] = g_DesignSettings.m_CurrentTrackWidth;
    g_DesignSettings.m_ViaSizeHistory[0]    = g_DesignSettings.m_CurrentViaSize;

/* NO, this is a global setting, and changing it here changes a loaded board's layer count when loading a module in the module editor since
    the module editor calls this when loading an existing module.
    g_DesignSettings.m_CopperLayerCount = 2;		// Default copper layers count set to 2: double layer board
*/

    Zoom_Automatique( TRUE );

    return TRUE;
}


/************************************************************/
void WinEDA_PcbFrame::Erase_Zones( bool query )
/************************************************************/
{
    if( query && !IsOK( this, _( "Delete Zones ?" ) ) )
        return;

    GetBoard()->m_Zone.DeleteAll();
    GetBoard()->DeleteZONEOutlines();

    GetScreen()->SetModify();
}


/*****************************************************************************/
void WinEDA_PcbFrame::Erase_Segments_Pcb( bool is_edges, bool query )
/*****************************************************************************/
{
    BOARD_ITEM*     PtStruct;
    BOARD_ITEM*     PtNext;
    int             masque_layer = (~EDGE_LAYER) & 0x1FFF0000;

    if( is_edges )
    {
        masque_layer = EDGE_LAYER;
        if( query && !IsOK( this, _( "Delete Board edges ?" ) ) )
            return;
    }
    else
    {
        if( query && !IsOK( this, _( "Delete draw items?" ) ) )
            return;
    }

    PtStruct = GetBoard()->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtNext )
    {
        PtNext = PtStruct->Next();

        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
        case TYPE_TEXTE:
        case TYPE_COTATION:
        case TYPE_MIRE:
            if( g_TabOneLayerMask[ PtStruct->GetLayer()] & masque_layer )
                GetBoard()->Delete( PtStruct );
            break;

        default:
            DisplayError( this, wxT( "Unknown/unexpected Draw Type" ) );
            break;
        }
    }

    GetScreen()->SetModify();
}


/**************************************************************************/
void WinEDA_PcbFrame::Erase_Pistes( wxDC * DC, int masque_type, bool query )
/**************************************************************************/

/* Efface les segments de piste, selon les autorisations affichees
 *  masque_type = masque des options de selection:
 *  SEGM_FIXE, SEGM_AR
 *  Si un des bits est a 1, il n'y a pas effacement du segment de meme bit a 1
 */
{
    TRACK*  pt_segm;
    TRACK*  PtNext;

    if( query && !IsOK( this, _( "Delete Tracks?" ) ) )
        return;

    /* Marquage des pistes a effacer */
    for( pt_segm = GetBoard()->m_Track; pt_segm != NULL; pt_segm = (TRACK*) PtNext )
    {
        PtNext = pt_segm->Next();

        if( pt_segm->GetState( SEGM_FIXE | SEGM_AR ) & masque_type )
            continue;

        pt_segm->DeleteStructure();
    }

    GetScreen()->SetModify();
    Compile_Ratsnest( DC, TRUE );
}


/**************************************************************/
void WinEDA_PcbFrame::Erase_Modules( bool query )
/**************************************************************/
{
    if( query && !IsOK( this, _( "Delete Modules?" ) ) )
        return;

    GetBoard()->m_Modules.DeleteAll();

    GetBoard()->m_Status_Pcb = 0;
    GetBoard()->m_NbNodes     = 0;
    GetBoard()->m_NbLinks     = 0;
    GetBoard()->m_NbNoconnect = 0;

    GetScreen()->SetModify();
}


/************************************************************/
void WinEDA_PcbFrame::Erase_Textes_Pcb( bool query )
/************************************************************/
{
    BOARD_ITEM* PtStruct, * PtNext;

    if( query && !IsOK( this, _( "Delete Pcb Texts" ) ) )
        return;

    PtStruct = GetBoard()->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtNext )
    {
        PtNext = PtStruct->Next();
        if( PtStruct->Type() == TYPE_TEXTE )
        {
            PtStruct->DeleteStructure();
        }
    }

    GetScreen()->SetModify();
}


/*******************************************/
void WinEDA_PcbFrame::Erase_Marqueurs()
/*******************************************/
{
    GetBoard()->DeleteMARKERs();
    GetScreen()->SetModify();   // @todo : why mark this if MARKERs are not saved in the *.brd file?
}

