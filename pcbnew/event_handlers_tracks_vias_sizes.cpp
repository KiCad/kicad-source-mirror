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

/** Function Tracks_and_Vias_Size_Event
 * Event handler for tracks and vias size selection (and some options)
 * relative to toolbars and popup events
 */
void WinEDA_PcbFrame::Tracks_and_Vias_Size_Event( wxCommandEvent& event )
{
    int ii;
    int id = event.GetId();

/* Note: none of these events require aborting the current command (if any)
 * (like move, edit or block command)
 * so we do not test for a current command in progress and call
 *  DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
 */
    switch( id )
    {
    case ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH:
        g_DesignSettings.m_UseConnectedTrackWidth = not g_DesignSettings.m_UseConnectedTrackWidth;
        g_DesignSettings.m_CurrentTrackWidth = GetBoard()->m_TrackWidthList[m_SelTrackWidthBox->GetChoice()];
        g_DesignSettings.m_CurrentViaSize = GetBoard()->m_ViaSizeList[m_SelViaSizeBox->GetChoice()];
        AuxiliaryToolBar_Update_UI( );
        break;

    case ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES:
        g_DesignSettings.m_UseConnectedTrackWidth = false;
        GetBoard()->m_TrackWidthSelector = 0;
        g_DesignSettings.m_CurrentTrackWidth = GetBoard()->m_TrackWidthList[0];
        GetBoard()->m_ViaSizeSelector = 0;
        g_DesignSettings.m_CurrentViaSize = GetBoard()->m_ViaSizeList[0];
        AuxiliaryToolBar_Update_UI( );
        break;

    case ID_POPUP_PCB_SELECT_AUTO_WIDTH:
        DrawPanel->MouseToCursorSchema();
        g_DesignSettings.m_UseConnectedTrackWidth = true;
        AuxiliaryToolBar_Update_UI( );
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
        g_DesignSettings.m_UseConnectedTrackWidth = false;
        ii = id - ID_POPUP_PCB_SELECT_WIDTH1;
        GetBoard()->m_TrackWidthSelector = ii;
        g_DesignSettings.m_CurrentTrackWidth = GetBoard()->m_TrackWidthList[ii];
        AuxiliaryToolBar_Update_UI( );
        break;

    case ID_POPUP_PCB_SELECT_VIASIZE1:
    case ID_POPUP_PCB_SELECT_VIASIZE2:
    case ID_POPUP_PCB_SELECT_VIASIZE3:
    case ID_POPUP_PCB_SELECT_VIASIZE4:
    case ID_POPUP_PCB_SELECT_VIASIZE5:
    case ID_POPUP_PCB_SELECT_VIASIZE6:
    case ID_POPUP_PCB_SELECT_VIASIZE7:
    case ID_POPUP_PCB_SELECT_VIASIZE8:      // selec the new current value for via size (via diameter)
        DrawPanel->MouseToCursorSchema();
        ii = id - ID_POPUP_PCB_SELECT_VIASIZE1;
        GetBoard()->m_ViaSizeSelector = ii;
        g_DesignSettings.m_CurrentViaSize = GetBoard()->m_ViaSizeList[ii];
        AuxiliaryToolBar_Update_UI( );
        break;


    case ID_AUX_TOOLBAR_PCB_TRACK_WIDTH:
        ii = m_SelTrackWidthBox->GetChoice();
        g_DesignSettings.m_CurrentTrackWidth = GetBoard()->m_TrackWidthList[ii];
        GetBoard()->m_TrackWidthSelector = ii;
        break;

    case ID_AUX_TOOLBAR_PCB_VIA_SIZE:
        ii = m_SelViaSizeBox->GetChoice();
        g_DesignSettings.m_CurrentViaSize = GetBoard()->m_ViaSizeList[ii];
        GetBoard()->m_ViaSizeSelector = ii;
        break;

    default:
        wxMessageBox( wxT( "WinEDA_PcbFrame::Tracks_and_Vias_Size_Event() error") );
        break;
    }
}
