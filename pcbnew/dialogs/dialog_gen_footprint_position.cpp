/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <string_utils.h>
#include <gestfich.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <bitmaps.h>
#include <reporter.h>
#include <tools/board_editor_control.h>
#include <board.h>
#include <wildcards_and_files_ext.h>
#include <kiface_base.h>
#include <widgets/wx_html_report_panel.h>
#include <widgets/std_bitmap_button.h>
#include <dialog_gen_footprint_position_file_base.h>
#include <exporters/place_file_exporter.h>
#include "gerber_placefile_writer.h"

#include <wx/dirdlg.h>


/**
 * The dialog to create footprint position files and choose options (one or 2 files, units
 * and force all SMD footprints in list)
 */
class DIALOG_GEN_FOOTPRINT_POSITION : public DIALOG_GEN_FOOTPRINT_POSITION_BASE
{
public:
    DIALOG_GEN_FOOTPRINT_POSITION( PCB_EDIT_FRAME * aParent ):
        DIALOG_GEN_FOOTPRINT_POSITION_BASE( aParent ),
        m_parent( aParent )
    {
        m_messagesPanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );
        m_reporter = &m_messagesPanel->Reporter();
        initDialog();

        SetupStandardButtons( { { wxID_OK,     _( "Generate Position File" ) },
                                { wxID_CANCEL, _( "Close" )                  } } );

        GetSizer()->SetSizeHints(this);
        Centre();
    }

private:
    void initDialog();
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;
    void OnGenerate( wxCommandEvent& event ) override;

    void onUpdateUIUnits( wxUpdateUIEvent& event ) override
    {
        m_radioBoxUnits->Enable( m_rbFormat->GetSelection() != 2 );
    }

    void onUpdateUIFileOpt( wxUpdateUIEvent& event ) override
    {
        m_radioBoxFilesCount->Enable( m_rbFormat->GetSelection() != 2 );
    }

    void onUpdateUIOnlySMD( wxUpdateUIEvent& event ) override
    {
        if( m_rbFormat->GetSelection() == 2 )
        {
            m_onlySMD->SetValue( false );
            m_onlySMD->Enable( false );
        }
        else
        {
            m_onlySMD->Enable( true );
        }
    }

    void onUpdateUInegXcoord( wxUpdateUIEvent& event ) override
    {
        if( m_rbFormat->GetSelection() == 2 )
        {
            m_negateXcb->SetValue( false );
            m_negateXcb->Enable( false );
        }
        else
        {
            m_negateXcb->Enable( true );
        }
    }

    void onUpdateUIExcludeTH( wxUpdateUIEvent& event ) override
    {
        if( m_rbFormat->GetSelection() == 2 )
        {
            m_excludeTH->SetValue( false );
            m_excludeTH->Enable( false );
        }
        else
        {
            m_excludeTH->Enable( true );
        }
    }

    void onUpdateUIincludeBoardEdge( wxUpdateUIEvent& event ) override
    {
        m_cbIncludeBoardEdge->Enable( m_rbFormat->GetSelection() == 2 );
    }

    /**
     * Creates files in text or csv format
     */
    bool CreateAsciiFiles();

    /**
     * Creates placement files in gerber format
     */
    bool CreateGerberFiles();

    // accessors to options:
    bool UnitsMM()
    {
        return m_radioBoxUnits->GetSelection() == 1;
    }

    bool OneFileOnly()
    {
        return m_radioBoxFilesCount->GetSelection() == 1;
    }

    bool OnlySMD()
    {
        return m_onlySMD->GetValue();
    }

    bool ExcludeAllTH()
    {
        return m_excludeTH->GetValue();
    }

private:
    PCB_EDIT_FRAME* m_parent;
    REPORTER*       m_reporter;
};



void DIALOG_GEN_FOOTPRINT_POSITION::initDialog()
{
    m_browseButton->SetBitmap( KiBitmap( BITMAPS::small_folder ) );

    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

    m_units = cfg->m_PlaceFile.units == 0 ? EDA_UNITS::INCHES : EDA_UNITS::MILLIMETRES;

    // Output directory
    m_outputDirectoryName->SetValue( cfg->m_PlaceFile.output_directory );

    // Update Options
    m_radioBoxUnits->SetSelection( cfg->m_PlaceFile.units );
    m_radioBoxFilesCount->SetSelection( cfg->m_PlaceFile.file_options );
    m_rbFormat->SetSelection( cfg->m_PlaceFile.file_format );
    m_cbIncludeBoardEdge->SetValue( cfg->m_PlaceFile.include_board_edge );
    m_useDrillPlaceOrigin->SetValue( cfg->m_PlaceFile.use_aux_origin );
    m_onlySMD->SetValue( cfg->m_PlaceFile.only_SMD );
    m_negateXcb->SetValue( cfg->m_PlaceFile.negate_xcoord );
    m_excludeTH->SetValue( cfg->m_PlaceFile.exclude_TH );

    // Update sizes and sizers:
    m_messagesPanel->MsgPanelSetMinSize( wxSize( -1, 160 ) );
    GetSizer()->SetSizeHints( this );
}

