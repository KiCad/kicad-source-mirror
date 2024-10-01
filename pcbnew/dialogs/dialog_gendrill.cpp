/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 Jean_Pierre Charras <jp.charras at wanadoo.fr>
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
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <paths.h>
#include <project.h>
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
    m_plotOpts = m_pcbEditFrame->GetPlotSettings();

    SetupStandardButtons( { { wxID_OK,     _( "Generate Drill File" ) },
                            { wxID_APPLY,  _( "Generate Map File" )   },
                            { wxID_CANCEL, _( "Close" )               } } );

    m_buttonsSizer->Layout();

    SetReturnCode( 1 );
    initDialog();
    GetSizer()->SetSizeHints( this );
}



DIALOG_GENDRILL::DIALOG_GENDRILL( PCB_EDIT_FRAME* aPcbEditFrame, JOB_EXPORT_PCB_DRILL* aJob,
                                  wxWindow* aParent ) :
    DIALOG_GENDRILL_BASE( aParent )
{
    m_pcbEditFrame = aPcbEditFrame;
    m_board = m_pcbEditFrame->GetBoard();
    m_job = aJob;

    // hide ui elements that dont belong for job config
    m_buttonReport->Hide();
    bMainSizer->Remove( bMsgSizer );
    m_messagesBox->Hide();

    m_sdbSizerApply->Hide();
    SetupStandardButtons( { { wxID_OK, _( "Save" ) }, { wxID_CANCEL, _( "Cancel" ) } } );
    m_buttonsSizer->Layout();

    SetReturnCode( 1 );

    GetSizer()->SetSizeHints( this );
}


// Static members of DIALOG_GENDRILL
int DIALOG_GENDRILL::m_UnitDrillIsInch  = true;     // Only for Excellon format
int DIALOG_GENDRILL::m_ZerosFormat      = EXCELLON_WRITER::DECIMAL_FORMAT;
bool DIALOG_GENDRILL::m_MinimalHeader   = false;    // Only for Excellon format
bool DIALOG_GENDRILL::m_Mirror          = false;    // Only for Excellon format
bool DIALOG_GENDRILL::m_Merge_PTH_NPTH  = false;    // Only for Excellon format
int DIALOG_GENDRILL::m_mapFileType      = 4;        // The last choice in m_Choice_Drill_Map
int DIALOG_GENDRILL::m_drillFileType    = 0;
bool DIALOG_GENDRILL::m_UseRouteModeForOvalHoles = true;    // Use G00 route mode to "drill" oval holes

DIALOG_GENDRILL::~DIALOG_GENDRILL()
{
}


bool DIALOG_GENDRILL::TransferDataFromWindow()
{
    return true;
}


bool DIALOG_GENDRILL::TransferDataToWindow()
{
	initDialog();
	return true;
}


void DIALOG_GENDRILL::initDialog()
{
    if( !m_job )
    {
        auto cfg = m_pcbEditFrame->GetPcbNewSettings();

        m_Merge_PTH_NPTH = cfg->m_GenDrill.merge_pth_npth;
        m_MinimalHeader = cfg->m_GenDrill.minimal_header;
        m_Mirror = cfg->m_GenDrill.mirror;
        m_UnitDrillIsInch = cfg->m_GenDrill.unit_drill_is_inch;
        m_UseRouteModeForOvalHoles = cfg->m_GenDrill.use_route_for_oval_holes;
        m_drillFileType = cfg->m_GenDrill.drill_file_type;
        m_mapFileType = cfg->m_GenDrill.map_file_type;
        m_ZerosFormat = cfg->m_GenDrill.zeros_format;

        // Ensure validity of m_mapFileType
        if( m_mapFileType < 0 || m_mapFileType >= (int) m_Choice_Drill_Map->GetCount() )
            m_mapFileType = m_Choice_Drill_Map->GetCount() - 1; // last item in list = default = PDF
	}

    m_drillOriginIsAuxAxis = m_plotOpts.GetUseAuxOrigin();

    InitDisplayParams();
}


