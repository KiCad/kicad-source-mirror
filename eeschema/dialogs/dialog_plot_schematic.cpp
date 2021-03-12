/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <dialogs/wx_html_report_panel.h>
#include <dialog_plot_schematic.h>
#include <eeschema_settings.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <drawing_sheet/ds_painter.h>
#include <sch_painter.h>
#include <schematic.h>

#include <wx/dirdlg.h>
#include <wx/msgdlg.h>

// static members (static to remember last state):
int DIALOG_PLOT_SCHEMATIC::m_pageSizeSelect = PAGE_SIZE_AUTO;
int DIALOG_PLOT_SCHEMATIC::m_HPGLPaperSizeSelect = PAGE_SIZE_AUTO;


DIALOG_PLOT_SCHEMATIC::DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* parent )
        : DIALOG_PLOT_SCHEMATIC_BASE( parent ),
          m_parent( parent ),
          m_plotFormat( PLOT_FORMAT::UNDEFINED ),
          m_HPGLPenSize( 1.0 ),
          m_defaultLineWidth( parent, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits ),
          m_penWidth( parent, m_penWidthLabel, m_penWidthCtrl, m_penWidthUnits )
{
    m_configChanged = false;

    m_browseButton->SetBitmap( KiBitmap( BITMAPS::small_folder ) );

    m_MessagesBox->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Plot All Pages" ) );
    m_sdbSizer1Apply->SetLabel( _( "Plot Current Page" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    m_sdbSizer1OK->SetDefault();
    initDlg();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


// Initialize the dialog options:
void DIALOG_PLOT_SCHEMATIC::initDlg()
{
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( cfg )
    {
        for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
        {
            int idx = m_colorTheme->Append( settings->GetName(), static_cast<void*>( settings ) );

            if( settings->GetFilename() == cfg->m_PlotPanel.color_theme )
                m_colorTheme->SetSelection( idx );
        }

        m_colorTheme->Enable( cfg->m_PlotPanel.color );

        m_plotBackgroundColor->Enable( cfg->m_PlotPanel.color );
        m_plotBackgroundColor->SetValue( cfg->m_PlotPanel.background_color );

        // Set color or B&W plot option
        setModeColor( cfg->m_PlotPanel.color );

        // Set plot or not frame reference option
        setPlotDrawingSheet( cfg->m_PlotPanel.frame_reference );

        // HPGL plot origin and unit system configuration
        m_plotOriginOpt->SetSelection( cfg->m_PlotPanel.hpgl_origin );

        m_HPGLPaperSizeSelect = cfg->m_PlotPanel.hpgl_paper_size;

        // HPGL Pen Size is stored in mm in config
        m_HPGLPenSize = cfg->m_PlotPanel.hpgl_pen_size * IU_PER_MM;

        // Switch to the last save plot format
        PLOT_FORMAT fmt = static_cast<PLOT_FORMAT>( cfg->m_PlotPanel.format );

        switch( fmt  )
        {
        default:
        case PLOT_FORMAT::POST: m_plotFormatOpt->SetSelection( 0 ); break;
        case PLOT_FORMAT::PDF:  m_plotFormatOpt->SetSelection( 1 ); break;
        case PLOT_FORMAT::SVG:  m_plotFormatOpt->SetSelection( 2 ); break;
        case PLOT_FORMAT::DXF:  m_plotFormatOpt->SetSelection( 3 ); break;
        case PLOT_FORMAT::HPGL: m_plotFormatOpt->SetSelection( 4 ); break;
        }

        if( fmt == PLOT_FORMAT::DXF || fmt == PLOT_FORMAT::HPGL )
            m_plotBackgroundColor->Disable();

        // Set the default line width (pen width which should be used for
        // items that do not have a pen size defined (like frame ref)
        // the default line width is stored in mils in config
        m_defaultLineWidth.SetValue( Mils2iu( cfg->m_Drawing.default_line_thickness ) );
    }

    // Initialize HPGL specific widgets
    m_penWidth.SetValue( m_HPGLPenSize );

    // Plot directory
    SCHEMATIC_SETTINGS& settings = m_parent->Schematic().Settings();
    wxString            path     = settings.m_PlotDirectoryName;
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
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputDirectoryName->GetValue(), &Prj() );
    path = Prj().AbsolutePath( path );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName      dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxFileName fn( Prj().AbsolutePath( m_parent->Schematic().Root().GetFileName() ) );
    wxString defaultPath = fn.GetPathWithSep();
    wxString msg;
    wxFileName relPathTest; // Used to test if we can make the path relative

    relPathTest.Assign( dirDialog.GetPath() );

    // Test if making the path relative is possible before asking the user if they want to do it
    if( relPathTest.MakeRelativeTo( defaultPath ) )
    {
        msg.Printf( _( "Do you want to use a path relative to\n\"%s\"" ), defaultPath );

        wxMessageDialog dialog( this, msg, _( "Plot Output Directory" ),
                                wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

        if( dialog.ShowModal() == wxID_YES )
            dirName.MakeRelativeTo( defaultPath );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


PLOT_FORMAT DIALOG_PLOT_SCHEMATIC::GetPlotFileFormat()
{
    switch( m_plotFormatOpt->GetSelection() )
    {
    default:
    case 0:
        return PLOT_FORMAT::POST;
    case 1:
        return PLOT_FORMAT::PDF;
    case 2:
        return PLOT_FORMAT::SVG;
    case 3:
        return PLOT_FORMAT::DXF;
    case 4:
        return PLOT_FORMAT::HPGL;
    }
}


void DIALOG_PLOT_SCHEMATIC::OnPageSizeSelected( wxCommandEvent& event )
{
    if( GetPlotFileFormat() == PLOT_FORMAT::HPGL )
        m_HPGLPaperSizeSelect = m_paperSizeOption->GetSelection();
    else
        m_pageSizeSelect = m_paperSizeOption->GetSelection();
}


void DIALOG_PLOT_SCHEMATIC::OnUpdateUI( wxUpdateUIEvent& event )
{
    PLOT_FORMAT fmt = GetPlotFileFormat();

    if( fmt != m_plotFormat )
    {
        m_plotFormat = fmt;

        wxArrayString paperSizes;
        paperSizes.push_back( _( "Schematic size" ) );

        int selection;

        if( fmt == PLOT_FORMAT::HPGL )
        {
            paperSizes.push_back( _( "A5" ) );
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

        m_defaultLineWidth.Enable(
                fmt == PLOT_FORMAT::POST || fmt == PLOT_FORMAT::PDF || fmt == PLOT_FORMAT::SVG );

        m_plotOriginTitle->Enable( fmt == PLOT_FORMAT::HPGL );
        m_plotOriginOpt->Enable( fmt == PLOT_FORMAT::HPGL );
        m_penWidth.Enable( fmt == PLOT_FORMAT::HPGL );

        m_plotBackgroundColor->Enable(
                fmt == PLOT_FORMAT::POST || fmt == PLOT_FORMAT::PDF || fmt == PLOT_FORMAT::SVG );

        m_colorTheme->Enable( fmt != PLOT_FORMAT::HPGL );
        m_ModeColorOption->Enable( fmt != PLOT_FORMAT::HPGL );
    }
}


void DIALOG_PLOT_SCHEMATIC::getPlotOptions( RENDER_SETTINGS* aSettings )
{
    m_HPGLPenSize = m_penWidth.GetValue();

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    COLOR_SETTINGS* colors = getColorSettings();

    if( cfg )
    {
        cfg->m_PlotPanel.background_color = m_plotBackgroundColor->GetValue();
        cfg->m_PlotPanel.color            = getModeColor();
        cfg->m_PlotPanel.color_theme      = colors->GetFilename();
        cfg->m_PlotPanel.frame_reference  = getPlotDrawingSheet();
        cfg->m_PlotPanel.format           = static_cast<int>( GetPlotFileFormat() );
        cfg->m_PlotPanel.hpgl_origin      = m_plotOriginOpt->GetSelection();
        cfg->m_PlotPanel.hpgl_paper_size  = m_HPGLPaperSizeSelect;

        // HPGL Pen Size is stored in mm in config
        cfg->m_PlotPanel.hpgl_pen_size = m_HPGLPenSize / IU_PER_MM;
    }

    aSettings->LoadColors( colors );
    aSettings->SetDefaultPenWidth( (int) m_defaultLineWidth.GetValue() );

    if( m_plotBackgroundColor->GetValue() )
        aSettings->SetBackgroundColor( colors->GetColor( LAYER_SCHEMATIC_BACKGROUND ) );
    else
        aSettings->SetBackgroundColor( COLOR4D::UNSPECIFIED );

    // Plot directory
    wxString path = m_outputDirectoryName->GetValue();
    path.Replace( '\\', '/' );

    SCHEMATIC_SETTINGS& settings = m_parent->Schematic().Settings();

    if( settings.m_PlotDirectoryName != path )
        m_configChanged = true;

    settings.m_PlotDirectoryName = path;
}


COLOR_SETTINGS* DIALOG_PLOT_SCHEMATIC::getColorSettings()
{
    int selection = m_colorTheme->GetSelection();

    if( selection < 0 )
        return Pgm().GetSettingsManager().GetColorSettings( "_builtin_default" );

    return static_cast<COLOR_SETTINGS*>( m_colorTheme->GetClientData( selection ) );
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
    KIGFX::SCH_RENDER_SETTINGS renderSettings( *m_parent->GetRenderSettings() );

    getPlotOptions( &renderSettings );

    switch( GetPlotFileFormat() )
    {
    default:
    case PLOT_FORMAT::POST:
        createPSFile( aPlotAll, getPlotDrawingSheet(), &renderSettings );
        break;
    case PLOT_FORMAT::DXF:
        CreateDXFFile( aPlotAll, getPlotDrawingSheet(), &renderSettings );
        break;
    case PLOT_FORMAT::PDF:
        createPDFFile( aPlotAll, getPlotDrawingSheet(), &renderSettings );
        break;
    case PLOT_FORMAT::SVG:
        createSVGFile( aPlotAll, getPlotDrawingSheet(), &renderSettings );
        break;
    case PLOT_FORMAT::HPGL:
        createHPGLFile( aPlotAll, getPlotDrawingSheet(), &renderSettings );
        break;
    }
}


wxFileName DIALOG_PLOT_SCHEMATIC::createPlotFileName( const wxString& aPlotFileName,
                                                      const wxString& aExtension,
                                                      REPORTER* aReporter )
{
    wxString   path = ExpandEnvVarSubstitutions( m_outputDirectoryName->GetValue(), &Prj() );
    wxFileName outputDir = wxFileName::DirName( path );
    wxString plotFileName;

    if( !aPlotFileName.IsEmpty() )
        plotFileName = Prj().AbsolutePath( aPlotFileName + wxT( "." ) + aExtension );
    else
        plotFileName = Prj().AbsolutePath( _( "Schematic" ) + wxT( "." ) + aExtension );

    if( !EnsureFileDirectoryExists( &outputDir, plotFileName, aReporter ) )
    {
        wxString msg = wxString::Format( _( "Could not write plot files to folder \"%s\"." ),
                                         outputDir.GetPath() );
        aReporter->Report( msg, RPT_SEVERITY_ERROR );
    }

    wxFileName fn( plotFileName );
    fn.SetPath( outputDir.GetFullPath() );
    return fn;
}
