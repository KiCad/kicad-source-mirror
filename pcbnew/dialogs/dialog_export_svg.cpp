/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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


#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <core/arraydim.h>
#include <core/kicad_algo.h>
#include <pcbplot.h>
#include <locale_io.h>
#include <board.h>
#include <dialog_export_svg_base.h>
#include <wx_html_report_panel.h>
#include <bitmaps.h>
#include <widgets/unit_binder.h>
#include <plotters/plotters_pslike.h>
#include <wx/dirdlg.h>

class DIALOG_EXPORT_SVG : public DIALOG_EXPORT_SVG_BASE
{
public:
    DIALOG_EXPORT_SVG( PCB_EDIT_FRAME* aParent, BOARD* aBoard );
    ~DIALOG_EXPORT_SVG() override;

private:
    BOARD*            m_board;
    PCB_EDIT_FRAME*   m_parent;
    LSET              m_printMaskLayer;
    // the list of existing board layers in wxCheckListBox, with the
    // board layers id:
    std::pair<wxCheckListBox*, int> m_boxSelectLayer[PCB_LAYER_ID_COUNT];
    bool              m_printBW;
    wxString          m_outputDirectory;
    bool              m_printMirror;
    bool              m_oneFileOnly;

    void initDialog();

    void OnButtonPlot( wxCommandEvent& event ) override;

    void onPagePerLayerClicked( wxCommandEvent& event ) override;
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;
    void ExportSVGFile( bool aOnlyOneFile );

    bool CreateSVGFile( const wxString& FullFileName );

    LSET getCheckBoxSelectedLayers() const;
};


/*
 * DIALOG_EXPORT_SVG functions
 */
DIALOG_EXPORT_SVG::DIALOG_EXPORT_SVG( PCB_EDIT_FRAME* aParent, BOARD* aBoard ) :
        DIALOG_EXPORT_SVG_BASE( aParent ),
        m_board( aBoard ),
        m_parent( aParent ),
        m_printBW( false ),
        m_printMirror( false ),
        m_oneFileOnly( false )
{
    m_browseButton->SetBitmap( KiBitmap( BITMAPS::small_folder ) );

    m_messagesPanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    initDialog();

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Export" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    finishDialogSettings();
}


DIALOG_EXPORT_SVG::~DIALOG_EXPORT_SVG()
{
    m_printBW = m_ModeColorOption->GetSelection();
    m_oneFileOnly = !m_checkboxPagePerLayer->GetValue();
    m_outputDirectory = m_outputDirectoryName->GetValue();
    m_outputDirectory.Replace( wxT( "\\" ), wxT( "/" ) );

    auto cfg = m_parent->GetPcbNewSettings();

    cfg->m_ExportSvg.black_and_white  = m_printBW;
    cfg->m_ExportSvg.mirror           = m_printMirror;
    cfg->m_ExportSvg.one_file         = m_oneFileOnly;
    cfg->m_ExportSvg.page_size        = m_rbSvgPageSizeOpt->GetSelection();
    cfg->m_ExportSvg.output_dir       = m_outputDirectory.ToStdString();

    if( m_checkboxPagePerLayer->GetValue() )
    {
        m_oneFileOnly = false;
        cfg->m_ExportSvg.plot_board_edges = m_checkboxEdgesOnAllPages->GetValue();
    }
    else
    {
        m_oneFileOnly = true;
    }

    cfg->m_ExportSvg.layers.clear();

    for( unsigned layer = 0; layer < arrayDim( m_boxSelectLayer ); ++layer )
    {
        if( !m_boxSelectLayer[layer].first )
            continue;

        if( m_boxSelectLayer[layer].first->IsChecked( m_boxSelectLayer[layer].second ) )
            cfg->m_ExportSvg.layers.push_back( m_boxSelectLayer[layer].second );
    }
}


