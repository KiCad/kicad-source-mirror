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
    m_currentLayer = FIRST_LAYER;
    m_TrackFilterAR->Enable( m_DelTracks->GetValue() );
    m_TrackFilterLocked->Enable( m_DelTracks->GetValue() );
    m_TrackFilterNormal->Enable( m_DelTracks->GetValue() );
    m_TrackFilterVias->Enable( m_DelTracks->GetValue() );
    m_ModuleFilterLocked->Enable( m_DelModules->GetValue() );
    m_ModuleFilterNormal->Enable( m_DelModules->GetValue() );
    SetFocus();

    GetSizer()->SetSizeHints( this );
    Centre();
}


void PCB_EDIT_FRAME::InstallPcbGlobalDeleteFrame( const wxPoint& pos )
{
    DIALOG_GLOBAL_DELETION dlg( this );
    dlg.SetCurrentLayer( getActiveLayer() );

    dlg.ShowModal();
}


void DIALOG_GLOBAL_DELETION::SetCurrentLayer( LAYER_NUM aLayer )
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


void DIALOG_GLOBAL_DELETION::OnCheckDeleteModules( wxCommandEvent& event )
{
    m_ModuleFilterLocked->Enable( m_DelModules->GetValue() );
    m_ModuleFilterNormal->Enable( m_DelModules->GetValue() );
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

        if( !IsOK( this, _( "Are you sure you want to delete the selected items?" ) ) )
            return;

        BOARD*            pcb = m_Parent->GetBoard();
        PICKED_ITEMS_LIST pickersList;
        ITEM_PICKER       itemPicker( NULL, UR_DELETED );
        BOARD_ITEM*       item, * nextitem;

        LAYER_MSK layers_filter = ALL_LAYERS;

        if( m_rbLayersOption->GetSelection() != 0 )     // Use current layer only
            layers_filter = GetLayerMask( m_currentLayer );

        if( m_DelZones->GetValue() )
        {
            int area_index = 0;
            item = pcb->GetArea( area_index );

            while( item != NULL )
            {

                if( GetLayerMask( item->GetLayer() ) & layers_filter )
                {
                    itemPicker.SetItem( item );
                    pickersList.PushItem( itemPicker );
                    pcb->Remove( item );
                    gen_rastnest = true;
                }
                else
                {
                    area_index++;
                }

                item = pcb->GetArea( area_index );
            }
        }

        if( m_DelDrawings->GetValue() || m_DelBoardEdges->GetValue() )
        {
            LAYER_MSK masque_layer = NO_LAYERS;

            if( m_DelDrawings->GetValue() )
                 masque_layer = (~EDGE_LAYER) & ALL_NO_CU_LAYERS;

            if( m_DelBoardEdges->GetValue() )
                 masque_layer |= EDGE_LAYER;

            masque_layer &= layers_filter;

            for( item = pcb->m_Drawings; item != NULL; item = nextitem )
            {
                nextitem = item->Next();

                if( ( item->Type() == PCB_LINE_T ) && ( GetLayerMask( item->GetLayer() ) & masque_layer) )
                {
                    itemPicker.SetItem( item );
                    pickersList.PushItem( itemPicker );
                    item->UnLink();
                }
            }
        }

        if( m_DelTexts->GetValue() )
        {
            LAYER_MSK del_text_layers = ALL_LAYERS & layers_filter;

            for( item = pcb->m_Drawings; item != NULL; item = nextitem )
            {
                nextitem = item->Next();

                if( ( item->Type() == PCB_TEXT_T ) && ( GetLayerMask( item->GetLayer() ) & del_text_layers ) )
                {
                    itemPicker.SetItem( item );
                    pickersList.PushItem( itemPicker );
                    item->UnLink();
                }
            }
        }

        if( m_DelModules->GetValue() )
        {

            for( item = pcb->m_Modules; item; item = nextitem )
            {
                nextitem = item->Next();

                if( ( GetLayerMask( item->GetLayer() ) & layers_filter ) &&
                    ( ( m_ModuleFilterNormal->GetValue() && !item->IsLocked() ) ||
                      ( m_ModuleFilterLocked->GetValue() && item->IsLocked() ) ) )
                {
                    itemPicker.SetItem( item );
                    pickersList.PushItem( itemPicker );
                    item->UnLink();
                    gen_rastnest = true;
                }
            }
        }

        if( m_DelTracks->GetValue() )
        {
            STATUS_FLAGS track_mask_filter = 0;

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

                if( (track->GetLayerMask() & layers_filter) == 0 )
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
