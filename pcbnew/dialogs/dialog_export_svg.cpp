/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <confirm.h>
#include <core/arraydim.h>
#include <pcbplot.h>
#include <locale_io.h>
#include <dialog_export_svg_base.h>
#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_html_report_panel.h>
#include <plotters/plotters_pslike.h>
#include <wx/dirdlg.h>
#include <pgm_base.h>
#include <project/project_file.h>
#include <exporters/export_svg.h>

class DIALOG_EXPORT_SVG : public DIALOG_EXPORT_SVG_BASE
{
public:
    DIALOG_EXPORT_SVG( PCB_EDIT_FRAME* aParent, BOARD* aBoard );
    ~DIALOG_EXPORT_SVG() override;

private:
    BOARD*            m_board;
    PCB_EDIT_FRAME*   m_parent;
    LSEQ              m_printMaskLayer;
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

    LSET getCheckBoxSelectedLayers() const;
};


DIALOG_EXPORT_SVG::DIALOG_EXPORT_SVG( PCB_EDIT_FRAME* aParent, BOARD* aBoard ) :
        DIALOG_EXPORT_SVG_BASE( aParent ),
        m_board( aBoard ),
        m_parent( aParent ),
        m_printBW( false ),
        m_printMirror( false ),
        m_oneFileOnly( false )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    m_messagesPanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    initDialog();

    SetupStandardButtons( { { wxID_OK,     _( "Export" ) },
                            { wxID_CANCEL, _( "Close" )  } } );

    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();
    m_cbUsedBoardTheme->SetValue( !cfg->m_ExportSvg.use_selected_theme );

    // Build color theme list, and select the previoulsy selected theme
    wxString theme_old_selection = cfg->m_ExportSvg.color_theme;

    for( COLOR_SETTINGS* settings : m_parent->GetSettingsManager()->GetColorSettingsList() )
    {
        int pos = m_colorTheme->Append( settings->GetName(), static_cast<void*>( settings ) );

        if( settings->GetName() == theme_old_selection )
            m_colorTheme->SetSelection( pos );
    }

    finishDialogSettings();
}


DIALOG_EXPORT_SVG::~DIALOG_EXPORT_SVG()
{
    m_printBW = m_ModeColorOption->GetSelection();
    m_oneFileOnly = !m_checkboxPagePerLayer->GetValue();
    m_outputDirectory = m_outputDirectoryName->GetValue();
    m_outputDirectory.Replace( wxT( "\\" ), wxT( "/" ) );

    m_parent->Prj().GetProjectFile().m_PcbLastPath[ LAST_PATH_SVG ] = m_outputDirectory;

    PCBNEW_SETTINGS* cfg = nullptr;

    try
    {
        cfg = m_parent->GetPcbNewSettings();
    }
    catch( const std::runtime_error& e )
    {
        wxFAIL_MSG( e.what() );
    }

    if( cfg )
    {
        cfg->m_ExportSvg.black_and_white  = m_printBW;
        cfg->m_ExportSvg.mirror           = m_printMirror;
        cfg->m_ExportSvg.one_file         = m_oneFileOnly;
        cfg->m_ExportSvg.page_size        = m_rbSvgPageSizeOpt->GetSelection();
        cfg->m_ExportSvg.output_dir       = m_outputDirectory.ToStdString();
        cfg->m_ExportSvg.use_selected_theme = !m_cbUsedBoardTheme->GetValue();
        cfg->m_ExportSvg.color_theme      = m_colorTheme->GetStringSelection();
    }

    if( m_checkboxPagePerLayer->GetValue() )
    {
        m_oneFileOnly = false;

        if( cfg )
            cfg->m_ExportSvg.plot_board_edges = m_checkboxEdgesOnAllPages->GetValue();
    }
    else
    {
        m_oneFileOnly = true;
    }

    if( cfg )
        cfg->m_ExportSvg.layers.clear();

    for( unsigned layer = 0; layer < arrayDim( m_boxSelectLayer ); ++layer )
    {
        if( !m_boxSelectLayer[layer].first )
            continue;

        if( m_boxSelectLayer[layer].first->IsChecked( m_boxSelectLayer[layer].second ) )
        {
            if( cfg )
                cfg->m_ExportSvg.layers.push_back( layer );
        }
    }
}


