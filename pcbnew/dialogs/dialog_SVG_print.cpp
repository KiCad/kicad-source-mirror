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
#include <appl_wxstruct.h>
#include <common.h>
#include <class_drawpanel.h>
#include <dcsvg.h>
#include <wxBasePcbFrame.h>
#include <class_pcb_screen.h>
#include <base_units.h>
#include <convert_from_iu.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <printout_controler.h>
#include <class_board.h>
#include <dialog_SVG_print.h>

// Keys for configuration
#define PLOTSVGMODECOLOR_KEY        wxT( "PlotSVGModeColor" )
#define PLOTSVGPAGESIZEOPT_KEY      wxT( "PlotSVGPageOpt" )
#define PLOTSVGPLOT_BRD_EDGE_KEY    wxT( "PlotSVGBrdEdge" )

// reasonnable values for default pen width
#define WIDTH_MAX_VALUE (2 * IU_PER_MM)
#define WIDTH_MIN_VALUE (0.05 * IU_PER_MM)

// Local variables:
static long s_SelectedLayers = LAYER_BACK | LAYER_FRONT |
                               SILKSCREEN_LAYER_FRONT | SILKSCREEN_LAYER_BACK;


/*!
 * DIALOG_SVG_PRINT functions
 */
DIALOG_SVG_PRINT::DIALOG_SVG_PRINT( EDA_DRAW_FRAME* parent ) :
    DIALOG_SVG_PRINT_base( parent )
{
    m_Parent    = (PCB_BASE_FRAME*) parent;
    m_Config    = wxGetApp().GetSettings();
    initDialog();
    GetSizer()->SetSizeHints( this );
    Centre();
}
bool DIALOG_SVG_PRINT::m_printMirror = false;
bool DIALOG_SVG_PRINT::m_oneFileOnly = false;


void DIALOG_SVG_PRINT::initDialog()
{
    if( m_Config )
    {
        m_Config->Read( PLOTSVGMODECOLOR_KEY, &m_printBW, false );
        long ltmp;
        m_Config->Read( PLOTSVGPAGESIZEOPT_KEY, &ltmp, 0 );
        m_rbSvgPageSizeOpt->SetSelection( ltmp );
        m_Config->Read( PLOTSVGPLOT_BRD_EDGE_KEY, &ltmp, 1 );
        m_PrintBoardEdgesCtrl->SetValue( ltmp );
    }

    if( m_printBW )
        m_ModeColorOption->SetSelection( 1 );
    else
        m_ModeColorOption->SetSelection( 0 );

    m_printMirrorOpt->SetValue( m_printMirror );
    m_rbFileOpt->SetSelection( m_oneFileOnly ? 1 : 0 );


    AddUnitSymbol( *m_TextPenWidth, g_UserUnit );
    m_DialogDefaultPenSize->SetValue(
        ReturnStringFromValue( g_UserUnit, g_DrawDefaultLineThickness ) );

    // Create layers list
    BOARD*  board = m_Parent->GetBoard();
    int     layer;

    for( layer = 0; layer < NB_LAYERS; ++layer )
    {
        if( !board->IsLayerEnabled( layer ) )
            m_BoxSelectLayer[layer] = NULL;
        else
            m_BoxSelectLayer[layer] =
                new wxCheckBox( this, -1, board->GetLayerName( layer ) );
    }

    // Add wxCheckBoxes in layers lists dialog
    // List layers in same order than in setup layers dialog
    // (Front or Top to Back or Bottom)
    DECLARE_LAYERS_ORDER_LIST( layersOrder );

    for( int layer_idx = 0; layer_idx < NB_LAYERS; ++layer_idx )
    {
        layer = layersOrder[layer_idx];

        wxASSERT( layer < NB_LAYERS );

        if( m_BoxSelectLayer[layer] == NULL )
            continue;

        long mask = 1 << layer;

        if( mask & s_SelectedLayers )
            m_BoxSelectLayer[layer]->SetValue( true );

        if( layer < 16 )
            m_CopperLayersBoxSizer->Add(  m_BoxSelectLayer[layer],
                                          0,
                                          wxGROW | wxALL,
                                          1 );
        else
            m_TechnicalBoxSizer->Add(  m_BoxSelectLayer[layer],
                                       0,
                                       wxGROW | wxALL,
                                       1 );
    }

    if( m_Config )
    {
        wxString layerKey;

        for( int layer = 0; layer<NB_LAYERS; ++layer )
        {
            bool option;

            if( m_BoxSelectLayer[layer] == NULL )
                continue;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );

            if( m_Config->Read( layerKey, &option ) )
                m_BoxSelectLayer[layer]->SetValue( option );
        }
    }
}


void DIALOG_SVG_PRINT::SetPenWidth()
{
    int pensize = ReturnValueFromTextCtrl( *m_DialogDefaultPenSize );

    if( pensize > WIDTH_MAX_VALUE )
    {
        pensize = WIDTH_MAX_VALUE;
    }

    if( pensize < WIDTH_MIN_VALUE )
    {
        pensize = WIDTH_MIN_VALUE;
    }

    g_DrawDefaultLineThickness = pensize;
    m_DialogDefaultPenSize->SetValue( ReturnStringFromValue( g_UserUnit, pensize ) );
}

