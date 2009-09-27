/****************************************************************
toolbars_update_user_interface.cpp
****************************************************************/
/*
function to update toolbars UI after changing parameters
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

/* helper to convert an integer value to a string, using mils or mm
 * according to g_UnitMetric value
 */
static wxString ReturnStringValue(int aValue)
{
    wxString text;
    const wxChar * format;
    double value = To_User_Unit( g_UnitMetric, aValue, PCB_INTERNAL_UNIT );
    if( g_UnitMetric == INCHES )
    {
        format = wxT( " %.1f" );
        value *= 1000;
    }
    else
        format = wxT( " %.3f" );
    text.Printf( format, value );
    if( g_UnitMetric == INCHES )
        text += _(" mils");
    else
        text += _(" mm");
    return text;
}

/**
 * Function AuxiliaryToolBar_DesignRules_Update_UI
 * update the displayed values: track widths, via sizes, clearance, Netclass name
 * used when a netclass is selected
 */
void WinEDA_PcbFrame::AuxiliaryToolBar_DesignRules_Update_UI( )
{
    wxString nclname = GetBoard()->m_CurrentNetClassName;
    wxString msg = _("NetClass: ") + nclname;
    m_NetClassSelectedBox->Clear();
    m_NetClassSelectedBox->AppendText( msg );

    NETCLASS* netclass = GetBoard()->m_NetClasses.Find( nclname );

    if( m_ClearanceBox )
    {
        wxString msg = _( "Clearance" ) + ReturnStringValue(netclass->GetClearance());
        m_ClearanceBox->Clear();
        m_ClearanceBox->AppendText( msg );
    }

}


/**
 * Function AuxiliaryToolBar_Update_UI
 * update the displayed values on auxiliary horizontal toolbar
 * (track width, via sizes, clearance ...
 */
void WinEDA_PcbFrame::AuxiliaryToolBar_Update_UI( )
{
    wxString msg;
    m_AuxiliaryToolBar->ToggleTool( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH,
                                    g_DesignSettings.m_UseConnectedTrackWidth );
    if( m_SelTrackWidthBox && m_SelTrackWidthBox_Changed )
    {
        m_SelTrackWidthBox_Changed = false;
        m_SelTrackWidthBox->Clear();

        for( int ii = 0; ii < HISTORY_NUMBER; ii++ )
        {
            if( g_DesignSettings.m_TrackWidthHistory[ii] == 0 )
                break; // Fin de liste
            msg = _( "Track" ) + ReturnStringValue(g_DesignSettings.m_TrackWidthHistory[ii]);

            m_SelTrackWidthBox->Append( msg );

            if( g_DesignSettings.m_TrackWidthHistory[ii] ==
                g_DesignSettings.m_CurrentTrackWidth )
                m_SelTrackWidthBox->SetSelection( ii );
        }
    }

    AuxiliaryToolBar_DesignRules_Update_UI( );

    if( m_SelViaSizeBox && m_SelViaSizeBox_Changed )
    {
        m_SelViaSizeBox_Changed = false;
        m_SelViaSizeBox->Clear();

        for( int ii = 0; ii < HISTORY_NUMBER; ii++ )
        {
            if( g_DesignSettings.m_ViaSizeHistory[ii] == 0 )
                break; // Fin de liste

            msg = _( "Via" ) + ReturnStringValue(g_DesignSettings.m_ViaSizeHistory[ii]);

            m_SelViaSizeBox->Append( msg );
            if( g_DesignSettings.m_ViaSizeHistory[ii] == g_DesignSettings.m_CurrentViaSize )
                m_SelViaSizeBox->SetSelection( ii );
        }
    }

    if( m_SelZoomBox )
    {
        bool not_found = true;
        for( int jj = 0; jj < (int)GetScreen()->m_ZoomList.GetCount(); jj++ )
        {
            if( GetScreen()->GetZoom() == GetScreen()->m_ZoomList[jj] )
            {
                m_SelZoomBox->SetSelection( jj + 1 );
                not_found = false;
                break;
            }
        }
        if ( not_found )
            m_SelZoomBox->SetSelection( -1 );
    }

    if( m_SelGridBox && GetScreen() )
    {
        int kk = m_SelGridBox->GetChoice();

        for( int ii = 0; ii < (int) GetScreen()->m_GridList.GetCount(); ii++ )
        {
            if( GetScreen()->GetGrid() == GetScreen()->m_GridList[ii].m_Size )
            {
                if( kk != ii )
                    m_SelGridBox->SetSelection( ii );
                kk = ii;
                break;
            }
        }
    }
}


