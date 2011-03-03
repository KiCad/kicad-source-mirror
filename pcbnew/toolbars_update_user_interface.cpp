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


void PCB_EDIT_FRAME::OnUpdateLayerPair( wxUpdateUIEvent& aEvent )
{
    PrepareLayerIndicator();
}


void PCB_EDIT_FRAME::OnUpdateSelectTrackWidth( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetId() == ID_AUX_TOOLBAR_PCB_TRACK_WIDTH )
    {
        if( m_SelTrackWidthBox->GetSelection() != (int) GetBoard()->m_TrackWidthSelector )
            m_SelTrackWidthBox->SetSelection( GetBoard()->m_TrackWidthSelector );
    }
    else
    {
        bool check = ( ( ( ID_POPUP_PCB_SELECT_WIDTH1 +
                           (int) GetBoard()->m_TrackWidthSelector ) == aEvent.GetId() ) &&
                       !GetBoard()->GetBoardDesignSettings()->m_UseConnectedTrackWidth );
        aEvent.Check( check );
    }
}


void PCB_EDIT_FRAME::OnUpdateSelectAutoTrackWidth( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( GetBoard()->GetBoardDesignSettings()->m_UseConnectedTrackWidth );
}


void PCB_EDIT_FRAME::OnUpdateSelectViaSize( wxUpdateUIEvent& aEvent )
{
    wxString msg;

    if( aEvent.GetId() == ID_AUX_TOOLBAR_PCB_VIA_SIZE )
    {
        if( m_SelViaSizeBox->GetSelection() != (int) GetBoard()->m_ViaSizeSelector )
            m_SelViaSizeBox->SetSelection( GetBoard()->m_ViaSizeSelector );
    }
    else
    {
        bool check = ( ( ( ID_POPUP_PCB_SELECT_VIASIZE1 +
                           (int) GetBoard()->m_ViaSizeSelector ) == aEvent.GetId() ) &&
                       !GetBoard()->GetBoardDesignSettings()->m_UseConnectedTrackWidth );

        aEvent.Check( check );
    }
}


void PCB_EDIT_FRAME::OnUpdateLayerSelectBox( wxUpdateUIEvent& aEvent )
{
    m_SelLayerBox->SetLayerSelection( getActiveLayer() );
}


void PCB_EDIT_FRAME::OnUpdateZoneDisplayStyle( wxUpdateUIEvent& aEvent )
{
    int selected = aEvent.GetId() - ID_TB_OPTIONS_SHOW_ZONES;

    if( aEvent.IsChecked() && ( DisplayOpt.DisplayZonesMode == selected ) )
        return;

    aEvent.Check( DisplayOpt.DisplayZonesMode == selected );
}


void PCB_EDIT_FRAME::OnUpdateDrcEnable( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !Drc_On );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_DRC_OFF,
                                        Drc_On ?
                                        _( "Disable design rule checking" ) :
                                        _( "Enable design rule checking" ) );
}

void PCB_EDIT_FRAME::OnUpdateShowBoardRatsnest( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_RATSNEST,
                                        GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) ?
                                        _( "Hide board ratsnest" ) :
                                        _( "Show board ratsnest" ) );
}


void PCB_EDIT_FRAME::OnUpdateShowModuleRatsnest( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( g_Show_Module_Ratsnest );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST,
                                        g_Show_Module_Ratsnest ?
                                        _( "Hide module ratsnest" ) :
                                        _( "Show module ratsnest" ) );
}


void PCB_EDIT_FRAME::OnUpdateAutoDeleteTrack( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( g_AutoDeleteOldTrack );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_AUTO_DEL_TRACK,
                                        g_AutoDeleteOldTrack ?
                                        _( "Disable auto delete old track" ) :
                                        _( "Enable auto delete old track" ) );
}


void PCB_EDIT_FRAME::OnUpdateViaDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayViaFill );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_VIAS_SKETCH,
                                        m_DisplayViaFill ?
                                        _( "Show vias in outline mode" ) :
                                        _( "Show vias in fill mode" ) );
}


void PCB_EDIT_FRAME::OnUpdateTraceDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayPcbTrackFill );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                                        m_DisplayPcbTrackFill ?
                                        _( "Show tracks in outline mode" ) :
                                        _( "Show tracks in fill mode" ) );
}


void PCB_EDIT_FRAME::OnUpdateHighContrastDisplayMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( DisplayOpt.ContrastModeDisplay );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                                        DisplayOpt.ContrastModeDisplay ?
                                        _( "Normal contrast display mode" ) :
                                        _( "High contrast display mode" ) );
}


void PCB_EDIT_FRAME::OnUpdateShowLayerManager( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_auimgr.GetPane( wxT( "m_LayersManagerToolBar" ) ).IsShown() );
}

void PCB_EDIT_FRAME::OnUpdateShowMicrowaveToolbar( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_auimgr.GetPane( wxT( "m_AuxVToolBar" ) ).IsShown() );
}


void PCB_EDIT_FRAME::OnUpdateSave( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetScreen()->IsModify() );
}


void PCB_EDIT_FRAME::OnUpdateVerticalToolbar( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_VToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}