void DIALOG_GEN_FOOTPRINT_POSITION::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
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
        wxString boardFilePath = ( (wxFileName) m_parent->GetBoard()->GetFileName() ).GetPath();

        if( !dirName.MakeRelativeTo( boardFilePath ) )
            wxMessageBox( _( "Cannot make path relative (target volume different from board "
                             "file volume)!" ),
                          _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


void DIALOG_GEN_FOOTPRINT_POSITION::OnGenerate( wxCommandEvent& event )
{
    m_units  = m_radioBoxUnits->GetSelection() == 0 ? EDA_UNITS::INCHES : EDA_UNITS::MILLIMETRES;

    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

    wxString dirStr = m_outputDirectoryName->GetValue();
    // Keep unix directory format convention in cfg files
    dirStr.Replace( wxT( "\\" ), wxT( "/" ) );

    cfg->m_PlaceFile.output_directory   = dirStr;

    cfg->m_PlaceFile.units              = m_units == EDA_UNITS::INCHES ? 0 : 1;
    cfg->m_PlaceFile.file_options       = m_radioBoxFilesCount->GetSelection();
    cfg->m_PlaceFile.file_format        = m_rbFormat->GetSelection();
    cfg->m_PlaceFile.include_board_edge = m_cbIncludeBoardEdge->GetValue();
    cfg->m_PlaceFile.exclude_TH         = m_excludeTH->GetValue();
    cfg->m_PlaceFile.only_SMD           = m_onlySMD->GetValue();
    cfg->m_PlaceFile.use_aux_origin     = m_useDrillPlaceOrigin->GetValue();
    cfg->m_PlaceFile.negate_xcoord      = m_negateXcb->GetValue();

    if( m_rbFormat->GetSelection() == 2 )
        CreateGerberFiles();
    else
        CreateAsciiFiles();
}


bool DIALOG_GEN_FOOTPRINT_POSITION::CreateGerberFiles()
{
    BOARD*     brd = m_parent->GetBoard();
    wxFileName fn;
    wxString   msg;
    int        fullcount = 0;

    // Create output directory if it does not exist (also transform it in absolute form).
    // Bail if it fails.

    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                // Handles board->GetTitleBlock() *and* board->GetProject()
                return m_parent->GetBoard()->ResolveTextVar( token, 0 );
            };

    wxString path = m_parent->GetPcbNewSettings()->m_PlaceFile.output_directory;
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, nullptr );

    wxFileName  outputDir = wxFileName::DirName( path );
    wxString    boardFilename = m_parent->GetBoard()->GetFileName();

    m_reporter = &m_messagesPanel->Reporter();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, m_reporter ) )
    {
        msg.Printf( _( "Could not write plot files to folder '%s'." ), outputDir.GetPath() );
        DisplayError( this, msg );
        return false;
    }

    fn = m_parent->GetBoard()->GetFileName();
    fn.SetPath( outputDir.GetPath() );

    // Create the Front and Top side placement files. Gerber P&P files are always separated.
    // Not also they include all footprints
    PLACEFILE_GERBER_WRITER exporter( brd );
    wxString filename = exporter.GetPlaceFileName( fn.GetFullPath(), F_Cu );

    int fpcount = exporter.CreatePlaceFile( filename, F_Cu, m_cbIncludeBoardEdge->GetValue() );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Failed to create file '%s'." ), fn.GetFullPath() );
        wxMessageBox( msg );
        m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    msg.Printf( _( "Front (top side) placement file: '%s'." ), filename );
    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    msg.Printf( _( "Component count: %d." ), fpcount );
    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    // Create the Back or Bottom side placement file
    fullcount = fpcount;

    filename = exporter.GetPlaceFileName( fn.GetFullPath(), B_Cu );

    fpcount = exporter.CreatePlaceFile( filename, B_Cu, m_cbIncludeBoardEdge->GetValue() );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Failed to create file '%s'." ), filename );
        m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        wxMessageBox( msg );
        return false;
    }

    // Display results
    msg.Printf( _( "Back (bottom side) placement file: '%s'." ), filename );
    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    msg.Printf( _( "Component count: %d." ), fpcount );
    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    fullcount += fpcount;
    msg.Printf( _( "Full component count: %d." ), fullcount );
    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    m_reporter->Report( _( "File generation successful." ), RPT_SEVERITY_INFO );

    return true;
}


