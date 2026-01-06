/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

/*
 *  1 - create ASCII files for automatic placement of smd components
 *  2 - create a footprint report (pos and footprint descr) (ascii file)
 */

#include "dialog_gen_footprint_position.h"

#include <wx/dirdlg.h>
#include <wx/msgdlg.h>

#include <confirm.h>
#include <pcb_edit_frame.h>
#include <project/project_file.h>
#include <bitmaps.h>
#include <reporter.h>
#include <tools/board_editor_control.h>
#include <wildcards_and_files_ext.h>
#include <kiface_base.h>
#include <string_utils.h>
#include <widgets/wx_html_report_panel.h>
#include <widgets/std_bitmap_button.h>
#include <exporters/place_file_exporter.h>
#include "gerber_placefile_writer.h"
#include <jobs/job_export_pcb_pos.h>


DIALOG_GEN_FOOTPRINT_POSITION::DIALOG_GEN_FOOTPRINT_POSITION( PCB_EDIT_FRAME* aEditFrame ) :
        DIALOG_GEN_FOOTPRINT_POSITION_BASE( aEditFrame ),
        m_editFrame( aEditFrame ),
        m_job( nullptr )
{
    m_messagesPanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );
    m_messagesPanel->MsgPanelSetMinSize( wxSize( -1, 160 ) );

    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    SetupStandardButtons( { { wxID_OK, _( "Generate Position File" ) },
                            { wxID_CANCEL, _( "Close" ) } } );

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_GEN_FOOTPRINT_POSITION::DIALOG_GEN_FOOTPRINT_POSITION( JOB_EXPORT_PCB_POS* aJob,
                                                              PCB_EDIT_FRAME*     aEditFrame,
                                                              wxWindow*           aParent ) :
        DIALOG_GEN_FOOTPRINT_POSITION_BASE( aParent ),
        m_editFrame( aEditFrame ),
        m_job( aJob )
{
    SetTitle( m_job->GetSettingsDialogTitle() );

    m_browseButton->Hide();
    m_units = m_job->m_units == JOB_EXPORT_PCB_POS::UNITS::INCH ? EDA_UNITS::INCH : EDA_UNITS::MM;
    m_staticTextDir->SetLabel( _( "Output file:" ) );

    m_messagesPanel->Hide();

    SetupStandardButtons();

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    GetSizer()->SetSizeHints( this );
    Centre();
}


bool DIALOG_GEN_FOOTPRINT_POSITION::TransferDataToWindow()
{
    if( m_job )
    {
        m_outputDirectoryName->SetValue( m_job->GetConfiguredOutputPath() );

        m_unitsCtrl->SetSelection( static_cast<int>( m_job->m_units ) );
        m_singleFile->SetValue( m_job->m_singleFile );
        m_formatCtrl->SetSelection( static_cast<int>( m_job->m_format ) );
        m_cbIncludeBoardEdge->SetValue( m_job->m_gerberBoardEdge );
        m_useDrillPlaceOrigin->SetValue( m_job->m_useDrillPlaceFileOrigin );
        m_onlySMD->SetValue( m_job->m_smdOnly );
        m_negateXcb->SetValue( m_job->m_negateBottomX );
        m_excludeTH->SetValue( m_job->m_excludeFootprintsWithTh );
        m_excludeDNP->SetValue( m_job->m_excludeDNP );
        m_excludeBOM->SetValue( m_job->m_excludeBOM );
    }

    return true;
}


void DIALOG_GEN_FOOTPRINT_POSITION::onUpdateUIUnits( wxUpdateUIEvent& event )
{
    m_unitsLabel->Enable( m_formatCtrl->GetSelection() != 2 );
    m_unitsCtrl->Enable( m_formatCtrl->GetSelection() != 2 );
}


void DIALOG_GEN_FOOTPRINT_POSITION::onUpdateUIFileOpt( wxUpdateUIEvent& event )
{
    m_singleFile->Enable( m_formatCtrl->GetSelection() != 2 );
}


