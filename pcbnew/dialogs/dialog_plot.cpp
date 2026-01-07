/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_plot.h"

#include <wx/bmpbuttn.h>
#include <wx/clntdata.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <wx/rearrangectrl.h>

#include <plotters/plotter.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <project/project_file.h>
#include <pcbplot.h>
#include <pgm_base.h>
#include <gerber_jobfile_writer.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <layer_ids.h>
#include <bitmaps.h>
#include <dialog_gendrill.h>
#include <string_utils.h>
#include <widgets/wx_html_report_panel.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/color_swatch.h>
#include <tool/tool_manager.h>
#include <tools/zone_filler_tool.h>
#include <tools/drc_tool.h>
#include <math/util.h>      // for KiROUND
#include <macros.h>
#include <jobs/job_export_pcb_gerbers.h>
#include <jobs/job_export_pcb_hpgl.h>
#include <jobs/job_export_pcb_dxf.h>
#include <jobs/job_export_pcb_pdf.h>
#include <jobs/job_export_pcb_ps.h>
#include <jobs/job_export_pcb_svg.h>
#include <plotters/plotters_pslike.h>
#include <pcb_plotter.h>


LSEQ DIALOG_PLOT::s_lastAllLayersOrder;


static double selectionToScale( int selection )
{
    switch( selection )
    {
    default: return 1.0;
    case 0:  return 0.0;
    case 2:  return 1.5;
    case 3:  return 2.0;
    case 4:  return 3.0;
    }
}


/**
 * A helper wxWidgets control client data object to store layer IDs.
 */
class PCB_LAYER_ID_CLIENT_DATA : public wxClientData
{
public:
    PCB_LAYER_ID_CLIENT_DATA() :
            m_id( UNDEFINED_LAYER )
    { }

    PCB_LAYER_ID_CLIENT_DATA( PCB_LAYER_ID aId ) :
            m_id( aId )
    { }

    void SetData( PCB_LAYER_ID aId ) { m_id = aId; }
    PCB_LAYER_ID Layer() const { return m_id; }

private:
    PCB_LAYER_ID m_id;
};


PCB_LAYER_ID_CLIENT_DATA* getLayerClientData( const wxRearrangeList* aList, int aIdx )
{
    return static_cast<PCB_LAYER_ID_CLIENT_DATA*>( aList->GetClientObject( aIdx ) );
}


DIALOG_PLOT::DIALOG_PLOT( PCB_EDIT_FRAME* aEditFrame )
    : DIALOG_PLOT( aEditFrame, aEditFrame )
{
}


DIALOG_PLOT::DIALOG_PLOT( PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent, JOB_EXPORT_PCB_PLOT* aJob ) :
        DIALOG_PLOT_BASE( aParent ),
        m_editFrame( aEditFrame ),
        m_trackWidthCorrection( m_editFrame, m_widthAdjustLabel, m_widthAdjustCtrl, m_widthAdjustUnits ),
        m_job( aJob )
{
    BOARD* board = m_editFrame->GetBoard();

    m_pdfBackgroundColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    SetName( DLG_WINDOW_NAME );
    m_DRCWarningTemplate = m_DRCExclusionsWarning->GetLabel();

    if( m_job )
    {
        SetTitle( aJob->GetSettingsDialogTitle() );

        PCB_PLOTTER::PlotJobToPlotOpts( m_plotOpts, m_job, m_messagesPanel->Reporter() );
        m_messagesPanel->Hide();

        m_browseButton->Hide();
        m_openDirButton->Hide();
        m_staticTextPlotFmt->Hide();
        m_plotFormatOpt->Hide();
        m_buttonDRC->Hide();
        m_DRCExclusionsWarning->Hide();
        m_sdbSizer1Apply->Hide();
        m_zoneFillCheck->SetLabel( _( "Refill zones before plotting" ) );
    }
    else
    {
        m_plotOpts = m_editFrame->GetPlotSettings();
        m_messagesPanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );
    }

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    int                       order = 0;
    wxArrayInt                plotAllLayersOrder;
    wxArrayString             plotAllLayersChoicesStrings;
    std::vector<PCB_LAYER_ID> layersIdChoiceList;
    int                       textWidth = 0;

    for( PCB_LAYER_ID layer : board->GetEnabledLayers().SeqStackupForPlotting() )
    {
        wxString     layerName = board->GetLayerName( layer );

        // wxCOL_WIDTH_AUTOSIZE doesn't work on all platforms, so we calculate width here
        textWidth = std::max( textWidth, KIUI::GetTextSize( layerName, m_layerCheckListBox ).x );

        plotAllLayersChoicesStrings.Add( layerName );
        layersIdChoiceList.push_back( layer );

        if( alg::contains( m_plotOpts.GetPlotOnAllLayersSequence(), layer ) )
            plotAllLayersOrder.push_back( order );
        else
            plotAllLayersOrder.push_back( ~order );

        order += 1;
    }

    int checkColSize = 22;
    int layerColSize = textWidth + 15;

#ifdef __WXMAC__
    // TODO: something in wxWidgets 3.1.x pads checkbox columns with extra space.  (It used to
    // also be that the width of the column would get set too wide (to 30), but that's patched in
    // our local wxWidgets fork.)
    checkColSize += 30;
