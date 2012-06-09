/**
 * @file pcbnew/pcbplot.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <plot_common.h>
#include <confirm.h>
#include <gestfich.h>
#include <wxPcbStruct.h>
#include <pcbplot.h>
#include <worksheet.h>
#include <pcbstruct.h>
#include <macros.h>
#include <base_units.h>

#include <class_board.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <protos.h>
#include <dialog_plot_base.h>
#include <pcb_plot_params.h>

/* Keywords to r/w options in m_config */
#define CONFIG_XFINESCALE_ADJ    wxT( "PlotXFineScaleAdj" )
#define CONFIG_YFINESCALE_ADJ    wxT( "PlotYFineScaleAdj" )
#define CONFIG_PS_FINEWIDTH_ADJ  wxT( "PSPlotFineWidthAdj" )

// Define min and max reasonable values for print scale
#define MIN_SCALE 0.01
#define MAX_SCALE 100.0


static bool setDouble( double* aDouble, double aValue, double aMin, double aMax )
{
    if( aValue < aMin )
    {
        *aDouble = aMin;
        return false;
    }
    else if( aValue > aMax )
    {
        *aDouble = aMax;
        return false;
    }

    *aDouble = aValue;
    return true;
}



/**
 * Class DIALOG_PLOT
 *
 */
class DIALOG_PLOT : public DIALOG_PLOT_BASE
{
public:
    DIALOG_PLOT( PCB_EDIT_FRAME* parent );


private:
    PCB_EDIT_FRAME*     m_parent;
    BOARD*              m_board;
    wxConfig*           m_config;
    std::vector<int>    layerList;               // List to hold CheckListBox layer numbers
    double              m_XScaleAdjust;
    double              m_YScaleAdjust;
    double              m_PSWidthAdjust;         // Global width correction for exact width postscript output.
    double              m_WidthAdjustMinValue;   // Global width correction
    double              m_WidthAdjustMaxValue;   // margins.

    PCB_PLOT_PARAMS     m_plotOpts;

    void Init_Dialog();
    void Plot( wxCommandEvent& event );
    void OnQuit( wxCommandEvent& event );
    void OnClose( wxCloseEvent& event );
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event );
    void SetPlotFormat( wxCommandEvent& event );
    void OnSetScaleOpt( wxCommandEvent& event );
    void applyPlotSettings();
    void CreateDrillFile( wxCommandEvent& event );
};


const int UNITS_MILS = 1000;

