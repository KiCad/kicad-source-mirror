/**
 * @file dialog_global_deletion.cpp
 */


#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "dialog_global_deletion.h"


DIALOG_GLOBAL_DELETION::DIALOG_GLOBAL_DELETION( PCB_EDIT_FRAME* parent )
    : DIALOG_GLOBAL_DELETION_BASE( parent )
{
    m_Parent = parent;
    SetFocus();

    GetSizer()->SetSizeHints(this);
    Centre();
}


/********************************************************************/
void PCB_EDIT_FRAME::InstallPcbGlobalDeleteFrame( const wxPoint& pos )
/********************************************************************/
{
    DIALOG_GLOBAL_DELETION dlg( this );
    dlg.ShowModal();
}


/***********************************************************************/
void DIALOG_GLOBAL_DELETION::AcceptPcbDelete( )
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

        if( m_DelBoardEdges->GetValue() )
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
                track_mask_filter |= TRACK_LOCKED;
            if( !m_TrackFilterAR->GetValue() )
                track_mask_filter |= TRACK_AR;
            for( item = pcb->m_Track; item != NULL; item = nextitem )
            {
                nextitem = item->Next();
                if( (item->GetState( TRACK_LOCKED | TRACK_AR ) & track_mask_filter) != 0 )
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
    m_Parent->OnModify();

    EndModal( 1 );
}

