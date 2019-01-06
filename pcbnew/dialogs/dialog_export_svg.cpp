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


#include <fctsys.h>
#include <kiface_i.h>
#include <common.h>
#include <pcb_base_frame.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>
#include <confirm.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <class_board.h>
#include <dialog_export_svg_base.h>
#include <invoke_pcb_dialog.h>
#include <wx_html_report_panel.h>
#include <bitmaps.h>
#include <widgets/unit_binder.h>

class DIALOG_EXPORT_SVG : public DIALOG_EXPORT_SVG_BASE
{
public:
    DIALOG_EXPORT_SVG( PCB_BASE_FRAME* aParent, BOARD* aBoard );
    ~DIALOG_EXPORT_SVG() override;

private:
    BOARD*            m_board;
    wxConfigBase*     m_config;
    LSET              m_printMaskLayer;
    // the list of existing board layers in wxCheckListBox, with the
    // board layers id:
    std::pair<wxCheckListBox*, int> m_boxSelectLayer[PCB_LAYER_ID_COUNT];
    bool              m_printBW;
    wxString          m_outputDirectory;
    bool              m_printMirror;
    bool              m_oneFileOnly;
    UNIT_BINDER       m_lineWidth;

    void initDialog();

    void OnButtonPlot( wxCommandEvent& event ) override;

    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;
    void ExportSVGFile( bool aOnlyOneFile );

    bool CreateSVGFile( const wxString& FullFileName );

    LSET getCheckBoxSelectedLayers() const;
};


// Keys for configuration
#define PLOTSVGMODECOLOR_KEY        wxT( "PlotSVGModeColor" )
#define PLOTSVGMODEMIRROR_KEY       wxT( "PlotSVGModeMirror" )
#define PLOTSVGMODEONEFILE_KEY      wxT( "PlotSVGModeOneFile" )
#define PLOTSVGPAGESIZEOPT_KEY      wxT( "PlotSVGPageOpt" )
#define PLOTSVGPLOT_BRD_EDGE_KEY    wxT( "PlotSVGBrdEdge" )
#define PLOTSVG_LAYERBASE           wxT( "PlotSVGLayer_%d" )
#define PLOTSVG_DIR_KEY             wxT( "PlotSVGDirectory" )

/*
 * DIALOG_EXPORT_SVG functions
 */
DIALOG_EXPORT_SVG::DIALOG_EXPORT_SVG( PCB_BASE_FRAME* aParent, BOARD* aBoard ) :
    DIALOG_EXPORT_SVG_BASE( aParent ),
    m_lineWidth( aParent, m_penWidthLabel, m_penWidthCtrl, m_penWidthUnits, true )
{
    m_board  = aBoard;
    m_config = Kiface().KifaceSettings();

    memset( m_boxSelectLayer, 0, sizeof( m_boxSelectLayer ) );

    m_browseButton->SetBitmap( KiBitmap( folder_xpm ) );

    initDialog();

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Export" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    FinishDialogSettings();
}


DIALOG_EXPORT_SVG::~DIALOG_EXPORT_SVG()
{
    g_DrawDefaultLineThickness = m_lineWidth.GetValue();
    m_printBW = m_ModeColorOption->GetSelection();
    m_oneFileOnly = m_rbFileOpt->GetSelection() == 1;
    m_outputDirectory = m_outputDirectoryName->GetValue();
    m_outputDirectory.Replace( wxT( "\\" ), wxT( "/" ) );

    if( m_config )
    {
        m_config->Write( PLOTSVG_DIR_KEY, m_outputDirectory );
        m_config->Write( PLOTSVGMODECOLOR_KEY, m_printBW );
        m_config->Write( PLOTSVGMODEMIRROR_KEY, m_printMirror );
        m_config->Write( PLOTSVGMODEONEFILE_KEY, m_oneFileOnly );
        m_config->Write( PLOTSVGPAGESIZEOPT_KEY, m_rbSvgPageSizeOpt->GetSelection() );
        m_config->Write( PLOTSVGPLOT_BRD_EDGE_KEY, m_PrintBoardEdgesCtrl->GetValue() );

        wxString layerKey;

        for( unsigned layer = 0; layer < arrayDim(m_boxSelectLayer);  ++layer )
        {
            if( !m_boxSelectLayer[layer].first )
                continue;

            layerKey.Printf( PLOTSVG_LAYERBASE, layer );
            m_config->Write( layerKey, m_boxSelectLayer[layer].first->IsChecked( m_boxSelectLayer[layer].second ) );
        }
    }
}