bool DIALOG_GEN_FOOTPRINT_POSITION::CreateAsciiFiles()
{
    BOARD *    brd = m_parent->GetBoard();
    wxFileName fn;
    wxString   msg;
    bool       singleFile = OneFileOnly();
    bool       useCSVfmt = m_rbFormat->GetSelection() == 1;
    bool       useAuxOrigin = m_useDrillPlaceOrigin->GetValue();
    int        fullcount = 0;
    int        topSide = true;
    int        bottomSide = true;
    bool       negateBottomX = m_negateXcb->GetValue();

    // Test for any footprint candidate in list.
    {
        PLACE_FILE_EXPORTER exporter( brd, UnitsMM(), OnlySMD(), ExcludeAllTH(), topSide,
                                      bottomSide, useCSVfmt, useAuxOrigin, negateBottomX );
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
                return m_parent->GetBoard()->ResolveTextVar( token, 0 );
            };

    wxString path = m_parent->GetPcbNewSettings()->m_PlaceFile.output_directory;
    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, nullptr );

    wxFileName  outputDir = wxFileName::DirName( path );
    wxString    boardFilename = m_parent->GetBoard()->GetFileName();

    m_reporter = &m_messagesPanel->Reporter();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, m_reporter ) )
    {
        msg.Printf( _( "Could not write plot files to folder '%s'." ), outputDir.GetPath() );
        DisplayError( this, msg );
        return false;
    }

    fn = m_parent->GetBoard()->GetFileName();
    fn.SetPath( outputDir.GetPath() );

    // Create the Front or Top side placement file, or a single file
    topSide = true;
    bottomSide = false;

    if( singleFile )
    {
        bottomSide = true;
        fn.SetName( fn.GetName() + wxT( "-" ) + wxT( "all" ) );
    }
    else
    {
        fn.SetName( fn.GetName() + wxT( "-" ) + PLACE_FILE_EXPORTER::GetFrontSideName().c_str() );
    }


    if( useCSVfmt )
    {
        fn.SetName( fn.GetName() + wxT( "-" ) + FootprintPlaceFileExtension );
        fn.SetExt( wxT( "csv" ) );
    }
    else
    {
        fn.SetExt( FootprintPlaceFileExtension );
    }

    int fpcount = m_parent->DoGenFootprintsPositionFile( fn.GetFullPath(), UnitsMM(), OnlySMD(),
                                                         ExcludeAllTH(), topSide, bottomSide,
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

    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    msg.Printf( _( "Component count: %d." ), fpcount );
    m_reporter->Report( msg, RPT_SEVERITY_INFO );

    if( singleFile  )
    {
        m_reporter->Report( _( "File generation successful." ), RPT_SEVERITY_INFO );
        return true;
    }

    // Create the Back or Bottom side placement file
    fullcount = fpcount;
    topSide = false;
    bottomSide = true;
    fn = brd->GetFileName();
    fn.SetPath( outputDir.GetPath() );
    fn.SetName( fn.GetName() + wxT( "-" ) + PLACE_FILE_EXPORTER::GetBackSideName().c_str() );

    if( useCSVfmt )
    {
        fn.SetName( fn.GetName() + wxT( "-" ) + FootprintPlaceFileExtension );
        fn.SetExt( wxT( "csv" ) );
    }
    else
    {
        fn.SetExt( FootprintPlaceFileExtension );
    }

    fpcount = m_parent->DoGenFootprintsPositionFile( fn.GetFullPath(), UnitsMM(), OnlySMD(),
                                                     ExcludeAllTH(), topSide, bottomSide, useCSVfmt,
                                                     useAuxOrigin, negateBottomX );

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
        m_reporter->Report( msg, RPT_SEVERITY_INFO );

        msg.Printf( _( "Component count: %d." ), fpcount );
        m_reporter->Report( msg, RPT_SEVERITY_INFO );
    }

    if( !singleFile )
    {
        fullcount += fpcount;
        msg.Printf( _( "Full component count: %d." ), fullcount );
        m_reporter->Report( msg, RPT_SEVERITY_INFO );
    }

    m_reporter->Report( _( "File generation successful." ), RPT_SEVERITY_INFO );

    return true;
}


int BOARD_EDITOR_CONTROL::GeneratePosFile( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    DIALOG_GEN_FOOTPRINT_POSITION dlg( editFrame );

    dlg.ShowModal();
    return 0;
}


int PCB_EDIT_FRAME::DoGenFootprintsPositionFile( const wxString& aFullFileName, bool aUnitsMM,
                                                 bool aOnlySMD, bool aNoTHItems, bool aTopSide,
                                                 bool aBottomSide, bool aFormatCSV,
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
    PLACE_FILE_EXPORTER exporter( GetBoard(), aUnitsMM, aOnlySMD, aNoTHItems, aTopSide, aBottomSide,
                                  aFormatCSV, aUseAuxOrigin, aNegateBottomX );
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

    wxString boardFilePath = ( (wxFileName) GetBoard()->GetFileName() ).GetPath();
    wxDirDialog dirDialog( this, _( "Select Output Directory" ), boardFilePath );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    fn = GetBoard()->GetFileName();
    fn.SetPath( dirDialog.GetPath() );
    fn.SetExt( wxT( "rpt" ) );

    bool unitMM = GetUserUnits() == EDA_UNITS::MILLIMETRES;
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
                                  true, true,           // aTopSide, aBottomSide
                                  false, true, false    // aFormatCSV, aUseAuxOrigin, aNegateBottomX
                                );
    data = exporter.GenReportData();

    fputs( data.c_str(), rptfile );
    fclose( rptfile );

    return true;
}
