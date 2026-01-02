/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include "string_utils.h"
#include "widgets/wx_html_report_panel.h"
#include <widgets/std_bitmap_button.h>
#include <dialog_plot_schematic.h>
#include <eeschema_settings.h>
#include <kiface_base.h>
#include <locale_io.h>
#include <plotters/plotter_dxf.h>
#include <plotters/plotters_pslike.h>
#include <reporter.h>
#include <trace_helpers.h>
#include <settings/settings_manager.h>
#include <wx_filename.h>
#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <kiplatform/environment.h>

#include <jobs/job_export_sch_plot.h>
#include <confirm.h>


DIALOG_PLOT_SCHEMATIC::DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* aEditFrame ) :
        DIALOG_PLOT_SCHEMATIC( aEditFrame, aEditFrame, nullptr )
{
}


DIALOG_PLOT_SCHEMATIC::DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* aEditFrame, wxWindow* aParent,
                                              JOB_EXPORT_SCH_PLOT* aJob ) :
        DIALOG_PLOT_SCHEMATIC_BASE( aEditFrame ),
        m_editFrame( aEditFrame ),
        m_defaultLineWidth( aEditFrame, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits ),
        m_job( aJob )
{
    if( !m_job )
    {
        m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
        m_MessagesBox->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

        SetupStandardButtons( { { wxID_OK,     _( "Plot All Pages" )    },
                                { wxID_APPLY,  _( "Plot Current Page" ) },
                                { wxID_CANCEL, _( "Close" )             } } );
    }
    else
    {
        SetTitle( m_job->GetSettingsDialogTitle() );

        m_browseButton->Hide();
        m_MessagesBox->Hide();

        m_sdbSizer1Apply->Hide();
        m_openFileAfterPlot->Hide();

        SetupStandardButtons();
    }

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
        m_colorTheme->Append( settings->GetName(), static_cast<void*>( settings ) );

    m_variantChoiceCtrl->Append( m_editFrame->Schematic().GetVariantNamesForUI() );
    m_variantChoiceCtrl->Select( 0 );


    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_PLOT_SCHEMATIC::TransferDataToWindow()
{
    if( !m_job )
    {
        EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
        wxASSERT( cfg );

        if( m_lineWidthCtrl->GetValue().IsEmpty() && cfg )
        {
            // Set the default line width (pen width which should be used for items that do not have a
            // pen size defined (like frame ref).
            // The default line width is stored in mils in config
            m_defaultLineWidth.SetValue( schIUScale.MilsToIU( cfg->m_Drawing.default_line_thickness ) );
        }
    }
    else
    {
        if( !m_colorTheme->SetStringSelection( m_job->m_theme ) )
            m_colorTheme->SetSelection( 0 );

        m_plotBackgroundColor->SetValue( m_job->m_useBackgroundColor );
        m_defaultLineWidth.SetValue( m_job->m_minPenWidth );
        m_plotPDFPropertyPopups->SetValue( m_job->m_PDFPropertyPopups );
        m_plotPDFHierarchicalLinks->SetValue( m_job->m_PDFHierarchicalLinks );
        m_plotPDFMetadata->SetValue( m_job->m_PDFMetadata );

        int paperSizeIndex = (int) m_job->m_pageSizeSelect;

        if( paperSizeIndex >= 0 && paperSizeIndex < (int) m_paperSizeOption->GetCount() )
            m_paperSizeOption->SetSelection( paperSizeIndex );

        m_plotDrawingSheet->SetValue( m_job->m_plotDrawingSheet );
        m_ModeColorOption->SetSelection( m_job->m_blackAndWhite ? 1 : 0 );

        // Set the plot format
        switch( m_job->m_plotFormat )
        {
        default:
        case SCH_PLOT_FORMAT::POST: m_plotFormatOpt->SetSelection( 0 ); break;
        case SCH_PLOT_FORMAT::PDF:  m_plotFormatOpt->SetSelection( 1 ); break;
        case SCH_PLOT_FORMAT::SVG:  m_plotFormatOpt->SetSelection( 2 ); break;
        case SCH_PLOT_FORMAT::DXF:  m_plotFormatOpt->SetSelection( 3 ); break;
        case SCH_PLOT_FORMAT::HPGL: /* no longer supported */           break;
        }

        // And then hide it
        m_plotFormatOpt->Hide();

        m_outputPath->SetValue( m_job->GetConfiguredOutputPath() );
    }

    wxCommandEvent dummy;
    onPlotFormatSelection( dummy );

    return true;
}


/**
 * @todo Copy of DIALOG_PLOT::onOutputDirectoryBrowseClicked in dialog_plot.cpp, maybe merge to
 *       a common method.
 */
void DIALOG_PLOT_SCHEMATIC::onOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputPath->GetValue(), &Prj() );

    // When editing a schematic that is not part of a project in the stand alone mode, the
    // project path is not defined so point to the users document path to save the plot files.
    if( Prj().IsNullProject() )
    {
        path = KIPLATFORM::ENV::GetDocumentsPath();
    }
    else
    {
        // Build the absolute path of current output directory to preselect it in the file browser.
        path = ExpandEnvVarSubstitutions( m_outputPath->GetValue(), &Prj() );
        path = Prj().AbsolutePath( path );
    }

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxFileName fn( Prj().AbsolutePath( m_editFrame->Schematic().Root().GetFileName() ) );
    wxString defaultPath = fn.GetPathWithSep();
    wxString msg;
    wxFileName relPathTest; // Used to test if we can make the path relative

    relPathTest.Assign( dirDialog.GetPath() );

    // Test if making the path relative is possible before asking the user if they want to do it
    if( relPathTest.MakeRelativeTo( defaultPath ) )
    {
        if( IsOK( this, wxString::Format( _( "Do you want to use a path relative to\n'%s'?" ), defaultPath ) ) )
            dirName.MakeRelativeTo( defaultPath );
    }

    m_outputPath->SetValue( dirName.GetFullPath() );
}


