/** @file dialog_plot_schematic.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
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
#include <pgm_base.h>
#include <kiface_i.h>
#include <worksheet.h>
#include <plot_common.h>
#include <class_sch_screen.h>
#include <schframe.h>
#include <base_units.h>
#include <sch_sheet.h>
#include <dialog_plot_schematic.h>

// Keys for configuration
#define PLOT_FORMAT_KEY wxT( "PlotFormat" )
#define PLOT_MODECOLOR_KEY wxT( "PlotModeColor" )
#define PLOT_FRAME_REFERENCE_KEY wxT( "PlotFrameRef" )
#define PLOT_HPGL_ORIGIN_KEY wxT( "PlotHPGLOrg" )
#define PLOT_HPGL_PAPERSIZE_KEY wxT( "PlotHPGLPaperSize" )
#define PLOT_HPGL_PEN_SIZE_KEY wxT( "PlotHPGLPenSize" )



// static members (static to remember last state):
int DIALOG_PLOT_SCHEMATIC::m_pageSizeSelect = PAGE_SIZE_AUTO;


void SCH_EDIT_FRAME::PlotSchematic( wxCommandEvent& event )
{
    DIALOG_PLOT_SCHEMATIC dlg( this );

    dlg.ShowModal();

    // save project config if the prj config has changed:
    if( dlg.PrjConfigChanged() )
        SaveProjectSettings( true );
}


DIALOG_PLOT_SCHEMATIC::DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* parent ) :
    DIALOG_PLOT_SCHEMATIC_BASE( parent )
{
    m_parent = parent;
    m_configChanged = false;
    m_config = Kiface().KifaceSettings();

    initDlg();

    GetSizer()->SetSizeHints( this );

    Centre();
}



// Initialize the dialog options:
void DIALOG_PLOT_SCHEMATIC::initDlg()
{
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

    m_config->Read( PLOT_HPGL_PAPERSIZE_KEY, &m_HPGLPaperSizeSelect, 0 );
    m_HPGLPaperSizeOption->SetSelection( m_HPGLPaperSizeSelect );

    // HPGL Pen Size is stored in mm in config
    m_config->Read( PLOT_HPGL_PEN_SIZE_KEY, &m_HPGLPenSize, 0.5 );
    m_HPGLPenSize *= IU_PER_MM;

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
    PutValueInLocalUnits( *m_DefaultLineSizeCtrl, GetDefaultLineThickness() );

    // Initialize HPGL specific widgets
    AddUnitSymbol( *m_penHPLGWidthTitle, g_UserUnit );
    PutValueInLocalUnits( *m_penHPGLWidthCtrl, m_HPGLPenSize );
    m_HPGLPaperSizeOption->SetSelection( m_HPGLPaperSizeSelect );

    // Plot directory
    wxString path = m_parent->GetPlotDirectoryName();
#ifdef __WINDOWS__
    path.Replace( '/', '\\' );
#endif
    m_outputDirectoryName->SetValue( path );

    // Hide/show widgets that are not always displayed:
    wxCommandEvent cmd_event;
    OnPlotFormatSelection( cmd_event );
}

/*
 * TODO: Copy of DIALOG_PLOT::OnOutputDirectoryBrowseClicked in dialog_plot.cpp, maybe merge to a common method.
 */
