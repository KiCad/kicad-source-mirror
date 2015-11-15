/**
 * @file dialog_plot.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <kiface_i.h>
#include <plot_common.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <pcbplot.h>
#include <base_units.h>
#include <macros.h>
#include <reporter.h>

#include <class_board.h>
#include <wx/ffile.h>
#include <dialog_plot.h>
#include <wx_html_report_panel.h>

DIALOG_PLOT::DIALOG_PLOT( PCB_EDIT_FRAME* aParent ) :
    DIALOG_PLOT_BASE( aParent ), m_parent( aParent ),
    m_board( aParent->GetBoard() ),
    m_plotOpts( aParent->GetPlotSettings() )
{
    m_config = Kiface().KifaceSettings();
    Init_Dialog();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void DIALOG_PLOT::Init_Dialog()
{
    wxString    msg;
    wxFileName  fileName;

    m_config->Read( OPTKEY_PLOT_X_FINESCALE_ADJ, &m_XScaleAdjust );
    m_config->Read( OPTKEY_PLOT_Y_FINESCALE_ADJ, &m_YScaleAdjust );

    // m_PSWidthAdjust is stored in mm in user config
    double dtmp;
    m_config->Read( CONFIG_PS_FINEWIDTH_ADJ, &dtmp, 0 );
    m_PSWidthAdjust = KiROUND( dtmp * IU_PER_MM );

    // The reasonable width correction value must be in a range of
    // [-(MinTrackWidth-1), +(MinClearanceValue-1)] decimils.
    m_widthAdjustMinValue   = -( m_board->GetDesignSettings().m_TrackMinWidth - 1 );
    m_widthAdjustMaxValue   = m_board->GetDesignSettings().GetSmallestClearanceValue() - 1;

    switch( m_plotOpts.GetFormat() )
    {
    default:
    case PLOT_FORMAT_GERBER:
        m_plotFormatOpt->SetSelection( 0 );
        break;

    case PLOT_FORMAT_POST:
        m_plotFormatOpt->SetSelection( 1 );
        break;

    case PLOT_FORMAT_SVG:
        m_plotFormatOpt->SetSelection( 2 );
        break;

    case PLOT_FORMAT_DXF:
        m_plotFormatOpt->SetSelection( 3 );
        break;

    case PLOT_FORMAT_HPGL:
        m_plotFormatOpt->SetSelection( 4 );
        break;

    case PLOT_FORMAT_PDF:
        m_plotFormatOpt->SetSelection( 5 );
        break;
    }

    msg = StringFromValue( g_UserUnit, m_board->GetDesignSettings().m_SolderMaskMargin, true );
    m_SolderMaskMarginCurrValue->SetLabel( msg );
    msg = StringFromValue( g_UserUnit, m_board->GetDesignSettings().m_SolderMaskMinWidth, true );
    m_SolderMaskMinWidthCurrValue->SetLabel( msg );

    // Set units and value for HPGL pen size (this param in in mils).
    AddUnitSymbol( *m_textPenSize, g_UserUnit );
    msg = StringFromValue( g_UserUnit,
                           m_plotOpts.GetHPGLPenDiameter() * IU_PER_MILS );
    m_HPGLPenSizeOpt->AppendText( msg );

    // Set units and value for HPGL pen overlay (this param in in mils).
    AddUnitSymbol( *m_textPenOvr, g_UserUnit );
    msg = StringFromValue( g_UserUnit,
                                 m_plotOpts.GetHPGLPenOverlay() * IU_PER_MILS );
    m_HPGLPenOverlayOpt->AppendText( msg );

    AddUnitSymbol( *m_textDefaultPenSize, g_UserUnit );
    msg = StringFromValue( g_UserUnit, m_plotOpts.GetLineWidth() );
    m_linesWidth->AppendText( msg );

    // Set units for PS global width correction.
    AddUnitSymbol( *m_textPSFineAdjustWidth, g_UserUnit );

    m_useAuxOriginCheckBox->SetValue( m_plotOpts.GetUseAuxOrigin() );

    // Test for a reasonable scale value. Set to 1 if problem
    if( m_XScaleAdjust < PLOT_MIN_SCALE || m_YScaleAdjust < PLOT_MIN_SCALE
        || m_XScaleAdjust > PLOT_MAX_SCALE || m_YScaleAdjust > PLOT_MAX_SCALE )
        m_XScaleAdjust = m_YScaleAdjust = 1.0;

    msg.Printf( wxT( "%f" ), m_XScaleAdjust );
    m_fineAdjustXscaleOpt->AppendText( msg );

    msg.Printf( wxT( "%f" ), m_YScaleAdjust );
    m_fineAdjustYscaleOpt->AppendText( msg );

    // Test for a reasonable PS width correction value. Set to 0 if problem.
    if( m_PSWidthAdjust < m_widthAdjustMinValue || m_PSWidthAdjust > m_widthAdjustMaxValue )
        m_PSWidthAdjust = 0.;

    msg.Printf( wxT( "%f" ), To_User_Unit( g_UserUnit, m_PSWidthAdjust ) );
    m_PSFineAdjustWidthOpt->AppendText( msg );

    m_plotPSNegativeOpt->SetValue( m_plotOpts.GetNegative() );
    m_forcePSA4OutputOpt->SetValue( m_plotOpts.GetA4Output() );

    // Could devote a PlotOrder() function in place of UIOrder().
    m_layerList = m_board->GetEnabledLayers().UIOrder();

    // Populate the check list box by all enabled layers names
    for( LSEQ seq = m_layerList;  seq;  ++seq )
    {
        LAYER_ID layer = *seq;

        int checkIndex = m_layerCheckListBox->Append( m_board->GetLayerName( layer ) );

        if( m_plotOpts.GetLayerSelection()[layer] )
            m_layerCheckListBox->Check( checkIndex );
    }

    // Option for using proper Gerber extensions
    m_useGerberExtensions->SetValue( m_plotOpts.GetUseGerberProtelExtensions() );

    // Option for including Gerber attributes (from Gerber X2 format) in the output
    m_useGerberAttributes->SetValue( m_plotOpts.GetUseGerberAttributes() );

    // Gerber precision for coordinates
    m_rbGerberFormat->SetSelection( m_plotOpts.GetGerberPrecision() == 5 ? 0 : 1 );

    // Option for excluding contents of "Edges Pcb" layer
    m_excludeEdgeLayerOpt->SetValue( m_plotOpts.GetExcludeEdgeLayer() );

    m_subtractMaskFromSilk->SetValue( m_plotOpts.GetSubtractMaskFromSilk() );

    // Option to plot page references:
    m_plotSheetRef->SetValue( m_plotOpts.GetPlotFrameRef() );

    // Option to allow pads on silkscreen layers
    m_plotPads_on_Silkscreen->SetValue( m_plotOpts.GetPlotPadsOnSilkLayer() );

    // Options to plot texts on footprints
    m_plotModuleValueOpt->SetValue( m_plotOpts.GetPlotValue() );
    m_plotModuleRefOpt->SetValue( m_plotOpts.GetPlotReference() );
    m_plotInvisibleText->SetValue( m_plotOpts.GetPlotInvisibleText() );

    // Options to plot pads and vias holes
    m_drillShapeOpt->SetSelection( m_plotOpts.GetDrillMarksType() );

    // Scale option
    m_scaleOpt->SetSelection( m_plotOpts.GetScaleSelection() );

    // Plot mode
    setPlotModeChoiceSelection( m_plotOpts.GetPlotMode() );

    // Plot mirror option
    m_plotMirrorOpt->SetValue( m_plotOpts.GetMirror() );

    // Put vias on mask layer
    m_plotNoViaOnMaskOpt->SetValue( m_plotOpts.GetPlotViaOnMaskLayer() );

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


// A helper function to show a popup menu, when the dialog is right clicked.
void DIALOG_PLOT::OnRightClick( wxMouseEvent& event )
{
    PopupMenu( m_popMenu );
}


// Select or deselect groups of layers in the layers list:
#include <layers_id_colors_and_visibility.h>
void DIALOG_PLOT::OnPopUpLayers( wxCommandEvent& event )
{
    unsigned int    i;

    switch( event.GetId() )
    {
    case ID_LAYER_FAB: // Select layers usually needed to build a board
        for( i = 0; i < m_layerList.size(); i++ )
        {
            LSET layermask( m_layerList[ i ] );

            if( ( layermask & ( LSET::AllCuMask() | LSET::AllTechMask() ) ).any() )
                m_layerCheckListBox->Check( i, true );
            else
                m_layerCheckListBox->Check( i, false );
        }
        break;

    case ID_SELECT_COPPER_LAYERS:
        for( i = 0; i < m_layerList.size(); i++ )
        {
            if( IsCopperLayer( m_layerList[i] ) )
                m_layerCheckListBox->Check( i, true );
        }
        break;

    case ID_DESELECT_COPPER_LAYERS:
        for( i = 0; i < m_layerList.size(); i++ )
        {
            if( IsCopperLayer( m_layerList[i] ) )
                m_layerCheckListBox->Check( i, false );
        }
        break;

    case ID_SELECT_ALL_LAYERS:
        for( i = 0; i < m_layerList.size(); i++ )
            m_layerCheckListBox->Check( i, true );
        break;

    case ID_DESELECT_ALL_LAYERS:
        for( i = 0; i < m_layerList.size(); i++ )
            m_layerCheckListBox->Check( i, false );
        break;

    default:
        break;
    }
}


void DIALOG_PLOT::CreateDrillFile( wxCommandEvent& event )
{
    m_parent->InstallDrillFrame( event );
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
    wxFileName  fn( m_outputDirectoryName->GetValue() );
    wxString    path = Prj().AbsolutePath( m_outputDirectoryName->GetValue() );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName      dirName = wxFileName::DirName( dirDialog.GetPath() );

    fn = Prj().AbsolutePath( m_parent->GetBoard()->GetFileName() );
    wxString defaultPath = fn.GetPathWithSep();
    wxString msg;
    msg.Printf( _( "Do you want to use a path relative to\n'%s'" ),
                   GetChars( defaultPath ) );

    wxMessageDialog dialog( this, msg, _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    if( dialog.ShowModal() == wxID_YES )
    {
        if( !dirName.MakeRelativeTo( defaultPath ) )
            wxMessageBox( _( "Cannot make path relative (target volume different from file volume)!" ),
                          _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


PlotFormat DIALOG_PLOT::getPlotFormat()
{
    // plot format id's are ordered like displayed in m_plotFormatOpt
    static const PlotFormat plotFmt[] =
    {
        PLOT_FORMAT_GERBER,
        PLOT_FORMAT_POST,
        PLOT_FORMAT_SVG,
        PLOT_FORMAT_DXF,
        PLOT_FORMAT_HPGL,
        PLOT_FORMAT_PDF
    };

    return plotFmt[ m_plotFormatOpt->GetSelection() ];
}


// Enable or disable widgets according to the plot format selected
// and clear also some optional values
void DIALOG_PLOT::SetPlotFormat( wxCommandEvent& event )
{
    switch( getPlotFormat() )
    {
    case PLOT_FORMAT_PDF:
    case PLOT_FORMAT_SVG:
        m_drillShapeOpt->Enable( true );
        m_plotModeOpt->Enable( false );
        setPlotModeChoiceSelection( FILLED );
        m_plotMirrorOpt->Enable( true );
        m_useAuxOriginCheckBox->Enable( false );
        m_useAuxOriginCheckBox->SetValue( false );
        m_linesWidth->Enable( true );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_subtractMaskFromSilk->Enable( false );
        m_subtractMaskFromSilk->SetValue( false );
        m_useGerberExtensions->Enable( false );
        m_useGerberExtensions->SetValue( false );
        m_useGerberAttributes->Enable( false );
        m_useGerberAttributes->SetValue( false );
        m_scaleOpt->Enable( false );
        m_scaleOpt->SetSelection( 1 );
        m_fineAdjustXscaleOpt->Enable( false );
        m_fineAdjustYscaleOpt->Enable( false );
        m_PSFineAdjustWidthOpt->Enable( false );
        m_plotPSNegativeOpt->Enable( true );
        m_forcePSA4OutputOpt->Enable( false );
        m_forcePSA4OutputOpt->SetValue( false );

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        break;

    case PLOT_FORMAT_POST:
        m_drillShapeOpt->Enable( true );
        m_plotModeOpt->Enable( true );
        m_plotMirrorOpt->Enable( true );
        m_useAuxOriginCheckBox->Enable( false );
        m_useAuxOriginCheckBox->SetValue( false );
        m_linesWidth->Enable( true );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_subtractMaskFromSilk->Enable( false );
        m_subtractMaskFromSilk->SetValue( false );
        m_useGerberExtensions->Enable( false );
        m_useGerberExtensions->SetValue( false );
        m_useGerberAttributes->Enable( false );
        m_useGerberAttributes->SetValue( false );
        m_scaleOpt->Enable( true );
        m_fineAdjustXscaleOpt->Enable( true );
        m_fineAdjustYscaleOpt->Enable( true );
        m_PSFineAdjustWidthOpt->Enable( true );
        m_plotPSNegativeOpt->Enable( true );
        m_forcePSA4OutputOpt->Enable( true );

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Show( m_PSOptionsSizer );
        break;

    case PLOT_FORMAT_GERBER:
        m_drillShapeOpt->Enable( false );
        m_drillShapeOpt->SetSelection( 0 );
        m_plotModeOpt->Enable( false );
        setPlotModeChoiceSelection( FILLED );
        m_plotMirrorOpt->Enable( false );
        m_plotMirrorOpt->SetValue( false );
        m_useAuxOriginCheckBox->Enable( true );
        m_linesWidth->Enable( true );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_subtractMaskFromSilk->Enable( true );
        m_useGerberExtensions->Enable( true );
        m_useGerberAttributes->Enable( true );
        m_scaleOpt->Enable( false );
        m_scaleOpt->SetSelection( 1 );
        m_fineAdjustXscaleOpt->Enable( false );
        m_fineAdjustYscaleOpt->Enable( false );
        m_PSFineAdjustWidthOpt->Enable( false );
        m_plotPSNegativeOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_forcePSA4OutputOpt->Enable( false );
        m_forcePSA4OutputOpt->SetValue( false );

        m_PlotOptionsSizer->Show( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        break;

    case PLOT_FORMAT_HPGL:
        m_drillShapeOpt->Enable( true );
        m_plotModeOpt->Enable( true );
        m_plotMirrorOpt->Enable( true );
        m_useAuxOriginCheckBox->Enable( false );
        m_useAuxOriginCheckBox->SetValue( false );
        m_linesWidth->Enable( false );
        m_HPGLPenSizeOpt->Enable( true );
        m_HPGLPenOverlayOpt->Enable( true );
        m_excludeEdgeLayerOpt->Enable( true );
        m_subtractMaskFromSilk->Enable( false );
        m_subtractMaskFromSilk->SetValue( false );
        m_useGerberExtensions->Enable( false );
        m_useGerberExtensions->SetValue( false );
        m_useGerberAttributes->Enable( false );
        m_useGerberAttributes->SetValue( false );
        m_scaleOpt->Enable( true );
        m_fineAdjustXscaleOpt->Enable( false );
        m_fineAdjustYscaleOpt->Enable( false );
        m_PSFineAdjustWidthOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        m_forcePSA4OutputOpt->Enable( true );

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Show( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        break;

    case PLOT_FORMAT_DXF:
        m_drillShapeOpt->Enable( true );
        m_plotModeOpt->Enable( false );
        m_plotMirrorOpt->Enable( false );
        m_plotMirrorOpt->SetValue( false );
        m_useAuxOriginCheckBox->Enable( true );
        m_linesWidth->Enable( false );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_subtractMaskFromSilk->Enable( false );
        m_subtractMaskFromSilk->SetValue( false );
        m_useGerberExtensions->Enable( false );
        m_useGerberExtensions->SetValue( false );
        m_useGerberAttributes->Enable( false );
        m_useGerberAttributes->SetValue( false );
        m_scaleOpt->Enable( false );
        m_scaleOpt->SetSelection( 1 );
        m_fineAdjustXscaleOpt->Enable( false );
        m_fineAdjustYscaleOpt->Enable( false );
        m_PSFineAdjustWidthOpt->Enable( false );
        m_plotPSNegativeOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_forcePSA4OutputOpt->Enable( false );
        m_forcePSA4OutputOpt->SetValue( false );

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        break;

    default:
        wxASSERT( false );
    }

    /* Update the interlock between scale and frame reference
     * (scaling would mess up the frame border...) */
    OnSetScaleOpt( event );

    Layout();
    m_MainSizer->SetSizeHints( this );
}


