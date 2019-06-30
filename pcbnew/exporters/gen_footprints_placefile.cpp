/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 *  1 - create ascii files for automatic placement of smd components
 *  2 - create a module report (pos and module descr) (ascii file)
 */

#include <fctsys.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <pcb_edit_frame.h>
#include <pgm_base.h>
#include <bitmaps.h>
#include <build_version.h>
#include <macros.h>
#include <reporter.h>
#include <tools/pcb_editor_control.h>
#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>
#include <wildcards_and_files_ext.h>
#include <kiface_i.h>
#include <wx_html_report_panel.h>
#include <dialog_gen_footprint_position_file_base.h>
#include <export_footprints_placefile.h>


#define PLACEFILE_UNITS_KEY  wxT( "PlaceFileUnits" )
#define PLACEFILE_OPT_KEY    wxT( "PlaceFileOpts" )
#define PLACEFILE_FORMAT_KEY wxT( "PlaceFileFormat" )


/**
 * The dialog to create footprint position files and choose options (one or 2 files, units
 * and force all SMD footprints in list)
 */
class DIALOG_GEN_FOOTPRINT_POSITION : public DIALOG_GEN_FOOTPRINT_POSITION_BASE
{
public:
    DIALOG_GEN_FOOTPRINT_POSITION( PCB_EDIT_FRAME * aParent ):
        DIALOG_GEN_FOOTPRINT_POSITION_BASE( aParent ),
        m_parent( aParent ),
        m_plotOpts( aParent->GetPlotSettings() )
    {
        m_reporter = &m_messagesPanel->Reporter();
        initDialog();

        // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
        // that requires us to correct the button labels here.
        m_sdbSizerOK->SetLabel( _( "Generate Position File" ) );
        m_sdbSizerCancel->SetLabel( _( "Close" ) );
        m_sdbSizer->Layout();

        m_sdbSizerOK->SetDefault();

        GetSizer()->SetSizeHints(this);
        Centre();
    }

private:
    PCB_EDIT_FRAME* m_parent;
    PCB_PLOT_PARAMS m_plotOpts;
    wxConfigBase* m_config;
    REPORTER* m_reporter;

    static int m_unitsOpt;
    static int m_fileOpt;
    static int m_fileFormat;

    void initDialog();
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;
    void OnGenerate( wxCommandEvent& event ) override;

    bool CreateFiles();

    // accessors to options:
    wxString GetOutputDirectory()
    {
        return m_outputDirectoryName->GetValue();
    }

    bool UnitsMM()
    {
        return m_radioBoxUnits->GetSelection() == 1;
    }

    bool OneFileOnly()
    {
        return m_radioBoxFilesCount->GetSelection() == 1;
    }

    bool ForceAllSmd()
    {
        return m_forceSMDOpt->GetValue();
    }
};


// Static members to remember choices
int DIALOG_GEN_FOOTPRINT_POSITION::m_unitsOpt = 0;
int DIALOG_GEN_FOOTPRINT_POSITION::m_fileOpt = 0;
int DIALOG_GEN_FOOTPRINT_POSITION::m_fileFormat = 0;


void DIALOG_GEN_FOOTPRINT_POSITION::initDialog()
{
    m_browseButton->SetBitmap( KiBitmap( folder_xpm ) );

    m_config = Kiface().KifaceSettings();
    m_config->Read( PLACEFILE_UNITS_KEY, &m_unitsOpt, 1 );
    m_config->Read( PLACEFILE_OPT_KEY, &m_fileOpt, 0 );
    m_config->Read( PLACEFILE_FORMAT_KEY, &m_fileFormat, 0 );

    // Output directory
    m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );
    m_radioBoxUnits->SetSelection( m_unitsOpt );
    m_radioBoxFilesCount->SetSelection( m_fileOpt );
    m_rbFormat->SetSelection( m_fileFormat );

    // Update sizes and sizers:
    m_messagesPanel->MsgPanelSetMinSize( wxSize( -1, 160 ) );
    GetSizer()->SetSizeHints( this );
}

