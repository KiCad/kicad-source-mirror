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

#include <confirm.h>
#include <core/arraydim.h>
#include <widgets/std_bitmap_button.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pcbplot.h>
#include <gendrill_Excellon_writer.h>
#include <gendrill_gerber_writer.h>
#include <bitmaps.h>
#include <tools/board_editor_control.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <paths.h>
#include <dialog_gendrill.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>
#include <wx/msgdlg.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <jobs/job_export_pcb_drill.h>

// list of allowed precision for EXCELLON files, for integer format:
// Due to difference between inches and mm,
// there are 2 precision values, one for inches and one for metric
// Note: for decimla format, the precision is not used
static DRILL_PRECISION precisionListForInches( 2, 4 );
static DRILL_PRECISION precisionListForMetric( 3, 3 );


// Static members of DIALOG_GENDRILL
int  DIALOG_GENDRILL::g_unitDrillIsInch  = false;     // Only for Excellon format
int  DIALOG_GENDRILL::g_zerosFormat      = EXCELLON_WRITER::DECIMAL_FORMAT;
bool DIALOG_GENDRILL::g_minimalHeader    = false;    // Only for Excellon format
bool DIALOG_GENDRILL::g_mirror           = false;    // Only for Excellon format
bool DIALOG_GENDRILL::g_merge_PTH_NPTH   = false;    // Only for Excellon format
bool DIALOG_GENDRILL::g_generateMap      = false;
int  DIALOG_GENDRILL::g_mapFileType      = 4;        // The last choice in m_Choice_Drill_Map
int  DIALOG_GENDRILL::g_drillFileType    = 0;

bool DIALOG_GENDRILL::g_useRouteModeForOvalHoles = true;    // Use G00 route mode to "drill" oval
                                                            // holes
DRILL_PRECISION  DIALOG_GENDRILL::g_precision;
VECTOR2I         DIALOG_GENDRILL::g_drillFileOffset;


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
        DIALOG_GENDRILL_BASE( aParent )
{
    m_pcbEditFrame = aPcbEditFrame;
    m_board  = m_pcbEditFrame->GetBoard();
    m_job = nullptr;
    m_plotOpts = m_pcbEditFrame->GetPlotSettings();

    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    SetupStandardButtons( { { wxID_OK,     _( "Generate" ) },
                            { wxID_CANCEL, _( "Close" ) } } );

    initDialog();
    finishDialogSettings();
}


DIALOG_GENDRILL::DIALOG_GENDRILL( PCB_EDIT_FRAME* aPcbEditFrame, JOB_EXPORT_PCB_DRILL* aJob,
                                  wxWindow* aParent ) :
        DIALOG_GENDRILL_BASE( aParent )
{
    m_pcbEditFrame = aPcbEditFrame;
    m_board = m_pcbEditFrame->GetBoard();
    m_job = aJob;

    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    // hide ui elements that dont belong for job config
    m_buttonReport->Hide();
    bMainSizer->Remove( bMsgSizer );
    m_messagesBox->Hide();

    SetupStandardButtons();

    initDialog();
    finishDialogSettings();
}


DIALOG_GENDRILL::~DIALOG_GENDRILL()
{
}


bool DIALOG_GENDRILL::TransferDataFromWindow()
{
    if( !m_job )
    {
        genDrillAndMapFiles( true, m_cbGenerateMap->GetValue() );
    }
    else
    {
        m_job->SetConfiguredOutputPath( m_outputDirectoryName->GetValue() );
        m_job->m_format = m_rbExcellon->GetValue() ? JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON
												   : JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER;
        m_job->m_drillUnits = m_units->GetSelection() == 0
                                                   ? JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MILLIMETERS
                                                   : JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCHES;
        m_job->m_drillOrigin = static_cast<JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN>( m_origin->GetSelection() );
        m_job->m_excellonCombinePTHNPTH = m_Check_Merge_PTH_NPTH->IsChecked();
        m_job->m_excellonMinimalHeader = m_Check_Minimal->IsChecked();
        m_job->m_excellonMirrorY = m_Check_Mirror->IsChecked();
        m_job->m_excellonOvalDrillRoute = !m_altDrillMode->GetValue();
        m_job->m_mapFormat = static_cast<JOB_EXPORT_PCB_DRILL::MAP_FORMAT>( m_choiceDrillMap->GetSelection() );
        m_job->m_zeroFormat = static_cast<JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT>( m_zeros->GetSelection() );
        m_job->m_generateMap = m_cbGenerateMap->IsChecked();
    }

    return true;
}