void DIALOG_GEN_FOOTPRINT_POSITION::onUpdateUIOnlySMD( wxUpdateUIEvent& event )
{
    if( m_formatCtrl->GetSelection() == 2 )
    {
        m_onlySMD->SetValue( false );
        m_onlySMD->Enable( false );
    }
    else
    {
        m_onlySMD->Enable( true );
    }
}


void DIALOG_GEN_FOOTPRINT_POSITION::onUpdateUInegXcoord( wxUpdateUIEvent& event )
{
    if( m_formatCtrl->GetSelection() == 2 )
    {
        m_negateXcb->SetValue( false );
        m_negateXcb->Enable( false );
    }
    else
    {
        m_negateXcb->Enable( true );
    }
}

void DIALOG_GEN_FOOTPRINT_POSITION::onUpdateUIExcludeTH( wxUpdateUIEvent& event )
{
    if( m_formatCtrl->GetSelection() == 2 )
    {
        if( event.GetEventObject() == m_excludeTH )
            m_excludeTH->SetValue( false );
        else if( event.GetEventObject() == m_excludeDNP )
            m_excludeDNP->SetValue( false );
        else if( event.GetEventObject() == m_excludeBOM )
            m_excludeBOM->SetValue( false );

        event.Enable( false );
    }
    else
    {
        event.Enable( true );
    }
}


void DIALOG_GEN_FOOTPRINT_POSITION::onUpdateUIincludeBoardEdge( wxUpdateUIEvent& event )
{
    m_cbIncludeBoardEdge->Enable( m_formatCtrl->GetSelection() == 2 );
}