#endif

    m_layerCheckListBox->SetMinClientSize( wxSize( checkColSize + layerColSize,
                                                   m_layerCheckListBox->GetMinClientSize().y ) );

    wxStaticBox*      allLayersLabel = new wxStaticBox( this, wxID_ANY, _( "Plot on All Layers" ) );
	wxStaticBoxSizer* sbSizer = new wxStaticBoxSizer( allLayersLabel, wxVERTICAL );

	m_plotAllLayersList = new wxRearrangeList( sbSizer->GetStaticBox(), wxID_ANY,
                                               wxDefaultPosition, wxDefaultSize,
                                               plotAllLayersOrder, plotAllLayersChoicesStrings, 0 );

    m_plotAllLayersList->SetMinClientSize( wxSize( checkColSize + layerColSize,
                                                   m_plotAllLayersList->GetMinClientSize().y ) );

    // Attach the LAYER_ID to each item in m_plotAllLayersList
    // plotAllLayersChoicesStrings and layersIdChoiceList are in the same order,
    // but m_plotAllLayersList has these strings in a different order
    for( size_t idx = 0; idx < layersIdChoiceList.size(); idx++ )
    {
        wxString& txt = plotAllLayersChoicesStrings[idx];
        int list_idx = m_plotAllLayersList->FindString( txt, true );

        PCB_LAYER_ID layer_id = layersIdChoiceList[idx];
        m_plotAllLayersList->SetClientObject( list_idx, new PCB_LAYER_ID_CLIENT_DATA( layer_id ) );
    }

    sbSizer->Add( m_plotAllLayersList, 1, wxALL | wxEXPAND | wxFIXED_MINSIZE, 3 );

    wxBoxSizer* bButtonSizer;
    bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

    m_bpMoveUp = new STD_BITMAP_BUTTON( sbSizer->GetStaticBox(), wxID_ANY, wxNullBitmap,
                                        wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW | 0 );
    m_bpMoveUp->SetToolTip( _( "Move current selection up" ) );
    m_bpMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );

    bButtonSizer->Add( m_bpMoveUp, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3 );

    m_bpMoveDown = new STD_BITMAP_BUTTON( sbSizer->GetStaticBox(), wxID_ANY, wxNullBitmap,
                                          wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW | 0 );
    m_bpMoveDown->SetToolTip( _( "Move current selection down" ) );
    m_bpMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    bButtonSizer->Add( m_bpMoveDown, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

    sbSizer->Add( bButtonSizer, 0, wxALL | wxEXPAND, 3 );

    bmiddleSizer->Insert( 1, sbSizer, 1, wxALL | wxEXPAND, 5 );

    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_openDirButton->SetBitmap( KiBitmapBundle( BITMAPS::small_new_window ) );

    if( m_job )
    {
        SetupStandardButtons();
    }
    else
    {
        SetupStandardButtons( { { wxID_OK, _( "Plot" ) },
                                { wxID_APPLY, _( "Generate Drill Files..." ) },
                                { wxID_CANCEL, _( "Close" ) } } );
    }

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );

    m_bpMoveUp->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &DIALOG_PLOT::onPlotAllListMoveUp, this );
    m_bpMoveDown->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &DIALOG_PLOT::onPlotAllListMoveDown, this );

    m_layerCheckListBox->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_PLOT::OnRightClickLayers ),
                                  nullptr, this );

    m_plotAllLayersList->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_PLOT::OnRightClickAllLayers ),
                                  nullptr, this );
}


DIALOG_PLOT::~DIALOG_PLOT()
{
    s_lastAllLayersOrder.clear();

    for( int ii = 0; ii < (int) m_plotAllLayersList->GetCount(); ++ii )
        s_lastAllLayersOrder.push_back( getLayerClientData( m_plotAllLayersList, ii )->Layer() );

    m_bpMoveDown->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &DIALOG_PLOT::onPlotAllListMoveDown, this );
    m_bpMoveUp->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &DIALOG_PLOT::onPlotAllListMoveUp, this );
}


bool DIALOG_PLOT::TransferDataToWindow()
{
    BOARD*      board = m_editFrame->GetBoard();
    wxFileName  fileName;

    PROJECT_FILE& projectFile = m_editFrame->Prj().GetProjectFile();

    // Could devote a PlotOrder() function in place of UIOrder().
    m_layerList = board->GetEnabledLayers().UIOrder();

    if( !m_job && !projectFile.m_PcbLastPath[ LAST_PATH_PLOT ].IsEmpty() )
        m_plotOpts.SetOutputDirectory( projectFile.m_PcbLastPath[ LAST_PATH_PLOT ] );

    if( m_job
        && !( m_job->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::POST
              && static_cast<JOB_EXPORT_PCB_PS*>( m_job )->m_useGlobalSettings ) )
    {
        // When we are using a job we get the PS adjust values from the plot options
        // The exception is when this is a fresh job and we want to get the global values as defaults
        m_fineAdjustXCtrl->SetValue( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                                          m_plotOpts.GetFineScaleAdjustX() ) );

        m_fineAdjustYCtrl->SetValue( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                                          m_plotOpts.GetFineScaleAdjustY() ) );
        m_trackWidthCorrection.SetValue( m_plotOpts.GetWidthAdjust() );
        m_zoneFillCheck->SetValue( m_job->m_checkZonesBeforePlot );
    }

    // The reasonable width correction value must be in a range of
    // [-(MinTrackWidth-1), +(MinClearanceValue-1)] decimils.
    m_widthAdjustMinValue   = -( board->GetDesignSettings().m_TrackMinWidth - 1 );
    m_widthAdjustMaxValue   = board->GetDesignSettings().GetSmallestClearanceValue() - 1;

    switch( m_plotOpts.GetFormat() )
    {
    default:
    case PLOT_FORMAT::GERBER: m_plotFormatOpt->SetSelection( 0 ); break;
    case PLOT_FORMAT::POST:   m_plotFormatOpt->SetSelection( 1 ); break;
    case PLOT_FORMAT::SVG:    m_plotFormatOpt->SetSelection( 2 ); break;
    case PLOT_FORMAT::DXF:    m_plotFormatOpt->SetSelection( 3 ); break;
    case PLOT_FORMAT::HPGL:   /* no longer supported */           break;
    case PLOT_FORMAT::PDF:    m_plotFormatOpt->SetSelection( 4 ); break;
    }

    m_plotPSNegativeOpt->SetValue( m_plotOpts.GetNegative() );
    m_forcePSA4OutputOpt->SetValue( m_plotOpts.GetA4Output() );

    // Populate the check list box by all enabled layers names.
    for( PCB_LAYER_ID layer : m_layerList )
    {
        int checkIndex = m_layerCheckListBox->Append( board->GetLayerName( layer ) );

        if( m_plotOpts.GetLayerSelection()[layer] )
            m_layerCheckListBox->Check( checkIndex );
    }

    arrangeAllLayersList( s_lastAllLayersOrder );

    // Option for disabling Gerber Aperture Macro (for broken Gerber readers)
    m_disableApertMacros->SetValue( m_plotOpts.GetDisableGerberMacros() );

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

    // SVG precision and units for coordinates
    m_svgPrecsision->SetValue( m_plotOpts.GetSvgPrecision() );
    m_SVG_fitPageToBoard->SetValue( m_plotOpts.GetSvgFitPagetoBoard() );

    m_sketchPadsOnFabLayers->SetValue( m_plotOpts.GetSketchPadsOnFabLayers() );
    m_plotPadNumbers->SetValue( m_plotOpts.GetPlotPadNumbers() );
    m_plotPadNumbers->Enable( m_plotOpts.GetSketchPadsOnFabLayers() );

    m_plotDNP->SetValue( m_plotOpts.GetHideDNPFPsOnFabLayers()
                            || m_plotOpts.GetSketchDNPFPsOnFabLayers()
                            || m_plotOpts.GetCrossoutDNPFPsOnFabLayers() );

    if( m_plotDNP->GetValue() )
    {
        if( m_plotOpts.GetHideDNPFPsOnFabLayers() )
            m_hideDNP->SetValue( true );
        else
            m_crossoutDNP->SetValue( true );
    }

    m_hideDNP->Enable( m_plotDNP->GetValue() );
    m_crossoutDNP->Enable( m_plotDNP->GetValue() );

    m_subtractMaskFromSilk->SetValue( m_plotOpts.GetSubtractMaskFromSilk() );

    m_useAuxOriginCheckBox->SetValue( m_plotOpts.GetUseAuxOrigin() );

    m_plotSheetRef->SetValue( m_plotOpts.GetPlotFrameRef() );

    // Options to plot pads and vias holes
    m_drillShapeOpt->SetSelection( (int) m_plotOpts.GetDrillMarksType() );

    // Scale option
    m_scaleOpt->SetSelection( m_plotOpts.GetScaleSelection() );

    // DXF outline mode
    m_DXF_plotModeOpt->SetValue( m_plotOpts.GetDXFPlotPolygonMode() );

    // DXF text mode
    m_DXF_plotTextStrokeFontOpt->SetValue( m_plotOpts.GetTextMode() == PLOT_TEXT_MODE::DEFAULT );

    // DXF units selection
    m_DXF_plotUnits->SetSelection( m_plotOpts.GetDXFPlotUnits() == DXF_UNITS::INCH ? 0 : 1 );

    // Plot mirror option
    m_plotMirrorOpt->SetValue( m_plotOpts.GetMirror() );

    // Black and white plotting
    m_SVGColorChoice->SetSelection( m_plotOpts.GetBlackAndWhite() ? 1 : 0 );
    m_PDFColorChoice->SetSelection( m_plotOpts.GetBlackAndWhite() ? 1 : 0 );
    m_frontFPPropertyPopups->SetValue( m_plotOpts.m_PDFFrontFPPropertyPopups );
    m_backFPPropertyPopups->SetValue( m_plotOpts.m_PDFBackFPPropertyPopups );
    m_pdfMetadata->SetValue( m_plotOpts.m_PDFMetadata );
    m_pdfSingle->SetValue( m_plotOpts.m_PDFSingle );
    m_pdfBackgroundColorSwatch->SetSwatchColor( m_plotOpts.m_PDFBackgroundColor, false );
    updatePdfColorOptions();

    // Initialize a few other parameters, which can also be modified
    // from the drill dialog
    reInitDialog();

    // Update options values:
    wxCommandEvent cmd_event;
    SetPlotFormat( cmd_event );

    return true;
}