void DIALOG_EXPORT_SVG::initDialog()
{
    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

    m_printBW         = cfg->m_ExportSvg.black_and_white;
    m_printMirror     = cfg->m_ExportSvg.mirror;
    m_oneFileOnly     = cfg->m_ExportSvg.one_file;
    m_outputDirectory = cfg->m_ExportSvg.output_dir;

    m_rbSvgPageSizeOpt->SetSelection( cfg->m_ExportSvg.page_size );
    m_checkboxPagePerLayer->SetValue( !m_oneFileOnly );

    wxCommandEvent dummy;
    onPagePerLayerClicked( dummy );

    m_outputDirectoryName->SetValue( m_outputDirectory );

    m_ModeColorOption->SetSelection( m_printBW ? 1 : 0 );
    m_printMirrorOpt->SetValue( m_printMirror );

    for( LSEQ seq = m_board->GetEnabledLayers().UIOrder(); seq; ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        int checkIndex;

        if( IsCopperLayer( layer ) )
        {
            checkIndex = m_CopperLayersList->Append( m_board->GetLayerName( layer ) );
            m_boxSelectLayer[layer] = std::make_pair( m_CopperLayersList, checkIndex );
        }
        else
        {
            checkIndex = m_TechnicalLayersList->Append( m_board->GetLayerName( layer ) );
            m_boxSelectLayer[layer] = std::make_pair( m_TechnicalLayersList, checkIndex );
        }

        if( alg::contains( cfg->m_ExportSvg.layers, layer ) )
            m_boxSelectLayer[layer].first->Check( checkIndex, true );
    }
}


LSET DIALOG_EXPORT_SVG::getCheckBoxSelectedLayers() const
{
    LSET ret;

    for( unsigned layer = 0; layer < arrayDim(m_boxSelectLayer);  ++layer )
    {
        if( !m_boxSelectLayer[layer].first )
            continue;

        if( m_boxSelectLayer[layer].first->IsChecked( m_boxSelectLayer[layer].second ) )
            ret.set( layer );
    }

    return ret;
}


void DIALOG_EXPORT_SVG::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputDirectoryName->GetValue(), &Prj() );
    path = Prj().AbsolutePath( path );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxMessageDialog dialog( this, _( "Use a relative path?" ), _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    if( dialog.ShowModal() == wxID_YES )
    {
        wxString boardFilePath = Prj().AbsolutePath( m_board->GetFileName() );

        boardFilePath = wxPathOnly( boardFilePath );

        if( !dirName.MakeRelativeTo( boardFilePath ) )
            wxMessageBox( _( "Cannot make path relative (target volume different from board file volume)!" ),
                          _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
    m_outputDirectory = m_outputDirectoryName->GetValue();
}


void DIALOG_EXPORT_SVG::onPagePerLayerClicked( wxCommandEvent& event )
{
    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

    if( m_checkboxPagePerLayer->GetValue() )
    {
        m_checkboxEdgesOnAllPages->Enable( true );
        m_checkboxEdgesOnAllPages->SetValue( cfg->m_ExportSvg.plot_board_edges );
    }
    else
    {
        m_checkboxEdgesOnAllPages->Enable( false );
        m_checkboxEdgesOnAllPages->SetValue( false );
    }
}


void DIALOG_EXPORT_SVG::ExportSVGFile( bool aOnlyOneFile )
{
    m_outputDirectory = m_outputDirectoryName->GetValue();

    // Create output directory if it does not exist (also transform it in
    // absolute form). Bail if it fails
    wxString   path = ExpandEnvVarSubstitutions( m_outputDirectory, &Prj() );
    wxFileName outputDir = wxFileName::DirName( path );
    wxString   boardFilename = m_board->GetFileName();

    REPORTER& reporter = m_messagesPanel->Reporter();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, &reporter ) )
    {
        wxString msg = wxString::Format( _( "Could not write plot files to folder \"%s\"." ),
                                         outputDir.GetPath() );
        DisplayError( this, msg );
        return;
    }

    m_printMirror = m_printMirrorOpt->GetValue();
    m_printBW = m_ModeColorOption->GetSelection();

    LSET all_selected = getCheckBoxSelectedLayers();

    for( LSEQ seq = all_selected.Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        wxFileName   fn( boardFilename );
        wxString     suffix = aOnlyOneFile ? wxT( "brd" ) : m_board->GetStandardLayerName( layer );

        BuildPlotFileName( &fn, outputDir.GetPath(), suffix, SVGFileExtension );
        wxString svgPath = fn.GetFullPath();

        m_printMaskLayer = aOnlyOneFile ? all_selected : LSET( layer );

        if( m_checkboxEdgesOnAllPages->GetValue() )
            m_printMaskLayer.set( Edge_Cuts );

        if( CreateSVGFile( svgPath ) )
        {
            reporter.Report( wxString::Format( _( "Exported \"%s\"." ), svgPath ), RPT_SEVERITY_ACTION );
        }
        else    // Error
        {
            reporter.Report( wxString::Format( _( "Unable to create file \"%s\"." ), svgPath ),
                             RPT_SEVERITY_ERROR );
        }

        if( aOnlyOneFile )
            break;
    }
}