void DIALOG_EXPORT_SVG::initDialog()
{
    if( m_config )
    {
        m_config->Read( PLOTSVG_DIR_KEY, &m_outputDirectory, wxEmptyString );
        m_config->Read( PLOTSVGMODECOLOR_KEY, &m_printBW, false );
        long ltmp;
        m_config->Read( PLOTSVGPAGESIZEOPT_KEY, &ltmp, 0 );
        m_rbSvgPageSizeOpt->SetSelection( ltmp );
        m_config->Read( PLOTSVGMODEMIRROR_KEY, &m_printMirror, false );
        m_config->Read( PLOTSVGMODEONEFILE_KEY, &m_oneFileOnly, false);
        m_config->Read( PLOTSVGPLOT_BRD_EDGE_KEY, &ltmp, 1 );
        m_PrintBoardEdgesCtrl->SetValue( ltmp );
    }

    m_outputDirectoryName->SetValue( m_outputDirectory );

    m_ModeColorOption->SetSelection( m_printBW ? 1 : 0 );
    m_printMirrorOpt->SetValue( m_printMirror );
    m_rbFileOpt->SetSelection( m_oneFileOnly ? 1 : 0 );

    m_lineWidth.SetValue( g_DrawDefaultLineThickness );

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

        if( m_config )
        {
            wxString layerKey;
            layerKey.Printf( PLOTSVG_LAYERBASE, layer );
            bool option;

            if( m_config && m_config->Read( layerKey, &option ) )
                m_boxSelectLayer[layer].first->Check( checkIndex, option );
        }
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
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxFileName  fn( m_outputDirectoryName->GetValue() );
    wxString    path = Prj().AbsolutePath( m_outputDirectoryName->GetValue() );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName      dirName = wxFileName::DirName( dirDialog.GetPath() );

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


void DIALOG_EXPORT_SVG::ExportSVGFile( bool aOnlyOneFile )
{
    m_outputDirectory = m_outputDirectoryName->GetValue();

    // Create output directory if it does not exist (also transform it in
    // absolute form). Bail if it fails
    wxFileName outputDir = wxFileName::DirName( m_outputDirectory );
    wxString boardFilename = m_board->GetFileName();

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
    g_DrawDefaultLineThickness = m_lineWidth.GetValue();

    LSET all_selected = getCheckBoxSelectedLayers();

    for( LSEQ seq = all_selected.Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        wxFileName   fn( boardFilename );
        wxString     suffix = aOnlyOneFile ? wxT( "brd" ) : m_board->GetStandardLayerName( layer );

        BuildPlotFileName( &fn, outputDir.GetPath(), suffix, SVGFileExtension );
        wxString     path = fn.GetFullPath();

        m_printMaskLayer = aOnlyOneFile ? all_selected : LSET( layer );

        if( m_PrintBoardEdgesCtrl->IsChecked() )
            m_printMaskLayer.set( Edge_Cuts );

        if( CreateSVGFile( path ) )
        {
            reporter.Report( wxString::Format( _( "Exported \"%s\"." ), path ),
                             REPORTER::RPT_ACTION );
        }
        else    // Error
        {
            reporter.Report( wxString::Format( _( "Unable to create file \"%s\"." ), path ),
                             REPORTER::RPT_ERROR );
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
    plot_opts.SetFormat( PLOT_FORMAT_SVG );

    PAGE_INFO   pageInfo = m_board->GetPageSettings();
    wxPoint     axisorigin = m_board->GetAuxOrigin();

    if( m_rbSvgPageSizeOpt->GetSelection() == 2 )   // Page is board boundary size
    {
        EDA_RECT    bbox = m_board->ComputeBoundingBox();
        PAGE_INFO   currpageInfo = m_board->GetPageSettings();

        currpageInfo.SetWidthMils(  bbox.GetWidth() / IU_PER_MILS );
        currpageInfo.SetHeightMils( bbox.GetHeight() / IU_PER_MILS );
        m_board->SetPageSettings( currpageInfo );
        plot_opts.SetUseAuxOrigin( true );
        wxPoint origin = bbox.GetOrigin();
        m_board->SetAuxOrigin( origin );
    }

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

    m_board->SetAuxOrigin( axisorigin );        // reset to the values saved earlier
    m_board->SetPageSettings( pageInfo );

    return true;
}


void DIALOG_EXPORT_SVG::OnButtonPlot( wxCommandEvent& event )
{
    m_oneFileOnly = m_rbFileOpt->GetSelection() == 1;
    ExportSVGFile( m_oneFileOnly );
}


bool InvokeExportSVG( PCB_BASE_FRAME* aCaller, BOARD* aBoard )
{
    DIALOG_EXPORT_SVG dlg( aCaller, aBoard );

    dlg.ShowModal();

    return true;
}
