/****************************************************************
 *   toolbars_update_user_interface.cpp
 ****************************************************************/

/*
 *  function to update toolbars UI after changing parameters
 */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "bitmaps.h"
#include "pcbnew_id.h"
#include "drc_stuff.h"
#include "3d_viewer.h"
#include "class_board_design_settings.h"
#include "dialog_helpers.h"


/**
 * Function OnUpdateAuxilaryToolbar
 * update the displayed values on auxiliary horizontal toolbar
 * (track width, via sizes, clearance ...
 * Display format for track and via lists
 *    first item = current selected class value
 *    next items (if any) = ordered list of sizes (extra sizes).
 *    So the current selected class value can be same as an other extra value
 */
void WinEDA_PcbFrame::OnUpdateAuxilaryToolbar( wxUpdateUIEvent& aEvent )
{
    wxString msg;

    if( m_AuxiliaryToolBar == NULL )
        return;

    aEvent.Check( GetBoard()->GetBoardDesignSettings()->m_UseConnectedTrackWidth );

    if( GetBoard()->m_TrackWidthSelector >= GetBoard()->m_TrackWidthList.size() )
        GetBoard()->m_TrackWidthSelector = 0;

    if( m_SelTrackWidthBox->GetSelection() != (int) GetBoard()->m_TrackWidthSelector )
        m_SelTrackWidthBox->SetSelection( GetBoard()->m_TrackWidthSelector );

    if( GetBoard()->m_ViaSizeSelector >= GetBoard()->m_ViasDimensionsList.size() )
        GetBoard()->m_ViaSizeSelector = 0;

    if( m_SelViaSizeBox->GetSelection() != (int) GetBoard()->m_ViaSizeSelector )
        m_SelViaSizeBox->SetSelection( GetBoard()->m_ViaSizeSelector );
}


void WinEDA_PcbFrame::OnUpdateZoneDisplayStyle( wxUpdateUIEvent& aEvent )
{
    int selected = aEvent.GetId() - ID_TB_OPTIONS_SHOW_ZONES;

    if( aEvent.IsChecked() && ( DisplayOpt.DisplayZonesMode == selected ) )
        return;

    aEvent.Check( DisplayOpt.DisplayZonesMode == selected );
}


void WinEDA_PcbFrame::OnUpdateDrcEnable( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !Drc_On );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_DRC_OFF,
                                        Drc_On ?
                                        _( "Disable design rule checking" ) :
                                        _( "Enable design rule checking" ) );
}

void WinEDA_PcbFrame::OnUpdateShowBoardRatsnest( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_RATSNEST,
                                        GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) ?
                                        _( "Hide board ratsnest" ) :
                                        _( "Show board ratsnest" ) );
}


void WinEDA_PcbFrame::OnUpdateShowModuleRatsnest( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( g_Show_Module_Ratsnest );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST,
                                        g_Show_Module_Ratsnest ?
                                        _( "Hide module ratsnest" ) :
                                        _( "Show module ratsnest" ) );
}


void WinEDA_PcbFrame::OnUpdateAutoDeleteTrack( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( g_AutoDeleteOldTrack );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_AUTO_DEL_TRACK,
                                        g_AutoDeleteOldTrack ?
                                        _( "Disable auto delete old track" ) :
                                        _( "Enable auto delete old track" ) );
}


void WinEDA_PcbFrame::OnUpdateViaDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayViaFill );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_VIAS_SKETCH,
                                        m_DisplayViaFill ?
                                        _( "Show vias in outline mode" ) :
                                        _( "Show vias in fill mode" ) );
}


void WinEDA_PcbFrame::OnUpdateTraceDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayPcbTrackFill );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                                        m_DisplayPcbTrackFill ?
                                        _( "Show tracks in outline mode" ) :
                                        _( "Show tracks in fill mode" ) );
}


void WinEDA_PcbFrame::OnUpdateHighContrastDisplayMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( DisplayOpt.ContrastModeDisplay );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                                        DisplayOpt.ContrastModeDisplay ?
                                        _( "Normal contrast display mode" ) :
                                        _( "High contrast display mode" ) );
}


void WinEDA_PcbFrame::OnUpdateShowLayerManager( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_auimgr.GetPane( wxT( "m_AuxVToolBar" ) ).IsShown() );
}


void WinEDA_PcbFrame::OnUpdateSave( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetScreen()->IsModify() );
}


void WinEDA_PcbFrame::OnUpdateVerticalToolbar( wxUpdateUIEvent& aEvent )
{
    if( m_ID_current_state == 0 )
        m_ID_current_state = ID_PCB_NO_TOOL;

    if( aEvent.GetEventObject() == m_VToolBar )
        aEvent.Check( m_ID_current_state == aEvent.GetId() );
}