void DIALOG_GEN_FOOTPRINT_POSITION::onOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputDirectoryName->GetValue(), &Prj() );
    path = Prj().AbsolutePath( path );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    if( IsOK( this, _( "Use a relative path?" ) ) )
    {
        wxString boardFilePath = ( (wxFileName) m_editFrame->GetBoard()->GetFileName() ).GetPath();

        if( !dirName.MakeRelativeTo( boardFilePath ) )
        {
            DisplayErrorMessage( this, _( "Cannot make path relative (target volume different from board "
                                          "file volume)!" ) );
        }
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


void DIALOG_GEN_FOOTPRINT_POSITION::onGenerate( wxCommandEvent& event )
{
    if( !m_job )
    {
        m_units  = m_unitsCtrl->GetSelection() == 0 ? EDA_UNITS::INCH : EDA_UNITS::MM;

        m_outputDirectory = m_outputDirectoryName->GetValue();
        // Keep unix directory format convention in cfg files
        m_outputDirectory.Replace( wxT( "\\" ), wxT( "/" ) );

        if( m_formatCtrl->GetSelection() == 2 )
            CreateGerberFiles();
        else
            CreateAsciiFiles();
    }
    else
    {
        m_job->SetConfiguredOutputPath( m_outputDirectoryName->GetValue() );
        m_job->m_units = m_unitsCtrl->GetSelection() == 0 ? JOB_EXPORT_PCB_POS::UNITS::INCH
                                                          : JOB_EXPORT_PCB_POS::UNITS::MM;
        m_job->m_format = static_cast<JOB_EXPORT_PCB_POS::FORMAT>( m_formatCtrl->GetSelection() );
        m_job->m_side = JOB_EXPORT_PCB_POS::SIDE::BOTH;
        m_job->m_singleFile = m_singleFile->GetValue();
        m_job->m_gerberBoardEdge = m_cbIncludeBoardEdge->GetValue();
        m_job->m_excludeFootprintsWithTh = m_excludeTH->GetValue();
        m_job->m_smdOnly = m_onlySMD->GetValue();
        m_job->m_useDrillPlaceFileOrigin = m_useDrillPlaceOrigin->GetValue();
        m_job->m_negateBottomX = m_negateXcb->GetValue();
        m_job->m_excludeDNP = m_excludeDNP->GetValue();
        m_job->m_excludeBOM = m_excludeBOM->GetValue();

        event.Skip();   // Allow normal close action
    }
}


bool DIALOG_GEN_FOOTPRINT_POSITION::CreateGerberFiles()
{
    BOARD*     brd = m_editFrame->GetBoard();
    wxString   msg;
    int        fullcount = 0;

    // Create output directory if it does not exist (also transform it in absolute form).
    // Bail if it fails.

    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                // Handles board->GetTitleBlock() *and* board->GetProject()
                return m_editFrame->GetBoard()->ResolveTextVar( token, 0 );
            };

    wxString path = m_outputDirectory;
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, nullptr );

    wxFileName outputDir = wxFileName::DirName( path );
    wxString   boardFilename = m_editFrame->GetBoard()->GetFileName();
    REPORTER*  reporter = &m_messagesPanel->Reporter();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, reporter ) )
    {
        msg.Printf( _( "Could not write plot files to folder '%s'." ), outputDir.GetPath() );
        DisplayError( this, msg );
        return false;
    }

    wxFileName fn = m_editFrame->GetBoard()->GetFileName();
    fn.SetPath( outputDir.GetPath() );

    // Create the Front and Top side placement files. Gerber P&P files are always separated.
    // Not also they include all footprints
    PLACEFILE_GERBER_WRITER exporter( brd );

    // Set the current variant for variant-aware DNP/BOM/position file filtering
    exporter.SetVariant( brd->GetCurrentVariant() );

    wxString                filename = exporter.GetPlaceFileName( fn.GetFullPath(), F_Cu );

    int fpcount = exporter.CreatePlaceFile( filename, F_Cu, m_cbIncludeBoardEdge->GetValue(),
                                            m_excludeDNP->GetValue(), ExcludeBOM() );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Failed to create file '%s'." ), fn.GetFullPath() );
        wxMessageBox( msg );
        reporter->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    msg.Printf( _( "Front (top side) placement file: '%s'." ), filename );
    reporter->Report( msg, RPT_SEVERITY_ACTION );

    msg.Printf( _( "Component count: %d." ), fpcount );
    reporter->Report( msg, RPT_SEVERITY_INFO );

    // Create the Back or Bottom side placement file
    fullcount = fpcount;

    filename = exporter.GetPlaceFileName( fn.GetFullPath(), B_Cu );

    fpcount = exporter.CreatePlaceFile( filename, B_Cu, m_cbIncludeBoardEdge->GetValue(),
                                        m_excludeDNP->GetValue(), ExcludeBOM() );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Failed to create file '%s'." ), filename );
        reporter->Report( msg, RPT_SEVERITY_ERROR );
        wxMessageBox( msg );
        return false;
    }

    // Display results
    msg.Printf( _( "Back (bottom side) placement file: '%s'." ), filename );
    reporter->Report( msg, RPT_SEVERITY_ACTION );

    msg.Printf( _( "Component count: %d." ), fpcount );
    reporter->Report( msg, RPT_SEVERITY_INFO );

    fullcount += fpcount;
    msg.Printf( _( "Full component count: %d." ), fullcount );
    reporter->Report( msg, RPT_SEVERITY_INFO );

    reporter->Report( _( "Done." ), RPT_SEVERITY_INFO );

    return true;
}