void DIALOG_PLOT::transferPlotParamsToJob()
{
    if( m_job->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::GERBER )
    {
        JOB_EXPORT_PCB_GERBERS* gJob = static_cast<JOB_EXPORT_PCB_GERBERS*>( m_job );
        gJob->m_disableApertureMacros = m_plotOpts.GetDisableGerberMacros();
        gJob->m_useProtelFileExtension = m_plotOpts.GetUseGerberProtelExtensions();
        gJob->m_useX2Format = m_plotOpts.GetUseGerberX2format();
        gJob->m_includeNetlistAttributes = m_plotOpts.GetIncludeGerberNetlistInfo();
        gJob->m_createJobsFile = m_plotOpts.GetCreateGerberJobFile();
        gJob->m_precision = m_plotOpts.GetGerberPrecision();
        gJob->m_useBoardPlotParams = false;
    }
    else
    {
        m_job->m_scale = selectionToScale( m_plotOpts.GetScaleSelection() );
    }

    if( m_job->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::SVG )
    {
        JOB_EXPORT_PCB_SVG* svgJob = static_cast<JOB_EXPORT_PCB_SVG*>( m_job );
        svgJob->m_precision = m_plotOpts.GetSvgPrecision();
        svgJob->m_genMode = JOB_EXPORT_PCB_SVG::GEN_MODE::MULTI;
        svgJob->m_fitPageToBoard = m_plotOpts.GetSvgFitPagetoBoard();
    }

    if( m_job->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::DXF )
    {
        JOB_EXPORT_PCB_DXF* dxfJob = static_cast<JOB_EXPORT_PCB_DXF*>( m_job );
        dxfJob->m_dxfUnits = m_plotOpts.GetDXFPlotUnits() == DXF_UNITS::INCH ? JOB_EXPORT_PCB_DXF::DXF_UNITS::INCH
                                                                             : JOB_EXPORT_PCB_DXF::DXF_UNITS::MM;
        dxfJob->m_plotGraphicItemsUsingContours = m_plotOpts.GetDXFPlotMode() == DXF_OUTLINE_MODE::SKETCH;
        dxfJob->m_polygonMode = m_plotOpts.GetDXFPlotPolygonMode();
        dxfJob->m_genMode = m_plotOpts.GetDXFMultiLayeredExportOption() ? JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE
                                                                        : JOB_EXPORT_PCB_DXF::GEN_MODE::MULTI;
    }

    if( m_job->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::POST )
    {
        JOB_EXPORT_PCB_PS* psJob = static_cast<JOB_EXPORT_PCB_PS*>( m_job );
        psJob->m_genMode = JOB_EXPORT_PCB_PS::GEN_MODE::MULTI;
        psJob->m_XScaleAdjust = m_plotOpts.GetFineScaleAdjustX();
        psJob->m_YScaleAdjust = m_plotOpts.GetFineScaleAdjustY();
        psJob->m_trackWidthCorrection = pcbIUScale.IUTomm( m_plotOpts.GetWidthAdjust() );
        psJob->m_forceA4 = m_plotOpts.GetA4Output();
        // For a fresh job we got the adjusts from the global pcbnew settings
        // After the user confirmed and/or changed them we stop using the global adjusts
        psJob->m_useGlobalSettings = false;
    }

    if( m_job->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::PDF )
    {
        JOB_EXPORT_PCB_PDF* pdfJob = static_cast<JOB_EXPORT_PCB_PDF*>( m_job );
        pdfJob->m_pdfFrontFPPropertyPopups = m_plotOpts.m_PDFFrontFPPropertyPopups;
        pdfJob->m_pdfBackFPPropertyPopups = m_plotOpts.m_PDFBackFPPropertyPopups;
        pdfJob->m_pdfMetadata = m_plotOpts.m_PDFMetadata;
        pdfJob->m_pdfSingle = m_plotOpts.m_PDFSingle;
        pdfJob->m_pdfBackgroundColor = m_plotOpts.m_PDFBackgroundColor.ToCSSString();

        // we need to embed this for the cli deprecation fix
        if( pdfJob->m_pdfSingle )
        {
            pdfJob->m_pdfGenMode = JOB_EXPORT_PCB_PDF::GEN_MODE::ONE_PAGE_PER_LAYER_ONE_FILE;
        }
        else
        {
            pdfJob->m_pdfGenMode = JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_SEPARATE_FILE;
        }
    }

    m_job->m_subtractSolderMaskFromSilk = m_plotOpts.GetSubtractMaskFromSilk();
    m_job->m_useDrillOrigin = m_plotOpts.GetUseAuxOrigin();
    m_job->m_crossoutDNPFPsOnFabLayers = m_plotOpts.GetCrossoutDNPFPsOnFabLayers();
    m_job->m_hideDNPFPsOnFabLayers = m_plotOpts.GetHideDNPFPsOnFabLayers();
    m_job->m_sketchDNPFPsOnFabLayers = m_plotOpts.GetSketchDNPFPsOnFabLayers();
    m_job->m_sketchPadsOnFabLayers = m_plotOpts.GetSketchPadsOnFabLayers();

    m_job->m_plotDrawingSheet = m_plotOpts.GetPlotFrameRef();
    m_job->m_plotPadNumbers = m_plotOpts.GetPlotPadNumbers();

    m_job->m_blackAndWhite = m_plotOpts.GetBlackAndWhite();
    m_job->m_mirror = m_plotOpts.GetMirror();
    m_job->m_negative = m_plotOpts.GetNegative();
    m_job->m_plotLayerSequence = m_plotOpts.GetLayerSelection().SeqStackupForPlotting();
    m_job->m_plotOnAllLayersSequence = m_plotOpts.GetPlotOnAllLayersSequence();

    if( m_job->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::SVG ||
        m_job->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::PDF )
    {
        switch( m_plotOpts.GetDrillMarksType() )
        {
        case DRILL_MARKS::NO_DRILL_SHAPE:    m_job->m_drillShapeOption = DRILL_MARKS::NO_DRILL_SHAPE;    break;
        case DRILL_MARKS::SMALL_DRILL_SHAPE: m_job->m_drillShapeOption = DRILL_MARKS::SMALL_DRILL_SHAPE; break;
        default:
        case DRILL_MARKS::FULL_DRILL_SHAPE:  m_job->m_drillShapeOption = DRILL_MARKS::FULL_DRILL_SHAPE;  break;
        }
    }

    m_job->SetConfiguredOutputPath( m_plotOpts.GetOutputDirectory() );

    // this exists outside plot opts because its usually globally saved
    m_job->m_checkZonesBeforePlot = m_zoneFillCheck->GetValue();
}