void DIALOG_SVG_PRINT::ExportSVGFile( bool aOnlyOneFile )
{
    wxFileName  fn;
    wxString    msg;

    m_printMirror = m_printMirrorOpt->GetValue();
    m_printBW = m_ModeColorOption->GetSelection();
    SetPenWidth();

    // Build layers mask
    int printMaskLayer = 0;

    for( int layer = 0; layer<NB_LAYERS; layer++ )
    {
        if( m_BoxSelectLayer[layer] && m_BoxSelectLayer[layer]->GetValue() )
            printMaskLayer |= 1 << layer;
    }

    for( int layer = 0; layer<NB_LAYERS; layer++ )
    {
        int currlayer_mask = 1 << layer;
        if( (printMaskLayer & currlayer_mask ) == 0 )
            continue;

        fn = m_FileNameCtrl->GetValue();

        if( !fn.IsOk() )
            fn = m_Parent->GetBoard()->GetFileName();

        if( aOnlyOneFile )
        {
            m_PrintMaskLayer = printMaskLayer;
            fn.SetName( fn.GetName() + wxT( "-brd" ) );
        }
        else
        {
            m_PrintMaskLayer = currlayer_mask;
            wxString suffix = m_Parent->GetBoard()->GetLayerName( layer, false );
            suffix.Trim();    // remove leading and trailing spaces if any
            suffix.Trim( false );
            fn.SetName( fn.GetName() + wxT( "-" ) + suffix );
        }

        fn.SetExt( wxT( "svg" ) );

        if( m_PrintBoardEdgesCtrl->IsChecked() )
            m_PrintMaskLayer |= EDGE_LAYER;

        if( CreateSVGFile( fn.GetFullPath() ) )
            msg.Printf( _( "Plot: %s OK\n" ), GetChars( fn.GetFullPath() ) );
        else    // Error
            msg.Printf( _( "** Unable to create %s **\n" ), GetChars( fn.GetFullPath() ) );
        m_MessagesBox->AppendText( msg );

        if( aOnlyOneFile )
            break;
    }
}


// Actual SVG file export  function.
bool DIALOG_SVG_PRINT::CreateSVGFile( const wxString& aFullFileName )
{
    BOARD*          brd = m_Parent->GetBoard();

    PCB_PLOT_PARAMS m_plotOpts;

    m_plotOpts.SetPlotFrameRef( PrintPageRef() );

    // Adding drill marks, for copper layers
    if( (m_PrintMaskLayer & ALL_CU_LAYERS) )
        m_plotOpts.SetDrillMarksType( PCB_PLOT_PARAMS::FULL_DRILL_SHAPE );
    else
        m_plotOpts.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

    m_plotOpts.SetSkipPlotNPTH_Pads( false );

    m_plotOpts.SetMirror( m_printMirror );
    m_plotOpts.SetFormat( PLOT_FORMAT_SVG );
    EDA_COLOR_T color = UNSPECIFIED_COLOR;      // Used layer color to plot ref and value
    m_plotOpts.SetReferenceColor( color );
    m_plotOpts.SetValueColor( color );

    PAGE_INFO pageInfo = brd->GetPageSettings();
    wxPoint axisorigin = brd->GetOriginAxisPosition();

    if( PageIsBoardBoundarySize() )
    {
        EDA_RECT bbox = brd->ComputeBoundingBox();
        PAGE_INFO currpageInfo = brd->GetPageSettings();
        currpageInfo.SetWidthMils(  bbox.GetWidth() / IU_PER_MILS );
        currpageInfo.SetHeightMils( bbox.GetHeight() / IU_PER_MILS );
        brd->SetPageSettings( currpageInfo );
        m_plotOpts.SetUseAuxOrigin( true );
        wxPoint origin = bbox.GetOrigin();
        brd->SetOriginAxisPosition( origin );
    }

    LOCALE_IO    toggle;
    SVG_PLOTTER* plotter = (SVG_PLOTTER*) StartPlotBoard( brd,
                                              &m_plotOpts, aFullFileName,
                                              wxEmptyString );

    if( plotter )
    {
        plotter->SetColorMode( m_ModeColorOption->GetSelection() == 0 );
        PlotStandardLayer( brd, plotter, m_PrintMaskLayer, m_plotOpts );
    }

    plotter->EndPlot();
    delete plotter;
    brd->SetOriginAxisPosition( axisorigin );
    brd->SetPageSettings( pageInfo );

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

    if( m_Config )
    {
        m_Config->Write( PLOTSVGMODECOLOR_KEY, m_printBW );
        m_Config->Write( PLOTSVGPAGESIZEOPT_KEY, m_rbSvgPageSizeOpt->GetSelection() );
        m_Config->Write( PLOTSVGPLOT_BRD_EDGE_KEY, m_PrintBoardEdgesCtrl->GetValue() );

        wxString layerKey;

        for( int layer = 0; layer<NB_LAYERS; ++layer )
        {
            if( m_BoxSelectLayer[layer] == NULL )
                continue;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );
            m_Config->Write( layerKey, m_BoxSelectLayer[layer]->IsChecked() );
        }
    }

    EndModal( 0 );
}
