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
//#include <pgm_base.h>
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
#include <dialog_SVG_print.h>

// Keys for configuration
#define PLOTSVGMODECOLOR_KEY        wxT( "PlotSVGModeColor" )
#define PLOTSVGPAGESIZEOPT_KEY      wxT( "PlotSVGPageOpt" )
#define PLOTSVGPLOT_BRD_EDGE_KEY    wxT( "PlotSVGBrdEdge" )

// reasonable values for default pen width
#define WIDTH_MAX_VALUE (2 * IU_PER_MM)
#define WIDTH_MIN_VALUE (0.05 * IU_PER_MM)

// Local variables:
static long s_SelectedLayers = LAYER_BACK | LAYER_FRONT |
                               SILKSCREEN_LAYER_FRONT | SILKSCREEN_LAYER_BACK;

/*
 * DIALOG_SVG_PRINT functions
 */
DIALOG_SVG_PRINT::DIALOG_SVG_PRINT( EDA_DRAW_FRAME* parent ) :
    DIALOG_SVG_PRINT_base( parent )
{
    m_parent    = (PCB_BASE_FRAME*) parent;
    m_config    = Kiface().KifaceSettings();
    initDialog();
    GetSizer()->SetSizeHints( this );
    Centre();
}
bool DIALOG_SVG_PRINT::m_printMirror = false;
bool DIALOG_SVG_PRINT::m_oneFileOnly = false;


void DIALOG_SVG_PRINT::initDialog()
{
    m_board = m_parent->GetBoard();

    if( m_config )
    {
        m_config->Read( PLOTSVGMODECOLOR_KEY, &m_printBW, false );
        long ltmp;
        m_config->Read( PLOTSVGPAGESIZEOPT_KEY, &ltmp, 0 );
        m_rbSvgPageSizeOpt->SetSelection( ltmp );
        m_config->Read( PLOTSVGPLOT_BRD_EDGE_KEY, &ltmp, 1 );
        m_PrintBoardEdgesCtrl->SetValue( ltmp );
    }

    m_outputDirectory = m_parent->GetPlotSettings().GetOutputDirectory();
    m_outputDirectoryName->SetValue( m_outputDirectory );

    if( m_printBW )
        m_ModeColorOption->SetSelection( 1 );
    else
        m_ModeColorOption->SetSelection( 0 );

    m_printMirrorOpt->SetValue( m_printMirror );
    m_rbFileOpt->SetSelection( m_oneFileOnly ? 1 : 0 );


    AddUnitSymbol( *m_TextPenWidth, g_UserUnit );
    m_DialogDefaultPenSize->SetValue(
        StringFromValue( g_UserUnit, g_DrawDefaultLineThickness ) );

    // Create layers list
    LAYER_NUM layer;
    for( layer = FIRST_LAYER; layer < NB_PCB_LAYERS; ++layer )
    {
        if( !m_board->IsLayerEnabled( layer ) )
            m_boxSelectLayer[layer] = NULL;
        else
            m_boxSelectLayer[layer] =
                new wxCheckBox( this, -1, m_board->GetLayerName( layer ) );
    }

    // Add wxCheckBoxes in layers lists dialog
    // List layers in same order than in setup layers dialog
    // (Front or Top to Back or Bottom)
    DECLARE_LAYERS_ORDER_LIST( layersOrder );

    for( LAYER_NUM layer_idx = FIRST_LAYER; layer_idx < NB_PCB_LAYERS; ++layer_idx )
    {
        layer = layersOrder[layer_idx];

        wxASSERT( layer < NB_PCB_LAYERS );

        if( m_boxSelectLayer[layer] == NULL )
            continue;

        LAYER_MSK mask = GetLayerMask( layer );

        if( mask & s_SelectedLayers )
            m_boxSelectLayer[layer]->SetValue( true );

        if( layer <= LAST_COPPER_LAYER )
            m_CopperLayersBoxSizer->Add(  m_boxSelectLayer[layer],
                                          0,
                                          wxGROW | wxALL,
                                          1 );
        else
            m_TechnicalBoxSizer->Add(  m_boxSelectLayer[layer],
                                       0,
                                       wxGROW | wxALL,
                                       1 );
    }

    if( m_config )
    {
        wxString layerKey;

        for( LAYER_NUM layer = FIRST_LAYER; layer < NB_PCB_LAYERS; ++layer )
        {
            bool option;

            if( m_boxSelectLayer[layer] == NULL )
                continue;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );

            if( m_config->Read( layerKey, &option ) )
                m_boxSelectLayer[layer]->SetValue( option );
        }
    }
}


