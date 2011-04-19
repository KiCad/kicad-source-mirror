/* event_handlers_tracks_vias_sizes.cpp
 *
 *  Handlers for popup and toolbars events relative
 *  to the tracks and vias sizes
 */


#include "fctsys.h"

//#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew_id.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "dialog_helpers.h"

/**
 * Function Tracks_and_Vias_Size_Event
 * Event handler for tracks and vias size selection (and some options)
 * relative to toolbars and popup events
 */
void PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event( wxCommandEvent& event )
{
    int ii;
    int id = event.GetId();

/* Note: none of these events require aborting the current command (if any)
 * (like move, edit or block command)
 * so we do not test for a current command in progress and call
 *  DrawPanel->m_endMouseCaptureCallback( DrawPanel, &dc );
 */
    switch( id )
    {
    case ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH:
        GetBoard()->GetBoardDesignSettings()->m_UseConnectedTrackWidth =
            not GetBoard()->GetBoardDesignSettings()->m_UseConnectedTrackWidth;
        break;

    case ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES:
        GetBoard()->GetBoardDesignSettings()->m_UseConnectedTrackWidth = false;
        GetBoard()->m_TrackWidthSelector = 0;
        GetBoard()->m_ViaSizeSelector = 0;
        break;

    case ID_POPUP_PCB_SELECT_AUTO_WIDTH:
        DrawPanel->MoveCursorToCrossHair();
        GetBoard()->GetBoardDesignSettings()->m_UseConnectedTrackWidth = true;
        break;

    case ID_POPUP_PCB_SELECT_WIDTH1:      // this is the default Netclass selection
    case ID_POPUP_PCB_SELECT_WIDTH2:      // this is a custom value selection
    case ID_POPUP_PCB_SELECT_WIDTH3:
    case ID_POPUP_PCB_SELECT_WIDTH4:
    case ID_POPUP_PCB_SELECT_WIDTH5:
    case ID_POPUP_PCB_SELECT_WIDTH6:
    case ID_POPUP_PCB_SELECT_WIDTH7:
    case ID_POPUP_PCB_SELECT_WIDTH8:
        DrawPanel->MoveCursorToCrossHair();
        GetBoard()->GetBoardDesignSettings()->m_UseConnectedTrackWidth = false;
        ii = id - ID_POPUP_PCB_SELECT_WIDTH1;
        GetBoard()->m_TrackWidthSelector = ii;
        break;

    case ID_POPUP_PCB_SELECT_VIASIZE1:   // this is the default Netclass selection
    case ID_POPUP_PCB_SELECT_VIASIZE2:   // this is a custom value selection
    case ID_POPUP_PCB_SELECT_VIASIZE3:
    case ID_POPUP_PCB_SELECT_VIASIZE4:
    case ID_POPUP_PCB_SELECT_VIASIZE5:
    case ID_POPUP_PCB_SELECT_VIASIZE6:
    case ID_POPUP_PCB_SELECT_VIASIZE7:
    case ID_POPUP_PCB_SELECT_VIASIZE8:   // select the new current value for via size (via diameter)
        DrawPanel->MoveCursorToCrossHair();
        ii = id - ID_POPUP_PCB_SELECT_VIASIZE1;
        GetBoard()->m_ViaSizeSelector = ii;
        break;

    case ID_AUX_TOOLBAR_PCB_TRACK_WIDTH:
        ii = m_SelTrackWidthBox->GetCurrentSelection();
        GetBoard()->m_TrackWidthSelector = ii;
        break;

    case ID_AUX_TOOLBAR_PCB_VIA_SIZE:
        ii = m_SelViaSizeBox->GetCurrentSelection();
        GetBoard()->m_ViaSizeSelector = ii;
        break;

    default:
        wxMessageBox( wxT( "PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event() error") );
        break;
    }
}