// A helper function to "clip" aValue between aMin and aMax
// and write result in * aResult
// return false if clipped, true if aValue is just copied into * aResult
static bool setDouble( double* aResult, double aValue, double aMin, double aMax )
{
    if( aValue < aMin )
    {
        *aResult = aMin;
        return false;
    }
    else if( aValue > aMax )
    {
        *aResult = aMax;
        return false;
    }

    *aResult = aValue;
    return true;
}


static bool setInt( int* aResult, int aValue, int aMin, int aMax )
{
    if( aValue < aMin )
    {
        *aResult = aMin;
        return false;
    }
    else if( aValue > aMax )
    {
        *aResult = aMax;
        return false;
    }

    *aResult = aValue;
    return true;
}


void DIALOG_PLOT::applyPlotSettings()
{
    REPORTER&   reporter = m_messagesPanel->Reporter();

    PCB_PLOT_PARAMS tempOptions;

    tempOptions.SetExcludeEdgeLayer( m_excludeEdgeLayerOpt->GetValue() );
    tempOptions.SetSubtractMaskFromSilk( m_subtractMaskFromSilk->GetValue() );
    tempOptions.SetPlotFrameRef( m_plotSheetRef->GetValue() );
    tempOptions.SetPlotPadsOnSilkLayer( m_plotPads_on_Silkscreen->GetValue() );
    tempOptions.SetUseAuxOrigin( m_useAuxOriginCheckBox->GetValue() );
    tempOptions.SetPlotValue( m_plotModuleValueOpt->GetValue() );
    tempOptions.SetPlotReference( m_plotModuleRefOpt->GetValue() );
    tempOptions.SetPlotInvisibleText( m_plotInvisibleText->GetValue() );
    tempOptions.SetScaleSelection( m_scaleOpt->GetSelection() );
    tempOptions.SetDrillMarksType( static_cast<PCB_PLOT_PARAMS::DrillMarksType>
                                   ( m_drillShapeOpt->GetSelection() ) );
    tempOptions.SetMirror( m_plotMirrorOpt->GetValue() );
    tempOptions.SetPlotMode( m_plotModeOpt->GetSelection() == 1 ? SKETCH : FILLED );
    tempOptions.SetPlotViaOnMaskLayer( m_plotNoViaOnMaskOpt->GetValue() );

    // Update settings from text fields. Rewrite values back to the fields,
    // since the values may have been constrained by the setters.

    // read HPLG pen size (this param is stored in mils)
    wxString    msg = m_HPGLPenSizeOpt->GetValue();
    int         tmp = ValueFromString( g_UserUnit, msg ) / IU_PER_MILS;

    if( !tempOptions.SetHPGLPenDiameter( tmp ) )
    {
        msg = StringFromValue( g_UserUnit, tempOptions.GetHPGLPenDiameter() * IU_PER_MILS );
        m_HPGLPenSizeOpt->SetValue( msg );
        msg.Printf( _( "HPGL pen size constrained." ) );
        reporter.Report( msg, REPORTER::RPT_INFO );
    }

    // Read HPGL pen overlay (this param is stored in mils)
    msg = m_HPGLPenOverlayOpt->GetValue();
    tmp = ValueFromString( g_UserUnit, msg ) / IU_PER_MILS;

    if( !tempOptions.SetHPGLPenOverlay( tmp ) )
    {
        msg = StringFromValue( g_UserUnit,
                                     tempOptions.GetHPGLPenOverlay() * IU_PER_MILS );
        m_HPGLPenOverlayOpt->SetValue( msg );
        msg.Printf( _( "HPGL pen overlay constrained." ) );
        reporter.Report( msg, REPORTER::RPT_INFO );
    }

    // Default linewidth
    msg = m_linesWidth->GetValue();
    tmp = ValueFromString( g_UserUnit, msg );

    if( !tempOptions.SetLineWidth( tmp ) )
    {
        msg = StringFromValue( g_UserUnit, tempOptions.GetLineWidth() );
        m_linesWidth->SetValue( msg );
        msg.Printf( _( "Default line width constrained." ) );
        reporter.Report( msg, REPORTER::RPT_INFO );
    }

    // X scale
    double tmpDouble;
    msg = m_fineAdjustXscaleOpt->GetValue();
    msg.ToDouble( &tmpDouble );

    if( !setDouble( &m_XScaleAdjust, tmpDouble, PLOT_MIN_SCALE, PLOT_MAX_SCALE ) )
    {
        msg.Printf( wxT( "%f" ), m_XScaleAdjust );
        m_fineAdjustXscaleOpt->SetValue( msg );
        msg.Printf( _( "X scale constrained." ) );
        reporter.Report( msg, REPORTER::RPT_INFO );
    }

    ConfigBaseWriteDouble( m_config, OPTKEY_PLOT_X_FINESCALE_ADJ, m_XScaleAdjust );

    // Y scale
    msg = m_fineAdjustYscaleOpt->GetValue();
    msg.ToDouble( &tmpDouble );

    if( !setDouble( &m_YScaleAdjust, tmpDouble, PLOT_MIN_SCALE, PLOT_MAX_SCALE ) )
    {
        msg.Printf( wxT( "%f" ), m_YScaleAdjust );
        m_fineAdjustYscaleOpt->SetValue( msg );
        msg.Printf( _( "Y scale constrained." ) );
        reporter.Report( msg, REPORTER::RPT_INFO );
    }

    ConfigBaseWriteDouble( m_config, OPTKEY_PLOT_Y_FINESCALE_ADJ, m_YScaleAdjust );

    // PS Width correction
    msg = m_PSFineAdjustWidthOpt->GetValue();
    int itmp = ValueFromString( g_UserUnit, msg );

    if( !setInt( &m_PSWidthAdjust, itmp, m_widthAdjustMinValue, m_widthAdjustMaxValue ) )
    {
        msg = StringFromValue( g_UserUnit, m_PSWidthAdjust );
        m_PSFineAdjustWidthOpt->SetValue( msg );
        msg.Printf( _( "Width correction constrained. "
                       "The reasonable width correction value must be in a range of "
                       " [%+f; %+f] (%s) for current design rules. " ),
                    To_User_Unit( g_UserUnit, m_widthAdjustMinValue ),
                    To_User_Unit( g_UserUnit, m_widthAdjustMaxValue ),
                    ( g_UserUnit == INCHES ) ? wxT( "\"" ) : wxT( "mm" ) );
        reporter.Report( msg, REPORTER::RPT_WARNING );
    }

    // Store m_PSWidthAdjust in mm in user config
    ConfigBaseWriteDouble( m_config, CONFIG_PS_FINEWIDTH_ADJ,
                           (double)m_PSWidthAdjust / IU_PER_MM );

    tempOptions.SetFormat( getPlotFormat() );

    tempOptions.SetUseGerberProtelExtensions( m_useGerberExtensions->GetValue() );
    tempOptions.SetUseGerberAttributes( m_useGerberAttributes->GetValue() );
    tempOptions.SetGerberPrecision( m_rbGerberFormat->GetSelection() == 0 ? 5 : 6 );

    LSET selectedLayers;

    for( unsigned i = 0; i < m_layerList.size(); i++ )
    {
        if( m_layerCheckListBox->IsChecked( i ) )
            selectedLayers.set( m_layerList[i] );
    }

    tempOptions.SetLayerSelection( selectedLayers );
    tempOptions.SetNegative( m_plotPSNegativeOpt->GetValue() );
    tempOptions.SetA4Output( m_forcePSA4OutputOpt->GetValue() );

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
    applyPlotSettings();

    // Create output directory if it does not exist (also transform it in
    // absolute form). Bail if it fails
    wxFileName  outputDir = wxFileName::DirName( m_plotOpts.GetOutputDirectory() );
    wxString    boardFilename = m_parent->GetBoard()->GetFileName();
    REPORTER&   reporter = m_messagesPanel->Reporter();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, &reporter ) )
    {
        wxString msg;
        msg.Printf( _( "Could not write plot files to folder \"%s\"." ),
                    GetChars( outputDir.GetPath() ) );
        DisplayError( this, msg );
        return;
    }

    m_plotOpts.SetAutoScale( false );
    m_plotOpts.SetScale( 1 );

    switch( m_plotOpts.GetScaleSelection() )
    {
    default:
        break;

    case 0:     // Autoscale option
        m_plotOpts.SetAutoScale( true );
        break;

    case 2:     // 3:2 option
        m_plotOpts.SetScale( 1.5 );
        break;

    case 3:     // 2:1 option
        m_plotOpts.SetScale( 2 );
        break;

    case 4:     // 3:1 option
        m_plotOpts.SetScale( 3 );
        break;
    }

    /* If the scale factor edit controls are disabled or the scale value
     * is 0, don't adjust the base scale factor.   This fixes a bug when
     * the default scale adjust is initialized to 0 and saved in program
     * settings resulting in a divide by zero fault.
     */
    if( m_fineAdjustXscaleOpt->IsEnabled()  && m_XScaleAdjust != 0.0 )
        m_plotOpts.SetFineScaleAdjustX( m_XScaleAdjust );

    if( m_fineAdjustYscaleOpt->IsEnabled() && m_YScaleAdjust != 0.0 )
        m_plotOpts.SetFineScaleAdjustY( m_YScaleAdjust );

    if( m_PSFineAdjustWidthOpt->IsEnabled() )
        m_plotOpts.SetWidthAdjust( m_PSWidthAdjust );

    wxString file_ext( GetDefaultPlotExtension( m_plotOpts.GetFormat() ) );

    // Test for a reasonable scale value
    // XXX could this actually happen? isn't it constrained in the apply
    // function?
    if( m_plotOpts.GetScale() < PLOT_MIN_SCALE )
        DisplayInfoMessage( this,
                            _( "Warning: Scale option set to a very small value" ) );

    if( m_plotOpts.GetScale() > PLOT_MAX_SCALE )
        DisplayInfoMessage( this,
                            _( "Warning: Scale option set to a very large value" ) );

    // Save the current plot options in the board
    m_parent->SetPlotSettings( m_plotOpts );

    wxBusyCursor dummy;

    for( LSEQ seq = m_plotOpts.GetLayerSelection().UIOrder();  seq;  ++seq )
    {
        LAYER_ID layer = *seq;

        // Pick the basename from the board file
        wxFileName fn( boardFilename );

        // Use Gerber Extensions based on layer number
        // (See http://en.wikipedia.org/wiki/Gerber_File)
        if( m_plotOpts.GetFormat() == PLOT_FORMAT_GERBER && m_useGerberExtensions->GetValue() )
            file_ext = GetGerberProtelExtension( layer );

        // Create file name (from the English default layer name for non copper layers).
        BuildPlotFileName( &fn, outputDir.GetPath(),
//                           m_board->GetStandardLayerName( layer ),
                           m_board->GetLayerName( layer ),
                           file_ext );

        LOCALE_IO toggle;

        BOARD*      board = m_parent->GetBoard();
        PLOTTER*    plotter = StartPlotBoard( board, &m_plotOpts, layer, fn.GetFullPath(), wxEmptyString );

        // Print diags in messages box:
        wxString msg;

        if( plotter )
        {
            PlotOneBoardLayer( board, plotter, layer, m_plotOpts );
            plotter->EndPlot();
            delete plotter;

            msg.Printf( _( "Plot file '%s' created." ), GetChars( fn.GetFullPath() ) );
            reporter.Report( msg, REPORTER::RPT_ACTION );
        }
        else
        {
            msg.Printf( _( "Unable to create file '%s'." ), GetChars( fn.GetFullPath() ) );
            reporter.Report( msg, REPORTER::RPT_ERROR );
        }
    }

    // If no layer selected, we have nothing plotted.
    // Prompt user if it happens because he could think there is a bug in Pcbnew.
    if( !m_plotOpts.GetLayerSelection().any() )
        DisplayError( this, _( "No layer selected" ) );
}
