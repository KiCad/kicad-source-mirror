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

/* helper to convert an integer value to a string, using mils or mm
 * according to g_UserUnit value
 */
static wxString ReturnStringValue( int aValue )
{
    wxString      text;
    const wxChar* format;
    double        value = To_User_Unit( g_UserUnit, aValue, PCB_INTERNAL_UNIT );

    if( g_UserUnit == INCHES )
    {
        format = wxT( " %.1f" );
        value *= 1000;
    }
    else
        format = wxT( " %.3f" );
    text.Printf( format, value );
    if( g_UserUnit == INCHES )
        text += _( " mils" );
    else
        text += _( " mm" );
    return text;
}


/**
 * Function AuxiliaryToolBar_DesignRules_Update_UI
 * update the displayed values: track widths, via sizes, clearance, Netclass name
 * used when a netclass is selected
 */
void WinEDA_PcbFrame::AuxiliaryToolBar_DesignRules_Update_UI()
{
    wxString nclname = GetBoard()->m_CurrentNetClassName;
    wxString msg     = _( "NetClass: " ) + nclname;

    m_NetClassSelectedBox->Clear();
    m_NetClassSelectedBox->AppendText( msg );

    NETCLASS* netclass = GetBoard()->m_NetClasses.Find( nclname );

    if( m_ClearanceBox )
    {
        wxString msg = _( "Clearance" ) + ReturnStringValue( netclass->GetClearance() );
        m_ClearanceBox->Clear();
        m_ClearanceBox->AppendText( msg );
    }
}


/**
 * Function AuxiliaryToolBar_Update_UI
 * update the displayed values on auxiliary horizontal toolbar
 * (track width, via sizes, clearance ...
 * Display format for track and via lists
 *    first item = current selected class value
 *    next items (if any) = ordered list of sizes (extra sizes).
 *    So the current selected class value can be same as an other extra value
 */
void WinEDA_PcbFrame::AuxiliaryToolBar_Update_UI()
{
    wxString msg;

    AuxiliaryToolBar_DesignRules_Update_UI();

    m_AuxiliaryToolBar->ToggleTool( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH,
                                    GetBoard()->GetBoardDesignSettings()->m_UseConnectedTrackWidth );

    if( m_SelTrackWidthBox && m_TrackAndViasSizesList_Changed )
    {
        m_SelTrackWidthBox->Clear();
        for( unsigned ii = 0; ii < GetBoard()->m_TrackWidthList.size(); ii++ )
        {
            msg = _( "Track" ) + ReturnStringValue( GetBoard()->m_TrackWidthList[ii] );
            if( ii == 0 )
                msg << _( " *" );
            m_SelTrackWidthBox->Append( msg );
        }
    }
    if( GetBoard()->m_TrackWidthSelector >= GetBoard()->m_TrackWidthList.size() )
        GetBoard()->m_TrackWidthSelector = 0;
    m_SelTrackWidthBox->SetSelection( GetBoard()->m_TrackWidthSelector );

    if( m_SelViaSizeBox && m_TrackAndViasSizesList_Changed )
    {
        m_SelViaSizeBox->Clear();
        for( unsigned ii = 0; ii < GetBoard()->m_ViasDimensionsList.size(); ii++ )
        {
            msg = _( "Via" );
            msg << ReturnStringValue( GetBoard()->m_ViasDimensionsList[ii].m_Diameter );

            if( GetBoard()->m_ViasDimensionsList[ii].m_Drill )
                msg  << wxT("/") << ReturnStringValue( GetBoard()->m_ViasDimensionsList[ii].m_Drill );

            if( ii == 0 )
                msg << _( " *" );
            m_SelViaSizeBox->Append( msg );
        }
    }
    if( GetBoard()->m_ViaSizeSelector >= GetBoard()->m_ViasDimensionsList.size() )
        GetBoard()->m_ViaSizeSelector = 0;
    m_SelViaSizeBox->SetSelection( GetBoard()->m_ViaSizeSelector );

    if( m_SelZoomBox )
    {
        bool not_found = true;
        for( unsigned jj = 0; jj < GetScreen()->m_ZoomList.GetCount(); jj++ )
        {
            if( GetScreen()->GetZoom() == GetScreen()->m_ZoomList[jj] )
            {
                m_SelZoomBox->SetSelection( jj + 1 );
                not_found = false;
                break;
            }
        }

        if( not_found )
            m_SelZoomBox->SetSelection( -1 );
    }

    if( m_SelGridBox )
        m_SelGridBox->SetSelection( m_LastGridSizeId );

    m_TrackAndViasSizesList_Changed = false;

    m_AuxiliaryToolBar->Refresh();

}


