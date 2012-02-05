/**
 * @file dialog_global_deletion.cpp
 */


#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <pcbcommon.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>

#include <dialog_global_deletion.h>


DIALOG_GLOBAL_DELETION::DIALOG_GLOBAL_DELETION( PCB_EDIT_FRAME* parent )
    : DIALOG_GLOBAL_DELETION_BASE( parent )
{
    m_Parent = parent;
    m_currentLayer = 0;
    m_TrackFilterAR->Enable( m_DelTracks->GetValue() );
    m_TrackFilterLocked->Enable( m_DelTracks->GetValue() );
    m_TrackFilterNormal->Enable( m_DelTracks->GetValue() );
    m_TrackFilterVias->Enable( m_DelTracks->GetValue() );
    SetFocus();

    GetSizer()->SetSizeHints(this);
    Centre();
}


void PCB_EDIT_FRAME::InstallPcbGlobalDeleteFrame( const wxPoint& pos )
{
    DIALOG_GLOBAL_DELETION dlg( this );
    dlg.SetCurrentLayer( getActiveLayer() );

    dlg.ShowModal();
}

void DIALOG_GLOBAL_DELETION::SetCurrentLayer( int aLayer )
{
    m_currentLayer = aLayer;
    m_textCtrlCurrLayer->SetValue( m_Parent->GetBoard()->GetLayerName( aLayer ) );
}

void DIALOG_GLOBAL_DELETION::OnCheckDeleteTracks( wxCommandEvent& event )
{
    m_TrackFilterAR->Enable( m_DelTracks->GetValue() );
    m_TrackFilterLocked->Enable( m_DelTracks->GetValue() );
    m_TrackFilterNormal->Enable( m_DelTracks->GetValue() );
    m_TrackFilterVias->Enable( m_DelTracks->GetValue() );
}

void DIALOG_GLOBAL_DELETION::AcceptPcbDelete( )
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

            /* SEG_ZONE items used in Zone filling selection are now deprecated :
            * and are deleted but not put in undo buffer if exist
            */
            pcb->m_Zone.DeleteAll();

            while( pcb->GetAreaCount() )
            {
                item = pcb->GetArea( 0 );
                itemPicker.SetItem( item );
                pickersList.PushItem( itemPicker );
                pcb->Remove( item );
            }
        }

        int masque_layer = 0;
        int layers_filter = ALL_LAYERS;
        if( m_rbLayersOption->GetSelection() != 0 )     // Use current layer only
            layers_filter = 1 << m_currentLayer;


        if( m_DelDrawings->GetValue() )
            masque_layer = (~EDGE_LAYER) & ALL_NO_CU_LAYERS;

        if( m_DelBoardEdges->GetValue() )
            masque_layer |= EDGE_LAYER;

        layers_filter &= layers_filter;

        for( item = pcb->m_Drawings; item != NULL; item = nextitem )
        {
            nextitem = item->Next();
            bool removeme = (GetLayerMask( item->GetLayer() ) & masque_layer) != 0;

            if( ( item->Type() == PCB_TEXT_T ) && m_DelTexts->GetValue() )
                removeme = true;

            if( removeme )
            {
                itemPicker.SetItem( item );
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
                itemPicker.SetItem( item );
                pickersList.PushItem( itemPicker );
                item->UnLink();
            }
        }

        if( m_DelTracks->GetValue() )
        {
            int track_mask_filter = 0;

            if( !m_TrackFilterLocked->GetValue() )
                track_mask_filter |= TRACK_LOCKED;

            if( !m_TrackFilterAR->GetValue() )
                track_mask_filter |= TRACK_AR;

            TRACK * nexttrack;
            for( TRACK *track = pcb->m_Track; track != NULL; track = nexttrack )
            {
                nexttrack = track->Next();

                if( (track->GetState( TRACK_LOCKED | TRACK_AR ) & track_mask_filter) != 0 )
                    continue;

                if( (track->GetState( TRACK_LOCKED | TRACK_AR ) == 0) &&
                    !m_TrackFilterNormal->GetValue() )
                    continue;

                if( (track->Type() == PCB_VIA_T)  && !m_TrackFilterVias->GetValue() )
                    continue;

                if( (track->ReturnMaskLayer() & layers_filter) == 0 )
                    continue;

                itemPicker.SetItem( track );
                pickersList.PushItem( itemPicker );
                track->UnLink();
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

    m_Parent->GetCanvas()->Refresh();
    m_Parent->OnModify();

    EndModal( 1 );
}

