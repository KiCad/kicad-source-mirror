/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_general_options.cpp
// Author:      jean-pierre Charras
/////////////////////////////////////////////////////////////////////////////

/* functions relatives to the dialogs opened from the main menu :
 *   Preferences/general
 *   Preferences/display
 */
#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "dialog_general_options.h"

#include "pcbnew_id.h"


Dialog_GeneralOptions::Dialog_GeneralOptions( WinEDA_PcbFrame* parent ) :
    DialogGeneralOptionsBoardEditor_base( parent )
{
    m_Parent = parent;
    init();

    GetSizer()->SetSizeHints( this );
    Center();
}


void Dialog_GeneralOptions::init()
{
    SetFocus();

    m_Board = m_Parent->GetBoard();

    /* Set display options */
    m_PolarDisplay->SetSelection( DisplayOpt.DisplayPolarCood ? 1 : 0 );
    m_UnitsSelection->SetSelection( g_UnitMetric ? 1 : 0 );
    m_CursorShape->SetSelection( m_Parent->m_CursorShape ? 1 : 0 );

    wxString timevalue;
    timevalue << g_TimeOut / 60;
    m_SaveTime->SetValue( timevalue );
    m_MaxShowLinks->SetValue( g_MaxLinksShowed );

    m_DrcOn->SetValue( Drc_On );
    m_ShowModuleRatsnest->SetValue( g_Show_Module_Ratsnest );
    m_ShowGlobalRatsnest->SetValue( m_Board->IsElementVisible(RATSNEST_VISIBLE) );
    m_TrackAutodel->SetValue( g_AutoDeleteOldTrack );
    m_Track_45_Only_Ctrl->SetValue( Track_45_Only );
    m_Segments_45_Only_Ctrl->SetValue( Segments_45_Only );
    m_AutoPANOpt->SetValue( m_Parent->DrawPanel->m_AutoPAN_Enable );
    m_Segments_45_Only_Ctrl->SetValue( Segments_45_Only );
    m_Track_DoubleSegm_Ctrl->SetValue( g_TwoSegmentTrackBuild );

    m_MagneticPadOptCtrl->SetSelection( g_MagneticPadOption );
    m_MagneticTrackOptCtrl->SetSelection( g_MagneticTrackOption );
}


void Dialog_GeneralOptions::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}


void Dialog_GeneralOptions::OnOkClick( wxCommandEvent& event )
{
    int ii;

    DisplayOpt.DisplayPolarCood =
        ( m_PolarDisplay->GetSelection() == 0 ) ? FALSE : true;
    ii = g_UnitMetric;
    g_UnitMetric = ( m_UnitsSelection->GetSelection() == 0 )  ? 0 : 1;
    if( ii != g_UnitMetric )
        m_Parent->ReCreateAuxiliaryToolbar();

    m_Parent->m_CursorShape = m_CursorShape->GetSelection();
    g_TimeOut = 60 * m_SaveTime->GetValue();

    /* Updating the combobox to display the active layer. */
    g_MaxLinksShowed = m_MaxShowLinks->GetValue();
    Drc_On = m_DrcOn->GetValue();
    if( m_Board->IsElementVisible(RATSNEST_VISIBLE) != m_ShowGlobalRatsnest->GetValue() )
    {
        m_Parent->SetElementVisibility(RATSNEST_VISIBLE, m_ShowGlobalRatsnest->GetValue() );
        m_Parent->DrawPanel->Refresh( );
    }
    g_Show_Module_Ratsnest = m_ShowModuleRatsnest->GetValue();
    g_AutoDeleteOldTrack   = m_TrackAutodel->GetValue();
    Segments_45_Only = m_Segments_45_Only_Ctrl->GetValue();
    Track_45_Only    = m_Track_45_Only_Ctrl->GetValue();
    m_Parent->DrawPanel->m_AutoPAN_Enable = m_AutoPANOpt->GetValue();
    g_TwoSegmentTrackBuild = m_Track_DoubleSegm_Ctrl->GetValue();

    g_MagneticPadOption   = m_MagneticPadOptCtrl->GetSelection();
    g_MagneticTrackOption = m_MagneticTrackOptCtrl->GetSelection();

    EndModal( 1 );
}