DIALOG_PLOT::DIALOG_PLOT( PCB_EDIT_FRAME* aParent ) :
    DIALOG_PLOT_BASE( aParent ),
    m_parent( aParent ),
    m_board( aParent->GetBoard() ),
    m_plotOpts( aParent->GetPlotSettings() )
{
    m_config = wxGetApp().GetSettings();

    Init_Dialog();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void DIALOG_PLOT::Init_Dialog()
{
    wxString   msg;
    wxFileName fileName;

    m_config->Read( CONFIG_XFINESCALE_ADJ, &m_XScaleAdjust );
    m_config->Read( CONFIG_YFINESCALE_ADJ, &m_YScaleAdjust );
    m_config->Read( CONFIG_PS_FINEWIDTH_ADJ, &m_PSWidthAdjust);

    // The reasonable width correction value must be in a range of
    // [-(MinTrackWidth-1), +(MinClearanceValue-1)] decimils.
    m_WidthAdjustMinValue = -(m_board->GetDesignSettings().m_TrackMinWidth - 1);
    m_WidthAdjustMaxValue = m_board->GetSmallestClearanceValue() - 1;

    m_plotFormatOpt->SetSelection( m_plotOpts.GetPlotFormat() );

    // Set units and value for HPGL pen size (this param in in mils).
    AddUnitSymbol( *m_textPenSize, g_UserUnit );
    msg = ReturnStringFromValue( g_UserUnit,
                                 m_plotOpts.GetHpglPenDiameter() * IU_PER_MILS );
    m_HPGLPenSizeOpt->AppendText( msg );

    // Units are *always* cm/s for HPGL pen speed, from 1 to 99.
    msg = ReturnStringFromValue( UNSCALED_UNITS, m_plotOpts.GetHpglPenSpeed() );
    m_HPGLPenSpeedOpt->AppendText( msg );

    // Set units and value for HPGL pen overlay (this param in in mils).
    AddUnitSymbol( *m_textPenOvr, g_UserUnit );
    msg = ReturnStringFromValue( g_UserUnit,
                                 m_plotOpts.GetHpglPenOverlay() * IU_PER_MILS );
    m_HPGLPenOverlayOpt->AppendText( msg );

    AddUnitSymbol( *m_textDefaultPenSize, g_UserUnit );
    msg = ReturnStringFromValue( g_UserUnit, m_plotOpts.GetPlotLineWidth() );
    m_linesWidth->AppendText( msg );

    // Set units for PS global width correction.
    AddUnitSymbol( *m_textPSFineAdjustWidth, g_UserUnit );

    m_useAuxOriginCheckBox->SetValue( m_plotOpts.GetUseAuxOrigin() );

    // Test for a reasonable scale value. Set to 1 if problem
    if( m_XScaleAdjust < MIN_SCALE || m_YScaleAdjust < MIN_SCALE
        || m_XScaleAdjust > MAX_SCALE || m_YScaleAdjust > MAX_SCALE )
        m_XScaleAdjust = m_YScaleAdjust = 1.0;

    msg.Printf( wxT( "%f" ), m_XScaleAdjust );
    m_fineAdjustXscaleOpt->AppendText( msg );

    msg.Printf( wxT( "%f" ), m_YScaleAdjust );
    m_fineAdjustYscaleOpt->AppendText( msg );

    // Test for a reasonable PS width correction value. Set to 0 if problem.
    if( m_PSWidthAdjust < m_WidthAdjustMinValue || m_PSWidthAdjust > m_WidthAdjustMaxValue )
        m_PSWidthAdjust = 0.;

    msg.Printf( wxT( "%f" ), To_User_Unit( g_UserUnit, m_PSWidthAdjust ) );
    m_PSFineAdjustWidthOpt->AppendText( msg );

    m_plotPSNegativeOpt->SetValue( m_plotOpts.m_PlotPSNegative );
    m_forcePSA4OutputOpt->SetValue( m_plotOpts.GetPsA4Output() );

    //  List layers in same order than in setup layers dialog
    // (Front or Top to Back or Bottom)
    DECLARE_LAYERS_ORDER_LIST( layersOrder );
    int layerIndex, checkIndex, layer;

    for( layerIndex = 0; layerIndex < NB_LAYERS; layerIndex++ )
    {
        layer = layersOrder[layerIndex];

        wxASSERT( layer < NB_LAYERS );

        if( !m_board->IsLayerEnabled( layer ) )
            continue;

        layerList.push_back( layer );
        checkIndex = m_layerCheckListBox->Append( m_board->GetLayerName( layer ) );

        if( m_plotOpts.GetLayerSelection() & ( 1 << layer ) )
            m_layerCheckListBox->Check( checkIndex );
    }

    // Option for using proper Gerber extensions
    m_useGerberExtensions->SetValue( m_plotOpts.GetUseGerberExtensions() );

    // Option for excluding contents of "Edges Pcb" layer
    m_excludeEdgeLayerOpt->SetValue( m_plotOpts.m_ExcludeEdgeLayer );

    m_subtractMaskFromSilk->SetValue( m_plotOpts.GetSubtractMaskFromSilk() );

    // Option to plot page references:
    if( m_parent->GetPrintBorderAndTitleBlock() )
    {
        m_plotSheetRef->SetValue( m_plotOpts.m_PlotFrameRef );
    }
    else
    {
        m_plotSheetRef->Enable( false );
        m_plotSheetRef->SetValue( false );
    }

    // Option to plot pads on silkscreen layers or all layers
    m_plotPads_on_Silkscreen->SetValue( m_plotOpts.m_PlotPadsOnSilkLayer );

    // Options to plot texts on footprints
    m_plotModuleValueOpt->SetValue( m_plotOpts.m_PlotValue );
    m_plotModuleRefOpt->SetValue( m_plotOpts.m_PlotReference );
    m_plotTextOther->SetValue( m_plotOpts.m_PlotTextOther );
    m_plotInvisibleText->SetValue( m_plotOpts.m_PlotInvisibleTexts );

    // Options to plot pads and vias holes
    m_drillShapeOpt->SetSelection( m_plotOpts.m_DrillShapeOpt );

    // Scale option
    m_scaleOpt->SetSelection( m_plotOpts.GetScaleSelection() );

    // Plot mode
    m_plotModeOpt->SetSelection( m_plotOpts.m_PlotMode );

    // Plot mirror option
    m_plotMirrorOpt->SetValue( m_plotOpts.m_PlotMirror );

    // Put vias on mask layer
    m_plotNoViaOnMaskOpt->SetValue( m_plotOpts.m_PlotViaOnMaskLayer );

    // Output directory
    m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );

    // Update options values:
    wxCommandEvent cmd_event;
    SetPlotFormat( cmd_event );
    OnSetScaleOpt( cmd_event );
}


