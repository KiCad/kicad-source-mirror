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

#include <dialog_gen_footprint_position.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <project/project_file.h>
#include <bitmaps.h>
#include <reporter.h>
#include <tools/board_editor_control.h>
#include <wildcards_and_files_ext.h>
#include <kiface_base.h>
#include <widgets/wx_html_report_panel.h>
#include <widgets/std_bitmap_button.h>
#include <exporters/place_file_exporter.h>
#include "gerber_placefile_writer.h"
#include <jobs/job_export_pcb_pos.h>

#include <wx/dirdlg.h>
#include <wx/msgdlg.h>


DIALOG_GEN_FOOTPRINT_POSITION::DIALOG_GEN_FOOTPRINT_POSITION( PCB_EDIT_FRAME* aEditFrame ) :
        DIALOG_GEN_FOOTPRINT_POSITION_BASE( aEditFrame ),
        m_editFrame( aEditFrame ),
        m_job( nullptr )
{
    m_messagesPanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );
    m_reporter = &m_messagesPanel->Reporter();
    initDialog();

    SetupStandardButtons( { { wxID_OK, _( "Generate Position File" ) },
                            { wxID_CANCEL, _( "Close" ) } } );

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
    m_messagesPanel->Hide();
    initDialog();

    SetupStandardButtons();

    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_GEN_FOOTPRINT_POSITION::initDialog()
{
    if( !m_job )
    {
        m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

        PROJECT_FILE&    projectFile = m_editFrame->Prj().GetProjectFile();
        PCBNEW_SETTINGS* cfg = m_editFrame->GetPcbNewSettings();

        m_units = cfg->m_PlaceFile.units == 0 ? EDA_UNITS::INCH : EDA_UNITS::MM;

        // Output directory
        m_outputDirectoryName->SetValue( projectFile.m_PcbLastPath[LAST_PATH_POS_FILES] );

        // Update Options
        m_unitsCtrl->SetSelection( cfg->m_PlaceFile.units );
        m_singleFile->SetValue( cfg->m_PlaceFile.file_options == 1 );
        m_formatCtrl->SetSelection( cfg->m_PlaceFile.file_format );
        m_cbIncludeBoardEdge->SetValue( cfg->m_PlaceFile.include_board_edge );
        m_useDrillPlaceOrigin->SetValue( cfg->m_PlaceFile.use_aux_origin );
        m_onlySMD->SetValue( cfg->m_PlaceFile.only_SMD );
        m_negateXcb->SetValue( cfg->m_PlaceFile.negate_xcoord );
        m_excludeTH->SetValue( cfg->m_PlaceFile.exclude_TH );

        // Update sizes and sizers:
        m_messagesPanel->MsgPanelSetMinSize( wxSize( -1, 160 ) );
    }
    else
    {
        SetTitle( m_job->GetSettingsDialogTitle() );

        m_browseButton->Hide();
        m_units = m_job->m_units == JOB_EXPORT_PCB_POS::UNITS::INCH ? EDA_UNITS::INCH : EDA_UNITS::MM;
        m_staticTextDir->SetLabel( _( "Output file:" ) );
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

        m_messagesPanel->Hide();
    }

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions (which have different sizes).
    m_hash_key = TO_UTF8( GetTitle() );

    GetSizer()->SetSizeHints( this );
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
        m_excludeTH->SetValue( false );
        m_excludeTH->Enable( false );
    }
    else
    {
        m_excludeTH->Enable( true );
    }
}


bool DIALOG_GEN_FOOTPRINT_POSITION::UnitsMM()
{
    return m_unitsCtrl->GetSelection() == 1;
}


bool DIALOG_GEN_FOOTPRINT_POSITION::OneFileOnly()
{
    return m_singleFile->GetValue();
}


bool DIALOG_GEN_FOOTPRINT_POSITION::OnlySMD()
{
    return m_onlySMD->GetValue();
}


bool DIALOG_GEN_FOOTPRINT_POSITION::ExcludeAllTH()
{
    return m_excludeTH->GetValue();
}


