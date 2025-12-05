/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 Jean_Pierre Charras <jp.charras at wanadoo.fr>
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

#include "dialog_gendrill.h"

#include <wx/msgdlg.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>

#include <confirm.h>
#include <core/arraydim.h>
#include <widgets/std_bitmap_button.h>
#include <pcb_edit_frame.h>
#include <pcbplot.h>
#include <gendrill_excellon_writer.h>
#include <gendrill_gerber_writer.h>
#include <bitmaps.h>
#include <tools/board_editor_control.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <paths.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>
#include <jobs/job_export_pcb_drill.h>

// List of allowed precision for EXCELLON files, for integer format.  Due to difference between inches and mm,
// there are 2 precision values, one for inches and one for metric.
// Note: for decimal format, the precision is not used.
static DRILL_PRECISION precisionListForInches( 2, 4 );
static DRILL_PRECISION precisionListForMetric( 3, 3 );


/* This function displays the dialog frame for drill tools
 */
int BOARD_EDITOR_CONTROL::GenerateDrillFiles( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    DIALOG_GENDRILL dlg( editFrame, editFrame );

    dlg.ShowModal();
    return 0;
}


DIALOG_GENDRILL::DIALOG_GENDRILL( PCB_EDIT_FRAME* aPcbEditFrame, wxWindow* aParent  ) :
        DIALOG_GENDRILL_BASE( aParent ),
        m_pcbEditFrame( aPcbEditFrame ),
        m_board( aPcbEditFrame->GetBoard() ),
        m_plotOpts( aPcbEditFrame->GetPlotSettings() ),
        m_job( nullptr )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    SetupStandardButtons( { { wxID_OK,     _( "Generate" ) },
                            { wxID_CANCEL, _( "Close" ) } } );

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    finishDialogSettings();
}


DIALOG_GENDRILL::DIALOG_GENDRILL( PCB_EDIT_FRAME* aPcbEditFrame, JOB_EXPORT_PCB_DRILL* aJob,
                                  wxWindow* aParent ) :
        DIALOG_GENDRILL_BASE( aParent ),
        m_pcbEditFrame( aPcbEditFrame ),
        m_board( m_pcbEditFrame->GetBoard() ),
        m_job( aJob )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    // hide ui elements that dont belong for job config
    m_buttonReport->Hide();
    bMainSizer->Remove( bMsgSizer );
    m_messagesBox->Hide();

    SetupStandardButtons();

    SetTitle( m_job->GetSettingsDialogTitle() );

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    finishDialogSettings();
}


bool DIALOG_GENDRILL::TransferDataToWindow()
{
    m_messagesBox->Clear();

    if( !m_job )
    {
        updatePrecisionOptions();

        m_origin->SetSelection( m_plotOpts.GetUseAuxOrigin() ? 1 : 0 );

        // Output directory
        m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );
    }
    else
    {
        m_browseButton->Hide();
        m_outputDirectoryName->SetValue( m_job->GetConfiguredOutputPath() );

        m_rbExcellon->SetValue( m_job->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON );
        m_rbGerberX2->SetValue( m_job->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER );
        m_units->SetSelection( m_job->m_drillUnits == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCH );
        m_zeros->SetSelection( static_cast<int>( m_job->m_zeroFormat ) );
        updatePrecisionOptions();
        m_Check_Minimal->SetValue( m_job->m_excellonMinimalHeader );

        m_origin->SetSelection( m_job->m_drillOrigin == JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::PLOT );

        m_Check_Mirror->SetValue( m_job->m_excellonMirrorY );
        m_Check_Merge_PTH_NPTH->SetValue( m_job->m_excellonCombinePTHNPTH );
        m_choiceDrillMap->SetSelection( static_cast<int>( m_job->m_mapFormat ) );
        m_altDrillMode->SetValue( !m_job->m_excellonOvalDrillRoute );
        m_cbGenerateMap->SetValue( m_job->m_generateMap );
        m_generateTentingLayers->SetValue( m_job->m_generateTenting );
    }

    wxCommandEvent dummy;
    onFileFormatSelection( dummy );
	return true;
}