bool DIALOG_GEN_FOOTPRINT_POSITION::CreateAsciiFiles()
{
    BOARD*     brd = m_editFrame->GetBoard();
    wxString   msg;
    bool       singleFile = OneFileOnly();
    bool       useCSVfmt = m_formatCtrl->GetSelection() == 1;
    bool       useAuxOrigin = m_useDrillPlaceOrigin->GetValue();
    int        fullcount = 0;
    int        topSide = true;
    int        bottomSide = true;
    bool       negateBottomX = m_negateXcb->GetValue();

    // Test for any footprint candidate in list.
    {
        PLACE_FILE_EXPORTER exporter( brd, UnitsMM(), OnlySMD(), ExcludeAllTH(), ExcludeDNP(),
                                      ExcludeBOM(), topSide, bottomSide, useCSVfmt, useAuxOrigin,
                                      negateBottomX );

        // Set the current variant for variant-aware DNP/BOM/position file filtering
        exporter.SetVariant( brd->GetCurrentVariant() );

        exporter.GenPositionData();

        if( exporter.GetFootprintCount() == 0 )
        {
            wxMessageBox( _( "No footprint for automated placement." ) );
            return false;
        }
    }

    // Create output directory if it does not exist (also transform it in absolute form).
    // Bail if it fails.

    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                // Handles board->GetTitleBlock() *and* board->GetProject()
                return m_editFrame->GetBoard()->ResolveTextVar( token, 0 );
            };

    wxString path = m_outputDirectory;
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, nullptr );

    wxFileName outputDir = wxFileName::DirName( path );
    wxString   boardFilename = m_editFrame->GetBoard()->GetFileName();
    REPORTER*  reporter = &m_messagesPanel->Reporter();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, reporter ) )
    {
        msg.Printf( _( "Could not write plot files to folder '%s'." ), outputDir.GetPath() );
        DisplayError( this, msg );
        return false;
    }

    wxFileName fn = m_editFrame->GetBoard()->GetFileName();
    fn.SetPath( outputDir.GetPath() );

    // Create the Front or Top side placement file, or a single file
    topSide = true;
    bottomSide = singleFile;

    fn.SetName( PLACE_FILE_EXPORTER::DecorateFilename( fn.GetName(), topSide, bottomSide ) );
    fn.SetExt( FILEEXT::FootprintPlaceFileExtension );

    if( useCSVfmt )
    {
        fn.SetName( fn.GetName() + wxT( "-" ) + FILEEXT::FootprintPlaceFileExtension );
        fn.SetExt( wxT( "csv" ) );
    }

    int fpcount = m_editFrame->DoGenFootprintsPositionFile( fn.GetFullPath(), UnitsMM(), OnlySMD(),
                                                            ExcludeAllTH(), ExcludeDNP(), ExcludeBOM(), topSide,
                                                            bottomSide, useCSVfmt, useAuxOrigin, negateBottomX );
    if( fpcount < 0 )
    {
        msg.Printf( _( "Failed to create file '%s'." ), fn.GetFullPath() );
        wxMessageBox( msg );
        reporter->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    if( singleFile  )
        msg.Printf( _( "Placement file: '%s'." ), fn.GetFullPath() );
    else
        msg.Printf( _( "Front (top side) placement file: '%s'." ), fn.GetFullPath() );

    reporter->Report( msg, RPT_SEVERITY_ACTION );

    msg.Printf( _( "Component count: %d." ), fpcount );
    reporter->Report( msg, RPT_SEVERITY_INFO );

    if( singleFile  )
    {
        reporter->Report( _( "Done." ), RPT_SEVERITY_INFO );
        return true;
    }

    // Create the Back or Bottom side placement file
    fullcount = fpcount;
    topSide = false;
    bottomSide = true;
    fn = brd->GetFileName();
    fn.SetPath( outputDir.GetPath() );
    fn.SetName( PLACE_FILE_EXPORTER::DecorateFilename( fn.GetName(), topSide, bottomSide ) );
    fn.SetExt( FILEEXT::FootprintPlaceFileExtension );

    if( useCSVfmt )
    {
        fn.SetName( fn.GetName() + wxT( "-" ) + FILEEXT::FootprintPlaceFileExtension );
        fn.SetExt( wxT( "csv" ) );
    }

    fpcount = m_editFrame->DoGenFootprintsPositionFile( fn.GetFullPath(), UnitsMM(), OnlySMD(),
                                                        ExcludeAllTH(), ExcludeDNP(), ExcludeBOM(), topSide,
                                                        bottomSide, useCSVfmt, useAuxOrigin, negateBottomX );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Failed to create file '%s'." ), fn.GetFullPath() );
        reporter->Report( msg, RPT_SEVERITY_ERROR );
        wxMessageBox( msg );
        return false;
    }

    // Display results
    if( !singleFile )
    {
        msg.Printf( _( "Back (bottom side) placement file: '%s'." ), fn.GetFullPath() );
        reporter->Report( msg, RPT_SEVERITY_ACTION );

        msg.Printf( _( "Component count: %d." ), fpcount );
        reporter->Report( msg, RPT_SEVERITY_INFO );
    }

    if( !singleFile )
    {
        fullcount += fpcount;
        msg.Printf( _( "Full component count: %d." ), fullcount );
        reporter->Report( msg, RPT_SEVERITY_INFO );
    }

    reporter->Report( _( "Done." ), RPT_SEVERITY_INFO );
    return true;
}