bool DIALOG_GEN_FOOTPRINT_POSITION::ExcludeDNP()
{
    return m_excludeDNP->GetValue();
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

    wxMessageDialog dialog( this, _( "Use a relative path?"), _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    if( dialog.ShowModal() == wxID_YES )
    {
        wxString boardFilePath = ( (wxFileName) m_editFrame->GetBoard()->GetFileName() ).GetPath();

        if( !dirName.MakeRelativeTo( boardFilePath ) )
        {
            wxMessageBox( _( "Cannot make path relative (target volume different from board "
                             "file volume)!" ),
                          _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
        }
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


void DIALOG_GEN_FOOTPRINT_POSITION::onGenerate( wxCommandEvent& event )
{
    if( !m_job )
    {
        m_units  = m_unitsCtrl->GetSelection() == 0 ? EDA_UNITS::INCH : EDA_UNITS::MM;

        PCBNEW_SETTINGS* cfg = m_editFrame->GetPcbNewSettings();

        wxString dirStr = m_outputDirectoryName->GetValue();
        // Keep unix directory format convention in cfg files
        dirStr.Replace( wxT( "\\" ), wxT( "/" ) );

        m_editFrame->Prj().GetProjectFile().m_PcbLastPath[LAST_PATH_POS_FILES] = dirStr;
        cfg->m_PlaceFile.output_directory   = dirStr;
        cfg->m_PlaceFile.units              = m_units == EDA_UNITS::INCH ? 0 : 1;
        cfg->m_PlaceFile.file_options       = m_singleFile->GetValue() ? 1 : 0;
        cfg->m_PlaceFile.file_format        = m_formatCtrl->GetSelection();
        cfg->m_PlaceFile.include_board_edge = m_cbIncludeBoardEdge->GetValue();
        cfg->m_PlaceFile.exclude_TH         = m_excludeTH->GetValue();
        cfg->m_PlaceFile.only_SMD           = m_onlySMD->GetValue();
        cfg->m_PlaceFile.use_aux_origin     = m_useDrillPlaceOrigin->GetValue();
        cfg->m_PlaceFile.negate_xcoord      = m_negateXcb->GetValue();

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

    wxString path = m_editFrame->GetPcbNewSettings()->m_PlaceFile.output_directory;
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, nullptr );

    wxFileName  outputDir = wxFileName::DirName( path );
    wxString   boardFilename = m_editFrame->GetBoard()->GetFileName();

    m_reporter = &m_messagesPanel->Reporter();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, m_reporter ) )
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
    wxString                filename = exporter.GetPlaceFileName( fn.GetFullPath(), F_Cu );

    int fpcount = exporter.CreatePlaceFile( filename, F_Cu, m_cbIncludeBoardEdge->GetValue(),
                                            m_excludeDNP->GetValue() );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Failed to create file '%s'." ), fn.GetFullPath() );
        wxMessageBox( msg );
        m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    msg.Printf( _( "Front (top side) placement file: '%s'." ), filename );
    m_reporter->Report( msg, RPT_SEVERITY_ACTION );

    msg.Printf( _( "Component count: %d." ), fpcount );
    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    // Create the Back or Bottom side placement file
    fullcount = fpcount;

    filename = exporter.GetPlaceFileName( fn.GetFullPath(), B_Cu );

    fpcount = exporter.CreatePlaceFile( filename, B_Cu, m_cbIncludeBoardEdge->GetValue(),
                                        m_excludeDNP->GetValue() );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Failed to create file '%s'." ), filename );
        m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        wxMessageBox( msg );
        return false;
    }

    // Display results
    msg.Printf( _( "Back (bottom side) placement file: '%s'." ), filename );
    m_reporter->Report( msg, RPT_SEVERITY_ACTION );

    msg.Printf( _( "Component count: %d." ), fpcount );
    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    fullcount += fpcount;
    msg.Printf( _( "Full component count: %d." ), fullcount );
    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    m_reporter->Report( _( "Done." ), RPT_SEVERITY_INFO );

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
                                      topSide, bottomSide, useCSVfmt, useAuxOrigin, negateBottomX );
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

    wxString path = m_editFrame->GetPcbNewSettings()->m_PlaceFile.output_directory;
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, nullptr );

    wxFileName outputDir = wxFileName::DirName( path );
    wxString   boardFilename = m_editFrame->GetBoard()->GetFileName();

    m_reporter = &m_messagesPanel->Reporter();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, m_reporter ) )
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
                                                            ExcludeAllTH(), ExcludeDNP(), topSide, bottomSide,
                                                            useCSVfmt, useAuxOrigin, negateBottomX );
    if( fpcount < 0 )
    {
        msg.Printf( _( "Failed to create file '%s'." ), fn.GetFullPath() );
        wxMessageBox( msg );
        m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    if( singleFile  )
        msg.Printf( _( "Placement file: '%s'." ), fn.GetFullPath() );
    else
        msg.Printf( _( "Front (top side) placement file: '%s'." ), fn.GetFullPath() );

    m_reporter->Report( msg, RPT_SEVERITY_ACTION );

    msg.Printf( _( "Component count: %d." ), fpcount );
    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    if( singleFile  )
    {
        m_reporter->Report( _( "Done." ), RPT_SEVERITY_INFO );
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
                                                        ExcludeAllTH(), ExcludeDNP(), topSide, bottomSide,
                                                        useCSVfmt, useAuxOrigin, negateBottomX );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Failed to create file '%s'." ), fn.GetFullPath() );
        m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        wxMessageBox( msg );
        return false;
    }

    // Display results
    if( !singleFile )
    {
        msg.Printf( _( "Back (bottom side) placement file: '%s'." ), fn.GetFullPath() );
        m_reporter->Report( msg, RPT_SEVERITY_ACTION );

        msg.Printf( _( "Component count: %d." ), fpcount );
        m_reporter->Report( msg, RPT_SEVERITY_INFO );
    }

    if( !singleFile )
    {
        fullcount += fpcount;
        msg.Printf( _( "Full component count: %d." ), fullcount );
        m_reporter->Report( msg, RPT_SEVERITY_INFO );
    }

    m_reporter->Report( _( "Done." ), RPT_SEVERITY_INFO );
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
                                                 bool aTopSide, bool aBottomSide, bool aFormatCSV,
                                                 bool aUseAuxOrigin, bool aNegateBottomX )
{
    FILE * file = nullptr;

    if( !aFullFileName.IsEmpty() )
    {
        file = wxFopen( aFullFileName, wxT( "wt" ) );

        if( file == nullptr )
            return -1;
    }

    std::string data;
    PLACE_FILE_EXPORTER exporter( GetBoard(), aUnitsMM, aOnlySMD, aNoTHItems, aExcludeDNP, aTopSide,
                                  aBottomSide, aFormatCSV, aUseAuxOrigin, aNegateBottomX );
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


void PCB_EDIT_FRAME::GenFootprintsReport( wxCommandEvent& event )
{
    wxFileName fn;

    wxString    boardFilePath = ( (wxFileName) GetBoard()->GetFileName() ).GetPath();
    wxDirDialog dirDialog( this, _( "Select Output Directory" ), boardFilePath );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    fn = GetBoard()->GetFileName();
    fn.SetPath( dirDialog.GetPath() );
    fn.SetExt( wxT( "rpt" ) );

    bool unitMM = GetUserUnits() == EDA_UNITS::MM;
    bool success = DoGenFootprintsReport( fn.GetFullPath(), unitMM );

    wxString msg;

    if( success )
    {
        msg.Printf( _( "Footprint report file created:\n'%s'." ), fn.GetFullPath() );
        wxMessageBox( msg, _( "Footprint Report" ), wxICON_INFORMATION );
    }
    else
    {
        msg.Printf( _( "Failed to create file '%s'." ), fn.GetFullPath() );
        DisplayError( this, msg );
    }
}


bool PCB_EDIT_FRAME::DoGenFootprintsReport( const wxString& aFullFilename, bool aUnitsMM )
{
    FILE* rptfile = wxFopen( aFullFilename, wxT( "wt" ) );

    if( rptfile == nullptr )
        return false;

    std::string data;
    PLACE_FILE_EXPORTER exporter( GetBoard(), aUnitsMM,
                                  false, false,         // SMD aOnlySMD, aNoTHItems
                                  false,                // aExcludeDNP
                                  true, true,           // aTopSide, aBottomSide
                                  false, true, false    // aFormatCSV, aUseAuxOrigin, aNegateBottomX
                                );
    data = exporter.GenReportData();

    fputs( data.c_str(), rptfile );
    fclose( rptfile );

    return true;
}