void DIALOG_GENDRILL::InitDisplayParams()
{
    if( !m_job )
    {
        m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

        m_rbExcellon->SetValue( m_drillFileType == 0 );
        m_rbGerberX2->SetValue( m_drillFileType == 1 );
        m_Choice_Unit->SetSelection( m_UnitDrillIsInch ? 1 : 0 );
        m_Choice_Zeros_Format->SetSelection( m_ZerosFormat );
        UpdatePrecisionOptions();
        m_Check_Minimal->SetValue( m_MinimalHeader );

        m_Choice_Drill_Offset->SetSelection( m_drillOriginIsAuxAxis ? 1 : 0 );

        m_Check_Mirror->SetValue( m_Mirror );
        m_Check_Merge_PTH_NPTH->SetValue( m_Merge_PTH_NPTH );
        m_Choice_Drill_Map->SetSelection( m_mapFileType );
        m_radioBoxOvalHoleMode->SetSelection( m_UseRouteModeForOvalHoles ? 0 : 1 );

        // Output directory
        m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );
    }
    else
    {
        m_browseButton->Hide();
        m_outputDirectoryName->SetValue( m_job->GetOutputPath() );

        m_rbExcellon->SetValue( m_job->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON );
        m_rbGerberX2->SetValue( m_job->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER );
        m_Choice_Unit->SetSelection( m_job->m_drillUnits == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCHES );
        m_Choice_Zeros_Format->SetSelection( static_cast<int>( m_job->m_zeroFormat ) );
        UpdatePrecisionOptions();
        m_Check_Minimal->SetValue( m_job->m_excellonMinimalHeader );

        m_Choice_Drill_Offset->SetSelection( m_job->m_drillOrigin == JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::PLOT );

        m_Check_Mirror->SetValue( m_job->m_excellonMirrorY );
        m_Check_Merge_PTH_NPTH->SetValue( m_job->m_excellonCombinePTHNPTH );
        m_Choice_Drill_Map->SetSelection( static_cast<int>( m_job->m_mapFormat ) );
        m_radioBoxOvalHoleMode->SetSelection( m_job->m_excellonOvalDrillRoute ? 0 : 1 );
    }

    m_platedPadsHoleCount    = 0;
    m_notplatedPadsHoleCount = 0;
    m_throughViasCount = 0;
    m_microViasCount   = 0;
    m_blindOrBuriedViasCount = 0;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( pad->GetDrillShape() == PAD_DRILL_SHAPE::CIRCLE )
            {
                if( pad->GetDrillSize().x != 0 )
                {
                    if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                        m_notplatedPadsHoleCount++;
                    else
                        m_platedPadsHoleCount++;
                }
            }
            else
            {
                if( pad->GetDrillSize().x != 0 && pad->GetDrillSize().y != 0 )
                {
                    if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                        m_notplatedPadsHoleCount++;
                    else
                        m_platedPadsHoleCount++;
                }
            }
        }
    }

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        const PCB_VIA *via = dynamic_cast<const PCB_VIA*>( track );

        if( via )
        {
            switch( via->GetViaType() )
            {
            case VIATYPE::THROUGH:      m_throughViasCount++;       break;
            case VIATYPE::MICROVIA:     m_microViasCount++;         break;
            case VIATYPE::BLIND_BURIED: m_blindOrBuriedViasCount++; break;
            default:                                                break;
            }
        }
    }

    // Display hole counts:
    m_PlatedPadsCountInfoMsg->SetLabel( wxString() << m_platedPadsHoleCount );
    m_NotPlatedPadsCountInfoMsg->SetLabel( wxString() << m_notplatedPadsHoleCount );
    m_ThroughViasInfoMsg->SetLabel( wxString() << m_throughViasCount );
    m_MicroViasInfoMsg->SetLabel( wxString() << m_microViasCount );
    m_BuriedViasInfoMsg->SetLabel( wxString() << m_blindOrBuriedViasCount );

    wxCommandEvent dummy;
    onFileFormatSelection( dummy );
}


void DIALOG_GENDRILL::onFileFormatSelection( wxCommandEvent& event )
{
    bool enbl_Excellon = m_rbExcellon->GetValue();

    m_drillFileType = enbl_Excellon ? 0 : 1;

    m_Choice_Unit->Enable( enbl_Excellon );
	m_Choice_Zeros_Format->Enable( enbl_Excellon );
    m_Check_Mirror->Enable( enbl_Excellon );
    m_Check_Minimal->Enable( enbl_Excellon );
    m_Check_Merge_PTH_NPTH->Enable( enbl_Excellon );
    m_radioBoxOvalHoleMode->Enable( enbl_Excellon );

    if( enbl_Excellon )
    {
        UpdatePrecisionOptions();
    }
    else
    {
        m_staticTextPrecision->Enable( true );
        m_staticTextPrecision->SetLabel( m_plotOpts.GetGerberPrecision() == 6 ? wxT( "4.6" )
                                                                              : wxT( "4.5" ) );
    }
}


void DIALOG_GENDRILL::UpdateConfig()
{
    UpdateDrillParams();

    auto cfg = m_pcbEditFrame->GetPcbNewSettings();

    cfg->m_GenDrill.merge_pth_npth           = m_Merge_PTH_NPTH;
    cfg->m_GenDrill.minimal_header           = m_MinimalHeader;
    cfg->m_GenDrill.mirror                   = m_Mirror;
    cfg->m_GenDrill.unit_drill_is_inch       = m_UnitDrillIsInch;
    cfg->m_GenDrill.use_route_for_oval_holes = m_UseRouteModeForOvalHoles;
    cfg->m_GenDrill.drill_file_type          = m_drillFileType;
    cfg->m_GenDrill.map_file_type            = m_mapFileType;
    cfg->m_GenDrill.zeros_format             = m_ZerosFormat;
}


void DIALOG_GENDRILL::OnSelDrillUnitsSelected( wxCommandEvent& event )
{
    UpdatePrecisionOptions();
}


void DIALOG_GENDRILL::OnGenMapFile( wxCommandEvent& event )
{
    if( !m_job )
    {
        GenDrillAndMapFiles( false, true );
    }
}


