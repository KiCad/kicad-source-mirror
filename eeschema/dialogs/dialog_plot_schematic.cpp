/** @file dialog_plot_schematic_PDF.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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
#include <worksheet.h>
#include <plot_common.h>
#include <class_sch_screen.h>
#include <wxEeschemaStruct.h>
#include <base_units.h>
#include <dialog_plot_schematic.h>

// Keys for configuration
#define PLOT_FORMAT_KEY wxT( "PlotFormat" )
#define PLOT_MODECOLOR_KEY wxT( "PlotModeColor" )
#define PLOT_FRAME_REFERENCE_KEY wxT( "PlotFrameRef" )
#define PLOT_HPGL_ORIGIN_KEY wxT( "PlotHPGLOrg" )



// static members (static to remember last state):
int DIALOG_PLOT_SCHEMATIC::m_pageSizeSelect = PAGE_SIZE_AUTO;
int DIALOG_PLOT_SCHEMATIC::m_HPGLPaperSizeSelect = 0;


void SCH_EDIT_FRAME::PlotSchematic( wxCommandEvent& event )
{
    DIALOG_PLOT_SCHEMATIC dlg( this );

    dlg.ShowModal();
}


DIALOG_PLOT_SCHEMATIC::DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* parent ) :
    DIALOG_PLOT_SCHEMATIC_BASE( parent )
{
    m_parent = parent;
    m_config   = wxGetApp().GetSettings();

    m_select_PlotAll = false;
    initDlg();

    GetSizer()->SetSizeHints( this );

    Centre();
}


// Initialize the dialog options:
void DIALOG_PLOT_SCHEMATIC::initDlg()
{
    SetFocus();    // make the ESC work

    // Set paper size option
    m_PaperSizeOption->SetSelection( m_pageSizeSelect );

    // Set color or B&W plot option
    bool tmp;
    m_config->Read( PLOT_MODECOLOR_KEY, &tmp, true );
    setModeColor( tmp );

    // Set plot or not frame reference option
    m_config->Read( PLOT_FRAME_REFERENCE_KEY, &tmp, true );
    setPlotFrameRef( tmp );

    // Set HPGL plot origin to center of paper of left bottom corner
    m_config->Read( PLOT_HPGL_ORIGIN_KEY, &tmp, false );
    SetPlotOriginCenter( tmp );

    // Switch to the last save plot format
    long plotfmt;
    m_config->Read( PLOT_FORMAT_KEY, &plotfmt, 0 );
    switch( plotfmt )
    {
        default:
        case PLOT_FORMAT_POST:
            m_plotFormatOpt->SetSelection( 0 );
            break;

        case PLOT_FORMAT_PDF:
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
    }

    // Set the default line width (pen width which should be used for
    // items that do not have a pen size defined (like frame ref)
    AddUnitSymbol( *m_defaultLineWidthTitle, g_UserUnit );
    PutValueInLocalUnits( *m_DefaultLineSizeCtrl, g_DrawDefaultLineThickness );

    // Initialize HPGL specific widgets
    AddUnitSymbol( *m_penHPLGWidthTitle, g_UserUnit );
    PutValueInLocalUnits( *m_penHPGLWidthCtrl, g_HPGL_Pen_Descr.m_Pen_Diam );
    m_HPGLPaperSizeOption->SetSelection( m_HPGLPaperSizeSelect );

    // Hide/show widgets that are not always displayed:
    wxCommandEvent cmd_event;
    OnPlotFormatSelection( cmd_event );
}

PlotFormat DIALOG_PLOT_SCHEMATIC::GetPlotFileFormat()
{
    switch( m_plotFormatOpt->GetSelection() )
    {
        default:
        case 0: return PLOT_FORMAT_POST;
        case 1: return PLOT_FORMAT_PDF;
        case 2: return PLOT_FORMAT_SVG;
        case 3: return PLOT_FORMAT_DXF;
        case 4: return PLOT_FORMAT_HPGL;
    }
}


void DIALOG_PLOT_SCHEMATIC::OnButtonCancelClick( wxCommandEvent& event )
{
    getPlotOptions();
    EndModal( 0 );
}


void DIALOG_PLOT_SCHEMATIC::getPlotOptions()
{
    m_config->Write( PLOT_MODECOLOR_KEY, getModeColor() );
    m_config->Write( PLOT_FRAME_REFERENCE_KEY, getPlotFrameRef() );
    m_config->Write( PLOT_FORMAT_KEY, (long) GetPlotFileFormat() );
    m_config->Write( PLOT_HPGL_ORIGIN_KEY, GetPlotOriginCenter() );
    m_HPGLPaperSizeSelect = m_HPGLPaperSizeOption->GetSelection();

    m_pageSizeSelect    = m_PaperSizeOption->GetSelection();
    g_DrawDefaultLineThickness = ReturnValueFromTextCtrl( *m_DefaultLineSizeCtrl );

    if( g_DrawDefaultLineThickness < 1 )
        g_DrawDefaultLineThickness = 1;
}

void DIALOG_PLOT_SCHEMATIC::OnPlotFormatSelection( wxCommandEvent& event )
{

    switch( m_plotFormatOpt->GetSelection() )
    {
        case 0:     // postscript
            m_paperOptionsSizer->Hide( m_paperHPGLSizer );
            m_paperOptionsSizer->Show( m_PaperSizeOption );
            m_PaperSizeOption->Enable( true );
            m_DefaultLineSizeCtrl->Enable( true );
            break;

        case 1:     // PDF
            m_paperOptionsSizer->Hide( m_paperHPGLSizer );
            m_paperOptionsSizer->Show(m_PaperSizeOption);
            m_PaperSizeOption->Enable( true );
            m_DefaultLineSizeCtrl->Enable( true );
            break;

        case 2:     // SVG
            m_paperOptionsSizer->Hide( m_paperHPGLSizer );
            m_paperOptionsSizer->Show(m_PaperSizeOption);
            m_PaperSizeOption->Enable( false );
            m_DefaultLineSizeCtrl->Enable( true );
           break;

        case 3:     // DXF
            m_paperOptionsSizer->Hide( m_paperHPGLSizer );
            m_paperOptionsSizer->Show(m_PaperSizeOption);
            m_PaperSizeOption->Enable( false );
            m_DefaultLineSizeCtrl->Enable( false );
            break;

        case 4:     //HPGL
            m_paperOptionsSizer->Show( m_paperHPGLSizer );
            m_paperOptionsSizer->Hide(m_PaperSizeOption);
            m_DefaultLineSizeCtrl->Enable( false );
           break;

    }

    GetSizer()->SetSizeHints( this );
}


void DIALOG_PLOT_SCHEMATIC::setupPlotPage( PLOTTER * plotter, SCH_SCREEN* screen )
{
    PAGE_INFO   plotPage;                               // page size selected to plot
    // Considerations on page size and scaling requests
    PAGE_INFO   actualPage = screen->GetPageSettings(); // page size selected in schematic

    switch( m_pageSizeSelect )
    {
    case PAGE_SIZE_A:
        plotPage.SetType( wxT( "A" ) );
        plotPage.SetPortrait( actualPage.IsPortrait() );
        break;

    case PAGE_SIZE_A4:
        plotPage.SetType( wxT( "A4" ) );
        plotPage.SetPortrait( actualPage.IsPortrait() );
        break;

    case PAGE_SIZE_AUTO:
    default:
        plotPage = actualPage;
        break;
    }

    double  scalex  = (double) plotPage.GetWidthMils() / actualPage.GetWidthMils();
    double  scaley  = (double) plotPage.GetHeightMils() / actualPage.GetHeightMils();
    double  scale   = MIN( scalex, scaley );
    plotter->SetPageSettings( plotPage );
    plotter->SetViewport( wxPoint( 0, 0 ), IU_PER_DECIMILS, scale, false );
}


void DIALOG_PLOT_SCHEMATIC::OnButtonPlotCurrentClick( wxCommandEvent& event )
{
    m_select_PlotAll = false;
    PlotSchematic();
}


void DIALOG_PLOT_SCHEMATIC::OnButtonPlotAllClick( wxCommandEvent& event )
{
    m_select_PlotAll = true;
    PlotSchematic();
}

void DIALOG_PLOT_SCHEMATIC::PlotSchematic()
{
    getPlotOptions();
    switch( GetPlotFileFormat() )
    {
        case PLOT_FORMAT_HPGL:
            createHPGLFile( m_select_PlotAll );
            break;

        default:
        case PLOT_FORMAT_POST:
            createPSFile();
            break;

        case PLOT_FORMAT_DXF:
            CreateDXFFile();
            break;

        case PLOT_FORMAT_PDF:
            createPDFFile();
            break;

        case PLOT_FORMAT_SVG:
            createSVGFile( m_select_PlotAll, getPlotFrameRef() );
            break;
    }
    m_MessagesBox->AppendText( wxT( "****\n" ) );
}