bool DIALOG_GENDRILL::TransferDataFromWindow()
{
    if( !m_job )
    {
        genDrillAndMapFiles( true, m_cbGenerateMap->GetValue(), m_generateTentingLayers->GetValue() );
        // Keep the window open so that the user can see the result
        return false;
    }
    else
    {
        m_job->SetConfiguredOutputPath( m_outputDirectoryName->GetValue() );
        m_job->m_format = m_rbExcellon->GetValue() ? JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON
												   : JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER;
        m_job->m_drillUnits = m_units->GetSelection() == 0 ? JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MM
                                                           : JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCH;
        m_job->m_drillOrigin = static_cast<JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN>( m_origin->GetSelection() );
        m_job->m_excellonCombinePTHNPTH = m_Check_Merge_PTH_NPTH->IsChecked();
        m_job->m_excellonMinimalHeader = m_Check_Minimal->IsChecked();
        m_job->m_excellonMirrorY = m_Check_Mirror->IsChecked();
        m_job->m_excellonOvalDrillRoute = !m_altDrillMode->GetValue();
        m_job->m_mapFormat = static_cast<JOB_EXPORT_PCB_DRILL::MAP_FORMAT>( m_choiceDrillMap->GetSelection() );
        m_job->m_zeroFormat = static_cast<JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT>( m_zeros->GetSelection() );
        m_job->m_generateMap = m_cbGenerateMap->IsChecked();
        m_job->m_generateTenting = m_generateTentingLayers->IsChecked();
    }

    return true;
}


void DIALOG_GENDRILL::onFileFormatSelection( wxCommandEvent& event )
{
    bool enbl_Excellon = m_rbExcellon->GetValue();

    m_unitsLabel->Enable( enbl_Excellon );
    m_units->Enable( enbl_Excellon );
    m_zerosLabel->Enable( enbl_Excellon );
	m_zeros->Enable( enbl_Excellon );
    m_Check_Mirror->Enable( enbl_Excellon );
    m_Check_Minimal->Enable( enbl_Excellon );
    m_Check_Merge_PTH_NPTH->Enable( enbl_Excellon );
    m_altDrillMode->Enable( enbl_Excellon );
    m_generateTentingLayers->Enable( !enbl_Excellon );

    if( enbl_Excellon )
    {
        updatePrecisionOptions();
    }
    else
    {
        m_precisionLabel->Enable( true );
        m_staticTextPrecision->Enable( true );
        m_staticTextPrecision->SetLabel( m_plotOpts.GetGerberPrecision() == 6 ? wxT( "4.6" )
                                                                              : wxT( "4.5" ) );
    }
}


void DIALOG_GENDRILL::updateConfig()
{
    // Set output directory and replace backslashes with forward ones
    wxString dirStr = m_outputDirectoryName->GetValue();
    dirStr.Replace( wxT( "\\" ), wxT( "/" ) );
    m_plotOpts.SetOutputDirectory( dirStr );
    m_plotOpts.SetUseAuxOrigin( m_origin->GetSelection() == 1 );

    if( !m_plotOpts.IsSameAs( m_board->GetPlotOptions() ) )
    {
        m_board->SetPlotOptions( m_plotOpts );
        m_pcbEditFrame->OnModify();
    }
}


void DIALOG_GENDRILL::onSelDrillUnitsSelected( wxCommandEvent& event )
{
    updatePrecisionOptions();
}


void DIALOG_GENDRILL::onSelZerosFmtSelected( wxCommandEvent& event )
{
    updatePrecisionOptions();
}


void DIALOG_GENDRILL::updatePrecisionOptions()
{
    if( m_units->GetSelection() == 1 )
    {
        // Units = inches
        m_staticTextPrecision->SetLabel( precisionListForInches.GetPrecisionString() );
    }
    else
    {
        // metric options
        m_staticTextPrecision->SetLabel( precisionListForMetric.GetPrecisionString() );
    }

    if( m_zeros->GetSelection() == EXCELLON_WRITER::DECIMAL_FORMAT )
    {
        m_precisionLabel->Enable( false );
        m_staticTextPrecision->Enable( false );
    }
    else
    {
        m_precisionLabel->Enable( true );
        m_staticTextPrecision->Enable( true );
    }
}