void DIALOG_PLOT::reInitDialog()
{
    // after calling the Drill or DRC dialogs some parameters can be modified....

    // Output directory
    m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );

    // Origin of coordinates:
    m_useAuxOriginCheckBox->SetValue( m_plotOpts.GetUseAuxOrigin() );

    int knownViolations = 0;
    int exclusions = 0;

    for( PCB_MARKER* marker : m_editFrame->GetBoard()->Markers() )
    {
        if( marker->GetSeverity() == RPT_SEVERITY_EXCLUSION )
            exclusions++;
        else
            knownViolations++;
    }

    if( !m_job && ( knownViolations || exclusions ) )
    {
        m_DRCExclusionsWarning->SetLabel( wxString::Format( m_DRCWarningTemplate, knownViolations,
                                                            exclusions ) );
        m_DRCExclusionsWarning->Show();
    }
    else
    {
        m_DRCExclusionsWarning->Hide();
    }

    BOARD* board = m_editFrame->GetBoard();
    const BOARD_DESIGN_SETTINGS& brd_settings = board->GetDesignSettings();

    if( getPlotFormat() == PLOT_FORMAT::GERBER &&
        ( brd_settings.m_SolderMaskExpansion || brd_settings.m_SolderMaskMinWidth ) )
    {
        m_PlotOptionsSizer->Show( m_SizerSolderMaskAlert );
    }
    else
    {
        m_PlotOptionsSizer->Hide( m_SizerSolderMaskAlert );
    }
}


void DIALOG_PLOT::arrangeAllLayersList( const LSEQ& aSeq )
{
    auto findLayer =
            [&]( wxRearrangeList* aList, PCB_LAYER_ID aLayer ) -> int
            {
                for( int ii = 0; ii < (int) aList->GetCount(); ++ii )
                {
                    if( getLayerClientData( aList, ii )->Layer() == aLayer )
                        return ii;
                }

                return -1;
            };

    int  idx = 0;

    for( PCB_LAYER_ID layer : aSeq )
    {
        int currentPos = findLayer( m_plotAllLayersList, layer );

        while( currentPos > idx )
        {
            m_plotAllLayersList->Select( currentPos );
            m_plotAllLayersList->MoveCurrentUp();
            currentPos--;
        }

        idx++;
    }
}


#define ID_LAYER_FAB              13001
#define ID_SELECT_COPPER_LAYERS   13002
#define ID_DESELECT_COPPER_LAYERS 13003
#define ID_SELECT_ALL_LAYERS      13004
#define ID_DESELECT_ALL_LAYERS    13005
#define ID_STACKUP_ORDER          13006


