/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <common.h>     // For ExpandEnvVarSubstitutions
#include "widgets/wx_html_report_panel.h"
#include <widgets/std_bitmap_button.h>
#include <dialog_plot_schematic.h>
#include <eeschema_settings.h>
#include <kiface_base.h>
#include <locale_io.h>
#include <plotters/plotter_hpgl.h>
#include <plotters/plotter_dxf.h>
#include <plotters/plotters_pslike.h>
#include <reporter.h>
#include <trace_helpers.h>
#include <settings/settings_manager.h>
#include <drawing_sheet/ds_painter.h>
#include <wx_filename.h>

#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <schematic.h>
#include <sch_screen.h>

#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <kiplatform/environment.h>
#include <wx/log.h>


// static members (static to remember last state):
int DIALOG_PLOT_SCHEMATIC::m_pageSizeSelect = PAGE_SIZE_AUTO;
HPGL_PAGE_SIZE DIALOG_PLOT_SCHEMATIC::m_HPGLPaperSizeSelect = HPGL_PAGE_SIZE::DEFAULT;


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

    SetupStandardButtons( { { wxID_OK,     _( "Plot All Pages" )    },
                            { wxID_APPLY,  _( "Plot Current Page" ) },
                            { wxID_CANCEL, _( "Close" )             } } );

    initDlg();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_PLOT_SCHEMATIC::initDlg()
{
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( cfg )
    {
        for( COLOR_SETTINGS* settings : m_parent->GetSettingsManager()->GetColorSettingsList() )
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

        setOpenFileAfterPlot( cfg->m_PlotPanel.open_file_after_plot );

        // HPGL plot origin and unit system configuration
        m_plotOriginOpt->SetSelection( cfg->m_PlotPanel.hpgl_origin );

        m_HPGLPaperSizeSelect = static_cast<HPGL_PAGE_SIZE>( cfg->m_PlotPanel.hpgl_paper_size );

        // HPGL Pen Size is stored in mm in config
        m_HPGLPenSize = cfg->m_PlotPanel.hpgl_pen_size * schIUScale.IU_PER_MM;

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
        m_defaultLineWidth.SetValue( schIUScale.MilsToIU( cfg->m_Drawing.default_line_thickness ) );
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

    // When editing a schematic that is not part of a project in the stand alone mode, the
    // project path is not defined so point to the users document path to save the plot files.
    if( Prj().IsNullProject() )
    {
        path = KIPLATFORM::ENV::GetDocumentsPath();
    }
    else
    {
        // Build the absolute path of current output directory to preselect it in the file browser.
        path = ExpandEnvVarSubstitutions( m_outputDirectoryName->GetValue(), &Prj() );
        path = Prj().AbsolutePath( path );
    }

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxFileName fn( Prj().AbsolutePath( m_parent->Schematic().Root().GetFileName() ) );
    wxString defaultPath = fn.GetPathWithSep();
    wxString msg;
    wxFileName relPathTest; // Used to test if we can make the path relative

    relPathTest.Assign( dirDialog.GetPath() );

    // Test if making the path relative is possible before asking the user if they want to do it
    if( relPathTest.MakeRelativeTo( defaultPath ) )
    {
        msg.Printf( _( "Do you want to use a path relative to\n'%s'?" ), defaultPath );

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
    case 0: return PLOT_FORMAT::POST;
    case 1: return PLOT_FORMAT::PDF;
    case 2: return PLOT_FORMAT::SVG;
    case 3: return PLOT_FORMAT::DXF;
    case 4: return PLOT_FORMAT::HPGL;
    }
}


void DIALOG_PLOT_SCHEMATIC::OnPageSizeSelected( wxCommandEvent& event )
{
    if( GetPlotFileFormat() == PLOT_FORMAT::HPGL )
        m_HPGLPaperSizeSelect = static_cast<HPGL_PAGE_SIZE>( m_paperSizeOption->GetSelection() );
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

            selection = static_cast<int>( m_HPGLPaperSizeSelect );
        }
        else
        {
            paperSizes.push_back( _( "A4" ) );
            paperSizes.push_back( _( "A" ) );

            selection = m_pageSizeSelect;
        }

        m_openFileAfterPlot->Enable( fmt == PLOT_FORMAT::PDF );

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
        cfg->m_PlotPanel.hpgl_paper_size  = static_cast<int>( m_HPGLPaperSizeSelect );
        cfg->m_PlotPanel.open_file_after_plot = getOpenFileAfterPlot();

        // HPGL Pen Size is stored in mm in config
        cfg->m_PlotPanel.hpgl_pen_size = m_HPGLPenSize / schIUScale.IU_PER_MM;

        aSettings->SetDefaultFont( cfg->m_Appearance.default_font );
    }

    aSettings->LoadColors( colors );
    aSettings->SetMinPenWidth( (int) m_defaultLineWidth.GetValue() );

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
    {
        return m_parent->GetSettingsManager()->GetColorSettings(
                COLOR_SETTINGS::COLOR_BUILTIN_DEFAULT );
    }

    return static_cast<COLOR_SETTINGS*>( m_colorTheme->GetClientData( selection ) );
}


void DIALOG_PLOT_SCHEMATIC::OnPlotCurrent( wxCommandEvent& event )
{
    plotSchematic( false );
}


void DIALOG_PLOT_SCHEMATIC::OnPlotAll( wxCommandEvent& event )
{
    plotSchematic( true );
}