PLOT_FORMAT DIALOG_PLOT_SCHEMATIC::getPlotFileFormat()
{
    switch( m_plotFormatOpt->GetSelection() )
    {
    case 0: return PLOT_FORMAT::POST;
    default:
    case 1: return PLOT_FORMAT::PDF;
    case 2: return PLOT_FORMAT::SVG;
    case 3: return PLOT_FORMAT::DXF;
    }
}


void DIALOG_PLOT_SCHEMATIC::onColorMode( wxCommandEvent& aEvent )
{
    bool backgroundColorAvailable = getPlotFileFormat() == PLOT_FORMAT::POST
                                    || getPlotFileFormat() == PLOT_FORMAT::PDF
                                    || getPlotFileFormat() == PLOT_FORMAT::SVG;

    m_colorThemeLabel->Enable( getModeColor() );
    m_colorTheme->Enable( getModeColor() );
    m_plotBackgroundColor->Enable( backgroundColorAvailable && getModeColor() );

    aEvent.Skip();
}


void DIALOG_PLOT_SCHEMATIC::getPlotOptions( RENDER_SETTINGS* aSettings )
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
        aSettings->SetDefaultFont( cfg->m_Appearance.default_font );

    COLOR_SETTINGS* colors = getColorSettings();

    aSettings->LoadColors( colors );
    aSettings->SetMinPenWidth( (int) m_defaultLineWidth.GetValue() );

    if( m_plotBackgroundColor->GetValue() )
        aSettings->SetBackgroundColor( colors->GetColor( LAYER_SCHEMATIC_BACKGROUND ) );
    else
        aSettings->SetBackgroundColor( COLOR4D::UNSPECIFIED );
}


COLOR_SETTINGS* DIALOG_PLOT_SCHEMATIC::getColorSettings()
{
    int selection = m_colorTheme->GetSelection();

    if( selection < 0 )
        return ::GetColorSettings( COLOR_SETTINGS::COLOR_BUILTIN_DEFAULT );

    return static_cast<COLOR_SETTINGS*>( m_colorTheme->GetClientData( selection ) );
}


void DIALOG_PLOT_SCHEMATIC::onPlotFormatSelection( wxCommandEvent& event )
{
    PLOT_FORMAT fmt = getPlotFileFormat();

    m_openFileAfterPlot->Enable( fmt == PLOT_FORMAT::PDF );
    m_plotPDFPropertyPopups->Enable( fmt == PLOT_FORMAT::PDF );
    m_plotPDFHierarchicalLinks->Enable( fmt == PLOT_FORMAT::PDF );
    m_plotPDFMetadata->Enable( fmt == PLOT_FORMAT::PDF );

    // Currently kicad-cli always exports in mm (also true in Pcbnew)
    m_staticTextDXF->Enable( fmt == PLOT_FORMAT::DXF && m_job == nullptr );
    m_DXF_plotUnits->Enable( fmt == PLOT_FORMAT::DXF && m_job == nullptr );

    m_paperSizeOption->SetSelection( m_paperSizeOption->GetSelection() );

    m_defaultLineWidth.Enable( fmt == PLOT_FORMAT::POST || fmt == PLOT_FORMAT::PDF || fmt == PLOT_FORMAT::SVG );

    wxCommandEvent dummy;
    onColorMode( dummy );

    event.Skip();
}


void DIALOG_PLOT_SCHEMATIC::OnPlotCurrent( wxCommandEvent& event )
{
    plotSchematic( false );
}


