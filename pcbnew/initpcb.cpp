/*********************************************/
/******* file initpcb.cpp ********************/
/*********************************************/


#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

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
    bool gen_rastnest = false;

    m_Parent->SetCurItem( NULL );

    if( m_DelAlls->GetValue() )
    {
        m_Parent->Clear_Pcb( true );
    }
    else
    {
        if( !IsOK( this, _( "Ok to delete selected items ?" ) ) )
            return;

        BOARD * pcb = m_Parent->GetBoard();
        PICKED_ITEMS_LIST pickersList;
        ITEM_PICKER       itemPicker( NULL, UR_DELETED );
        BOARD_ITEM*       item, * nextitem;

        if( m_DelZones->GetValue() )
        {
            gen_rastnest = true;

            /* ZEG_ZONE items used in Zone filling selection are now deprecated :
            * and are deleted but not put in undo buffer if exist
            */
            pcb->m_Zone.DeleteAll();

            while( pcb->GetAreaCount() )
            {
                item = pcb->GetArea( 0 );
                itemPicker.m_PickedItem = item;
                pickersList.PushItem( itemPicker );
                pcb->Remove( item );
            }
        }

        int masque_layer = 0;
        if( m_DelDrawings->GetValue() )
            masque_layer = (~EDGE_LAYER) & 0x1FFF0000;

        if( m_DelEdges->GetValue() )
            masque_layer |= EDGE_LAYER;

        for( item = pcb->m_Drawings; item != NULL; item = nextitem )
        {
            nextitem = item->Next();
            bool removeme = (g_TabOneLayerMask[ item->GetLayer()] & masque_layer) != 0;
            if( ( item->Type() == TYPE_TEXTE ) && m_DelTexts->GetValue() )
                removeme = true;
            if( removeme )
            {
                itemPicker.m_PickedItem = item;
                pickersList.PushItem( itemPicker );
                item->UnLink();
            }
        }

        if( m_DelModules->GetValue() )
        {
            gen_rastnest = true;
            for( item = pcb->m_Modules; item; item = nextitem )
            {
                nextitem = item->Next();
                itemPicker.m_PickedItem = item;
                pickersList.PushItem( itemPicker );
                item->UnLink();
            }
        }

        if( m_DelTracks->GetValue() )
        {
            int track_mask_filter = 0;
            if( !m_TrackFilterLocked->GetValue() )
                track_mask_filter |= SEGM_FIXE;
            if( !m_TrackFilterAR->GetValue() )
                track_mask_filter |= SEGM_AR;
            for( item = pcb->m_Track; item != NULL; item = nextitem )
            {
                nextitem = item->Next();
                if( (item->GetState( SEGM_FIXE | SEGM_AR ) & track_mask_filter) != 0 )
                    continue;
                itemPicker.m_PickedItem = item;
                pickersList.PushItem( itemPicker );
                item->UnLink();
                gen_rastnest = true;
            }
        }

        if( pickersList.GetCount() )
            m_Parent->SaveCopyInUndoList( pickersList, UR_DELETED );

        if( m_DelMarkers->GetValue() )
            pcb->DeleteMARKERs();

        if( gen_rastnest )
            m_Parent->Compile_Ratsnest( NULL, true );
    }

    m_Parent->DrawPanel->Refresh();

    EndModal( 1 );
}


/** function WinEDA_PcbFrame::Clear_Pcb()
 * delete all and reinitialize the current board
 * @param aQuery = true to prompt user for confirmation, false to initialize silently
 */
bool WinEDA_PcbFrame::Clear_Pcb( bool aQuery )
{
    if( GetBoard() == NULL )
        return FALSE;

    if( aQuery )
    {
        if( GetBoard()->m_Drawings || GetBoard()->m_Modules
            || GetBoard()->m_Track || GetBoard()->m_Zone )
        {
            if( !IsOK( this,
                _( "Current Board will be lost and this operation and cannot be undone. Continue ?" ) ) )
                return FALSE;
        }
    }

    // Clear undo and redo lists because we want a full deletion
    GetScreen()->ClearUndoRedoList();

    // delete the old BOARD and create a new BOARD so that the default
    // layer names are put into the BOARD.
    SetBoard( new BOARD( NULL, this ) );

    /* init pointeurs  et variables */
    GetScreen()->m_FileName.Empty();

    SetCurItem( NULL );

    /* Init parametres de gestion */
    wxRealPoint gridsize = GetScreen()->GetGrid();
    GetScreen()->Init();
    GetScreen()->SetGrid( gridsize );

    g_HightLigt_Status = 0;

    for( int ii = 1; ii < HISTORY_NUMBER; ii++ )
    {
        g_DesignSettings.m_ViaSizeHistory[ii] =
            g_DesignSettings.m_TrackWidthHistory[ii] = 0;
        g_DesignSettings.m_TrackClearanceHistory[ii] = 0;
    }

    g_DesignSettings.m_TrackWidthHistory[0]     = g_DesignSettings.m_CurrentTrackWidth;
    g_DesignSettings.m_TrackClearanceHistory[0] = g_DesignSettings.m_TrackClearance;
    g_DesignSettings.m_ViaSizeHistory[0]        = g_DesignSettings.m_CurrentViaSize;
    g_DesignSettings.m_CopperLayerCount = 2;		// Default copper layers count set to 2: double layer board

    Zoom_Automatique( true );

    return true;
}



/** function WinEDA_ModuleEditFrame::Clear_Pcb()
 * delete all and reinitialize the current board
 * @param aQuery = true to prompt user for confirmation, false to initialize silently
 */
bool WinEDA_ModuleEditFrame::Clear_Pcb( bool aQuery )
{
    if( GetBoard() == NULL )
        return FALSE;

    if( aQuery && GetScreen()->IsModify() )
    {
        if( GetBoard()->m_Modules )
        {
            if( !IsOK( this,
                _( "Current Footprint will be lost and this operation and cannot be undone. Continue ?" ) ) )
                return FALSE;
        }
    }

    // Clear undo and redo lists
    GetScreen()->ClearUndoRedoList();

    // Delete the current footprint
    GetBoard()->m_Modules.DeleteAll();

    /* init pointeurs  et variables */
    GetScreen()->m_FileName.Empty();

    SetCurItem( NULL );

    /* Init parametres de gestion */
    wxRealPoint gridsize = GetScreen()->GetGrid();
    GetScreen()->Init();
    GetScreen()->SetGrid( gridsize );

    Zoom_Automatique( true );

    return true;
}