/***************************************/
void WinEDA_PcbFrame::SetToolbars()
/***************************************/

/*
 *  Active ou desactive les tools des toolbars, en fonction des commandes
 *  en cours
 */
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
    m_HToolBar->EnableTool( ID_UNDO_BUTT, state );

    state = GetScreen()->GetRedoCommandCount() > 0;
    m_HToolBar->EnableTool( ID_REDO_BUTT, state );

    if( m_OptionsToolBar )
    {
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_DRC_OFF,
                                      !Drc_On );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_DRC_OFF,
                                            Drc_On ?
                                            _( "DRC Off (Disable !!!), Currently: DRC is active" ) :
                                            _( "DRC On (Currently: DRC is inactive !!!)" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_MM,
                                      g_UnitMetric == MILLIMETRE ? TRUE : false );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH,
                                      g_UnitMetric == INCHES ? TRUE : false );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                      DisplayOpt.DisplayPolarCood );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                            DisplayOpt.DisplayPolarCood ?
                                            _( "Polar Coords not show" ) :
                                            _( "Display Polar Coords" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID,
                                      m_Draw_Grid );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID,
                                            m_Draw_Grid ? _( "Grid not show" ) : _( "Show Grid" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_CURSOR,
                                      m_CursorShape );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_RATSNEST,
                                      g_Show_Ratsnest );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_RATSNEST,
                                            g_Show_Ratsnest ?
                                            _( "Hide General ratsnest" ) :
                                            _( "Show General ratsnest" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST,
                                      g_Show_Module_Ratsnest );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST,
                                            g_Show_Module_Ratsnest ?
                                            _( "Hide Module ratsnest" ) :
                                            _( "Show Module ratsnest" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_AUTO_DEL_TRACK,
                                      g_AutoDeleteOldTrack );

        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_AUTO_DEL_TRACK,
                                            g_AutoDeleteOldTrack ?
                                            _( "Disable Auto Delete old Track" ) :
                                            _( "Enable Auto Delete old Track" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                      !m_DisplayPadFill );

        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                            m_DisplayPadFill ?
                                            _( "Show Pads Sketch mode" ) :
                                            _( "Show pads filled mode" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                                      !m_DisplayPcbTrackFill );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                                            m_DisplayPcbTrackFill ?
                                            _( "Show Tracks Sketch mode" ) :
                                            _( "Show Tracks filled mode" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                                      DisplayOpt.ContrastModeDisplay );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                                            DisplayOpt.ContrastModeDisplay ?
                                            _( "Normal Contrast Mode Display" ) :
                                            _( "High Contrast Mode Display" ) );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_INVISIBLE_TEXT_MODE,
                g_ModuleTextNOVColor & ITEM_NOT_SHOW );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_INVISIBLE_TEXT_MODE,
                       g_ModuleTextNOVColor & (ITEM_NOT_SHOW) ?
                                                   _( "Show Invisible Text" ) :
                                                   _( "Hide Invisible Text" ) );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR1, m_AuxVToolBar ? true : false );
    }

    if( m_AuxiliaryToolBar )
        AuxiliaryToolBar_Update_UI( );

    UpdateToolbarLayerInfo();
    PrepareLayerIndicator();
    DisplayUnitsMsg();
}