bool DIALOG_GENDRILL::TransferDataToWindow()
{
    if( !m_job )
    {
        m_rbExcellon->SetValue( g_drillFileType == 0 );
        m_rbGerberX2->SetValue( g_drillFileType == 1 );
        m_units->SetSelection( g_unitDrillIsInch ? 1 : 0 );
        m_zeros->SetSelection( g_zerosFormat );
        updatePrecisionOptions();
        m_Check_Minimal->SetValue( g_minimalHeader );

        m_origin->SetSelection( m_drillOriginIsAuxAxis ? 1 : 0 );

        m_Check_Mirror->SetValue( g_mirror );
        m_Check_Merge_PTH_NPTH->SetValue( g_merge_PTH_NPTH );
        m_choiceDrillMap->SetSelection( g_mapFileType );
        m_altDrillMode->SetValue( !g_useRouteModeForOvalHoles );
        m_cbGenerateMap->SetValue( g_generateMap );

        // Output directory
        m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );
    }
    else
    {
        m_browseButton->Hide();
        m_outputDirectoryName->SetValue( m_job->GetConfiguredOutputPath() );

        m_rbExcellon->SetValue( m_job->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON );
        m_rbGerberX2->SetValue( m_job->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER );
        m_units->SetSelection( m_job->m_drillUnits == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCHES );
        m_zeros->SetSelection( static_cast<int>( m_job->m_zeroFormat ) );
        updatePrecisionOptions();
        m_Check_Minimal->SetValue( m_job->m_excellonMinimalHeader );

        m_origin->SetSelection( m_job->m_drillOrigin == JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::PLOT );

        m_Check_Mirror->SetValue( m_job->m_excellonMirrorY );
        m_Check_Merge_PTH_NPTH->SetValue( m_job->m_excellonCombinePTHNPTH );
        m_choiceDrillMap->SetSelection( static_cast<int>( m_job->m_mapFormat ) );
        m_altDrillMode->SetValue( !m_job->m_excellonOvalDrillRoute );
        m_cbGenerateMap->SetValue( m_job->m_generateMap );
    }

    wxCommandEvent dummy;
    onFileFormatSelection( dummy );
	return true;
}


void DIALOG_GENDRILL::initDialog()
{
    if( m_job )
    {
        SetTitle( m_job->GetSettingsDialogTitle() );
    }
    else
    {
        auto cfg = m_pcbEditFrame->GetPcbNewSettings();

        g_merge_PTH_NPTH = cfg->m_GenDrill.merge_pth_npth;
        g_minimalHeader = cfg->m_GenDrill.minimal_header;
        g_mirror = cfg->m_GenDrill.mirror;
        g_unitDrillIsInch = cfg->m_GenDrill.unit_drill_is_inch;
        g_useRouteModeForOvalHoles = cfg->m_GenDrill.use_route_for_oval_holes;
        g_drillFileType = cfg->m_GenDrill.drill_file_type;
        g_mapFileType = cfg->m_GenDrill.map_file_type;
        g_zerosFormat = cfg->m_GenDrill.zeros_format;
        g_generateMap = cfg->m_GenDrill.generate_map;

        // Ensure validity of g_mapFileType
        if( g_mapFileType < 0 || g_mapFileType >= (int) m_choiceDrillMap->GetCount() )
            g_mapFileType = m_choiceDrillMap->GetCount() - 1; // last item in list = default = PDF
	}

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions (which have different sizes).
    m_hash_key = TO_UTF8( GetTitle() );

    m_drillOriginIsAuxAxis = m_plotOpts.GetUseAuxOrigin();
}