void DIALOG_SVG_PRINT::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxFileName  fn( m_outputDirectoryName->GetValue() );
    wxString    path;

    if( fn.IsRelative() )
        path = wxGetCwd() + fn.GetPathSeparator() + m_outputDirectoryName->GetValue();
    else
        path = m_outputDirectoryName->GetValue();

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName      dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxMessageDialog dialog( this, _( "Use a relative path? " ),
                            _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    if( dialog.ShowModal() == wxID_YES )
    {
        wxString boardFilePath = ( (wxFileName) m_board->GetFileName() ).GetPath();

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

    if( !EnsureOutputDirectory( &outputDir, boardFilename, &reporter ) )
    {
        wxString msg;
        msg.Printf( _( "Could not write plot files to folder \"%s\"." ),
                    GetChars( outputDir.GetPath() ) );
        DisplayError( this, msg );
        return;
    }

    m_printMirror = m_printMirrorOpt->GetValue();
    m_printBW = m_ModeColorOption->GetSelection();
    SetPenWidth();

    // Build layers mask
    LAYER_MSK printMaskLayer = NO_LAYERS;

    for( LAYER_NUM layer = FIRST_LAYER; layer < NB_PCB_LAYERS; ++layer )
    {
        if( m_boxSelectLayer[layer] && m_boxSelectLayer[layer]->GetValue() )
            printMaskLayer |= GetLayerMask( layer );
    }

    wxString    msg;

    for( LAYER_NUM layer = FIRST_LAYER; layer < NB_PCB_LAYERS; ++layer )
    {
        LAYER_MSK currlayer_mask = GetLayerMask( layer );

        if( (printMaskLayer & currlayer_mask ) == 0 )
            continue;

        wxString suffix = m_board->GetStandardLayerName( layer );

        if( aOnlyOneFile )
        {
            m_printMaskLayer = printMaskLayer;
            suffix = wxT( "brd" );
        }
        else
        {
            m_printMaskLayer = currlayer_mask;
            suffix = m_board->GetStandardLayerName( layer );
        }

        wxFileName fn(boardFilename);
        BuildPlotFileName( &fn, outputDir.GetPath(), suffix, SVGFileExtension );

        if( m_PrintBoardEdgesCtrl->IsChecked() )
            m_printMaskLayer |= EDGE_LAYER;

        if( CreateSVGFile( fn.GetFullPath() ) )
            msg.Printf( _( "Plot: %s OK\n" ), GetChars( fn.GetFullPath() ) );
        else    // Error
            msg.Printf( _( "** Unable to create %s **\n" ), GetChars( fn.GetFullPath() ) );
        m_messagesBox->AppendText( msg );

        if( aOnlyOneFile )
            break;
    }
}


// Actual SVG file export  function.
bool DIALOG_SVG_PRINT::CreateSVGFile( const wxString& aFullFileName )
{
    PCB_PLOT_PARAMS m_plotOpts;

    m_plotOpts.SetPlotFrameRef( PrintPageRef() );

    // Adding drill marks, for copper layers
    if( (m_printMaskLayer & ALL_CU_LAYERS) )
        m_plotOpts.SetDrillMarksType( PCB_PLOT_PARAMS::FULL_DRILL_SHAPE );
    else
        m_plotOpts.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

    m_plotOpts.SetSkipPlotNPTH_Pads( false );

    m_plotOpts.SetMirror( m_printMirror );
    m_plotOpts.SetFormat( PLOT_FORMAT_SVG );
    EDA_COLOR_T color = UNSPECIFIED_COLOR;      // Used layer color to plot ref and value
    m_plotOpts.SetReferenceColor( color );
    m_plotOpts.SetValueColor( color );

    PAGE_INFO pageInfo = m_board->GetPageSettings();
    wxPoint axisorigin = m_board->GetAuxOrigin();

    if( PageIsBoardBoundarySize() )
    {
        EDA_RECT bbox = m_board->ComputeBoundingBox();
        PAGE_INFO currpageInfo = m_board->GetPageSettings();
        currpageInfo.SetWidthMils(  bbox.GetWidth() / IU_PER_MILS );
        currpageInfo.SetHeightMils( bbox.GetHeight() / IU_PER_MILS );
        m_board->SetPageSettings( currpageInfo );
        m_plotOpts.SetUseAuxOrigin( true );
        wxPoint origin = bbox.GetOrigin();
        m_board->SetAuxOrigin( origin );
    }

    LOCALE_IO    toggle;
    SVG_PLOTTER* plotter = (SVG_PLOTTER*) StartPlotBoard( m_board,
                                                          &m_plotOpts,
                                                          UNDEFINED_LAYER,
                                                          aFullFileName,
                                                          wxEmptyString );

    if( plotter )
    {
        plotter->SetColorMode( m_ModeColorOption->GetSelection() == 0 );
        PlotStandardLayer( m_board, plotter, m_printMaskLayer, m_plotOpts );
        plotter->EndPlot();
    }

    delete plotter;
    m_board->SetAuxOrigin( axisorigin );
    m_board->SetPageSettings( pageInfo );

    return true;
}

void DIALOG_SVG_PRINT::OnButtonPlot( wxCommandEvent& event )
{
    m_oneFileOnly = m_rbFileOpt->GetSelection() == 1;
    ExportSVGFile( m_oneFileOnly );
}


void DIALOG_SVG_PRINT::OnButtonCancelClick( wxCommandEvent& event )
{
    Close();
}


void DIALOG_SVG_PRINT::OnCloseWindow( wxCloseEvent& event )
{
    SetPenWidth();
    m_printBW = m_ModeColorOption->GetSelection();
    m_oneFileOnly = m_rbFileOpt->GetSelection() == 1;

    if( m_config )
    {
        m_config->Write( PLOTSVGMODECOLOR_KEY, m_printBW );
        m_config->Write( PLOTSVGPAGESIZEOPT_KEY, m_rbSvgPageSizeOpt->GetSelection() );
        m_config->Write( PLOTSVGPLOT_BRD_EDGE_KEY, m_PrintBoardEdgesCtrl->GetValue() );

        wxString layerKey;

        for( LAYER_NUM layer = FIRST_LAYER; layer < NB_PCB_LAYERS; ++layer )
        {
            if( m_boxSelectLayer[layer] == NULL )
                continue;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );
            m_config->Write( layerKey, m_boxSelectLayer[layer]->IsChecked() );
        }
    }

    // Set output directory and replace backslashes with forward ones
    wxString dirStr;
    dirStr = m_outputDirectoryName->GetValue();
    dirStr.Replace( wxT( "\\" ), wxT( "/" ) );

    if( dirStr != m_parent->GetPlotSettings().GetOutputDirectory() )
    {
        PCB_PLOT_PARAMS tempOptions( m_parent->GetPlotSettings() );
        tempOptions.SetOutputDirectory( dirStr );
        m_parent->SetPlotSettings( tempOptions );
        m_parent->OnModify();
    }

    EndModal( 0 );
}
