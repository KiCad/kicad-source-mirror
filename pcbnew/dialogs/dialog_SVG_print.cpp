/**
 * @file pcbnew/dialogs/dialog_SVG_print.cpp
 */

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
#include <class_drawpanel.h>
#include <wxBasePcbFrame.h>
#include <class_pcb_screen.h>
#include <base_units.h>
#include <convert_from_iu.h>
#include <wildcards_and_files_ext.h>
#include <macros.h>
#include <reporter.h>
#include <confirm.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <printout_controler.h>
#include <class_board.h>
#include <dialog_SVG_print_base.h>
#include <invoke_pcb_dialog.h>


class DIALOG_SVG_PRINT : public DIALOG_SVG_PRINT_base
{
public:
    DIALOG_SVG_PRINT( wxTopLevelWindow* aParent, BOARD* aBoard, PCB_PLOT_PARAMS* aSettings );

private:
    bool            m_did_print;
    BOARD*          m_board;
    PCB_PLOT_PARAMS* m_callers_params;
    wxConfigBase*   m_config;
    LSET            m_printMaskLayer;
    wxCheckBox*     m_boxSelectLayer[LAYER_ID_COUNT];
    bool            m_printBW;
    wxString        m_outputDirectory;
    bool            m_printMirror;
    bool            m_oneFileOnly;

    void initDialog();

    void OnCloseWindow( wxCloseEvent& event );
    void OnButtonPlot( wxCommandEvent& event );

    void OnButtonCloseClick( wxCommandEvent& event );

    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event );
    void SetPenWidth();
    void ExportSVGFile( bool aOnlyOneFile );

    bool PageIsBoardBoundarySize()
    {
        return m_rbSvgPageSizeOpt->GetSelection() == 2;
    }

    bool PrintPageRef()
    {
        return m_rbSvgPageSizeOpt->GetSelection() == 0;
    }

    bool CreateSVGFile( const wxString& FullFileName, bool aOnlyOneFile );

    LSET getCheckBoxSelectedLayers() const;
};



// Keys for configuration
#define PLOTSVGMODECOLOR_KEY        wxT( "PlotSVGModeColor" )
#define PLOTSVGMODEMIRROR_KEY       wxT( "PlotSVGModeMirror" )
#define PLOTSVGMODEONEFILE_KEY      wxT( "PlotSVGModeOneFile" )
#define PLOTSVGPAGESIZEOPT_KEY      wxT( "PlotSVGPageOpt" )
#define PLOTSVGPLOT_BRD_EDGE_KEY    wxT( "PlotSVGBrdEdge" )

// reasonable values for default pen width
#define WIDTH_MAX_VALUE             (2 * IU_PER_MM)
#define WIDTH_MIN_VALUE             (0.05 * IU_PER_MM)

// Local variables:
static LSET s_SelectedLayers( 4, B_Cu, F_Cu, F_SilkS, B_SilkS );

/*
 * DIALOG_SVG_PRINT functions
 */
DIALOG_SVG_PRINT::DIALOG_SVG_PRINT( wxTopLevelWindow* aParent, BOARD* aBoard, PCB_PLOT_PARAMS* aSettings ) :
    DIALOG_SVG_PRINT_base( aParent ),
    m_did_print( false ),
    m_callers_params( aSettings )
{
    m_board  = aBoard;
    m_config = Kiface().KifaceSettings();

    memset( m_boxSelectLayer, 0, sizeof( m_boxSelectLayer ) );

    initDialog();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_SVG_PRINT::initDialog()
{
    if( m_config )
    {
        m_config->Read( PLOTSVGMODECOLOR_KEY, &m_printBW, false );
        long ltmp;
        m_config->Read( PLOTSVGPAGESIZEOPT_KEY, &ltmp, 0 );
        m_config->Read( PLOTSVGMODEMIRROR_KEY, &m_printMirror, false );
        m_config->Read( PLOTSVGMODEONEFILE_KEY, &m_oneFileOnly, false);
        m_rbSvgPageSizeOpt->SetSelection( ltmp );
        m_config->Read( PLOTSVGPLOT_BRD_EDGE_KEY, &ltmp, 1 );
        m_PrintBoardEdgesCtrl->SetValue( ltmp );
    }

    m_outputDirectory = m_callers_params->GetOutputDirectory();
    m_outputDirectoryName->SetValue( m_outputDirectory );

    m_ModeColorOption->SetSelection( m_printBW ? 1 : 0 );
    m_printMirrorOpt->SetValue( m_printMirror );
    m_rbFileOpt->SetSelection( m_oneFileOnly ? 1 : 0 );

    AddUnitSymbol( *m_TextPenWidth, g_UserUnit );

    m_DialogDefaultPenSize->SetValue( StringFromValue( g_UserUnit, g_DrawDefaultLineThickness ) );

    LSEQ seq = m_board->GetEnabledLayers().UIOrder();

    for(  ;  seq;  ++seq )
    {
        LAYER_ID layer = *seq;

        // The layers in m_boxSelectLayer[] are in LAYER_ID order.  This may be
        // different than the order on screen.
        m_boxSelectLayer[layer] = new wxCheckBox( this, -1, m_board->GetLayerName( layer ) );

        if( s_SelectedLayers[layer] )
            m_boxSelectLayer[layer]->SetValue( true );

        if( IsCopperLayer( layer ) )
            m_CopperLayersBoxSizer->Add( m_boxSelectLayer[layer], 0, wxGROW | wxALL, 1 );
        else
            m_TechnicalBoxSizer->Add( m_boxSelectLayer[layer], 0, wxGROW | wxALL, 1 );
    }

    if( m_config )
    {
        wxString layerKey;

        for( seq.Rewind();  seq;  ++seq )
        {
            bool option;

            LAYER_NUM layer = *seq;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );

            if( m_config->Read( layerKey, &option ) )
                m_boxSelectLayer[layer]->SetValue( option );
        }
    }
}


