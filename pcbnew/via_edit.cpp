/**********************************************/
/* vi_edit.cpp: some editing function for vias */
/**********************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbnew_id.h"


/**********************************************************************************/
void WinEDA_PcbFrame::Via_Edit_Control( wxCommandEvent& event )
/**********************************************************************************/

/*
  * Execute edit commands relative to vias
 */
{
    TRACK* via_struct;
    SEGVIA* via = (SEGVIA*) GetCurItem();
    wxClientDC  dc( DrawPanel );
    DrawPanel->CursorOff( &dc );
    DrawPanel->PrepareGraphicContext( &dc );

    wxASSERT( via->Type() == TYPE_VIA);

    PICKED_ITEMS_LIST itemsListPicker;
    ITEM_PICKER picker( NULL, UR_CHANGED );

    switch( event.GetId() )
    {
    case ID_POPUP_PCB_VIA_HOLE_ENTER_VALUE:     // Enter a new alternate value for drill via
        InstallPcbOptionsFrame( wxDefaultPosition, &dc, ID_PCB_TRACK_SIZE_SETUP );
        DrawPanel->MouseToCursorSchema();

    case ID_POPUP_PCB_VIA_HOLE_TO_VALUE:        // Set the drill via to custom
        if( (g_DesignSettings.m_ViaDrillCustomValue > 0) && (g_DesignSettings.m_ViaDrillCustomValue < via->m_Width) )
        {
            SaveCopyInUndoList(via, UR_CHANGED);
            via->Draw( DrawPanel, &dc, GR_XOR );
            via->SetDrillValue( g_DesignSettings.m_ViaDrillCustomValue );
            via->Draw( DrawPanel, &dc, GR_OR );
            GetScreen()->SetModify();
        }
        else
            DisplayError( this, _( "Incorrect value for Via drill. No via drill change" ) );
        break;


    case ID_POPUP_PCB_VIA_HOLE_EXPORT:      // Export the current drill value as the new custom value
        if( via->GetDrillValue() > 0 )
            g_DesignSettings.m_ViaDrillCustomValue = via->GetDrillValue();
        break;

    case ID_POPUP_PCB_VIA_HOLE_EXPORT_TO_OTHERS:    // Export the current drill value to via which héave the same size
        if( via->GetDrillValue() > 0 )
            g_DesignSettings.m_ViaDrillCustomValue = via->GetDrillValue();
        via_struct = GetBoard()->m_Track;
        for( ; via_struct != NULL; via_struct = via_struct->Next() )
        {
            if( via_struct->Type() == TYPE_VIA )     /* mise a jour du diametre de la via */
            {
                if( via_struct->m_Width != via->m_Width )
                    continue;
                picker.m_PickedItem = via_struct;
                picker.m_Link = via_struct->Copy();
                itemsListPicker.PushItem(picker);
                via_struct->Draw( DrawPanel, &dc, GR_XOR );
                via_struct->SetDrillValue( via->GetDrillValue() );
                via_struct->Draw( DrawPanel, &dc, GR_OR );
            }
        }
        SaveCopyInUndoList(itemsListPicker, UR_CHANGED);

        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_VIA_HOLE_TO_DEFAULT:
        SaveCopyInUndoList(via, UR_CHANGED);
        via->Draw( DrawPanel, &dc, GR_XOR );
        via->SetDrillDefault();
        via->Draw( DrawPanel, &dc, GR_OR );
        GetScreen()->SetModify();
        break;

    case ID_POPUP_PCB_VIA_HOLE_RESET_TO_DEFAULT:        // Reset all via hole to default value
        via_struct = GetBoard()->m_Track;
        for( ; via_struct != NULL; via_struct = via_struct->Next() )
        {
            if( via_struct->Type() == TYPE_VIA )     /* mise a jour du diametre de la via */
            {
                if( ! via_struct->IsDrillDefault() )
                {
                    picker.m_PickedItem = via_struct;
                    picker.m_Link = via_struct->Copy();
                    itemsListPicker.PushItem(picker);
                    via_struct->Draw( DrawPanel, &dc, GR_XOR );
                    via_struct->SetDrillDefault();
                    via_struct->Draw( DrawPanel, &dc, GR_OR );
                }
            }
        }
        SaveCopyInUndoList(itemsListPicker, UR_CHANGED);

        GetScreen()->SetModify();
        break;

    default:
        wxMessageBox( wxT( "WinEDA_PcbFrame::Via_Edit_Control() error: unknown command" ) );
        break;
    }

    DrawPanel->CursorOn( &dc );
    DrawPanel->MouseToCursorSchema();
}