// A helper function to show a popup menu, when the dialog is right clicked.
void DIALOG_PLOT::OnRightClickLayers( wxMouseEvent& event )
{
    // Build a list of layers for usual fabrication: copper layers + tech layers without courtyard
    LSET fab_layer_set = ( LSET::AllCuMask() | LSET::AllTechMask() ) & ~LSET( { B_CrtYd, F_CrtYd } );

    wxMenu menu;
    menu.Append( new wxMenuItem( &menu, ID_LAYER_FAB, _( "Select Fab Layers" ) ) );

    menu.AppendSeparator();
    menu.Append( new wxMenuItem( &menu, ID_SELECT_COPPER_LAYERS, _( "Select All Copper Layers" ) ) );
    menu.Append( new wxMenuItem( &menu, ID_DESELECT_COPPER_LAYERS, _( "Deselect All Copper Layers" ) ) );

    menu.AppendSeparator();
    menu.Append( new wxMenuItem( &menu, ID_SELECT_ALL_LAYERS, _( "Select All Layers" ) ) );
    menu.Append( new wxMenuItem( &menu, ID_DESELECT_ALL_LAYERS, _( "Deselect All Layers" ) ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
            [&]( wxCommandEvent& aCmd )
            {
                switch( aCmd.GetId() )
                {
                case ID_LAYER_FAB: // Select layers usually needed to build a board
                {
                    for( unsigned i = 0; i < m_layerList.size(); i++ )
                    {
                        LSET layermask( { m_layerList[ i ] } );

                        if( ( layermask & fab_layer_set ).any() )
                            m_layerCheckListBox->Check( i, true );
                        else
                            m_layerCheckListBox->Check( i, false );
                    }

                    break;
                }

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
                    aCmd.Skip();
                }
            } );

    PopupMenu( &menu );
}


void DIALOG_PLOT::OnRightClickAllLayers( wxMouseEvent& event )
{
    wxMenu menu;
    menu.Append( new wxMenuItem( &menu, ID_SELECT_ALL_LAYERS, _( "Select All Layers" ) ) );
    menu.Append( new wxMenuItem( &menu, ID_DESELECT_ALL_LAYERS, _( "Deselect All Layers" ) ) );

    menu.AppendSeparator();
    menu.Append( new wxMenuItem( &menu, ID_STACKUP_ORDER, _( "Order as Board Stackup" ) ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
            [&]( wxCommandEvent& aCmd )
            {
                switch( aCmd.GetId() )
                {
                case ID_SELECT_ALL_LAYERS:
                    for( unsigned i = 0; i < m_plotAllLayersList->GetCount(); i++ )
                        m_plotAllLayersList->Check( i, true );

                    break;

                case ID_DESELECT_ALL_LAYERS:
                    for( unsigned i = 0; i < m_plotAllLayersList->GetCount(); i++ )
                        m_plotAllLayersList->Check( i, false );

                    break;

                case ID_STACKUP_ORDER:
                {
                    LSEQ stackup = m_editFrame->GetBoard()->GetEnabledLayers().SeqStackupForPlotting();
                    arrangeAllLayersList( stackup );
                    m_plotAllLayersList->Select( -1 );
                    break;
                }

                default:
                    aCmd.Skip();
                }
            } );

    PopupMenu( &menu );
}


void DIALOG_PLOT::CreateDrillFile( wxCommandEvent& event )
{
    // Be sure drill file use the same settings (axis option, plot directory) as plot files:
    applyPlotSettings();

    DIALOG_GENDRILL dlg( m_editFrame, this );
    dlg.ShowModal();

    // a few plot settings can be modified: take them in account
    m_plotOpts = m_editFrame->GetPlotSettings();
    reInitDialog();
}


void DIALOG_PLOT::OnChangeDXFPlotMode( wxCommandEvent& event )
{
    // m_DXF_plotTextStrokeFontOpt is disabled if m_DXF_plotModeOpt is checked (plot in DXF
    // polygon mode)
    m_DXF_plotTextStrokeFontOpt->Enable( !m_DXF_plotModeOpt->GetValue() );

    // if m_DXF_plotTextStrokeFontOpt option is disabled (plot DXF in polygon mode), force
    // m_DXF_plotTextStrokeFontOpt to true to use Pcbnew stroke font
    if( !m_DXF_plotTextStrokeFontOpt->IsEnabled() )
        m_DXF_plotTextStrokeFontOpt->SetValue( true );
}


void DIALOG_PLOT::onOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                return m_editFrame->GetBoard()->ResolveTextVar( token, 0 );
            };

    wxString path = m_outputDirectoryName->GetValue();
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, &Prj() );
    path = Prj().AbsolutePath( path );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxFileName fn( Prj().AbsolutePath( m_editFrame->GetBoard()->GetFileName() ) );
    wxString   defaultPath = fn.GetPathWithSep();
    wxString   msg;
    wxFileName relPathTest; // Used to test if we can make the path relative

    relPathTest.Assign( dirDialog.GetPath() );

    // Test if making the path relative is possible before asking the user if they want to do it
    if( relPathTest.MakeRelativeTo( defaultPath ) )
    {
        if( IsOK( this, wxString::Format( _( "Do you want to use a path relative to\n'%s'?" ), defaultPath ) ) )
            dirName.MakeRelativeTo( defaultPath );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


PLOT_FORMAT DIALOG_PLOT::getPlotFormat()
{
    // plot format id's are ordered like displayed in m_plotFormatOpt
    static const PLOT_FORMAT plotFmt[] = {
            PLOT_FORMAT::GERBER,
            PLOT_FORMAT::POST,
            PLOT_FORMAT::SVG,
            PLOT_FORMAT::DXF,
            PLOT_FORMAT::PDF };

    return plotFmt[m_plotFormatOpt->GetSelection()];
}


void DIALOG_PLOT::SetPlotFormat( wxCommandEvent& event )
{
    // this option exist only in DXF format:
    m_DXF_plotModeOpt->Enable( getPlotFormat() == PLOT_FORMAT::DXF );

    // The alert message about non 0 solder mask min width and margin is shown
    // only in gerber format and if min mask width or mask margin is not 0
    BOARD* board = m_editFrame->GetBoard();
    const BOARD_DESIGN_SETTINGS& brd_settings = board->GetDesignSettings();

    if( getPlotFormat() == PLOT_FORMAT::GERBER
            && ( brd_settings.m_SolderMaskExpansion || brd_settings.m_SolderMaskMinWidth ) )
    {
        m_PlotOptionsSizer->Show( m_SizerSolderMaskAlert );
    }
    else
    {
        m_PlotOptionsSizer->Hide( m_SizerSolderMaskAlert );
    }

    switch( getPlotFormat() )
    {
    case PLOT_FORMAT::SVG:
    case PLOT_FORMAT::PDF:
        m_drillShapeOpt->Enable( true );
        m_plotMirrorOpt->Enable( true );
        m_useAuxOriginCheckBox->Enable( true );
        m_scaleOpt->Enable( true );
        m_fineAdjustXCtrl->Enable( false );
        m_fineAdjustYCtrl->Enable( false );
        m_trackWidthCorrection.Enable( false );
        m_plotPSNegativeOpt->Enable( true );
        m_forcePSA4OutputOpt->Enable( false );
        m_forcePSA4OutputOpt->SetValue( false );

        if( getPlotFormat() == PLOT_FORMAT::SVG )
        {
            m_PlotOptionsSizer->Show( m_svgOptionsSizer );
            m_PlotOptionsSizer->Hide( m_PDFOptionsSizer );
        }
        else
        {
            m_PlotOptionsSizer->Hide( m_svgOptionsSizer );
            m_PlotOptionsSizer->Show( m_PDFOptionsSizer );
        }

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        m_PlotOptionsSizer->Hide( m_SizerDXF_options );
        break;

    case PLOT_FORMAT::POST:
        m_drillShapeOpt->Enable( true );
        m_plotMirrorOpt->Enable( true );
        m_useAuxOriginCheckBox->Enable( false );
        m_useAuxOriginCheckBox->SetValue( false );
        m_scaleOpt->Enable( true );
        m_fineAdjustXCtrl->Enable( true );
        m_fineAdjustYCtrl->Enable( true );
        m_trackWidthCorrection.Enable( true );
        m_plotPSNegativeOpt->Enable( true );
        m_forcePSA4OutputOpt->Enable( true );

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Show( m_PSOptionsSizer );
        m_PlotOptionsSizer->Hide( m_SizerDXF_options );
        m_PlotOptionsSizer->Hide( m_svgOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PDFOptionsSizer );
        break;

    case PLOT_FORMAT::GERBER:
        m_drillShapeOpt->Enable( false );
        m_drillShapeOpt->SetSelection( 0 );
        m_plotMirrorOpt->Enable( false );
        m_plotMirrorOpt->SetValue( false );
        m_useAuxOriginCheckBox->Enable( true );
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
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        m_PlotOptionsSizer->Hide( m_SizerDXF_options );
        m_PlotOptionsSizer->Hide( m_svgOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PDFOptionsSizer );
        break;

    case PLOT_FORMAT::DXF:
        m_drillShapeOpt->Enable( true );
        m_plotMirrorOpt->Enable( false );
        m_plotMirrorOpt->SetValue( false );
        m_useAuxOriginCheckBox->Enable( true );
        m_scaleOpt->Enable( true );
        m_fineAdjustXCtrl->Enable( false );
        m_fineAdjustYCtrl->Enable( false );
        m_trackWidthCorrection.Enable( false );
        m_plotPSNegativeOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_forcePSA4OutputOpt->Enable( false );
        m_forcePSA4OutputOpt->SetValue( false );

        m_PlotOptionsSizer->Hide( m_GerberOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PSOptionsSizer );
        m_PlotOptionsSizer->Show( m_SizerDXF_options );
        m_PlotOptionsSizer->Hide( m_svgOptionsSizer );
        m_PlotOptionsSizer->Hide( m_PDFOptionsSizer );

        OnChangeDXFPlotMode( event );
        break;

    default:
    case PLOT_FORMAT::HPGL:
    case PLOT_FORMAT::UNDEFINED:
        break;
    }

    Layout();
    m_MainSizer->SetSizeHints( this );
}


// A helper function to "clip" aValue between aMin and aMax and write result in * aResult
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
    PCB_PLOT_PARAMS tempOptions;

    tempOptions.SetSubtractMaskFromSilk( m_subtractMaskFromSilk->GetValue() );
    tempOptions.SetPlotFrameRef( m_plotSheetRef->GetValue() );
    tempOptions.SetSketchPadsOnFabLayers( m_sketchPadsOnFabLayers->GetValue() );
    tempOptions.SetPlotPadNumbers( m_plotPadNumbers->GetValue() );
    tempOptions.SetHideDNPFPsOnFabLayers( m_plotDNP->GetValue()
                                            && m_hideDNP->GetValue() );
    tempOptions.SetSketchDNPFPsOnFabLayers( m_plotDNP->GetValue()
                                            && m_crossoutDNP->GetValue() );
    tempOptions.SetCrossoutDNPFPsOnFabLayers( m_plotDNP->GetValue()
                                            && m_crossoutDNP->GetValue() );
    tempOptions.SetUseAuxOrigin( m_useAuxOriginCheckBox->GetValue() );
    tempOptions.SetScaleSelection( m_scaleOpt->GetSelection() );

    int sel = m_drillShapeOpt->GetSelection();
    tempOptions.SetDrillMarksType( static_cast<DRILL_MARKS>( sel ) );

    tempOptions.SetMirror( m_plotMirrorOpt->GetValue() );
    tempOptions.SetDXFPlotPolygonMode( m_DXF_plotModeOpt->GetValue() );

    sel = m_DXF_plotUnits->GetSelection();
    tempOptions.SetDXFPlotUnits( sel == 0 ? DXF_UNITS::INCH : DXF_UNITS::MM );

    if( !m_DXF_plotTextStrokeFontOpt->IsEnabled() ) // Currently, only DXF supports this option
        tempOptions.SetTextMode( PLOT_TEXT_MODE::DEFAULT );
    else
        tempOptions.SetTextMode( m_DXF_plotTextStrokeFontOpt->GetValue() ? PLOT_TEXT_MODE::DEFAULT :
                                                                           PLOT_TEXT_MODE::NATIVE );

    tempOptions.SetDXFMultiLayeredExportOption( m_DXF_exportAsMultiLayeredFile->GetValue() );

    if( getPlotFormat() == PLOT_FORMAT::SVG )
    {
        tempOptions.SetBlackAndWhite( m_SVGColorChoice->GetSelection() == 1 );
    }
    else if( getPlotFormat() == PLOT_FORMAT::PDF )
    {
        tempOptions.SetBlackAndWhite( m_PDFColorChoice->GetSelection() == 1 );
        tempOptions.m_PDFFrontFPPropertyPopups = m_frontFPPropertyPopups->GetValue();
        tempOptions.m_PDFBackFPPropertyPopups = m_backFPPropertyPopups->GetValue();
        tempOptions.m_PDFMetadata = m_pdfMetadata->GetValue();
        tempOptions.m_PDFSingle = m_pdfSingle->GetValue();
        tempOptions.m_PDFBackgroundColor = m_pdfBackgroundColorSwatch->GetSwatchColor();
    }
    else
    {
        tempOptions.SetBlackAndWhite( true );
    }

    // Update settings from text fields. Rewrite values back to the fields,
    // since the values may have been constrained by the setters.

    // X scale
    double   tmpDouble;
    wxString msg = m_fineAdjustXCtrl->GetValue();
    msg.ToDouble( &tmpDouble );

    if( !setDouble( &m_XScaleAdjust, tmpDouble, PLOT_MIN_SCALE, PLOT_MAX_SCALE ) )
    {
        msg.Printf( wxT( "%f" ), m_XScaleAdjust );
        m_fineAdjustXCtrl->SetValue( msg );
        msg.Printf( _( "X scale constrained." ) );
        reporter.Report( msg, RPT_SEVERITY_INFO );
    }

    // Y scale
    msg = m_fineAdjustYCtrl->GetValue();
    msg.ToDouble( &tmpDouble );

    if( !setDouble( &m_YScaleAdjust, tmpDouble, PLOT_MIN_SCALE, PLOT_MAX_SCALE ) )
    {
        msg.Printf( wxT( "%f" ), m_YScaleAdjust );
        m_fineAdjustYCtrl->SetValue( msg );
        msg.Printf( _( "Y scale constrained." ) );
        reporter.Report( msg, RPT_SEVERITY_INFO );
    }

    // PS Width correction
    if( !setInt( &m_PSWidthAdjust, m_trackWidthCorrection.GetIntValue(), m_widthAdjustMinValue,
                 m_widthAdjustMaxValue ) )
    {
        m_trackWidthCorrection.SetValue( m_PSWidthAdjust );
        msg.Printf( _( "Width correction constrained.  The width correction value must be in the"
                       " range of [%s; %s] for the current design rules." ),
                    m_editFrame->StringFromValue( m_widthAdjustMinValue, true ),
                    m_editFrame->StringFromValue( m_widthAdjustMaxValue, true ) );
        reporter.Report( msg, RPT_SEVERITY_WARNING );
    }

    if( m_job )
    {
        // When using a job we store the adjusts in the plot options
        tempOptions.SetFineScaleAdjustX( m_XScaleAdjust );
        tempOptions.SetFineScaleAdjustY( m_YScaleAdjust );
        tempOptions.SetWidthAdjust( m_PSWidthAdjust );
    }

    tempOptions.SetFormat( getPlotFormat() );

    tempOptions.SetDisableGerberMacros( m_disableApertMacros->GetValue() );
    tempOptions.SetUseGerberProtelExtensions( m_useGerberExtensions->GetValue() );
    tempOptions.SetUseGerberX2format( m_useGerberX2Format->GetValue() );
    tempOptions.SetIncludeGerberNetlistInfo( m_useGerberNetAttributes->GetValue() );
    tempOptions.SetCreateGerberJobFile( m_generateGerberJobFile->GetValue() );

    tempOptions.SetGerberPrecision( m_coordFormatCtrl->GetSelection() == 0 ? 5 : 6 );
    tempOptions.SetSvgPrecision( m_svgPrecsision->GetValue() );
    tempOptions.SetSvgFitPageToBoard( m_SVG_fitPageToBoard->GetValue() );

    LSET selectedLayers;

    for( unsigned i = 0; i < m_layerList.size(); i++ )
    {
        if( m_layerCheckListBox->IsChecked( i ) )
            selectedLayers.set( m_layerList[i] );
    }

    // Get a list of copper layers that aren't being used by inverting enabled layers.
    LSET disabledCopperLayers = LSET::AllCuMask() & ~m_editFrame->GetBoard()->GetEnabledLayers();

    // Add selected layers from plot on all layers list in order set by user.
    wxArrayInt plotOnAllLayers;
    LSEQ commonLayers;

    if( m_plotAllLayersList->GetCheckedItems( plotOnAllLayers ) )
    {
        size_t count = plotOnAllLayers.GetCount();

        for( size_t i = 0; i < count; i++ )
        {
            int          index = plotOnAllLayers.Item( i );
            PCB_LAYER_ID client_layer = getLayerClientData( m_plotAllLayersList, index )->Layer();

            commonLayers.push_back( client_layer );
        }
    }

    tempOptions.SetPlotOnAllLayersSequence( commonLayers );

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
    m_editFrame->Prj().GetProjectFile().m_PcbLastPath[ LAST_PATH_PLOT ] = dirStr;

    if( !m_job && !m_plotOpts.IsSameAs( tempOptions ) )
    {
        m_editFrame->SetPlotSettings( tempOptions );
        m_editFrame->OnModify();
        m_plotOpts = tempOptions;
    }
    else
    {
        m_plotOpts = tempOptions;
    }
}