LSET DIALOG_SVG_PRINT::getCheckBoxSelectedLayers() const
{
    LSET ret;

    // the layers in m_boxSelectLayer[] are in LAYER_ID order.
    for( unsigned layer=0; layer<DIM(m_boxSelectLayer);  ++layer )
    {
        if( m_boxSelectLayer[layer] && m_boxSelectLayer[layer]->GetValue() )
            ret.set( layer );
    }

    return ret;
}


void DIALOG_SVG_PRINT::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxFileName  fn( m_outputDirectoryName->GetValue() );
    wxString    path = Prj().AbsolutePath( m_outputDirectoryName->GetValue() );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName      dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxMessageDialog dialog( this, _( "Use a relative path? " ),
                            _( "Plot Output Directory" ),
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


void DIALOG_SVG_PRINT::SetPenWidth()
{
    int pensize = ValueFromTextCtrl( *m_DialogDefaultPenSize );

    if( pensize > WIDTH_MAX_VALUE )
    {
        pensize = WIDTH_MAX_VALUE;
    }

    if( pensize < WIDTH_MIN_VALUE )
    {
        pensize = WIDTH_MIN_VALUE;
    }

    g_DrawDefaultLineThickness = pensize;
    m_DialogDefaultPenSize->SetValue( StringFromValue( g_UserUnit, pensize ) );
}


void DIALOG_SVG_PRINT::ExportSVGFile( bool aOnlyOneFile )
{
    m_outputDirectory = m_outputDirectoryName->GetValue();

    // Create output directory if it does not exist (also transform it in
    // absolute form). Bail if it fails
    wxFileName outputDir = wxFileName::DirName( m_outputDirectory );
    wxString boardFilename = m_board->GetFileName();

    WX_TEXT_CTRL_REPORTER reporter( m_messagesBox );

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, &reporter ) )
    {
        wxString msg = wxString::Format(
                _( "Could not write plot files to folder '%s'." ),
                GetChars( outputDir.GetPath() )
                );
        DisplayError( this, msg );
        return;
    }

    m_printMirror = m_printMirrorOpt->GetValue();
    m_printBW = m_ModeColorOption->GetSelection();
    SetPenWidth();

    LSET all_selected = getCheckBoxSelectedLayers();

    for( LSEQ seq = all_selected.Seq();  seq;  ++seq )
    {
        LAYER_ID layer = *seq;

        wxFileName fn( boardFilename );

        wxString suffix = aOnlyOneFile ? wxT( "brd" ) : m_board->GetStandardLayerName( layer );

        BuildPlotFileName( &fn, outputDir.GetPath(), suffix, SVGFileExtension );

        m_printMaskLayer = aOnlyOneFile ? all_selected : LSET( layer );

        if( m_PrintBoardEdgesCtrl->IsChecked() )
            m_printMaskLayer.set( Edge_Cuts );

        if( CreateSVGFile( fn.GetFullPath(), aOnlyOneFile ) )
        {
            m_messagesBox->AppendText(
                    wxString::Format( _( "Plot: '%s' OK\n" ), GetChars( fn.GetFullPath() ) )
                    );
        }
        else    // Error
        {
            m_messagesBox->AppendText(
                    wxString::Format( _( "** Unable to create '%s'**\n" ), GetChars( fn.GetFullPath() ) )
                    );
        }

        if( aOnlyOneFile )
            break;
    }
}