int BOARD_EDITOR_CONTROL::GeneratePosFile( const TOOL_EVENT& aEvent )
{
    DIALOG_GEN_FOOTPRINT_POSITION dlg( getEditFrame<PCB_EDIT_FRAME>() );
    dlg.ShowModal();
    return 0;
}


int PCB_EDIT_FRAME::DoGenFootprintsPositionFile( const wxString& aFullFileName, bool aUnitsMM,
                                                 bool aOnlySMD, bool aNoTHItems, bool aExcludeDNP,
                                                 bool aExcludeBOM, bool aTopSide, bool aBottomSide,
                                                 bool aFormatCSV, bool aUseAuxOrigin, bool aNegateBottomX )
{
    FILE * file = nullptr;

    if( !aFullFileName.IsEmpty() )
    {
        file = wxFopen( aFullFileName, wxT( "wt" ) );

        if( file == nullptr )
            return -1;
    }

    std::string data;
    PLACE_FILE_EXPORTER exporter( GetBoard(), aUnitsMM, aOnlySMD, aNoTHItems, aExcludeDNP,
                                  aExcludeBOM, aTopSide, aBottomSide, aFormatCSV, aUseAuxOrigin,
                                  aNegateBottomX );

    // Set the current variant for variant-aware DNP/BOM/position file filtering
    exporter.SetVariant( GetBoard()->GetCurrentVariant() );

    data = exporter.GenPositionData();

    // if aFullFileName is empty, the file is not created, only the
    // count of footprints to place is returned
    if( file )
    {
        // Creates a footprint position file
        // aSide = 0 -> Back (bottom) side)
        // aSide = 1 -> Front (top) side)
        // aSide = 2 -> both sides
        fputs( data.c_str(), file );
        fclose( file );
    }

    return exporter.GetFootprintCount();
}


int BOARD_EDITOR_CONTROL::GenFootprintsReport( const TOOL_EVENT& aEvent )
{
    BOARD*     board = m_frame->GetBoard();
    wxFileName fn;

    wxString    boardFilePath = ( (wxFileName) board->GetFileName() ).GetPath();
    wxDirDialog dirDialog( m_frame, _( "Select Output Directory" ), boardFilePath );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return 0;

    fn = board->GetFileName();
    fn.SetPath( dirDialog.GetPath() );
    fn.SetExt( wxT( "rpt" ) );

    FILE* rptfile = wxFopen( fn.GetFullPath(), wxT( "wt" ) );

    if( rptfile == nullptr )
    {
        wxMessageBox( wxString::Format( _( "Footprint report file created:\n'%s'." ), fn.GetFullPath() ),
                      _( "Footprint Report" ), wxICON_INFORMATION );

        return 0;
    }

    std::string data;
    PLACE_FILE_EXPORTER exporter( board, m_frame->GetUserUnits() == EDA_UNITS::MM,
                                  false,        // aOnlySMD
                                  false,        // aNoTHItems
                                  false,        // aExcludeDNP
                                  false,        // aExcludeBOM
                                  true, true,   // aTopSide, aBottomSide
                                  false,        // aFormatCSV
                                  true,         // aUseAuxOrigin
                                  false );      // aNegateBottomX

    // Set the current variant for variant-aware filtering
    exporter.SetVariant( board->GetCurrentVariant() );

    data = exporter.GenReportData();

    fputs( data.c_str(), rptfile );
    fclose( rptfile );

    wxMessageBox( wxString::Format( _( "Footprint report file created:\n'%s'." ), fn.GetFullPath() ),
                  _( "Footprint Report" ), wxICON_INFORMATION );

    return 0;
}