void DIALOG_PLOT::OnGerberX2Checked( wxCommandEvent& event )
{
    // Currently: do nothing
}


void DIALOG_PLOT::Plot( wxCommandEvent& event )
{
    if( m_job )
    {
        applyPlotSettings();
        transferPlotParamsToJob();
        EndModal( wxID_OK );
    }
    else
    {
        BOARD* board = m_editFrame->GetBoard();

        applyPlotSettings();

        PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

        m_plotOpts.SetColorSettings( ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME ) );

        m_plotOpts.SetSketchPadLineWidth( board->GetDesignSettings().GetLineThickness( F_Fab ) );

        // If no layer selected, we have nothing plotted.
        // Prompt user if it happens because he could think there is a bug in Pcbnew.
        if( !m_plotOpts.GetLayerSelection().any() )
        {
            DisplayError( this, _( "No layer selected, Nothing to plot" ) );
            return;
        }

        // Create output directory if it does not exist (also transform it in absolute form).
        // Bail if it fails.

        std::function<bool( wxString* )> textResolver =
                [&]( wxString* token ) -> bool
                {
                    // Handles board->GetTitleBlock() *and* board->GetProject()
                    return m_editFrame->GetBoard()->ResolveTextVar( token, 0 );
                };

        wxString path = m_plotOpts.GetOutputDirectory();
        path = ExpandTextVars( path, &textResolver );
        path = ExpandEnvVarSubstitutions( path, board->GetProject() );

        wxFileName outputDir = wxFileName::DirName( path );
        wxString   boardFilename = m_editFrame->GetBoard()->GetFileName();
        REPORTER&  reporter = m_messagesPanel->Reporter();

        if( !EnsureFileDirectoryExists( &outputDir, boardFilename, &reporter ) )
        {
            wxString msg;
            msg.Printf( _( "Could not write plot files to folder '%s'." ), outputDir.GetPath() );
            DisplayError( this, msg );
            return;
        }

        if( m_zoneFillCheck->GetValue() )
            m_editFrame->GetToolManager()->GetTool<ZONE_FILLER_TOOL>()->CheckAllZones( this );

        m_plotOpts.SetAutoScale( false );

        switch( m_plotOpts.GetScaleSelection() )
        {
        default: m_plotOpts.SetScale( 1 );        break;
        case 0:  m_plotOpts.SetAutoScale( true ); break;
        case 2:  m_plotOpts.SetScale( 1.5 );      break;
        case 3:  m_plotOpts.SetScale( 2 );        break;
        case 4:  m_plotOpts.SetScale( 3 );        break;
        }

        /* If the scale factor edit controls are disabled or the scale value
         * is 0, don't adjust the base scale factor. This fixes a bug when
         * the default scale adjust is initialized to 0 and saved in program
         * settings resulting in a divide by zero fault.
         */
        if( getPlotFormat() == PLOT_FORMAT::POST )
        {
            if( m_XScaleAdjust != 0.0 )
                m_plotOpts.SetFineScaleAdjustX( m_XScaleAdjust );

            if( m_YScaleAdjust != 0.0 )
                m_plotOpts.SetFineScaleAdjustY( m_YScaleAdjust );

            m_plotOpts.SetWidthAdjust( m_PSWidthAdjust );
        }

        // Test for a reasonable scale value
        // XXX could this actually happen? isn't it constrained in the apply function?
        if( m_plotOpts.GetScale() < PLOT_MIN_SCALE )
            DisplayInfoMessage( this, _( "Warning: Scale option set to a very small value" ) );

        if( m_plotOpts.GetScale() > PLOT_MAX_SCALE )
            DisplayInfoMessage( this, _( "Warning: Scale option set to a very large value" ) );


        // Save the current plot options in the board
        m_editFrame->SetPlotSettings( m_plotOpts );

        PCB_PLOTTER pcbPlotter( m_editFrame->GetBoard(), &reporter, m_plotOpts );

        LSEQ layersToPlot = m_plotOpts.GetLayerSelection().UIOrder();

        wxArrayInt plotOnAllLayers;
        LSEQ commonLayers;

        if( m_plotAllLayersList->GetCheckedItems( plotOnAllLayers ) )
        {
            size_t count = plotOnAllLayers.GetCount();

            for( size_t i = 0; i < count; i++ )
            {
                int          index = plotOnAllLayers.Item( i );
                PCB_LAYER_ID client_layer = getLayerClientData( m_plotAllLayersList, index )->Layer();

                commonLayers.push_back( client_layer );
            }
        }

        pcbPlotter.Plot( outputDir.GetPath(), layersToPlot, commonLayers, m_useGerberExtensions->GetValue() );
    }
}