/*
 * Enable or disable the toolbar's controls, depending on the current
 * state.
 *
 * @todo: All of this should be perform in appropriate wxUpdateUIEvent
 *        handles.  This is not how it how updating user interface controls
 *        is handle in wxWidgets.
 */
void WinEDA_PcbFrame::SetToolbars()
{
    bool state;

    if( m_ID_current_state == ID_TRACK_BUTT )
    {
        if( Drc_On )
            DrawPanel->SetCursor( wxCursor( wxCURSOR_PENCIL ) );
        else
            DrawPanel->SetCursor( wxCursor( wxCURSOR_QUESTION_ARROW ) );
    }


    if( m_HToolBar == NULL )
        return;

    m_HToolBar->EnableTool( ID_SAVE_BOARD, GetScreen()->IsModify() );

    state = GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE;
    m_HToolBar->EnableTool( wxID_CUT, state );
    m_HToolBar->EnableTool( wxID_COPY, state );

    m_HToolBar->EnableTool( wxID_PASTE, false );

    state = GetScreen()->GetUndoCommandCount() > 0;
    m_HToolBar->EnableTool( wxID_UNDO, state );

    state = GetScreen()->GetRedoCommandCount() > 0;
    m_HToolBar->EnableTool( wxID_REDO, state );
    syncLayerBox();
    PrepareLayerIndicator();
    m_HToolBar->Refresh(true);

    if( m_OptionsToolBar )
    {
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_DRC_OFF,
                                      !Drc_On );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_DRC_OFF,
                                            Drc_On ?
                                            _( "Disable design rule checking" ) :
                                            _( "Enable design rule checking" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_MM,
                                      g_UserUnit == MILLIMETRES );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH,
                                      g_UserUnit == INCHES );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                      DisplayOpt.DisplayPolarCood );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                            DisplayOpt.DisplayPolarCood ?
                                            _( "Display rectangular coordinates" ) :
                                            _( "Display polar coordinates" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID, IsGridVisible( ) );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID,
                                            IsGridVisible( ) ?
                                            _( "Hide grid" ) :
                                            _( "Show grid" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_CURSOR,
                                      m_CursorShape );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_RATSNEST,
                                      GetBoard()->IsElementVisible(RATSNEST_VISIBLE) );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_RATSNEST,
                                            GetBoard()->IsElementVisible(RATSNEST_VISIBLE) ?
                                            _( "Hide board ratsnest" ) :
                                            _( "Show board ratsnest" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST,
                                      g_Show_Module_Ratsnest );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST,
                                            g_Show_Module_Ratsnest ?
                                            _( "Hide module ratsnest" ) :
                                            _( "Show module ratsnest" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_AUTO_DEL_TRACK,
                                      g_AutoDeleteOldTrack );

        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_AUTO_DEL_TRACK,
                                            g_AutoDeleteOldTrack ?
                                            _( "Disable auto delete old track" ) :
                                            _( "Enable auto delete old track" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                      !m_DisplayPadFill );

        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                            m_DisplayPadFill ?
                                            _( "Show pads in outline mode" ) :
                                            _( "Show pads in fill mode" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_VIAS_SKETCH,
                                      !m_DisplayViaFill );

        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_VIAS_SKETCH,
                                            m_DisplayViaFill ?
                                            _( "Show vias in outline mode" ) :
                                            _( "Show vias in fill mode" ) );


        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                                      !m_DisplayPcbTrackFill );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                                            m_DisplayPcbTrackFill ?
                                            _( "Show tracks in outline mode" ) :
                                            _( "Show tracks in fill mode" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                                      DisplayOpt.ContrastModeDisplay );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                                            DisplayOpt.ContrastModeDisplay ?
                                            _( "Normal contrast display mode" ) :
                                            _( "High contrast display mode" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR1,
                                      m_auimgr.GetPane(wxT("m_AuxVToolBar")).IsShown() );
        m_OptionsToolBar->Refresh();
    }

    if( m_AuxiliaryToolBar )
        AuxiliaryToolBar_Update_UI();
    DisplayUnitsMsg();
}
