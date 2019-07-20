/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <plotter.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbplot.h>
#include <gerber_jobfile_writer.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <layers_id_colors_and_visibility.h>
#include <bitmaps.h>
#include <class_board.h>
#include <dialog_plot.h>
#include <dialog_gendrill.h>
#include <wx_html_report_panel.h>
#include <tool/tool_manager.h>
#include <tools/drc.h>
#include <tools/zone_filler_tool.h>


DIALOG_PLOT::DIALOG_PLOT( PCB_EDIT_FRAME* aParent ) :
    DIALOG_PLOT_BASE( aParent ), m_parent( aParent ),
    m_defaultLineWidth( aParent, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits, true ),
    m_defaultPenSize( aParent, m_hpglPenLabel, m_hpglPenCtrl, m_hpglPenUnits, true ),
    m_trackWidthCorrection( aParent, m_widthAdjustLabel, m_widthAdjustCtrl, m_widthAdjustUnits, true )
{
    SetName( DLG_WINDOW_NAME );
    m_config = Kiface().KifaceSettings();
    m_plotOpts = aParent->GetPlotSettings();
    init_Dialog();

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Plot" ) );
    m_sdbSizer1Apply->SetLabel( _( "Generate Drill Files..." ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sizerButtons->Layout();

    m_sdbSizer1OK->SetDefault();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void DIALOG_PLOT::init_Dialog()
{
    BOARD*      board = m_parent->GetBoard();
    wxFileName  fileName;

    m_config->Read( OPTKEY_PLOT_X_FINESCALE_ADJ, &m_XScaleAdjust );
    m_config->Read( OPTKEY_PLOT_Y_FINESCALE_ADJ, &m_YScaleAdjust );

    bool checkZones;
    m_config->Read( OPTKEY_PLOT_CHECK_ZONES, &checkZones, true );
    m_zoneFillCheck->SetValue( checkZones );

    m_browseButton->SetBitmap( KiBitmap( folder_xpm ) );

    // m_PSWidthAdjust is stored in mm in user config
    double dtmp;
    m_config->Read( CONFIG_PS_FINEWIDTH_ADJ, &dtmp, 0 );
    m_PSWidthAdjust = KiROUND( dtmp * IU_PER_MM );

    // The reasonable width correction value must be in a range of
    // [-(MinTrackWidth-1), +(MinClearanceValue-1)] decimils.
    m_widthAdjustMinValue   = -( board->GetDesignSettings().m_TrackMinWidth - 1 );
    m_widthAdjustMaxValue   = board->GetDesignSettings().GetSmallestClearanceValue() - 1;

    switch( m_plotOpts.GetFormat() )
    {
    default:
    case PLOT_FORMAT_GERBER: m_plotFormatOpt->SetSelection( 0 ); break;
    case PLOT_FORMAT_POST:   m_plotFormatOpt->SetSelection( 1 ); break;
    case PLOT_FORMAT_SVG:    m_plotFormatOpt->SetSelection( 2 ); break;
    case PLOT_FORMAT_DXF:    m_plotFormatOpt->SetSelection( 3 ); break;
    case PLOT_FORMAT_HPGL:   m_plotFormatOpt->SetSelection( 4 ); break;
    case PLOT_FORMAT_PDF:    m_plotFormatOpt->SetSelection( 5 ); break;
    }

    // Set units and value for HPGL pen size (this param is in mils).
    m_defaultPenSize.SetValue( m_plotOpts.GetHPGLPenDiameter() * IU_PER_MILS );

    m_defaultLineWidth.SetValue( m_plotOpts.GetLineWidth() );

    // Test for a reasonable scale value. Set to 1 if problem
    if( m_XScaleAdjust < PLOT_MIN_SCALE || m_YScaleAdjust < PLOT_MIN_SCALE
        || m_XScaleAdjust > PLOT_MAX_SCALE || m_YScaleAdjust > PLOT_MAX_SCALE )
        m_XScaleAdjust = m_YScaleAdjust = 1.0;

    m_fineAdjustXCtrl->SetValue( StringFromValue( UNSCALED_UNITS, m_XScaleAdjust ) );
    m_fineAdjustYCtrl->SetValue( StringFromValue( UNSCALED_UNITS, m_YScaleAdjust ) );

    // Test for a reasonable PS width correction value. Set to 0 if problem.
    if( m_PSWidthAdjust < m_widthAdjustMinValue || m_PSWidthAdjust > m_widthAdjustMaxValue )
        m_PSWidthAdjust = 0.;

    m_trackWidthCorrection.SetValue( m_PSWidthAdjust );

    m_plotPSNegativeOpt->SetValue( m_plotOpts.GetNegative() );
    m_forcePSA4OutputOpt->SetValue( m_plotOpts.GetA4Output() );

    // Could devote a PlotOrder() function in place of UIOrder().
    m_layerList = board->GetEnabledLayers().UIOrder();

    // Populate the check list box by all enabled layers names
    for( LSEQ seq = m_layerList;  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;

        int checkIndex = m_layerCheckListBox->Append( board->GetLayerName( layer ) );

        if( m_plotOpts.GetLayerSelection()[layer] )
            m_layerCheckListBox->Check( checkIndex );
    }

    // Option for using proper Gerber extensions. Note also Protel extensions are
    // a broken feature. However, for now, we need to handle it.
    m_useGerberExtensions->SetValue( m_plotOpts.GetUseGerberProtelExtensions() );

    // Option for including Gerber attributes, from Gerber X2 format, in the output
    // In X1 format, they will be added as comments
    m_useGerberX2Format->SetValue( m_plotOpts.GetUseGerberX2format() );

    // Option for including Gerber netlist info (from Gerber X2 format) in the output
    m_useGerberNetAttributes->SetValue( m_plotOpts.GetIncludeGerberNetlistInfo() );

    // Option to generate a Gerber job file
    m_generateGerberJobFile->SetValue( m_plotOpts.GetCreateGerberJobFile() );

    // Gerber precision for coordinates
    m_coordFormatCtrl->SetSelection( m_plotOpts.GetGerberPrecision() == 5 ? 0 : 1 );

    // Option for excluding contents of "Edges Pcb" layer
    m_excludeEdgeLayerOpt->SetValue( m_plotOpts.GetExcludeEdgeLayer() );

    // Option to exclude pads from silkscreen layers
    m_excludePadsFromSilkscreen->SetValue( !m_plotOpts.GetPlotPadsOnSilkLayer() );

    // Option to tent vias
    m_subtractMaskFromSilk->SetValue( m_plotOpts.GetSubtractMaskFromSilk() );

    // Option to use aux origin
    m_useAuxOriginCheckBox->SetValue( m_plotOpts.GetUseAuxOrigin() );

    // Option to plot page references:
    m_plotSheetRef->SetValue( m_plotOpts.GetPlotFrameRef() );

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

    // DXF outline mode
    m_DXF_plotModeOpt->SetValue( m_plotOpts.GetDXFPlotPolygonMode() );

    // DXF text mode
    m_DXF_plotTextStrokeFontOpt->SetValue( m_plotOpts.GetTextMode() == PLOTTEXTMODE_DEFAULT );

    // DXF units selection
    m_DXF_plotUnits->SetSelection( static_cast<int>( m_plotOpts.GetDXFPlotUnits() ) );

    // Plot mirror option
    m_plotMirrorOpt->SetValue( m_plotOpts.GetMirror() );

    // Put vias on mask layer
    m_plotNoViaOnMaskOpt->SetValue( m_plotOpts.GetPlotViaOnMaskLayer() );

    // Initialize a few other parameters, which can also be modified
    // from the drill dialog
    reInitDialog();

    // Update options values:
    wxCommandEvent cmd_event;
    SetPlotFormat( cmd_event );
    OnSetScaleOpt( cmd_event );
}


void DIALOG_PLOT::reInitDialog()
{
    // after calling drill dialog, some parameters can be modified.
    // update them

    // Output directory
    m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );

    // Origin of coordinates:
    m_useAuxOriginCheckBox->SetValue( m_plotOpts.GetUseAuxOrigin() );
}


// A helper function to show a popup menu, when the dialog is right clicked.
void DIALOG_PLOT::OnRightClick( wxMouseEvent& event )
{
    PopupMenu( m_popMenu );
}


// Select or deselect groups of layers in the layers list:
void DIALOG_PLOT::OnPopUpLayers( wxCommandEvent& event )
{
    // Build a list of layers for usual fabrication:
    // copper layers + tech layers without courtyard
    LSET fab_layer_set = ( LSET::AllCuMask() | LSET::AllTechMask() )
                         & ~LSET( 2, B_CrtYd, F_CrtYd );

    switch( event.GetId() )
    {
    case ID_LAYER_FAB: // Select layers usually needed to build a board
        for( unsigned i = 0; i < m_layerList.size(); i++ )
        {
            LSET layermask( m_layerList[ i ] );

            if( ( layermask & fab_layer_set ).any() )
                m_layerCheckListBox->Check( i, true );
            else
                m_layerCheckListBox->Check( i, false );
        }
        break;

    case ID_SELECT_COPPER_LAYERS:
        for( unsigned i = 0; i < m_layerList.size(); i++ )
        {
            if( IsCopperLayer( m_layerList[i] ) )
                m_layerCheckListBox->Check( i, true );
        }
        break;

    case ID_DESELECT_COPPER_LAYERS:
        for( unsigned i = 0; i < m_layerList.size(); i++ )
        {
            if( IsCopperLayer( m_layerList[i] ) )
                m_layerCheckListBox->Check( i, false );
        }
        break;

    case ID_SELECT_ALL_LAYERS:
        for( unsigned i = 0; i < m_layerList.size(); i++ )
            m_layerCheckListBox->Check( i, true );
        break;

    case ID_DESELECT_ALL_LAYERS:
        for( unsigned i = 0; i < m_layerList.size(); i++ )
            m_layerCheckListBox->Check( i, false );
        break;

    default:
        break;
    }
}


void DIALOG_PLOT::CreateDrillFile( wxCommandEvent& event )
{
    // Be sure drill file use the same settings (axis option, plot directory)
    // as plot files:
    applyPlotSettings();

    DIALOG_GENDRILL dlg( m_parent, this );
    dlg.ShowModal();

    // a few plot settings can be modified: take them in account
    m_plotOpts = m_parent->GetPlotSettings();
    reInitDialog();
}


void DIALOG_PLOT::OnChangeDXFPlotMode( wxCommandEvent& event )
{
    // m_DXF_plotTextStrokeFontOpt is disabled if m_DXF_plotModeOpt
    // is checked (plot in DXF polygon mode)
    m_DXF_plotTextStrokeFontOpt->Enable( !m_DXF_plotModeOpt->GetValue() );

    // if m_DXF_plotTextStrokeFontOpt option is disabled (plot DXF in polygon mode),
    // force m_DXF_plotTextStrokeFontOpt to true to use Pcbnew stroke font
    if( !m_DXF_plotTextStrokeFontOpt->IsEnabled() )
        m_DXF_plotTextStrokeFontOpt->SetValue( true );
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
    msg.Printf( _( "Do you want to use a path relative to\n\"%s\"" ),
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
    // this option exist only in DXF format:
    m_DXF_plotModeOpt->Enable( getPlotFormat() == PLOT_FORMAT_DXF );

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
        m_defaultLineWidth.Enable( true );
        m_defaultPenSize.Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_scaleOpt->Enable( false );
        m_scaleOpt->SetSelection( 1 );
        m_fineAdjustXCtrl->Enable( false );
        m_fineAdjustYCtrl->Enable( false );
        m_trackWidthCorrection.Enable( false );
        m_plotPSNegativeOpt->Enable( true );
        m_forcePSA4OutputOpt->Enable( false );
        m_forcePSA4OutputOpt->SetValue( false );

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        m_PlotOptionsSizer->Hide( m_SizerDXF_options );
        break;

    case PLOT_FORMAT_POST:
        m_drillShapeOpt->Enable( true );
        m_plotModeOpt->Enable( true );
        m_plotMirrorOpt->Enable( true );
        m_useAuxOriginCheckBox->Enable( false );
        m_useAuxOriginCheckBox->SetValue( false );
        m_defaultLineWidth.Enable( true );
        m_defaultPenSize.Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_scaleOpt->Enable( true );
        m_fineAdjustXCtrl->Enable( true );
        m_fineAdjustYCtrl->Enable( true );
        m_trackWidthCorrection.Enable( true );
        m_plotPSNegativeOpt->Enable( true );
        m_forcePSA4OutputOpt->Enable( true );

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Show( m_PSOptionsSizer );
        m_PlotOptionsSizer->Hide( m_SizerDXF_options );
        break;

    case PLOT_FORMAT_GERBER:
        m_drillShapeOpt->Enable( false );
        m_drillShapeOpt->SetSelection( 0 );
        m_plotModeOpt->Enable( false );
        setPlotModeChoiceSelection( FILLED );
        m_plotMirrorOpt->Enable( false );
        m_plotMirrorOpt->SetValue( false );
        m_useAuxOriginCheckBox->Enable( true );
        m_defaultLineWidth.Enable( true );
        m_defaultPenSize.Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_scaleOpt->Enable( false );
        m_scaleOpt->SetSelection( 1 );
        m_fineAdjustXCtrl->Enable( false );
        m_fineAdjustYCtrl->Enable( false );
        m_trackWidthCorrection.Enable( false );
        m_plotPSNegativeOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_forcePSA4OutputOpt->Enable( false );
        m_forcePSA4OutputOpt->SetValue( false );

        m_PlotOptionsSizer->Show( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        m_PlotOptionsSizer->Hide( m_SizerDXF_options );
        break;

    case PLOT_FORMAT_HPGL:
        m_drillShapeOpt->Enable( true );
        m_plotModeOpt->Enable( true );
        m_plotMirrorOpt->Enable( true );
        m_useAuxOriginCheckBox->Enable( false );
        m_useAuxOriginCheckBox->SetValue( false );
        m_defaultLineWidth.Enable( false );
        m_defaultPenSize.Enable( true );
        m_excludeEdgeLayerOpt->Enable( true );
        m_scaleOpt->Enable( true );
        m_fineAdjustXCtrl->Enable( false );
        m_fineAdjustYCtrl->Enable( false );
        m_trackWidthCorrection.Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        m_forcePSA4OutputOpt->Enable( true );

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Show( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        m_PlotOptionsSizer->Hide( m_SizerDXF_options );
        break;

    case PLOT_FORMAT_DXF:
        m_drillShapeOpt->Enable( true );
        m_plotModeOpt->Enable( false );
        setPlotModeChoiceSelection( FILLED );
        m_plotMirrorOpt->Enable( false );
        m_plotMirrorOpt->SetValue( false );
        m_useAuxOriginCheckBox->Enable( true );
        m_defaultLineWidth.Enable( false );
        m_defaultPenSize.Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_scaleOpt->Enable( false );
        m_scaleOpt->SetSelection( 1 );
        m_fineAdjustXCtrl->Enable( false );
        m_fineAdjustYCtrl->Enable( false );
        m_trackWidthCorrection.Enable( false );
        m_plotPSNegativeOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_forcePSA4OutputOpt->Enable( false );
        m_forcePSA4OutputOpt->SetValue( false );

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_HPGLOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        m_PlotOptionsSizer->Show( m_SizerDXF_options );

        OnChangeDXFPlotMode( event );
        break;

    case PLOT_FORMAT_UNDEFINED:
        break;
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
    REPORTER&       reporter = m_messagesPanel->Reporter();
    int             sel;
    PCB_PLOT_PARAMS tempOptions;

    tempOptions.SetExcludeEdgeLayer( m_excludeEdgeLayerOpt->GetValue() );
    tempOptions.SetSubtractMaskFromSilk( m_subtractMaskFromSilk->GetValue() );
    tempOptions.SetPlotFrameRef( m_plotSheetRef->GetValue() );
    tempOptions.SetPlotPadsOnSilkLayer( !m_excludePadsFromSilkscreen->GetValue() );
    tempOptions.SetUseAuxOrigin( m_useAuxOriginCheckBox->GetValue() );
    tempOptions.SetPlotValue( m_plotModuleValueOpt->GetValue() );
    tempOptions.SetPlotReference( m_plotModuleRefOpt->GetValue() );
    tempOptions.SetPlotInvisibleText( m_plotInvisibleText->GetValue() );
    tempOptions.SetScaleSelection( m_scaleOpt->GetSelection() );

    sel = m_drillShapeOpt->GetSelection();
    tempOptions.SetDrillMarksType( static_cast<PCB_PLOT_PARAMS::DrillMarksType>( sel ) );

    tempOptions.SetMirror( m_plotMirrorOpt->GetValue() );
    tempOptions.SetPlotMode( m_plotModeOpt->GetSelection() == 1 ? SKETCH : FILLED );
    tempOptions.SetDXFPlotPolygonMode( m_DXF_plotModeOpt->GetValue() );

    sel = m_DXF_plotUnits->GetSelection();
    tempOptions.SetDXFPlotUnits( static_cast<DXF_PLOTTER::DXF_UNITS>( sel ) );

    tempOptions.SetPlotViaOnMaskLayer( m_plotNoViaOnMaskOpt->GetValue() );

    if( !m_DXF_plotTextStrokeFontOpt->IsEnabled() )     // Currently, only DXF supports this option
        tempOptions.SetTextMode( PLOTTEXTMODE_DEFAULT  );
    else
        tempOptions.SetTextMode( m_DXF_plotTextStrokeFontOpt->GetValue() ?
                                 PLOTTEXTMODE_DEFAULT : PLOTTEXTMODE_NATIVE );

    // Update settings from text fields. Rewrite values back to the fields,
    // since the values may have been constrained by the setters.
    wxString    msg;

    // read HPLG pen size (this param is stored in mils)
    // However, due to issues when converting this value from or to mm
    // that can slightly change the value, update this param only if it
    // is in use
    if( getPlotFormat() == PLOT_FORMAT_HPGL )
    {
        if( !tempOptions.SetHPGLPenDiameter( m_defaultPenSize.GetValue() / IU_PER_MILS ) )
        {
            m_defaultPenSize.SetValue( tempOptions.GetHPGLPenDiameter() * IU_PER_MILS );
            msg.Printf( _( "HPGL pen size constrained." ) );
            reporter.Report( msg, REPORTER::RPT_INFO );
        }
    }
    else    // keep the last value (initial value if no HPGL plot made)
        tempOptions.SetHPGLPenDiameter( m_plotOpts.GetHPGLPenDiameter() );

    // Default linewidth
    if( !tempOptions.SetLineWidth( m_defaultLineWidth.GetValue() ) )
    {
        m_defaultLineWidth.SetValue( tempOptions.GetLineWidth() );
        msg.Printf( _( "Default line width constrained." ) );
        reporter.Report( msg, REPORTER::RPT_INFO );
    }

    // X scale
    double tmpDouble;
    msg = m_fineAdjustXCtrl->GetValue();
    msg.ToDouble( &tmpDouble );

    if( !setDouble( &m_XScaleAdjust, tmpDouble, PLOT_MIN_SCALE, PLOT_MAX_SCALE ) )
    {
        msg.Printf( wxT( "%f" ), m_XScaleAdjust );
        m_fineAdjustXCtrl->SetValue( msg );
        msg.Printf( _( "X scale constrained." ) );
        reporter.Report( msg, REPORTER::RPT_INFO );
    }

    ConfigBaseWriteDouble( m_config, OPTKEY_PLOT_X_FINESCALE_ADJ, m_XScaleAdjust );

    // Y scale
    msg = m_fineAdjustYCtrl->GetValue();
    msg.ToDouble( &tmpDouble );

    if( !setDouble( &m_YScaleAdjust, tmpDouble, PLOT_MIN_SCALE, PLOT_MAX_SCALE ) )
    {
        msg.Printf( wxT( "%f" ), m_YScaleAdjust );
        m_fineAdjustYCtrl->SetValue( msg );
        msg.Printf( _( "Y scale constrained." ) );
        reporter.Report( msg, REPORTER::RPT_INFO );
    }

    ConfigBaseWriteDouble( m_config, OPTKEY_PLOT_Y_FINESCALE_ADJ, m_YScaleAdjust );

    m_config->Write( OPTKEY_PLOT_CHECK_ZONES, m_zoneFillCheck->GetValue() );

    // PS Width correction
    if( !setInt( &m_PSWidthAdjust, m_trackWidthCorrection.GetValue(),
                 m_widthAdjustMinValue, m_widthAdjustMaxValue ) )
    {
        m_trackWidthCorrection.SetValue( m_PSWidthAdjust );
        msg.Printf( _( "Width correction constrained. "
                       "The reasonable width correction value must be in a range of "
                       " [%s; %s] (%s) for current design rules." ),
                    StringFromValue( GetUserUnits(), m_widthAdjustMinValue, false, true ),
                    StringFromValue( GetUserUnits(), m_widthAdjustMaxValue, false, true ),
                    GetAbbreviatedUnitsLabel( GetUserUnits(), true ) );
        reporter.Report( msg, REPORTER::RPT_WARNING );
    }

    // Store m_PSWidthAdjust in mm in user config
    ConfigBaseWriteDouble( m_config, CONFIG_PS_FINEWIDTH_ADJ,
                           (double)m_PSWidthAdjust / IU_PER_MM );

    tempOptions.SetFormat( getPlotFormat() );

    tempOptions.SetUseGerberProtelExtensions( m_useGerberExtensions->GetValue() );
    tempOptions.SetUseGerberX2format( m_useGerberX2Format->GetValue() );
    tempOptions.SetIncludeGerberNetlistInfo( m_useGerberNetAttributes->GetValue() );
    tempOptions.SetCreateGerberJobFile( m_generateGerberJobFile->GetValue() );

    tempOptions.SetGerberPrecision( m_coordFormatCtrl->GetSelection() == 0 ? 5 : 6 );

    LSET selectedLayers;
    for( unsigned i = 0; i < m_layerList.size(); i++ )
    {
        if( m_layerCheckListBox->IsChecked( i ) )
            selectedLayers.set( m_layerList[i] );
    }
    // Get a list of copper layers that aren't being used by inverting enabled layers.
    LSET disabledCopperLayers = LSET::AllCuMask() & ~m_parent->GetBoard()->GetEnabledLayers();
    // Enable all of the disabled copper layers.
    // If someone enables more copper layers they will be selected by default.
    selectedLayers = selectedLayers | disabledCopperLayers;
    tempOptions.SetLayerSelection( selectedLayers );

    tempOptions.SetNegative( m_plotPSNegativeOpt->GetValue() );
    tempOptions.SetA4Output( m_forcePSA4OutputOpt->GetValue() );

    // Set output directory and replace backslashes with forward ones
    wxString dirStr;
    dirStr = m_outputDirectoryName->GetValue();
    dirStr.Replace( wxT( "\\" ), wxT( "/" ) );
    tempOptions.SetOutputDirectory( dirStr );

    if( !m_plotOpts.IsSameAs( tempOptions, false ) )
    {
        // First, mark board as modified only for parameters saved in file
        if( !m_plotOpts.IsSameAs( tempOptions, true ) )
            m_parent->OnModify();

        // Now, save any change, for the session
        m_parent->SetPlotSettings( tempOptions );
        m_plotOpts = tempOptions;
    }
}


void DIALOG_PLOT::OnGerberX2Checked( wxCommandEvent& event )
{
    // Currently: do nothing
}


void DIALOG_PLOT::Plot( wxCommandEvent& event )
{
    BOARD* board = m_parent->GetBoard();

    applyPlotSettings();

    // If no layer selected, we have nothing plotted.
    // Prompt user if it happens because he could think there is a bug in Pcbnew.
    if( !m_plotOpts.GetLayerSelection().any() )
    {
        DisplayError( this, _( "No layer selected, Nothing to plot" ) );
        return;
    }

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

    if( m_zoneFillCheck->GetValue() )
        m_parent->GetToolManager()->GetTool<ZONE_FILLER_TOOL>()->CheckAllZones( this );

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
     * is 0, don't adjust the base scale factor. This fixes a bug when
     * the default scale adjust is initialized to 0 and saved in program
     * settings resulting in a divide by zero fault.
     */
    if( getPlotFormat() == PLOT_FORMAT_POST )
    {
        if( m_XScaleAdjust != 0.0 )
            m_plotOpts.SetFineScaleAdjustX( m_XScaleAdjust );

        if( m_YScaleAdjust != 0.0 )
            m_plotOpts.SetFineScaleAdjustY( m_YScaleAdjust );

        m_plotOpts.SetWidthAdjust( m_PSWidthAdjust );
    }

    wxString file_ext( GetDefaultPlotExtension( m_plotOpts.GetFormat() ) );

    // Test for a reasonable scale value
    // XXX could this actually happen? isn't it constrained in the apply
    // function?
    if( m_plotOpts.GetScale() < PLOT_MIN_SCALE )
        DisplayInfoMessage( this, _( "Warning: Scale option set to a very small value" ) );

    if( m_plotOpts.GetScale() > PLOT_MAX_SCALE )
        DisplayInfoMessage( this, _( "Warning: Scale option set to a very large value" ) );

    GERBER_JOBFILE_WRITER jobfile_writer( board, &reporter );

    // Save the current plot options in the board
    m_parent->SetPlotSettings( m_plotOpts );

    wxBusyCursor dummy;

    for( LSEQ seq = m_plotOpts.GetLayerSelection().UIOrder();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;

        // All copper layers that are disabled are actually selected
        // This is due to wonkyness in automatically selecting copper layers
        // for plotting when adding more than two layers to a board.
        // If plot options become accessible to the layers setup dialog
        // please move this functionality there!
        // This skips a copper layer if it is actually disabled on the board.
        if( ( LSET::AllCuMask() & ~board->GetEnabledLayers() )[layer] )
            continue;

        // Pick the basename from the board file
        wxFileName fn( boardFilename );

        // Use Gerber Extensions based on layer number
        // (See http://en.wikipedia.org/wiki/Gerber_File)
        if( m_plotOpts.GetFormat() == PLOT_FORMAT_GERBER && m_useGerberExtensions->GetValue() )
            file_ext = GetGerberProtelExtension( layer );

        BuildPlotFileName( &fn, outputDir.GetPath(), board->GetLayerName( layer ), file_ext );
        wxString fullname = fn.GetFullName();
        jobfile_writer.AddGbrFile( layer, fullname );

        LOCALE_IO toggle;

        PLOTTER*    plotter = StartPlotBoard( board, &m_plotOpts, layer, fn.GetFullPath(), wxEmptyString );

        // Print diags in messages box:
        wxString msg;

        if( plotter )
        {
            PlotOneBoardLayer( board, plotter, layer, m_plotOpts );
            plotter->EndPlot();
            delete plotter;

            msg.Printf( _( "Plot file \"%s\" created." ), GetChars( fn.GetFullPath() ) );
            reporter.Report( msg, REPORTER::RPT_ACTION );
        }
        else
        {
            msg.Printf( _( "Unable to create file \"%s\"." ), GetChars( fn.GetFullPath() ) );
            reporter.Report( msg, REPORTER::RPT_ERROR );
        }
    }

    if( m_plotOpts.GetFormat() == PLOT_FORMAT_GERBER && m_plotOpts.GetCreateGerberJobFile() )
    {
        // Pick the basename from the board file
        wxFileName fn( boardFilename );
        // Build gerber job file from basename
        BuildPlotFileName( &fn, outputDir.GetPath(), "job", GerberJobFileExtension );
        jobfile_writer.CreateJobFile( fn.GetFullPath() );
    }
}


void DIALOG_PLOT::onRunDRC( wxCommandEvent& event )
{
    PCB_EDIT_FRAME* parent = dynamic_cast<PCB_EDIT_FRAME*>( GetParent() );

    if( parent )
    {
        DRC* drcTool = parent->GetToolManager()->GetTool<DRC>();

        // First close an existing dialog if open
        // (low probability, but can happen)
        drcTool->DestroyDRCDialog( wxID_OK );

        // Open a new drc dialod, with the right parent frame, and in Modal Mode
        drcTool->ShowDRCDialog( this );
    }
}