void DIALOG_GENDRILL::onFileFormatSelection( wxCommandEvent& event )
{
    bool enbl_Excellon = m_rbExcellon->GetValue();

    g_drillFileType = enbl_Excellon ? 0 : 1;

    m_unitsLabel->Enable( enbl_Excellon );
    m_units->Enable( enbl_Excellon );
    m_zerosLabel->Enable( enbl_Excellon );
	m_zeros->Enable( enbl_Excellon );
    m_Check_Mirror->Enable( enbl_Excellon );
    m_Check_Minimal->Enable( enbl_Excellon );
    m_Check_Merge_PTH_NPTH->Enable( enbl_Excellon );
    m_altDrillMode->Enable( enbl_Excellon );

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
    UpdateDrillParams();

    PCBNEW_SETTINGS* cfg = m_pcbEditFrame->GetPcbNewSettings();

    cfg->m_GenDrill.merge_pth_npth           = g_merge_PTH_NPTH;
    cfg->m_GenDrill.minimal_header           = g_minimalHeader;
    cfg->m_GenDrill.mirror                   = g_mirror;
    cfg->m_GenDrill.unit_drill_is_inch       = g_unitDrillIsInch;
    cfg->m_GenDrill.use_route_for_oval_holes = g_useRouteModeForOvalHoles;
    cfg->m_GenDrill.drill_file_type          = g_drillFileType;
    cfg->m_GenDrill.map_file_type            = g_mapFileType;
    cfg->m_GenDrill.zeros_format             = g_zerosFormat;
    cfg->m_GenDrill.generate_map             = g_generateMap;
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
    wxString   msg;
    msg.Printf( _( "Do you want to use a path relative to\n'%s'?" ), defaultPath );

    wxMessageDialog dialog( this, msg, _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    if( dialog.ShowModal() == wxID_YES )
    {
        if( !dirName.MakeRelativeTo( defaultPath ) )
        {
            wxMessageBox( _( "Cannot make path relative (target volume different from board "
                             "file volume)!" ),
                          _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
        }
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


void DIALOG_GENDRILL::UpdateDrillParams()
{
    // Set output directory and replace backslashes with forward ones
    wxString dirStr;
    dirStr = m_outputDirectoryName->GetValue();
    dirStr.Replace( wxT( "\\" ), wxT( "/" ) );
    m_plotOpts.SetOutputDirectory( dirStr );
    m_drillOriginIsAuxAxis = m_origin->GetSelection() == 1;
    m_plotOpts.SetUseAuxOrigin( m_drillOriginIsAuxAxis );

    g_mapFileType = m_choiceDrillMap->GetSelection();

    g_unitDrillIsInch = ( m_units->GetSelection() == 0 ) ? false : true;
    g_minimalHeader = m_Check_Minimal->IsChecked();
    g_mirror = m_Check_Mirror->IsChecked();
    g_merge_PTH_NPTH = m_Check_Merge_PTH_NPTH->IsChecked();
    g_zerosFormat = m_zeros->GetSelection();
    g_useRouteModeForOvalHoles = !m_altDrillMode->GetValue();
    g_generateMap = m_cbGenerateMap->IsChecked();

    if( m_origin->GetSelection() == 0 )
        g_drillFileOffset = VECTOR2I( 0, 0 );
    else
        g_drillFileOffset = m_board->GetDesignSettings().GetAuxOrigin();

    if( g_unitDrillIsInch )
        g_precision = precisionListForInches;
    else
        g_precision = precisionListForMetric;

    if( !m_plotOpts.IsSameAs( m_board->GetPlotOptions() ) )
    {
        m_board->SetPlotOptions( m_plotOpts );
        m_pcbEditFrame->OnModify();
    }
}


void DIALOG_GENDRILL::genDrillAndMapFiles( bool aGenDrill, bool aGenMap )
{
    updateConfig();     // set params and Save drill options

    m_pcbEditFrame->ClearMsgPanel();
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
        wxString msg;
        msg.Printf( _( "Could not write drill and/or map files to folder '%s'." ),
                    outputDir.GetPath() );
        DisplayError( this, msg );
        return;
    }

    if( g_drillFileType == 0 )
    {
        EXCELLON_WRITER excellonWriter( m_board );
        excellonWriter.SetFormat( !g_unitDrillIsInch, (EXCELLON_WRITER::ZEROS_FMT) g_zerosFormat,
                                  g_precision.m_Lhs, g_precision.m_Rhs );
        excellonWriter.SetOptions( g_mirror, g_minimalHeader, g_drillFileOffset, g_merge_PTH_NPTH );
        excellonWriter.SetRouteModeForOvalHoles( g_useRouteModeForOvalHoles );
        excellonWriter.SetMapFileFormat( filefmt[choice] );

        excellonWriter.CreateDrillandMapFilesSet( outputDir.GetFullPath(), aGenDrill, aGenMap,
                                                  &reporter );
    }
    else
    {
        GERBER_WRITER gerberWriter( m_board );
        // Set gerber precision: only 5 or 6 digits for mantissa are allowed
        // (SetFormat() accept 5 or 6, and any other value set the precision to 5)
        // the integer part precision is always 4, and units always mm
        gerberWriter.SetFormat( m_plotOpts.GetGerberPrecision() );
        gerberWriter.SetOptions( g_drillFileOffset );
        gerberWriter.SetMapFileFormat( filefmt[choice] );

        gerberWriter.CreateDrillandMapFilesSet( outputDir.GetFullPath(), aGenDrill, aGenMap,
                                                &reporter );
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

    bool success;

    // Info is slightly different between Excellon and Gerber
    // (file ext, Merge PTH/NPTH option)
    if( g_drillFileType == 0 )
    {
        EXCELLON_WRITER excellonWriter( m_board );
        excellonWriter.SetMergeOption( g_merge_PTH_NPTH );
        success = excellonWriter.GenDrillReportFile( dlg.GetPath() );
    }
    else
    {
        GERBER_WRITER gerberWriter( m_board );
        success = gerberWriter.GenDrillReportFile( dlg.GetPath() );
    }

    wxString   msg;

    if( ! success )
    {
        msg.Printf(  _( "Failed to create file '%s'." ), dlg.GetPath() );
        m_messagesBox->AppendText( msg );
    }
    else
    {
        msg.Printf( _( "Report file '%s' created." ), dlg.GetPath() );
        m_messagesBox->AppendText( msg );
    }
}