void DIALOG_PLOT_SCHEMATIC::OnPlotAll( wxCommandEvent& event )
{
    if( !m_job )
    {
        plotSchematic( true );
    }
    else
    {
        m_job->m_blackAndWhite = !getModeColor();
        m_job->m_useBackgroundColor = m_plotBackgroundColor->GetValue();
        m_job->m_minPenWidth = m_defaultLineWidth.GetIntValue();
        m_job->m_pageSizeSelect = static_cast<JOB_PAGE_SIZE>( m_paperSizeOption->GetSelection() );
        m_job->m_PDFPropertyPopups = m_plotPDFPropertyPopups->GetValue();
        m_job->m_PDFHierarchicalLinks = m_plotPDFHierarchicalLinks->GetValue();
        m_job->m_PDFMetadata = m_plotPDFMetadata->GetValue();
        m_job->m_plotDrawingSheet = m_plotDrawingSheet->GetValue();
        m_job->m_plotAll = true;
        m_job->SetConfiguredOutputPath( m_outputPath->GetValue() );
        m_job->m_theme = getColorSettings()->GetName();
        m_job->m_variant = getSelectedVariant();

        event.Skip(); // Allow normal close action
    }
}


void DIALOG_PLOT_SCHEMATIC::plotSchematic( bool aPlotAll )
{
    wxBusyCursor dummy;

    SCH_RENDER_SETTINGS renderSettings( *m_editFrame->GetRenderSettings() );
    renderSettings.m_ShowHiddenPins = false;
    renderSettings.m_ShowHiddenFields = false;

    getPlotOptions( &renderSettings );

    std::unique_ptr<SCH_PLOTTER> schPlotter = std::make_unique<SCH_PLOTTER>( m_editFrame );

    SCH_PLOT_OPTS plotOpts;
    plotOpts.m_plotDrawingSheet = m_plotDrawingSheet->GetValue();
    plotOpts.m_plotAll = aPlotAll;
    plotOpts.m_blackAndWhite = !getModeColor();
    plotOpts.m_useBackgroundColor = m_plotBackgroundColor->GetValue();
    plotOpts.m_theme = getColorSettings()->GetFilename();
    plotOpts.m_PDFPropertyPopups = m_plotPDFPropertyPopups->GetValue();
    plotOpts.m_PDFHierarchicalLinks = m_plotPDFHierarchicalLinks->GetValue();
    plotOpts.m_PDFMetadata = m_plotPDFMetadata->GetValue();
    plotOpts.m_outputDirectory = getOutputPath();
    plotOpts.m_pageSizeSelect = m_paperSizeOption->GetSelection();
    plotOpts.m_plotHopOver = m_editFrame->Schematic().Settings().m_HopOverScale > 0.0;

    // Select the DXF file unit
    plotOpts.m_DXF_File_Unit = m_DXF_plotUnits->GetSelection() == 0 ? DXF_UNITS::INCH : DXF_UNITS::MM;
    plotOpts.m_variant = getSelectedVariant();
    schPlotter->Plot( getPlotFileFormat(), plotOpts, &renderSettings, &m_MessagesBox->Reporter() );

    if( getPlotFileFormat() == PLOT_FORMAT::PDF && m_openFileAfterPlot->GetValue() )
        wxLaunchDefaultApplication( schPlotter->GetLastOutputFilePath() );
}


wxString DIALOG_PLOT_SCHEMATIC::getOutputPath()
{
    wxString extMsg = wxString::Format( _( "Falling back to user path '%s'." ),
                                          KIPLATFORM::ENV::GetDocumentsPath() );

    wxFileName fn;

    // Build the absolute path of current output directory to preselect it in the file browser.
    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                SCHEMATIC& schematic = m_editFrame->Schematic();
                return schematic.ResolveTextVar( &schematic.CurrentSheet(), token, 0 );
            };

    wxString path = m_outputPath->GetValue();
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
        SCH_SCREEN* screen = m_editFrame->Schematic().RootScreen();

        if( screen && !screen->GetFileName().IsEmpty() )
        {
            fn = screen->GetFileName();
            path = fn.GetPathWithSep() + path;
            fn.SetPath( path );

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
                DisplayErrorMessage( this, wxString::Format( _( "Cannot normalize path '%s'." ), path ), extMsg );
                path = KIPLATFORM::ENV::GetDocumentsPath();
            }
        }
        else
        {
            DisplayErrorMessage( this, _( "No project or path defined for the current schematic." ), extMsg );
            // Always fall back to user's document path if no other absolute path can be normalized.
            path = KIPLATFORM::ENV::GetDocumentsPath();
        }
    }
    else
    {
        // Build the absolute path of current output directory and the project path.
        path = Prj().GetProjectPath() + path;
        fn.SetPath( path );

        if( fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS ) )
        {
            path = fn.GetPath();
        }
        else
        {
            DisplayErrorMessage( this, wxString::Format( _( "Cannot normalize path '%s'." ), path ), extMsg );
            path = KIPLATFORM::ENV::GetDocumentsPath();
        }
    }

    return path;
}


wxString DIALOG_PLOT_SCHEMATIC::getSelectedVariant() const
{
    wxString variant;
    int      selection = m_variantChoiceCtrl->GetSelection();

    if( ( selection != 0 ) && ( selection != wxNOT_FOUND ) )
        variant = m_variantChoiceCtrl->GetString( selection );

    return variant;
}