void DIALOG_GENDRILL::onOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputDirectoryName->GetValue(), &Prj() );
    path = Prj().AbsolutePath( path );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );
    wxFileName fn( Prj().AbsolutePath( m_board->GetFileName() ) );
    wxString   defaultPath = fn.GetPathWithSep();

    if( IsOK( this, wxString::Format( _( "Do you want to use a path relative to\n'%s'?" ), defaultPath ) ) )
    {
        if( !dirName.MakeRelativeTo( defaultPath ) )
        {
            DisplayErrorMessage( this, _( "Cannot make path relative (target volume different from board "
                                          "file volume)!" ) );
        }
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


void DIALOG_GENDRILL::genDrillAndMapFiles( bool aGenDrill, bool aGenMap, bool aGenTenting )
{
    updateConfig();     // set params and Save drill options

    m_pcbEditFrame->ClearMsgPanel();
    m_messagesBox->Clear();
    WX_TEXT_CTRL_REPORTER reporter( m_messagesBox );

    const PLOT_FORMAT filefmt[] = {
        // Keep these format ids in the same order than m_Choice_Drill_Map choices
        PLOT_FORMAT::POST,
        PLOT_FORMAT::GERBER,    // Only X2 format because we need the .FileFunction attribute
        PLOT_FORMAT::DXF,
        PLOT_FORMAT::SVG,
        PLOT_FORMAT::PDF
    };

    unsigned choice = (unsigned) m_choiceDrillMap->GetSelection();

    if( choice >= arrayDim( filefmt ) )
        choice = arrayDim( filefmt )-1;     // Last choice = PDF

    // Create output directory if it does not exist (also transform it in absolute form).
    // Bail if it fails.

    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                // Handles m_board->GetTitleBlock() *and* m_board->GetProject()
                return m_board->ResolveTextVar( token, 0 );
            };

    wxString path = m_plotOpts.GetOutputDirectory();
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, nullptr );

    wxFileName  outputDir = wxFileName::DirName( path );
    wxString    boardFilename = m_board->GetFileName();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, &reporter ) )
    {
        DisplayError( this, wxString::Format( _( "Could not write drill and/or map files to folder '%s'." ),
                                              outputDir.GetPath() ) );
        return;
    }

    VECTOR2I        drillFileOffset;
    DRILL_PRECISION precision;

    if( m_origin->GetSelection() == 0 )
        drillFileOffset = VECTOR2I( 0, 0 );
    else
        drillFileOffset = m_board->GetDesignSettings().GetAuxOrigin();

    if( m_units->GetSelection() == 0 )
        precision = precisionListForMetric;
    else
        precision = precisionListForInches;

    if( m_rbExcellon->GetValue() )
    {
        EXCELLON_WRITER excellonWriter( m_board );
        excellonWriter.SetFormat( m_units->GetSelection() == 0, (EXCELLON_WRITER::ZEROS_FMT) m_zeros->GetSelection(),
                                  precision.m_Lhs, precision.m_Rhs );
        excellonWriter.SetOptions( m_Check_Mirror->IsChecked(), m_Check_Minimal->IsChecked(), drillFileOffset,
                                   m_Check_Merge_PTH_NPTH->IsChecked() );
        excellonWriter.SetRouteModeForOvalHoles( !m_altDrillMode->GetValue() );
        excellonWriter.SetMapFileFormat( filefmt[choice] );
        excellonWriter.SetPageInfo( &m_board->GetPageSettings() );

        excellonWriter.CreateDrillandMapFilesSet( outputDir.GetFullPath(), aGenDrill, aGenMap, &reporter );
    }
    else
    {
        GERBER_WRITER gerberWriter( m_board );
        // Set gerber precision: only 5 or 6 digits for mantissa are allowed
        // (SetFormat() accept 5 or 6, and any other value set the precision to 5)
        // the integer part precision is always 4, and units always mm
        gerberWriter.SetFormat( m_plotOpts.GetGerberPrecision() );
        gerberWriter.SetOptions( drillFileOffset );
        gerberWriter.SetMapFileFormat( filefmt[choice] );
        gerberWriter.SetPageInfo( &m_board->GetPageSettings() );

        gerberWriter.CreateDrillandMapFilesSet( outputDir.GetFullPath(), aGenDrill, aGenMap, aGenTenting, &reporter );
    }
}


void DIALOG_GENDRILL::onGenReportFile( wxCommandEvent& event )
{
    updateConfig(); // set params and Save drill options

    wxFileName fn = m_board->GetFileName();

    fn.SetName( fn.GetName() + wxT( "-drl" ) );
    fn.SetExt( FILEEXT::ReportFileExtension );

    wxString defaultPath = ExpandEnvVarSubstitutions( m_plotOpts.GetOutputDirectory(), &Prj() );
    defaultPath = Prj().AbsolutePath( defaultPath );

    if( defaultPath.IsEmpty() )
        defaultPath = PATHS::GetDefaultUserProjectsPath();

    wxFileDialog dlg( this, _( "Save Drill Report File" ), defaultPath, fn.GetFullName(),
                      FILEEXT::ReportFileWildcard(), wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_messagesBox->Clear();
    bool success;

    // Info is slightly different between Excellon and Gerber
    // (file ext, Merge PTH/NPTH option)
    if( m_rbExcellon->GetValue() == 0 )
    {
        EXCELLON_WRITER excellonWriter( m_board );
        excellonWriter.SetMergeOption( m_Check_Merge_PTH_NPTH->IsChecked() );
        success = excellonWriter.GenDrillReportFile( dlg.GetPath() );
    }
    else
    {
        GERBER_WRITER gerberWriter( m_board );
        success = gerberWriter.GenDrillReportFile( dlg.GetPath() );
    }

    if( !success )
        m_messagesBox->AppendText( wxString::Format( _( "Failed to create file '%s'." ), dlg.GetPath() ) );
    else
        m_messagesBox->AppendText( wxString::Format( _( "Report file '%s' created." ), dlg.GetPath() ) );
}
