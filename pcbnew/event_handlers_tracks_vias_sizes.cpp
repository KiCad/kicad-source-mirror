/**
 * @file event_handlers_tracks_vias_sizes.cpp
 * @brief Handlers for popup and toolbars events relative to the tracks and vias sizes.
 */


#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <dialog_helpers.h>

#include <pcbnew_id.h>
#include <pcbnew.h>

#include <class_board.h>
#include <class_module.h>


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
 *  m_canvas->m_endMouseCaptureCallback( m_canvas, &dc );
 */
    switch( id )
    {
    case ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH:
        GetDesignSettings().m_UseConnectedTrackWidth =
            not GetDesignSettings().m_UseConnectedTrackWidth;
        break;

    case ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES:
        GetDesignSettings().m_UseConnectedTrackWidth = false;
        GetDesignSettings().SetTrackWidthIndex( 0 );
        GetDesignSettings().SetViaSizeIndex( 0 );
        break;

    case ID_POPUP_PCB_SELECT_AUTO_WIDTH:
        m_canvas->MoveCursorToCrossHair();
        GetDesignSettings().m_UseConnectedTrackWidth = true;
        break;

    case ID_POPUP_PCB_SELECT_WIDTH1:      // this is the default Netclass selection
    case ID_POPUP_PCB_SELECT_WIDTH2:      // this is a custom value selection
    case ID_POPUP_PCB_SELECT_WIDTH3:
    case ID_POPUP_PCB_SELECT_WIDTH4:
    case ID_POPUP_PCB_SELECT_WIDTH5:
    case ID_POPUP_PCB_SELECT_WIDTH6:
    case ID_POPUP_PCB_SELECT_WIDTH7:
    case ID_POPUP_PCB_SELECT_WIDTH8:
    case ID_POPUP_PCB_SELECT_WIDTH9:
    case ID_POPUP_PCB_SELECT_WIDTH10:
    case ID_POPUP_PCB_SELECT_WIDTH11:
    case ID_POPUP_PCB_SELECT_WIDTH12:
    case ID_POPUP_PCB_SELECT_WIDTH13:
    case ID_POPUP_PCB_SELECT_WIDTH14:
    case ID_POPUP_PCB_SELECT_WIDTH15:
    case ID_POPUP_PCB_SELECT_WIDTH16:
        m_canvas->MoveCursorToCrossHair();
        GetDesignSettings().m_UseConnectedTrackWidth = false;
        ii = id - ID_POPUP_PCB_SELECT_WIDTH1;
        GetDesignSettings().SetTrackWidthIndex( ii );
        break;

    case ID_POPUP_PCB_SELECT_VIASIZE1:   // this is the default Netclass selection
    case ID_POPUP_PCB_SELECT_VIASIZE2:   // this is a custom value selection
    case ID_POPUP_PCB_SELECT_VIASIZE3:
    case ID_POPUP_PCB_SELECT_VIASIZE4:
    case ID_POPUP_PCB_SELECT_VIASIZE5:
    case ID_POPUP_PCB_SELECT_VIASIZE6:
    case ID_POPUP_PCB_SELECT_VIASIZE7:
    case ID_POPUP_PCB_SELECT_VIASIZE8:
    case ID_POPUP_PCB_SELECT_VIASIZE9:
    case ID_POPUP_PCB_SELECT_VIASIZE10:
    case ID_POPUP_PCB_SELECT_VIASIZE11:
    case ID_POPUP_PCB_SELECT_VIASIZE12:
    case ID_POPUP_PCB_SELECT_VIASIZE13:
    case ID_POPUP_PCB_SELECT_VIASIZE14:
    case ID_POPUP_PCB_SELECT_VIASIZE15:
    case ID_POPUP_PCB_SELECT_VIASIZE16:
        // select the new current value for via size (via diameter)
        m_canvas->MoveCursorToCrossHair();
        ii = id - ID_POPUP_PCB_SELECT_VIASIZE1;
        GetDesignSettings().SetViaSizeIndex( ii );
        break;

    case ID_AUX_TOOLBAR_PCB_TRACK_WIDTH:
        ii = m_SelTrackWidthBox->GetCurrentSelection();
        GetDesignSettings().SetTrackWidthIndex( ii );
        break;

    case ID_AUX_TOOLBAR_PCB_VIA_SIZE:
        ii = m_SelViaSizeBox->GetCurrentSelection();
        GetDesignSettings().SetViaSizeIndex( ii );
        break;

    default:
        wxMessageBox( wxT( "PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event() error") );
        break;
    }

    // Refresh track in progress, if any, by forcing a mouse event,
    // to call the current function attached to the mouse
    /*if( m_canvas->IsMouseCaptured() )
    {
        wxMouseEvent event(wxEVT_MOTION);
        wxPostEvent( m_canvas, event );
    }*/
    //+hp
    //Refresh canvas, that we can see changes instantly. I use this because it dont,t throw  mouse up-left corner.
    m_canvas->Refresh();
}