void DIALOG_GEN_FOOTPRINT_POSITION::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxString    path = Prj().AbsolutePath( m_outputDirectoryName->GetValue() );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxMessageDialog dialog( this, _( "Use a relative path?"),
                            _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    if( dialog.ShowModal() == wxID_YES )
    {
        wxString boardFilePath = ( (wxFileName) m_parent->GetBoard()->GetFileName()).GetPath();

        if( !dirName.MakeRelativeTo( boardFilePath ) )
            wxMessageBox( _( "Cannot make path relative (target volume different from board file volume)!" ),
                          _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}

void DIALOG_GEN_FOOTPRINT_POSITION::OnGenerate( wxCommandEvent& event )
{
    m_unitsOpt = m_radioBoxUnits->GetSelection();
    m_fileOpt = m_radioBoxFilesCount->GetSelection();
    m_fileFormat = m_rbFormat->GetSelection();


    m_config->Write( PLACEFILE_UNITS_KEY, m_unitsOpt );
    m_config->Write( PLACEFILE_OPT_KEY, m_fileOpt );
    m_config->Write( PLACEFILE_FORMAT_KEY, m_fileFormat );

    // Set output directory and replace backslashes with forward ones
    // (Keep unix convention in cfg files)
    wxString dirStr;
    dirStr = m_outputDirectoryName->GetValue();
    dirStr.Replace( wxT( "\\" ), wxT( "/" ) );

    m_plotOpts.SetOutputDirectory( dirStr );
    m_parent->SetPlotSettings( m_plotOpts );

    CreateFiles();
}


bool DIALOG_GEN_FOOTPRINT_POSITION::CreateFiles()
{
    BOARD * brd = m_parent->GetBoard();
    wxFileName  fn;
    wxString    msg;
    bool singleFile = OneFileOnly();
    bool useCSVfmt = m_fileFormat == 1;
    int fullcount = 0;
    int top_side = true;
    int bottom_side = true;

    // Test for any footprint candidate in list, and display the list of forced footprints
    // if ForceAllSmd() is true
    {
        PLACE_FILE_EXPORTER exporter( brd, UnitsMM(), ForceAllSmd(), top_side, bottom_side, useCSVfmt );
        exporter.GenPositionData();

        if( exporter.GetFootprintCount() == 0)
        {
            wxMessageBox( _( "No footprint for automated placement." ) );
            return false;
        }

        if( ForceAllSmd() )
        {
            std::vector<MODULE*>& fp_no_smd_list = exporter.GetSmdFootprintsNotLabeledSMD();

            for( MODULE* item : fp_no_smd_list )
            {
                msg.Printf( _( "footprint %s (not set as SMD) forced in list" ), item->GetReference() );
                m_reporter->Report( msg, REPORTER::RPT_INFO );
            }
        }
    }

    // Create output directory if it does not exist.
    // Also transform it in absolute path.
    // Bail if it fails
    wxFileName  outputDir = wxFileName::DirName( m_plotOpts.GetOutputDirectory() );
    wxString    boardFilename = m_parent->GetBoard()->GetFileName();

    m_reporter = &m_messagesPanel->Reporter();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, m_reporter ) )
    {
        msg.Printf( _( "Could not write plot files to folder \"%s\"." ),
                    GetChars( outputDir.GetPath() ) );
        DisplayError( this, msg );
        return false;
    }

    fn = m_parent->GetBoard()->GetFileName();
    fn.SetPath( outputDir.GetPath() );

    // Create the the Front or Top side placement file, or a single file
    top_side = true;
    bottom_side = false;

    if( singleFile )
    {
        bottom_side = true;
        fn.SetName( fn.GetName() + wxT( "-" ) + wxT("all") );
    }
    else
        fn.SetName( fn.GetName() + wxT( "-" ) + PLACE_FILE_EXPORTER::GetFrontSideName().c_str() );


    if( useCSVfmt )
    {
        fn.SetName( fn.GetName() + wxT( "-" ) + FootprintPlaceFileExtension );
        fn.SetExt( wxT( "csv" ) );
    }
    else
        fn.SetExt( FootprintPlaceFileExtension );

    int fpcount = m_parent->DoGenFootprintsPositionFile( fn.GetFullPath(), UnitsMM(),
                                                         ForceAllSmd(),
                                                         top_side, bottom_side, useCSVfmt );
    if( fpcount < 0 )
    {
        msg.Printf( _( "Unable to create \"%s\"." ), fn.GetFullPath() );
        wxMessageBox( msg );
        m_reporter->Report( msg, REPORTER::RPT_ERROR );
        return false;
    }

    if( singleFile  )
        msg.Printf( _( "Place file: \"%s\"." ), fn.GetFullPath() );
    else
        msg.Printf( _( "Front side (top side) place file: \"%s\"." ),
                    fn.GetFullPath() );
    m_reporter->Report( msg, REPORTER::RPT_INFO );

    msg.Printf( _( "Component count: %d." ), fpcount );
    m_reporter->Report( msg, REPORTER::RPT_INFO );

    if( singleFile  )
    {
        m_reporter->Report( _( "Component Placement File generation OK." ), REPORTER::RPT_ACTION );
        return true;
    }

    // Create the Back or Bottom side placement file
    fullcount = fpcount;
    top_side = false;
    bottom_side = true;
    fn = brd->GetFileName();
    fn.SetPath( outputDir.GetPath() );
    fn.SetName( fn.GetName() + wxT( "-" ) + PLACE_FILE_EXPORTER::GetBackSideName().c_str() );

    if( useCSVfmt )
    {
        fn.SetName( fn.GetName() + wxT( "-" ) + FootprintPlaceFileExtension );
        fn.SetExt( wxT( "csv" ) );
    }
    else
        fn.SetExt( FootprintPlaceFileExtension );

    fpcount = m_parent->DoGenFootprintsPositionFile( fn.GetFullPath(), UnitsMM(),
                                                    ForceAllSmd(), top_side, bottom_side, useCSVfmt );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Unable to create file \"%s\"." ), fn.GetFullPath() );
        m_reporter->Report( msg, REPORTER::RPT_ERROR );
        wxMessageBox( msg );
        return false;
    }

    // Display results
    if( !singleFile )
    {
        msg.Printf( _( "Back side (bottom side) place file: \"%s\"." ), fn.GetFullPath() );
        m_reporter->Report( msg, REPORTER::RPT_INFO );

        msg.Printf( _( "Component count: %d." ), fpcount );

        m_reporter->Report( msg, REPORTER::RPT_INFO );
    }

    if( !singleFile )
    {
        fullcount += fpcount;
        msg.Printf( _( "Full component count: %d\n" ), fullcount );
        m_reporter->Report( msg, REPORTER::RPT_INFO );
    }

    m_reporter->Report( _( "Component Placement File generation OK." ), REPORTER::RPT_ACTION );

    return true;
}