void DIALOG_PLOT::onRunDRC( wxCommandEvent& event )
{
    PCB_EDIT_FRAME* parent = dynamic_cast<PCB_EDIT_FRAME*>( GetParent() );

    if( parent )
    {
        DRC_TOOL* drcTool = parent->GetToolManager()->GetTool<DRC_TOOL>();

        // First close an existing dialog if open
        // (low probability, but can happen)
        drcTool->DestroyDRCDialog();

        // Open a new drc dialog, with the right parent frame, and in Modal Mode
        drcTool->ShowDRCDialog( this );

        // Update DRC warnings on return to this dialog
        reInitDialog();
    }
}


void DIALOG_PLOT::onOpenOutputDirectory( wxCommandEvent& event )
{
    std::function<bool( wxString* )> textResolver = [&]( wxString* token ) -> bool
    {
        return m_editFrame->GetBoard()->ResolveTextVar( token, 0 );
    };

    wxString path = m_outputDirectoryName->GetValue();
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, &Prj() );
    path = Prj().AbsolutePath( path );

    if( !wxDirExists( path ) )
    {
        DisplayError( this, wxString::Format( _( "Directory '%s' does not exist." ), path ) );
        return;
    }

    wxLaunchDefaultApplication( path );
}


void DIALOG_PLOT::onBoardSetup( wxHyperlinkEvent& aEvent )
{
    PCB_EDIT_FRAME* parent = dynamic_cast<PCB_EDIT_FRAME*>( GetParent() );

    if( parent )
    {
        parent->ShowBoardSetupDialog( _( "Solder Mask/Paste" ) );

        // Update warnings on return to this dialog
        reInitDialog();
    }
}