// Actual SVG file export function.
bool DIALOG_EXPORT_SVG::CreateSVGFile( const wxString& aFullFileName )
{
    PCB_PLOT_PARAMS plot_opts;

    plot_opts.SetPlotFrameRef( m_rbSvgPageSizeOpt->GetSelection() == 0 );

    // Adding drill marks, for copper layers
    if( ( m_printMaskLayer & LSET::AllCuMask() ).any() )
        plot_opts.SetDrillMarksType( PCB_PLOT_PARAMS::FULL_DRILL_SHAPE );
    else
        plot_opts.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

    plot_opts.SetSkipPlotNPTH_Pads( false );

    plot_opts.SetMirror( m_printMirror );
    plot_opts.SetFormat( PLOT_FORMAT::SVG );

    PAGE_INFO   savedPageInfo = m_board->GetPageSettings();
    wxPoint     savedAuxOrigin = m_board->GetDesignSettings().m_AuxOrigin;

    if( m_rbSvgPageSizeOpt->GetSelection() == 2 )   // Page is board boundary size
    {
        EDA_RECT    bbox = m_board->ComputeBoundingBox();
        PAGE_INFO   currpageInfo = m_board->GetPageSettings();

        currpageInfo.SetWidthMils(  bbox.GetWidth() / IU_PER_MILS );
        currpageInfo.SetHeightMils( bbox.GetHeight() / IU_PER_MILS );
        m_board->SetPageSettings( currpageInfo );
        plot_opts.SetUseAuxOrigin( true );
        wxPoint origin = bbox.GetOrigin();
        m_board->GetDesignSettings().m_AuxOrigin = origin;
    }

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    PCBNEW_SETTINGS*  cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>();

    plot_opts.SetColorSettings( mgr.GetColorSettings( cfg->m_ColorTheme ) );

    LOCALE_IO    toggle;

    SVG_PLOTTER* plotter = (SVG_PLOTTER*) StartPlotBoard( m_board, &plot_opts, UNDEFINED_LAYER,
                                                          aFullFileName, wxEmptyString );

    if( plotter )
    {
        plotter->SetColorMode( !m_printBW );

        for( LSEQ seq = m_printMaskLayer.SeqStackupBottom2Top();  seq;  ++seq )
            PlotOneBoardLayer( m_board, plotter, *seq, plot_opts );

        plotter->EndPlot();
    }

    delete plotter;

    // reset to the values saved earlier
    m_board->GetDesignSettings().m_AuxOrigin = savedAuxOrigin;
    m_board->SetPageSettings( savedPageInfo );

    return true;
}


void DIALOG_EXPORT_SVG::OnButtonPlot( wxCommandEvent& event )
{
    m_oneFileOnly = !m_checkboxPagePerLayer->GetValue();
    ExportSVGFile( m_oneFileOnly );
}


bool InvokeExportSVG( PCB_EDIT_FRAME* aCaller, BOARD* aBoard )
{
    DIALOG_EXPORT_SVG dlg( aCaller, aBoard );

    dlg.ShowModal();

    return true;
}