void DIALOG_EXPORT_SVG::initDialog()
{
    PROJECT_FILE&    projectFile = m_parent->Prj().GetProjectFile();
    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

    m_printBW         = cfg->m_ExportSvg.black_and_white;
    m_printMirror     = cfg->m_ExportSvg.mirror;
    m_oneFileOnly     = cfg->m_ExportSvg.one_file;

    if( !projectFile.m_PcbLastPath[ LAST_PATH_SVG ].IsEmpty() )
        m_outputDirectory = projectFile.m_PcbLastPath[ LAST_PATH_SVG ];
    else
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
            wxMessageBox( _( "Cannot make path relative (target volume different from board "
                             "file volume)!" ),
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

    // Create output directory if it does not exist (also transform it in absolute form).
    // Bail if it fails.

    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                // Handles m_board->GetTitleBlock() *and* m_board->GetProject()
                return m_board->ResolveTextVar( token, 0 );
            };

    wxString path = m_outputDirectory;
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, nullptr );

    wxFileName outputDir = wxFileName::DirName( path );
    wxString   boardFilename = m_board->GetFileName();

    REPORTER& reporter = m_messagesPanel->Reporter();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, &reporter ) )
    {
        wxString msg = wxString::Format( _( "Could not write plot files to folder '%s'." ),
                                         outputDir.GetPath() );
        DisplayError( this, msg );
        return;
    }

    m_printMirror = m_printMirrorOpt->GetValue();
    m_printBW = m_ModeColorOption->GetSelection();

    LSET all_selected = getCheckBoxSelectedLayers();

    PCB_PLOT_SVG_OPTIONS svgPlotOptions;
    svgPlotOptions.m_negative = false;
    svgPlotOptions.m_blackAndWhite = m_printBW;
    svgPlotOptions.m_printMaskLayer = m_printMaskLayer;
    svgPlotOptions.m_pageSizeMode = m_rbSvgPageSizeOpt->GetSelection();

    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();
    wxString export_theme = m_cbUsedBoardTheme->GetValue()
                            ? cfg->m_ColorTheme
                            : m_colorTheme->GetStringSelection();

    svgPlotOptions.m_colorTheme = export_theme;

    svgPlotOptions.m_mirror = m_printMirror;
    svgPlotOptions.m_plotFrame = svgPlotOptions.m_pageSizeMode == 0;
    svgPlotOptions.m_drillShapeOption = 2;  // actual size hole.

    for( LSEQ seq = all_selected.Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        wxFileName   fn( boardFilename );
        wxString     suffix = aOnlyOneFile ? wxString( wxT( "brd" ) ) : m_board->GetStandardLayerName( layer );

        BuildPlotFileName( &fn, outputDir.GetPath(), suffix, FILEEXT::SVGFileExtension );
        wxString svgPath = fn.GetFullPath();

        m_printMaskLayer = aOnlyOneFile ? all_selected.SeqStackupForPlotting() : LSEQ( { layer } );

        if( m_checkboxEdgesOnAllPages->GetValue() )
            m_printMaskLayer.push_back( Edge_Cuts );

        svgPlotOptions.m_outputFile = svgPath;
        svgPlotOptions.m_printMaskLayer = m_printMaskLayer;

        if( EXPORT_SVG::Plot(m_board, svgPlotOptions ) )
        {
            reporter.Report( wxString::Format( _( "Exported '%s'." ), svgPath ),
                             RPT_SEVERITY_ACTION );
        }
        else    // Error
        {
            reporter.Report( wxString::Format( _( "Failed to create file '%s'." ), svgPath ),
                             RPT_SEVERITY_ERROR );
        }

        if( aOnlyOneFile )
            break;
    }
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