void DIALOG_PLOT::OnQuit( wxCommandEvent& event )
{
    Close( true );    // true is to force the frame to close
}


void DIALOG_PLOT::OnClose( wxCloseEvent& event )
{
    applyPlotSettings();
    EndModal( 0 );
}


void DIALOG_PLOT::CreateDrillFile( wxCommandEvent& event )
{
    ( (PCB_EDIT_FRAME*) m_parent )->InstallDrillFrame( event );
}


void DIALOG_PLOT::OnSetScaleOpt( wxCommandEvent& event )
{
    /* Disable sheet reference for scale != 1:1 */
    bool scale1 = ( m_scaleOpt->GetSelection() == 1 );

    m_plotSheetRef->Enable( scale1 );

    if( !scale1 )
        m_plotSheetRef->SetValue( false );
}


void DIALOG_PLOT::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxFileName fn( m_outputDirectoryName->GetValue() );
    wxString path;

    if( fn.IsRelative() )
        path = wxGetCwd() + fn.GetPathSeparator() + m_outputDirectoryName->GetValue();
    else
        path = m_outputDirectoryName->GetValue();

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxMessageDialog dialog( this, _( "Use a relative path? "),
                            _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    if( dialog.ShowModal() == wxID_YES ) {
        wxString boardFilePath = ( (wxFileName) m_parent->GetScreen()->GetFileName()).GetPath();

        if( !dirName.MakeRelativeTo( boardFilePath ) )
            wxMessageBox( _( "Cannot make path relative (target volume different from board file volume)!" ),
                          _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


void DIALOG_PLOT::SetPlotFormat( wxCommandEvent& event )
{
    switch( m_plotFormatOpt->GetSelection() )
    {
    case PLOT_FORMAT_POST:
    default:
        m_drillShapeOpt->Enable( true );
        m_plotModeOpt->Enable( true );
        m_plotMirrorOpt->Enable( true );
        m_useAuxOriginCheckBox->Enable( false );
        m_linesWidth->Enable( true );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->SetValue( false );
        m_excludeEdgeLayerOpt->Enable( false );
        m_subtractMaskFromSilk->Enable( false );
        m_useGerberExtensions->Enable( false );
        m_scaleOpt->Enable( true );
        m_fineAdjustXscaleOpt->Enable( true );
        m_fineAdjustYscaleOpt->Enable( true );
        m_PSFineAdjustWidthOpt->Enable( true );
        m_plotPSNegativeOpt->Enable( true );
        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Show( m_PSOptionsSizer );
        Layout();
        m_MainSizer->SetSizeHints( this );
        break;

    case PLOT_FORMAT_GERBER:
        m_drillShapeOpt->Enable( false );
        m_plotModeOpt->SetSelection( 1 );
        m_plotModeOpt->Enable( false );
        m_plotMirrorOpt->SetValue( false );
        m_plotMirrorOpt->Enable( false );
        m_useAuxOriginCheckBox->Enable( true );
        m_linesWidth->Enable( true );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_subtractMaskFromSilk->Enable( true );
        m_useGerberExtensions->Enable( true );
        m_scaleOpt->SetSelection( 1 );
        m_scaleOpt->Enable( false );
        m_fineAdjustXscaleOpt->Enable( false );
        m_fineAdjustYscaleOpt->Enable( false );
        m_PSFineAdjustWidthOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        m_PlotOptionsSizer->Show( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        Layout();
        m_MainSizer->SetSizeHints( this );
        break;

    case PLOT_FORMAT_HPGL:
        m_plotMirrorOpt->Enable( true );
        m_drillShapeOpt->Enable( false );
        m_plotModeOpt->Enable( true );
        m_useAuxOriginCheckBox->Enable( false );
        m_linesWidth->Enable( false );
        m_HPGLPenSizeOpt->Enable( true );
        m_HPGLPenSpeedOpt->Enable( true );
        m_HPGLPenOverlayOpt->Enable( true );
        m_excludeEdgeLayerOpt->SetValue( false );
        m_excludeEdgeLayerOpt->Enable( false );
        m_subtractMaskFromSilk->Enable( false );
        m_useGerberExtensions->Enable( false );
        m_scaleOpt->Enable( true );
        m_fineAdjustXscaleOpt->Enable( false );
        m_fineAdjustYscaleOpt->Enable( false );
        m_PSFineAdjustWidthOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Show( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        Layout();
        m_MainSizer->SetSizeHints( this );
        break;

    case PLOT_FORMAT_DXF:
        m_plotMirrorOpt->Enable( false );
        m_plotMirrorOpt->SetValue( false );
        m_drillShapeOpt->Enable( false );
        m_plotModeOpt->Enable( true );
        m_useAuxOriginCheckBox->Enable( false );
        m_linesWidth->Enable( false );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->SetValue( false );
        m_excludeEdgeLayerOpt->Enable( false );
        m_subtractMaskFromSilk->Enable( false );
        m_useGerberExtensions->Enable( false );
        m_scaleOpt->Enable( false );
        m_scaleOpt->SetSelection( 1 );
        m_fineAdjustXscaleOpt->Enable( false );
        m_fineAdjustYscaleOpt->Enable( false );
        m_PSFineAdjustWidthOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        Layout();
        m_MainSizer->SetSizeHints( this );
        break;
    }
}


void DIALOG_PLOT::applyPlotSettings()
{
    PCB_PLOT_PARAMS tempOptions;

    tempOptions.m_ExcludeEdgeLayer = m_excludeEdgeLayerOpt->GetValue();

    tempOptions.SetSubtractMaskFromSilk( m_subtractMaskFromSilk->GetValue() );

    if( m_plotSheetRef )
        tempOptions.m_PlotFrameRef = m_plotSheetRef->GetValue();

    tempOptions.m_PlotPadsOnSilkLayer = m_plotPads_on_Silkscreen->GetValue();

    tempOptions.SetUseAuxOrigin( m_useAuxOriginCheckBox->GetValue() );

    tempOptions.m_PlotValue     = m_plotModuleValueOpt->GetValue();
    tempOptions.m_PlotReference = m_plotModuleRefOpt->GetValue();
    tempOptions.m_PlotTextOther = m_plotTextOther->GetValue();
    tempOptions.m_PlotInvisibleTexts = m_plotInvisibleText->GetValue();

    tempOptions.SetScaleSelection( m_scaleOpt->GetSelection() );

    tempOptions.m_DrillShapeOpt =
        (PCB_PLOT_PARAMS::DrillShapeOptT) m_drillShapeOpt->GetSelection();
    tempOptions.m_PlotMirror = m_plotMirrorOpt->GetValue();
    tempOptions.m_PlotMode   = (EDA_DRAW_MODE_T) m_plotModeOpt->GetSelection();
    tempOptions.m_PlotViaOnMaskLayer = m_plotNoViaOnMaskOpt->GetValue();

    // Update settings from text fields. Rewrite values back to the fields,
    // since the values may have been constrained by the setters.

    // read HPLG pen size (this param is stored in mils)
    wxString msg = m_HPGLPenSizeOpt->GetValue();
    int      tmp = ReturnValueFromString( g_UserUnit, msg ) / IU_PER_MILS;

    if( !tempOptions.SetHpglPenDiameter( tmp ) )
    {
        msg = ReturnStringFromValue( g_UserUnit, tempOptions.GetHpglPenDiameter() * IU_PER_MILS );
        m_HPGLPenSizeOpt->SetValue( msg );
        msg.Printf( _( "HPGL pen size constrained!\n" ) );
        m_messagesBox->AppendText( msg );
    }

    // read HPGL pen speed (this param is stored in cm/s)
    msg = m_HPGLPenSpeedOpt->GetValue();
    tmp = ReturnValueFromString( UNSCALED_UNITS, msg );

    if( !tempOptions.SetHpglPenSpeed( tmp ) )
    {
        msg = ReturnStringFromValue( UNSCALED_UNITS, tempOptions.GetHpglPenSpeed() );
        m_HPGLPenSpeedOpt->SetValue( msg );
        msg.Printf( _( "HPGL pen speed constrained!\n" ) );
        m_messagesBox->AppendText( msg );
    }

    // Read HPGL pen overlay (this param is stored in mils)
    msg = m_HPGLPenOverlayOpt->GetValue();
    tmp = ReturnValueFromString( g_UserUnit, msg ) / IU_PER_MILS;

    if( !tempOptions.SetHpglPenOverlay( tmp ) )
    {
        msg = ReturnStringFromValue( g_UserUnit,
                                     tempOptions.GetHpglPenOverlay() * IU_PER_MILS );
        m_HPGLPenOverlayOpt->SetValue( msg );
        msg.Printf( _( "HPGL pen overlay constrained!\n" ) );
        m_messagesBox->AppendText( msg );
    }

    // Default linewidth
    msg = m_linesWidth->GetValue();
    tmp = ReturnValueFromString( g_UserUnit, msg );

    if( !tempOptions.SetPlotLineWidth( tmp ) )
    {
        msg = ReturnStringFromValue( g_UserUnit, tempOptions.GetPlotLineWidth() );
        m_linesWidth->SetValue( msg );
        msg.Printf( _( "Default linewidth constrained!\n" ) );
        m_messagesBox->AppendText( msg );
    }

    // X scale
    double tmpDouble;
    msg = m_fineAdjustXscaleOpt->GetValue();
    msg.ToDouble( &tmpDouble );

    if( !setDouble( &m_XScaleAdjust, tmpDouble, MIN_SCALE, MAX_SCALE ) )
    {
        msg.Printf( wxT( "%f" ), m_XScaleAdjust );
        m_fineAdjustXscaleOpt->SetValue( msg );
        msg.Printf( _( "X scale constrained!\n" ) );
        m_messagesBox->AppendText( msg );
    }

    m_config->Write( CONFIG_XFINESCALE_ADJ, m_XScaleAdjust );

    // Y scale
    msg = m_fineAdjustYscaleOpt->GetValue();
    msg.ToDouble( &tmpDouble );

    if( !setDouble( &m_YScaleAdjust, tmpDouble, MIN_SCALE, MAX_SCALE ) )
    {
        msg.Printf( wxT( "%f" ), m_YScaleAdjust );
        m_fineAdjustYscaleOpt->SetValue( msg );
        msg.Printf( _( "Y scale constrained!\n" ) );
        m_messagesBox->AppendText( msg );
    }

    m_config->Write( CONFIG_YFINESCALE_ADJ, m_YScaleAdjust );

    // PS Width correction
    msg = m_PSFineAdjustWidthOpt->GetValue();
    tmpDouble = ReturnValueFromString( g_UserUnit, msg );

    if( !setDouble( &m_PSWidthAdjust, tmpDouble, m_WidthAdjustMinValue, m_WidthAdjustMaxValue ) )
    {
        msg = ReturnStringFromValue( g_UserUnit, m_PSWidthAdjust );
        m_PSFineAdjustWidthOpt->SetValue( msg );
        msg.Printf( _( "Width correction constrained!\n"
"The reasonable width correction value must be in a range of\n"
" [%+f; %+f] (%s) for current design rules!\n" ),
                    To_User_Unit( g_UserUnit, m_WidthAdjustMinValue ),
                    To_User_Unit( g_UserUnit, m_WidthAdjustMaxValue ),
                    ( g_UserUnit == INCHES )? wxT("\"") : wxT("mm") );
        m_messagesBox->AppendText( msg );
    }

    m_config->Write( CONFIG_PS_FINEWIDTH_ADJ, m_PSWidthAdjust );

    tempOptions.SetUseGerberExtensions( m_useGerberExtensions->GetValue() );

    tempOptions.SetPlotFormat( m_plotFormatOpt->GetSelection() );

    long selectedLayers = 0;
    unsigned int i;

    for( i = 0; i < layerList.size(); i++ )
    {
        if( m_layerCheckListBox->IsChecked( i ) )
            selectedLayers |= (1 << layerList[i]);
    }

    tempOptions.SetLayerSelection( selectedLayers );
    tempOptions.m_PlotPSNegative = m_plotPSNegativeOpt->GetValue();
    tempOptions.SetPsA4Output( m_forcePSA4OutputOpt->GetValue() );

    // Set output directory and replace backslashes with forward ones
    wxString dirStr;
    dirStr = m_outputDirectoryName->GetValue();
    dirStr.Replace( wxT( "\\" ), wxT( "/" ) );
    tempOptions.SetOutputDirectory( dirStr );

    if( m_plotOpts != tempOptions )
    {
        m_parent->SetPlotSettings( tempOptions );
        m_plotOpts = tempOptions;
        m_parent->OnModify();
    }
}


void DIALOG_PLOT::Plot( wxCommandEvent& event )
{
    int        layer;
    wxFileName fn;
    wxString   ext;

    applyPlotSettings();

    // Create output directory if it does not exist
    wxFileName outputDir = wxFileName::DirName( m_plotOpts.GetOutputDirectory() );
    wxString boardFilePath = ( (wxFileName) m_parent->GetScreen()->GetFileName()).GetPath();

    if( !outputDir.MakeAbsolute( boardFilePath ) )
    {
        wxString msg;
        msg.Printf( _( " Cannot make %s absolute with respect to %s!" ),
                    GetChars( outputDir.GetPath() ),
                    GetChars( boardFilePath ) );
        wxMessageBox( msg, _( "Plot" ), wxOK | wxICON_ERROR );
        return;
    }

    if( !wxFileName::DirExists( outputDir.GetPath() ) )
    {
        if( wxMkdir( outputDir.GetPath() ) )
        {
            wxString msg;
            msg.Printf( _( "Directory %s created.\n" ), GetChars( outputDir.GetPath() ) );
            m_messagesBox->AppendText( msg );
        }
        else
        {
            wxMessageBox( _( "Cannot create output directory!" ),
                          _( "Plot" ), wxOK | wxICON_ERROR );
            return;
        }
    }

    m_plotOpts.m_AutoScale = false;
    m_plotOpts.m_PlotScale = 1;

    switch( m_plotOpts.GetScaleSelection() )
    {
    default:
        break;

    case 0:
        m_plotOpts.m_AutoScale = true;
        break;

    case 2:
        m_plotOpts.m_PlotScale = 1.5;
        break;

    case 3:
        m_plotOpts.m_PlotScale = 2;
        break;

    case 4:
        m_plotOpts.m_PlotScale = 3;
        break;
    }

    /* If the scale factor edit controls are disabled or the scale value
     * is 0, don't adjust the base scale factor.   This fixes a bug when
     * the default scale adjust is initialized to 0 and saved in program
     * settings resulting in a divide by zero fault.
     */
    if( m_fineAdjustXscaleOpt->IsEnabled()  && m_XScaleAdjust != 0.0 )
        m_plotOpts.m_FineScaleAdjustX = m_XScaleAdjust;

    if( m_fineAdjustYscaleOpt->IsEnabled() && m_YScaleAdjust != 0.0 )
        m_plotOpts.m_FineScaleAdjustY = m_YScaleAdjust;

    if( m_PSFineAdjustWidthOpt->IsEnabled() )
        m_plotOpts.m_FineWidthAdjust = m_PSWidthAdjust;

    switch( m_plotOpts.GetPlotFormat() )
    {
    case PLOT_FORMAT_POST:
        ext = wxT( "ps" );
        break;

    case PLOT_FORMAT_GERBER:
        m_plotOpts.m_PlotScale = 1.0; // No scale option allowed in gerber format
        ext = wxT( "pho" );
        break;

    case PLOT_FORMAT_HPGL:
        ext = wxT( "plt" );
        break;

    case PLOT_FORMAT_DXF:
        m_plotOpts.m_PlotScale = 1.0;
        ext = wxT( "dxf" );
        break;
    }

    // Test for a reasonable scale value
    if( m_plotOpts.m_PlotScale < MIN_SCALE )
        DisplayInfoMessage( this,
                            _( "Warning: Scale option set to a very small value" ) );

    if( m_plotOpts.m_PlotScale > MAX_SCALE )
        DisplayInfoMessage( this,
                            _( "Warning: Scale option set to a very large value" ) );

    m_parent->SetPlotSettings( m_plotOpts );

    long layerMask = 1;

    for( layer = 0; layer < NB_LAYERS; layer++, layerMask <<= 1 )
    {
        bool success = false;

        if( m_plotOpts.GetLayerSelection() & layerMask )
        {
            fn = m_parent->GetScreen()->GetFileName();
            fn.SetPath( outputDir.GetPath() );

            // Create file name.
            wxString layername = m_board->GetLayerName( layer );
            layername.Trim( true ); layername.Trim( false );    // remove leading and trailing spaces if any
            fn.SetName( fn.GetName() + wxT( "-" ) + layername );

            // Use Gerber Extensions based on layer number
            // (See http://en.wikipedia.org/wiki/Gerber_File)
            if( ( m_plotOpts.GetPlotFormat() == PLOT_FORMAT_GERBER )
                && m_useGerberExtensions->GetValue() )
            {
                switch( layer )
                {
                case LAYER_N_FRONT:
                    fn.SetExt( wxT( "gtl" ) );
                    break;

                case LAYER_N_2:
                case LAYER_N_3:
                case LAYER_N_4:
                case LAYER_N_5:
                case LAYER_N_6:
                case LAYER_N_7:
                case LAYER_N_8:
                case LAYER_N_9:
                case LAYER_N_10:
                case LAYER_N_11:
                case LAYER_N_12:
                case LAYER_N_13:
                case LAYER_N_14:
                case LAYER_N_15:

                    // TODO: see if we use .gbr or a layer identifier (gb1 .. gbnn ?)
                    // according to the new internal layers designation
                    // (1 is the first internal layer from the front layer)
                    fn.SetExt( wxT( "gbr" ) );
                    break;

                case LAYER_N_BACK:
                    fn.SetExt( wxT( "gbl" ) );
                    break;

                case ADHESIVE_N_BACK:
                    fn.SetExt( wxT( "gba" ) );
                    break;

                case ADHESIVE_N_FRONT:
                    fn.SetExt( wxT( "gta" ) );
                    break;

                case SOLDERPASTE_N_BACK:
                    fn.SetExt( wxT( "gbp" ) );
                    break;

                case SOLDERPASTE_N_FRONT:
                    fn.SetExt( wxT( "gtp" ) );
                    break;

                case SILKSCREEN_N_BACK:
                    fn.SetExt( wxT( "gbo" ) );
                    break;

                case SILKSCREEN_N_FRONT:
                    fn.SetExt( wxT( "gto" ) );
                    break;

                case SOLDERMASK_N_BACK:
                    fn.SetExt( wxT( "gbs" ) );
                    break;

                case SOLDERMASK_N_FRONT:
                    fn.SetExt( wxT( "gts" ) );
                    break;

                case DRAW_N:
                case COMMENT_N:
                case ECO1_N:
                case ECO2_N:
                case EDGE_N:
                default:
                    fn.SetExt( wxT( "gbr" ) );
                    break;
                }
            }
            else
            {
                fn.SetExt( ext );
            }

            switch( m_plotOpts.GetPlotFormat() )
            {
            case PLOT_FORMAT_POST:
                success = m_parent->ExportToPostScriptFile( fn.GetFullPath(), layer,
                                                            m_plotOpts.GetPsA4Output(),
                                                            m_plotOpts.m_PlotMode );
                break;

            case PLOT_FORMAT_GERBER:
                success = m_parent->ExportToGerberFile( fn.GetFullPath(), layer,
                                                        m_plotOpts.GetUseAuxOrigin(),
                                                        m_plotOpts.m_PlotMode );
                break;

            case PLOT_FORMAT_HPGL:
                success = m_parent->ExportToHpglFile( fn.GetFullPath(), layer,
                                                      m_plotOpts.m_PlotMode );
                break;

            case PLOT_FORMAT_DXF:
                success = m_parent->ExportToDxfFile( fn.GetFullPath(), layer,
                                                     m_plotOpts.m_PlotMode );
                break;
            }

            // Print diags in messages box:
            wxString msg;

            if( success )
                msg.Printf( _( "Plot file <%s> created" ), GetChars( fn.GetFullPath() ) );
            else
                msg.Printf( _( "Unable to create <%s>" ), GetChars( fn.GetFullPath() ) );

            msg << wxT( "\n" );
            m_messagesBox->AppendText( msg );
        }
    }

    // If no layer selected, we have nothing plotted.
    // Prompt user if it happens because he could think there is a bug in Pcbnew.
    if( !m_plotOpts.GetLayerSelection() )
        DisplayError( this, _( "No layer selected" ) );
}


void PCB_EDIT_FRAME::ToPlotter( wxCommandEvent& event )
{
    DIALOG_PLOT dlg( this );
    dlg.ShowModal();
}
