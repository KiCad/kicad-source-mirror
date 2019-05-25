/** @file dialog_plot_schematic.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <pgm_base.h>
#include <kiface_i.h>
#include <bitmaps.h>
#include <ws_painter.h>
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
int DIALOG_PLOT_SCHEMATIC::m_HPGLPaperSizeSelect = PAGE_SIZE_AUTO;


void SCH_EDIT_FRAME::PlotSchematic()
{
    DIALOG_PLOT_SCHEMATIC dlg( this );

    dlg.ShowModal();

    // save project config if the prj config has changed:
    if( dlg.PrjConfigChanged() )
        SaveProjectSettings( false );
}


DIALOG_PLOT_SCHEMATIC::DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* parent ) :
    DIALOG_PLOT_SCHEMATIC_BASE( parent ),
    m_parent( parent ),
    m_plotFormat( PLOT_FORMAT_UNDEFINED ),
    m_defaultLineWidth( parent, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits, true ),
    m_penWidth( parent, m_penWidthLabel, m_penWidthCtrl, m_penWidthUnits, true )
{
    m_config = Kiface().KifaceSettings();
    m_configChanged = false;

    m_browseButton->SetBitmap( KiBitmap( folder_xpm ) );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Plot All Pages" ) );
    m_sdbSizer1Apply->SetLabel( _( "Plot Current Page" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    m_sdbSizer1OK->SetDefault();
    initDlg();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


// Initialize the dialog options:
void DIALOG_PLOT_SCHEMATIC::initDlg()
{
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

    // HPGL Pen Size is stored in mm in config
    m_config->Read( PLOT_HPGL_PEN_SIZE_KEY, &m_HPGLPenSize, 0.5 );
    m_HPGLPenSize *= IU_PER_MM;

    // Switch to the last save plot format
    long plotfmt;
    m_config->Read( PLOT_FORMAT_KEY, &plotfmt, 0 );

    switch( plotfmt )
    {
    default:
    case PLOT_FORMAT_POST: m_plotFormatOpt->SetSelection( 0 ); break;
    case PLOT_FORMAT_PDF:  m_plotFormatOpt->SetSelection( 1 ); break;
    case PLOT_FORMAT_SVG:  m_plotFormatOpt->SetSelection( 2 ); break;
    case PLOT_FORMAT_DXF:  m_plotFormatOpt->SetSelection( 3 ); break;
    case PLOT_FORMAT_HPGL: m_plotFormatOpt->SetSelection( 4 ); break;
    }

    // Set the default line width (pen width which should be used for
    // items that do not have a pen size defined (like frame ref)
    m_defaultLineWidth.SetValue( GetDefaultLineThickness() );

    // Initialize HPGL specific widgets
    m_penWidth.SetValue( m_HPGLPenSize );

    // Plot directory
    wxString path = m_parent->GetPlotDirectoryName();
#ifdef __WINDOWS__
    path.Replace( '/', '\\' );
#endif
    m_outputDirectoryName->SetValue( path );
}


/**
 * @todo Copy of DIALOG_PLOT::OnOutputDirectoryBrowseClicked in dialog_plot.cpp, maybe merge to
 *       a common method.
 */