void DIALOG_PLOT_SCHEMATIC::plotSchematic( bool aPlotAll )
{
    wxBusyCursor dummy;

    KIGFX::SCH_RENDER_SETTINGS renderSettings( *m_parent->GetRenderSettings() );

    getPlotOptions( &renderSettings );

    std::unique_ptr<SCH_PLOTTER> schPlotter = std::make_unique<SCH_PLOTTER>( m_parent );

    COLOR_SETTINGS*   colors = getColorSettings();

    SCH_PLOT_SETTINGS plotSettings;
    plotSettings.m_plotDrawingSheet = getPlotDrawingSheet();
    plotSettings.m_plotAll = aPlotAll;
    plotSettings.m_blackAndWhite = !getModeColor();
    plotSettings.m_useBackgroundColor = m_plotBackgroundColor->GetValue();
    plotSettings.m_theme = colors->GetFilename();
    plotSettings.m_HPGLPenSize = m_HPGLPenSize;
    plotSettings.m_HPGLPaperSizeSelect = static_cast<HPGL_PAGE_SIZE>( m_HPGLPaperSizeSelect );
    plotSettings.m_HPGLPlotOrigin =
            static_cast<HPGL_PLOT_ORIGIN_AND_UNITS>( m_plotOriginOpt->GetSelection() );
    plotSettings.m_outputDirectory = getOutputPath();
    plotSettings.m_pageSizeSelect = m_pageSizeSelect;


    schPlotter->Plot( GetPlotFileFormat(), plotSettings, &renderSettings,
                      &m_MessagesBox->Reporter() );

    if( GetPlotFileFormat() == PLOT_FORMAT::PDF && getOpenFileAfterPlot() )
        wxLaunchDefaultApplication( schPlotter->GetLastOutputFilePath() );
}


void DIALOG_PLOT_SCHEMATIC::setHpglPenWidth()
{
    m_HPGLPenSize = m_penWidth.GetValue();

    if( m_HPGLPenSize > schIUScale.mmToIU( 2 ) )
        m_HPGLPenSize = schIUScale.mmToIU( 2 );

    if( m_HPGLPenSize < schIUScale.mmToIU( 0.01 ) )
        m_HPGLPenSize = schIUScale.mmToIU( 0.01 );
}


wxString DIALOG_PLOT_SCHEMATIC::getOutputPath()
{
    wxString msg;
    wxString extMsg;
    wxFileName fn;

    extMsg.Printf( _( "Falling back to user path '%s'." ), KIPLATFORM::ENV::GetDocumentsPath() );

    // Build the absolute path of current output directory to preselect it in the file browser.
    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                return m_parent->Schematic().ResolveTextVar( token, 0 );
            };

    wxString path = m_outputDirectoryName->GetValue();
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, &Prj() );

    fn.SetPath( path );

    // If the contents of the path edit control results in an absolute path, return it as is.
    if( fn.IsAbsolute() )
        return path;

    // When editing a schematic that is not part of a project in the stand alone mode, the
    // project path is not defined.
    if( Prj().IsNullProject() )
    {
        SCH_SCREEN* screen = m_parent->Schematic().RootScreen();

        if( screen && !screen->GetFileName().IsEmpty() )
        {
            fn = screen->GetFileName();
            msg.Printf( _( "Cannot normalize path '%s%s'." ), fn.GetPathWithSep(), path );
            fn.SetPath( fn.GetPathWithSep() + path );

            // Normalize always returns true for a non-empty file name so clear the file name
            // and extension so that only the path is normalized.
            fn.SetName( wxEmptyString );
            fn.SetExt( wxEmptyString );

            if( fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS ) )
            {
                path = fn.GetPath();
            }
            else
            {
                wxMessageDialog dlg( this, msg, _( "Warning" ), wxOK | wxCENTER | wxRESIZE_BORDER
                                     | wxICON_EXCLAMATION | wxSTAY_ON_TOP );

                dlg.SetExtendedMessage( extMsg );
                dlg.ShowModal();

                path = KIPLATFORM::ENV::GetDocumentsPath();
            }
        }
        else
        {
            msg = _( "No project or path defined for the current schematic." );

            wxMessageDialog dlg( this, msg, _( "Warning" ), wxOK | wxCENTER | wxRESIZE_BORDER
                                 | wxICON_EXCLAMATION | wxSTAY_ON_TOP );
            dlg.SetExtendedMessage( extMsg );
            dlg.ShowModal();

            // Always fall back to user's document path if no other absolute path can be normalized.
            path = KIPLATFORM::ENV::GetDocumentsPath();
        }
    }
    else
    {
        msg.Printf( _( "Cannot normalize path '%s%s'." ), Prj().GetProjectPath(), path );

        // Build the absolute path of current output directory and the project path.
        fn.SetPath( Prj().GetProjectPath() + path );

        if( fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS ) )
        {
            path = fn.GetPath();
        }
        else
        {
            wxMessageDialog dlg( this, msg, _( "Warning" ),
                                 wxOK | wxCENTER | wxRESIZE_BORDER | wxICON_EXCLAMATION |
                                 wxSTAY_ON_TOP );

            dlg.SetExtendedMessage( extMsg );
            dlg.ShowModal();

            path = KIPLATFORM::ENV::GetDocumentsPath();
        }
    }

    return path;
}