void DIALOG_PLOT::onPlotAllListMoveUp( wxCommandEvent& aEvent )
{
    if( m_plotAllLayersList->CanMoveCurrentUp() )
        m_plotAllLayersList->MoveCurrentUp();
}


void DIALOG_PLOT::onPlotAllListMoveDown( wxCommandEvent& aEvent )
{
    if( m_plotAllLayersList->CanMoveCurrentDown() )
        m_plotAllLayersList->MoveCurrentDown();
}


void DIALOG_PLOT::onDNPCheckbox( wxCommandEvent& aEvent )
{
    m_hideDNP->Enable( aEvent.IsChecked() );
    m_crossoutDNP->Enable( aEvent.IsChecked() );
}


void DIALOG_PLOT::onSketchPads( wxCommandEvent& aEvent )
{
    m_plotPadNumbers->Enable( aEvent.IsChecked() );
}


void DIALOG_PLOT::updatePdfColorOptions()
{
    if( m_PDFColorChoice->GetSelection() == 1 )
    {
        m_pdfBackgroundColorSwatch->Disable();
        m_pdfBackgroundColorText->Disable();
    }
    else
    {
        m_pdfBackgroundColorSwatch->Enable();
        m_pdfBackgroundColorText->Enable();
    }
}


void DIALOG_PLOT::onPDFColorChoice( wxCommandEvent& aEvent )
{
    updatePdfColorOptions();
}