void DIALOG_PLOT_SCHEMATIC::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxFileName  fn( m_outputDirectoryName->GetValue() );
    wxString    path = Prj().AbsolutePath( m_outputDirectoryName->GetValue() );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName      dirName = wxFileName::DirName( dirDialog.GetPath() );

    fn = Prj().AbsolutePath( g_RootSheet->GetFileName() );
    wxString defaultPath = fn.GetPathWithSep();
    wxString msg;
    msg.Printf( _( "Do you want to use a path relative to\n\"%s\"" ), GetChars( defaultPath ) );

    wxMessageDialog dialog( this, msg, _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    // relative directory selected
    if( dialog.ShowModal() == wxID_YES )
    {
        if( !dirName.MakeRelativeTo( defaultPath ) )
            wxMessageBox( _( "Cannot make path relative (target volume different from file "
                             "volume)!" ), _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
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


void DIALOG_PLOT_SCHEMATIC::OnPageSizeSelected( wxCommandEvent& event )
{
    if( GetPlotFileFormat() == PLOT_FORMAT_HPGL )
        m_HPGLPaperSizeSelect = m_paperSizeOption->GetSelection();
    else
        m_pageSizeSelect = m_paperSizeOption->GetSelection();
}


void DIALOG_PLOT_SCHEMATIC::OnUpdateUI( wxUpdateUIEvent& event )
{
    PlotFormat fmt = GetPlotFileFormat();

    if( fmt != m_plotFormat )
    {
        m_plotFormat = fmt;

        wxArrayString paperSizes;
        paperSizes.push_back( _( "Schematic size" ) );

        int selection;

        if( fmt == PLOT_FORMAT_HPGL )
        {
            paperSizes.push_back( _( "A4" ) );
            paperSizes.push_back( _( "A3" ) );
            paperSizes.push_back( _( "A2" ) );
            paperSizes.push_back( _( "A1" ) );
            paperSizes.push_back( _( "A0" ) );
            paperSizes.push_back( _( "A" ) );
            paperSizes.push_back( _( "B" ) );
            paperSizes.push_back( _( "C" ) );
            paperSizes.push_back( _( "D" ) );
            paperSizes.push_back( _( "E" ) );

            selection = m_HPGLPaperSizeSelect;
        }
        else
        {
            paperSizes.push_back( _( "A4" ) );
            paperSizes.push_back( _( "A" ) );

            selection = m_pageSizeSelect;
        }

        m_paperSizeOption->Set( paperSizes );
        m_paperSizeOption->SetSelection( selection );

        m_defaultLineWidth.Enable( fmt == PLOT_FORMAT_POST || fmt == PLOT_FORMAT_PDF
                                   || fmt == PLOT_FORMAT_SVG );

        m_plotOriginTitle->Enable( fmt == PLOT_FORMAT_HPGL );
        m_plotOriginOpt->Enable( fmt == PLOT_FORMAT_HPGL );
        m_penWidth.Enable( fmt == PLOT_FORMAT_HPGL );
    }
}


void DIALOG_PLOT_SCHEMATIC::getPlotOptions()
{
    m_HPGLPenSize = m_penWidth.GetValue();

    m_config->Write( PLOT_MODECOLOR_KEY, getModeColor() );
    m_config->Write( PLOT_FRAME_REFERENCE_KEY, getPlotFrameRef() );
    m_config->Write( PLOT_FORMAT_KEY, (long) GetPlotFileFormat() );
    m_config->Write( PLOT_HPGL_ORIGIN_KEY, GetPlotOriginCenter() );
    m_config->Write( PLOT_HPGL_PAPERSIZE_KEY, m_HPGLPaperSizeSelect );

    // HPGL Pen Size is stored in mm in config
    m_config->Write( PLOT_HPGL_PEN_SIZE_KEY, m_HPGLPenSize/IU_PER_MM );

    SetDefaultLineThickness( m_defaultLineWidth.GetValue() );

    // Plot directory
    wxString path = m_outputDirectoryName->GetValue();
    path.Replace( '\\', '/' );

    if(  m_parent->GetPlotDirectoryName() != path )
        m_configChanged = true;

    m_parent->SetPlotDirectoryName( path );
}


void DIALOG_PLOT_SCHEMATIC::OnPlotCurrent( wxCommandEvent& event )
{
    PlotSchematic( false );
}


void DIALOG_PLOT_SCHEMATIC::OnPlotAll( wxCommandEvent& event )
{
    PlotSchematic( true );
}


void DIALOG_PLOT_SCHEMATIC::PlotSchematic( bool aPlotAll )
{
    getPlotOptions();

    switch( GetPlotFileFormat() )
    {
    default:
    case PLOT_FORMAT_POST: createPSFile( aPlotAll, getPlotFrameRef() );   break;
    case PLOT_FORMAT_DXF:  CreateDXFFile( aPlotAll, getPlotFrameRef() );  break;
    case PLOT_FORMAT_PDF:  createPDFFile( aPlotAll, getPlotFrameRef() );  break;
    case PLOT_FORMAT_SVG:  createSVGFile( aPlotAll, getPlotFrameRef() );  break;
    case PLOT_FORMAT_HPGL: createHPGLFile( aPlotAll, getPlotFrameRef() ); break;
    }
}


wxFileName DIALOG_PLOT_SCHEMATIC::createPlotFileName( wxTextCtrl* aOutputDirectoryName,
                                                      wxString& aPlotFileName,
                                                      wxString& aExtension,
                                                      REPORTER* aReporter )
{
    wxString outputDirName = aOutputDirectoryName->GetValue();
    wxFileName outputDir = wxFileName::DirName( outputDirName );

    wxString plotFileName = Prj().AbsolutePath( aPlotFileName + wxT( "." ) + aExtension);

    if( !EnsureFileDirectoryExists( &outputDir, plotFileName, aReporter ) )
    {
        wxString msg = wxString::Format( _( "Could not write plot files to folder \"%s\"." ),
                                         outputDir.GetPath() );
        aReporter->Report( msg, REPORTER::RPT_ERROR );
    }

    wxFileName fn( plotFileName );
    fn.SetPath( outputDir.GetFullPath() );
    return fn;
}