// Actual SVG file export  function.
bool DIALOG_SVG_PRINT::CreateSVGFile( const wxString& aFullFileName, bool aOnlyOneFile )
{
    PCB_PLOT_PARAMS plot_opts;

    plot_opts.SetPlotFrameRef( PrintPageRef() );

    // Adding drill marks, for copper layers
    if( ( m_printMaskLayer & LSET::AllCuMask() ).any() )
        plot_opts.SetDrillMarksType( PCB_PLOT_PARAMS::FULL_DRILL_SHAPE );
    else
        plot_opts.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

    plot_opts.SetSkipPlotNPTH_Pads( false );

    plot_opts.SetMirror( m_printMirror );
    plot_opts.SetFormat( PLOT_FORMAT_SVG );

    EDA_COLOR_T color = UNSPECIFIED_COLOR;      // Used layer color to plot ref and value

    plot_opts.SetReferenceColor( color );
    plot_opts.SetValueColor( color );

    PAGE_INFO   pageInfo = m_board->GetPageSettings();
    wxPoint     axisorigin = m_board->GetAuxOrigin();

    if( PageIsBoardBoundarySize() )
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

    SVG_PLOTTER* plotter = (SVG_PLOTTER*) StartPlotBoard( m_board,
                     &plot_opts, UNDEFINED_LAYER, aFullFileName, wxEmptyString );

    if( plotter )
    {
        plotter->SetColorMode( !m_printBW );
        if( aOnlyOneFile )
        {
            for( LSEQ seq = m_printMaskLayer.SeqStackupBottom2Top();  seq;  ++seq )
                PlotOneBoardLayer( m_board, plotter, *seq, plot_opts );
        }
        else
        {
            PlotStandardLayer( m_board, plotter, m_printMaskLayer, plot_opts );
        }
        plotter->EndPlot();
    }

    delete plotter;

    m_board->SetAuxOrigin( axisorigin );        // reset to the values saved earlier
    m_board->SetPageSettings( pageInfo );

    return true;
}


void DIALOG_SVG_PRINT::OnButtonPlot( wxCommandEvent& event )
{
    m_oneFileOnly = m_rbFileOpt->GetSelection() == 1;
    ExportSVGFile( m_oneFileOnly );

    m_did_print = true;
}


void DIALOG_SVG_PRINT::OnButtonCloseClick( wxCommandEvent& event )
{
    Close();
}


void DIALOG_SVG_PRINT::OnCloseWindow( wxCloseEvent& event )
{
    if( m_did_print )   // unless output was created, this is tantamount to a cancel.
    {
        SetPenWidth();
        m_printBW = m_ModeColorOption->GetSelection();
        m_oneFileOnly = m_rbFileOpt->GetSelection() == 1;

        // Why are SVG layer choices co-mingled with other plot layer choices in the config file?
        // The string OPTKEY_LAYERBASE is used in multiple places.
        // fix this.

        wxString dirStr = m_outputDirectoryName->GetValue();
        dirStr.Replace( wxT( "\\" ), wxT( "/" ) );

        m_callers_params->SetOutputDirectory( dirStr );

        if( m_config )
        {
            m_config->Write( PLOTSVGMODECOLOR_KEY, m_printBW );
            m_config->Write( PLOTSVGMODEMIRROR_KEY, m_printMirror );
            m_config->Write( PLOTSVGMODEONEFILE_KEY, m_oneFileOnly );
            m_config->Write( PLOTSVGPAGESIZEOPT_KEY, m_rbSvgPageSizeOpt->GetSelection() );
            m_config->Write( PLOTSVGPLOT_BRD_EDGE_KEY, m_PrintBoardEdgesCtrl->GetValue() );

            wxString layerKey;

            for( unsigned layer = 0; layer < DIM(m_boxSelectLayer);  ++layer )
            {
                if( !m_boxSelectLayer[layer] )
                    continue;

                layerKey.Printf( OPTKEY_LAYERBASE, layer );
                m_config->Write( layerKey, m_boxSelectLayer[layer]->IsChecked() );
            }
        }
    }

    EndModal( m_did_print ? wxID_OK : wxID_CANCEL );
}


bool InvokeSVGPrint( wxTopLevelWindow* aCaller, BOARD* aBoard, PCB_PLOT_PARAMS* aSettings )
{
    DIALOG_SVG_PRINT dlg( aCaller, aBoard, aSettings );

    return dlg.ShowModal() == wxID_OK;
}