#include "dialog_graphic_items_options.cpp"


void WinEDA_PcbFrame::InstallPcbOptionsFrame( int id )
{
    switch( id )
    {
    case ID_PCB_DRAWINGS_WIDTHS_SETUP:
    {
        WinEDA_GraphicItemsOptionsDialog dlg( this );
        dlg.ShowModal();
    }
    break;

    default:
        wxMessageBox( wxT( "InstallPcbOptionsFrame() id error" ) );
        break;
    }
}


void WinEDA_ModuleEditFrame::InstallOptionsFrame( const wxPoint& pos )
{
    WinEDA_GraphicItemsOptionsDialog dlg( this );

    dlg.ShowModal();
}


/* Must be called on a click on the left toolbar (options toolbar
 * Update variables according to tools states
 */
void WinEDA_PcbFrame::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int id = event.GetId();
    bool state = m_OptionsToolBar->GetToolState( id );

    switch( id )
    {
    case ID_TB_OPTIONS_DRC_OFF:
        Drc_On = state ? FALSE : true;
        break;

    case ID_TB_OPTIONS_SHOW_GRID:
        SetElementVisibility(GRID_VISIBLE, state);
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_RATSNEST:
        SetElementVisibility(RATSNEST_VISIBLE, state);
        DrawPanel->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_RATSNEST:
        g_Show_Module_Ratsnest = state; // TODO: use the visibility list
        break;

    case ID_TB_OPTIONS_SELECT_UNIT_MM:
        g_UnitMetric = MILLIMETRE;

    case ID_TB_OPTIONS_SELECT_UNIT_INCH:
        if( id == ID_TB_OPTIONS_SELECT_UNIT_INCH )
            g_UnitMetric = INCHES;
        m_TrackAndViasSizesList_Changed = true;
        UpdateStatusBar();
        ReCreateAuxiliaryToolbar();
        DisplayUnitsMsg();
        break;

    case ID_TB_OPTIONS_SHOW_POLAR_COORD:
        Affiche_Message( wxEmptyString );
        DisplayOpt.DisplayPolarCood = state;
        UpdateStatusBar();
        break;

    case ID_TB_OPTIONS_SELECT_CURSOR:
        m_CursorShape = state;
        break;

    case ID_TB_OPTIONS_AUTO_DEL_TRACK:
        g_AutoDeleteOldTrack = state;
        break;

    case ID_TB_OPTIONS_SHOW_ZONES:
        DisplayOpt.DisplayZonesMode = 0;
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_ZONES_DISABLE:
        DisplayOpt.DisplayZonesMode = 1;
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_ZONES_OUTLINES_ONLY:
        DisplayOpt.DisplayZonesMode = 2;
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_PADS_SKETCH:
        if( state )
        {
            m_DisplayPadFill = DisplayOpt.DisplayPadFill = false;
        }
        else
        {
            m_DisplayPadFill = DisplayOpt.DisplayPadFill = true;
        }
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_VIAS_SKETCH:
        if( state )
        {
            m_DisplayViaFill = DisplayOpt.DisplayViaFill = false;
        }
        else
        {
            m_DisplayViaFill = DisplayOpt.DisplayViaFill = true;
        }
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_TRACKS_SKETCH:
        m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill = !state;
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE:
        DisplayOpt.ContrastModeDisplay = state;
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR1:
        m_show_microwave_tools = state;
        m_auimgr.GetPane( wxT( "m_AuxVToolBar" ) ).Show( m_show_microwave_tools );
        m_auimgr.Update();
        break;

    case ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR:
        // show auxiliary Vertical layers and visibility manager toolbar
        m_show_layer_manager_tools = state;
        m_auimgr.GetPane( wxT( "m_LayersManagerToolBar" ) ).Show( m_show_layer_manager_tools );
        m_auimgr.Update();
        if( m_show_layer_manager_tools )
            GetMenuBar()->SetLabel(ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                                _("Hide &Layers Manager" ) );
        else
            GetMenuBar()->SetLabel(ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                                _("Show &Layers Manager" ) );
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_PcbFrame::OnSelectOptionToolbar error \n (event not handled!)" ) );
        break;
    }

    SetToolbars();
}