void DIALOG_GENDRILL::OnGenDrillFile( wxCommandEvent& event )
{
    if( !m_job )
    {
        GenDrillAndMapFiles( true, false );
    }
    else
    {
        m_job->SetOutputPath( m_outputDirectoryName->GetValue() );
        m_job->m_format = m_rbExcellon->GetValue() ? JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON
												   : JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER;
        m_job->m_drillUnits = m_Choice_Unit->GetSelection() == 0
							? JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MILLIMETERS
							: JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCHES;
        m_job->m_drillOrigin = static_cast<JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN>( m_Choice_Drill_Offset->GetSelection() );
        m_job->m_excellonCombinePTHNPTH = m_Check_Merge_PTH_NPTH->IsChecked();
        m_job->m_excellonMinimalHeader = m_Check_Minimal->IsChecked();
        m_job->m_excellonMirrorY = m_Check_Mirror->IsChecked();
        m_job->m_excellonOvalDrillRoute = m_radioBoxOvalHoleMode->GetSelection() == 0;
        m_job->m_mapFormat = static_cast<JOB_EXPORT_PCB_DRILL::MAP_FORMAT>( m_Choice_Drill_Map->GetSelection() );
        m_job->m_zeroFormat = static_cast<JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT>( m_Choice_Zeros_Format->GetSelection() );
        Close();
    }
}


void DIALOG_GENDRILL::OnSelZerosFmtSelected( wxCommandEvent& event )
{
    UpdatePrecisionOptions();
}


void DIALOG_GENDRILL::UpdatePrecisionOptions()
{
    if( m_Choice_Unit->GetSelection()== 1 )
    {
        // Units = inches
        m_staticTextPrecision->SetLabel( precisionListForInches.GetPrecisionString() );
    }
    else
    {
        // metric options
        m_staticTextPrecision->SetLabel( precisionListForMetric.GetPrecisionString() );
    }

    if( m_Choice_Zeros_Format->GetSelection() == EXCELLON_WRITER::DECIMAL_FORMAT )
        m_staticTextPrecision->Enable( false );
    else
        m_staticTextPrecision->Enable( true );
}


void DIALOG_GENDRILL::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
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
    m_drillOriginIsAuxAxis = m_Choice_Drill_Offset->GetSelection() == 1;
    m_plotOpts.SetUseAuxOrigin( m_drillOriginIsAuxAxis );

    m_mapFileType = m_Choice_Drill_Map->GetSelection();

    m_UnitDrillIsInch = (m_Choice_Unit->GetSelection() == 0) ? false : true;
    m_MinimalHeader = m_Check_Minimal->IsChecked();
    m_Mirror = m_Check_Mirror->IsChecked();
    m_Merge_PTH_NPTH = m_Check_Merge_PTH_NPTH->IsChecked();
    m_ZerosFormat = m_Choice_Zeros_Format->GetSelection();
    m_UseRouteModeForOvalHoles = m_radioBoxOvalHoleMode->GetSelection() == 0;

    if( m_Choice_Drill_Offset->GetSelection() == 0 )
        m_DrillFileOffset = VECTOR2I( 0, 0 );
    else
        m_DrillFileOffset = m_board->GetDesignSettings().GetAuxOrigin();

    if( m_UnitDrillIsInch )
        m_Precision = precisionListForInches;
    else
        m_Precision = precisionListForMetric;

    if( !m_plotOpts.IsSameAs( m_board->GetPlotOptions() ) )
    {
        m_board->SetPlotOptions( m_plotOpts );
        m_pcbEditFrame->OnModify();
    }
}


void DIALOG_GENDRILL::GenDrillAndMapFiles( bool aGenDrill, bool aGenMap )
{
    UpdateConfig();     // set params and Save drill options

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

    unsigned choice = (unsigned) m_Choice_Drill_Map->GetSelection();

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

    if( m_drillFileType == 0 )
    {
        EXCELLON_WRITER excellonWriter( m_board );
        excellonWriter.SetFormat( !m_UnitDrillIsInch, (EXCELLON_WRITER::ZEROS_FMT) m_ZerosFormat,
                                  m_Precision.m_Lhs, m_Precision.m_Rhs );
        excellonWriter.SetOptions( m_Mirror, m_MinimalHeader, m_DrillFileOffset, m_Merge_PTH_NPTH );
        excellonWriter.SetRouteModeForOvalHoles( m_UseRouteModeForOvalHoles );
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
        gerberWriter.SetOptions( m_DrillFileOffset );
        gerberWriter.SetMapFileFormat( filefmt[choice] );

        gerberWriter.CreateDrillandMapFilesSet( outputDir.GetFullPath(), aGenDrill, aGenMap,
                                                &reporter );
    }
}


void DIALOG_GENDRILL::OnGenReportFile( wxCommandEvent& event )
{
    UpdateConfig(); // set params and Save drill options

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
    if( m_drillFileType == 0 )
    {
        EXCELLON_WRITER excellonWriter( m_board );
        excellonWriter.SetMergeOption( m_Merge_PTH_NPTH );
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