int PCB_EDITOR_CONTROL::GeneratePosFile( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    DIALOG_GEN_FOOTPRINT_POSITION dlg( editFrame );

    dlg.ShowModal();
    return 0;
}


int PCB_EDIT_FRAME::DoGenFootprintsPositionFile( const wxString& aFullFileName, bool aUnitsMM,
                                                 bool aForceSmdItems, bool aTopSide, bool BottomSide, bool aFormatCSV )
{
    FILE * file = NULL;

    if( !aFullFileName.IsEmpty() )
    {
        file = wxFopen( aFullFileName, wxT( "wt" ) );

        if( file == NULL )
            return -1;
    }

    std::string data;
    PLACE_FILE_EXPORTER exporter( GetBoard(), aUnitsMM, aForceSmdItems,
                                  aTopSide, BottomSide, aFormatCSV );
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

    wxString boardFilePath = ( (wxFileName) GetBoard()->GetFileName()).GetPath();
    wxDirDialog dirDialog( this, _( "Select Output Directory" ), boardFilePath );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    fn = GetBoard()->GetFileName();
    fn.SetPath( dirDialog.GetPath() );
    fn.SetExt( wxT( "rpt" ) );

    bool unitMM = GetUserUnits() != INCHES;
    bool success = DoGenFootprintsReport( fn.GetFullPath(), unitMM );

    wxString msg;
    if( success )
    {
        msg.Printf( _( "Footprint report file created:\n\"%s\"" ), fn.GetFullPath() );
        wxMessageBox( msg, _( "Footprint Report" ), wxICON_INFORMATION );
    }

    else
    {
        msg.Printf( _( "Unable to create \"%s\"" ), fn.GetFullPath() );
        DisplayError( this, msg );
    }
}

/* Print a module report.
 */
bool PCB_EDIT_FRAME::DoGenFootprintsReport( const wxString& aFullFilename, bool aUnitsMM )
{
    wxString msg;
    FILE*    rptfile;
    wxPoint  module_pos;

    rptfile = wxFopen( aFullFilename, wxT( "wt" ) );

    if( rptfile == NULL )
        return false;

    std::string data;
    PLACE_FILE_EXPORTER exporter ( GetBoard(), aUnitsMM, false, true, true, false );
    data = exporter.GenReportData();

    fputs( data.c_str(), rptfile );
    fclose( rptfile );

    return true;
}
