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

    /* Set display options */
    m_PolarDisplay->SetSelection( DisplayOpt.DisplayPolarCood ? 1 : 0 );
    m_UnitsSelection->SetSelection( g_UnitMetric ? 1 : 0 );
    m_CursorShape->SetSelection( m_Parent->m_CursorShape ? 1 : 0 );

    wxString timevalue;
    timevalue << g_TimeOut / 60;
    m_SaveTime->SetValue( timevalue );

/*
 *   int layer_count[] = {1,2,4,6,8,10,12,14,16};
 *   m_LayerNumber->SetSelection(1);
 *   for ( unsigned ii = 0; ii < sizeof(layer_count); ii++ )
 *   {
 *       if ( g_DesignSettings.m_CopperLayerCount != layer_count[ii] )
 *           continue;
 *       m_LayerNumber->SetSelection(ii);
 *       break;
 *   }
 */
    m_MaxShowLinks->SetValue( g_MaxLinksShowed );

    m_DrcOn->SetValue( Drc_On );
    m_ShowModuleRatsnest->SetValue( g_Show_Module_Ratsnest );
    m_ShowGlobalRatsnest->SetValue( g_Show_Ratsnest );
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
    if( g_Show_Ratsnest != m_ShowGlobalRatsnest->GetValue() )
    {
        g_Show_Ratsnest = m_ShowGlobalRatsnest->GetValue();
        m_Parent->Ratsnest_On_Off( NULL );
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
 * Update variables according to the tools states
 */
void WinEDA_PcbFrame::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int id = event.GetId();


    switch( id )
    {
    case ID_TB_OPTIONS_DRC_OFF:
        Drc_On = m_OptionsToolBar->GetToolState( id ) ? FALSE : true;
        break;

    case ID_TB_OPTIONS_SHOW_GRID:
        m_Draw_Grid = m_OptionsToolBar->GetToolState( id );
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_RATSNEST:
        g_Show_Ratsnest = m_OptionsToolBar->GetToolState( id );
        Ratsnest_On_Off( NULL );
        DrawPanel->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_RATSNEST:
        g_Show_Module_Ratsnest = m_OptionsToolBar->GetToolState( id );
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
        DisplayOpt.DisplayPolarCood = m_OptionsToolBar->GetToolState( id );
        UpdateStatusBar();
        break;

    case ID_TB_OPTIONS_SELECT_CURSOR:
        m_CursorShape = m_OptionsToolBar->GetToolState( id );
        break;

    case ID_TB_OPTIONS_AUTO_DEL_TRACK:
        g_AutoDeleteOldTrack = m_OptionsToolBar->GetToolState( id );
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
        if( m_OptionsToolBar->GetToolState( id ) )
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
        if( m_OptionsToolBar->GetToolState( id ) )
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
        m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill =
                                    !m_OptionsToolBar->GetToolState( id );
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE:
        DisplayOpt.ContrastModeDisplay =
            m_OptionsToolBar->GetToolState( id );
        DrawPanel->Refresh();
        break;

    case ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR1:
        m_show_microwave_tools = m_OptionsToolBar->GetToolState( id );
#if !defined(KICAD_AUIMANAGER)
        // show auxiliary Vertical toolbar (Microwave tool)
        m_AuxVToolBar->Show(m_show_microwave_tools);
        {
        wxSizeEvent SizeEv( GetSize() );
        OnSize( SizeEv );
        }
#else
        m_auimgr.GetPane( wxT( "m_AuxVToolBar" ) ).Show( m_show_microwave_tools );
        m_auimgr.Update();
#endif
        break;

    case ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR:
#if defined(KICAD_AUIMANAGER)
        // show auxiliary Vertical layers and visibility manager toolbar
        m_show_layer_manager_tools = m_OptionsToolBar->GetToolState( id );
        m_auimgr.GetPane( wxT( "m_LayersManagerToolBar" ) ).Show( m_show_layer_manager_tools );
        m_auimgr.Update();
        if( m_show_layer_manager_tools )
            GetMenuBar()->SetLabel(ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                                _("Hide &Layers Manager" ) );
        else
            GetMenuBar()->SetLabel(ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                                _("Show &Layers Manager" ) );
#endif
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_PcbFrame::OnSelectOptionToolbar error \n (event not handled!)" ) );
        break;
    }

    SetToolbars();
}