void DIALOG_PLOT_SCHEMATIC::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxFileName  fn( m_outputDirectoryName->GetValue() );
    wxString    path = Prj().AbsolutePath( m_outputDirectoryName->GetValue() );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
    {
        return;
    }

    wxFileName      dirName = wxFileName::DirName( dirDialog.GetPath() );

    fn = Prj().AbsolutePath( g_RootSheet->GetFileName() );
    wxString defaultPath = fn.GetPathWithSep();
    wxString msg;
    msg.Printf( _( "Do you want to use a path relative to\n'%s'" ),
                GetChars( defaultPath ) );

    wxMessageDialog dialog( this, msg, _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    // relative directory selected
    if( dialog.ShowModal() == wxID_YES )
    {
        if( !dirName.MakeRelativeTo( defaultPath ) )
            wxMessageBox( _( "Cannot make path relative (target volume different from file volume)!" ),
                          _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
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
    EndModal( wxID_CANCEL );
}


void DIALOG_PLOT_SCHEMATIC::getPlotOptions()
{
    m_config->Write( PLOT_MODECOLOR_KEY, getModeColor() );
    m_config->Write( PLOT_FRAME_REFERENCE_KEY, getPlotFrameRef() );
    m_config->Write( PLOT_FORMAT_KEY, (long) GetPlotFileFormat() );
    m_config->Write( PLOT_HPGL_ORIGIN_KEY, GetPlotOriginCenter() );
    m_HPGLPaperSizeSelect = m_HPGLPaperSizeOption->GetSelection();
    m_config->Write( PLOT_HPGL_PAPERSIZE_KEY, m_HPGLPaperSizeSelect );
    // HPGL Pen Size is stored in mm in config
    m_config->Write( PLOT_HPGL_PEN_SIZE_KEY, m_HPGLPenSize/IU_PER_MM );

    m_pageSizeSelect    = m_PaperSizeOption->GetSelection();
    SetDefaultLineThickness( ValueFromTextCtrl( *m_DefaultLineSizeCtrl ) );

    // Plot directory
    wxString path = m_outputDirectoryName->GetValue();
    path.Replace( '\\', '/' );

    if(  m_parent->GetPlotDirectoryName() != path )
        m_configChanged = true;

    m_parent->SetPlotDirectoryName( path );

}


void DIALOG_PLOT_SCHEMATIC::OnPlotFormatSelection( wxCommandEvent& event )
{

    switch( GetPlotFileFormat() )
    {
    default:
    case PLOT_FORMAT_POST:
        m_paperOptionsSizer->Hide( m_paperHPGLSizer );
        m_paperOptionsSizer->Show( m_PaperSizeOption );
        m_PaperSizeOption->Enable( true );
        m_DefaultLineSizeCtrl->Enable( true );
        break;

    case PLOT_FORMAT_PDF:
        m_paperOptionsSizer->Hide( m_paperHPGLSizer );
        m_paperOptionsSizer->Show(m_PaperSizeOption);
        m_PaperSizeOption->Enable( true );
        m_DefaultLineSizeCtrl->Enable( true );
        break;

    case PLOT_FORMAT_SVG:
        m_paperOptionsSizer->Hide( m_paperHPGLSizer );
        m_paperOptionsSizer->Show(m_PaperSizeOption);
        m_PaperSizeOption->Enable( false );
        m_DefaultLineSizeCtrl->Enable( true );
        break;

    case PLOT_FORMAT_DXF:
        m_paperOptionsSizer->Hide( m_paperHPGLSizer );
        m_paperOptionsSizer->Show(m_PaperSizeOption);
        m_PaperSizeOption->Enable( false );
        m_DefaultLineSizeCtrl->Enable( false );
        break;

    case PLOT_FORMAT_HPGL:
        m_paperOptionsSizer->Show( m_paperHPGLSizer );
        m_paperOptionsSizer->Hide(m_PaperSizeOption);
        m_DefaultLineSizeCtrl->Enable( false );
        break;

    }

    GetSizer()->SetSizeHints( this );
}


void DIALOG_PLOT_SCHEMATIC::OnButtonPlotCurrentClick( wxCommandEvent& event )
{
    PlotSchematic( false );
}


void DIALOG_PLOT_SCHEMATIC::OnButtonPlotAllClick( wxCommandEvent& event )
{
    PlotSchematic( true );
}


void DIALOG_PLOT_SCHEMATIC::PlotSchematic( bool aPlotAll )
{
    getPlotOptions();

    switch( GetPlotFileFormat() )
    {
    case PLOT_FORMAT_HPGL:
        createHPGLFile( aPlotAll, getPlotFrameRef() );
        break;

    case PLOT_FORMAT_DXF:
        CreateDXFFile( aPlotAll, getPlotFrameRef() );
        break;

    case PLOT_FORMAT_PDF:
        createPDFFile( aPlotAll, getPlotFrameRef() );
        break;

    case PLOT_FORMAT_SVG:
        createSVGFile( aPlotAll, getPlotFrameRef() );
        break;

    case PLOT_FORMAT_POST:
    // Fall through.  Default to Postscript.
    default:
        createPSFile( aPlotAll, getPlotFrameRef() );
        break;

    }

    m_MessagesBox->AppendText( wxT( "****\n" ) );
}

wxFileName DIALOG_PLOT_SCHEMATIC::createPlotFileName( wxTextCtrl* aOutputDirectoryName,
                                                      wxString& aPlotFileName,
                                                      wxString& aExtension,
                                                      REPORTER* aReporter )
{
    wxString outputDirName = aOutputDirectoryName->GetValue();
    wxFileName outputDir = wxFileName::DirName( outputDirName );

    wxString plotFileName = Prj().AbsolutePath( aPlotFileName + wxT(".") + aExtension);

    if( !EnsureFileDirectoryExists( &outputDir, plotFileName, aReporter ) )
    {
        wxString msg;
        msg.Printf( _( "Could not write plot files to folder \"%s\"." ),
                    GetChars( outputDir.GetPath() ) );
        msg << wxT( "\n" );
        aReporter->Report( msg );
    }

    wxFileName fn( plotFileName );
    fn.SetPath( outputDir.GetFullPath() );
    return fn;

}
