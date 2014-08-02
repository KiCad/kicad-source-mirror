/**
 * @file dialog_gendrill.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <pcbplot.h>
#include <gendrill_Excellon_writer.h>
#include <macros.h>

#include <class_board.h>
#include <class_track.h>
#include <class_module.h>

#include <dialog_gendrill.h>
#include <wildcards_and_files_ext.h>


// Keywords for read and write config
#define ZerosFormatKey          wxT( "DrillZerosFormat" )
#define PrecisionKey            wxT( "DrilltPrecisionOpt" )
#define MirrorKey               wxT( "DrillMirrorYOpt" )
#define MinimalHeaderKey        wxT( "DrillMinHeader" )
#define MergePTHNPTHKey         wxT( "DrillMergePTHNPTH" )
#define UnitDrillInchKey        wxT( "DrillUnit" )
#define DrillOriginIsAuxAxisKey wxT( "DrillAuxAxis" )
#define DrillMapFileTypeKey     wxT( "DrillMapFileType" )

// list of allowed precision for EXCELLON files, for integer format:
// Due to difference between inches and mm,
// there are 2 precision values, one for inches and one for metric
static DRILL_PRECISION precisionListForInches( 2, 4 );
static DRILL_PRECISION precisionListForMetric( 3, 3 );


/* This function displays the dialog frame for drill tools
 */
void PCB_EDIT_FRAME::InstallDrillFrame( wxCommandEvent& event )
{
    DIALOG_GENDRILL dlg( this );

    dlg.ShowModal();
}


DIALOG_GENDRILL::DIALOG_GENDRILL( PCB_EDIT_FRAME* parent ) :
    DIALOG_GENDRILL_BASE( parent )
{
    m_parent = parent;
    m_board  = parent->GetBoard();
    m_config = wxGetApp().GetSettings();
    m_plotOpts = m_parent->GetPlotSettings();

    SetReturnCode( 1 );
    initDialog();
    GetSizer()->SetSizeHints( this );
}


// Static members of DIALOG_GENDRILL
int DIALOG_GENDRILL::m_UnitDrillIsInch = true;
int DIALOG_GENDRILL::m_ZerosFormat     = EXCELLON_WRITER::DECIMAL_FORMAT;
bool DIALOG_GENDRILL::m_MinimalHeader   = false;
bool DIALOG_GENDRILL::m_Mirror = false;
bool DIALOG_GENDRILL::m_Merge_PTH_NPTH = false;
bool DIALOG_GENDRILL::m_DrillOriginIsAuxAxis = false;
int DIALOG_GENDRILL::m_mapFileType = 1;


DIALOG_GENDRILL::~DIALOG_GENDRILL()
{
    UpdateConfig();
}


void DIALOG_GENDRILL::initDialog()
{
    m_config->Read( ZerosFormatKey, &m_ZerosFormat );
    m_config->Read( MirrorKey, &m_Mirror );
    m_config->Read( MergePTHNPTHKey, &m_Merge_PTH_NPTH );
    m_config->Read( MinimalHeaderKey, &m_MinimalHeader );
    m_config->Read( UnitDrillInchKey, &m_UnitDrillIsInch );
    m_config->Read( DrillOriginIsAuxAxisKey, &m_DrillOriginIsAuxAxis );
    m_config->Read( DrillMapFileTypeKey, &m_mapFileType );

    InitDisplayParams();
}


void DIALOG_GENDRILL::InitDisplayParams()
{
    wxString msg;

    m_Choice_Unit->SetSelection( m_UnitDrillIsInch ? 1 : 0 );
    m_Choice_Zeros_Format->SetSelection( m_ZerosFormat );
    UpdatePrecisionOptions();
    m_Check_Minimal->SetValue( m_MinimalHeader );

    if( m_DrillOriginIsAuxAxis )
        m_Choice_Drill_Offset->SetSelection( 1 );

    m_Check_Mirror->SetValue( m_Mirror );
    m_Check_Merge_PTH_NPTH->SetValue( m_Merge_PTH_NPTH );
    m_Choice_Drill_Map->SetSelection( m_mapFileType );
    m_ViaDrillValue->SetLabel( _( "Use Netclasses values" ) );
    m_MicroViaDrillValue->SetLabel( _( "Use Netclasses values" ) );

    // See if we have some buried vias or/and microvias, and display
    // microvias drill value if so
    m_throughViasCount = 0;
    m_microViasCount   = 0;
    m_blindOrBuriedViasCount = 0;

    for( TRACK* track = m_parent->GetBoard()->m_Track; track != NULL; track = track->Next() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        if( track->GetShape() == VIA_THROUGH )
            m_throughViasCount++;
        else if( track->GetShape() == VIA_MICROVIA )
            m_microViasCount++;
        else if( track->GetShape() == VIA_BLIND_BURIED )
            m_blindOrBuriedViasCount++;
    }

    m_MicroViaDrillValue->Enable( m_microViasCount );

    // Count plated pad holes and not plated pad holes:
    m_platedPadsHoleCount    = 0;
    m_notplatedPadsHoleCount = 0;

    for( MODULE* module = m_parent->GetBoard()->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad != NULL; pad = pad->Next() )
        {
            if( pad->GetDrillShape() == PAD_DRILL_CIRCLE )
            {
                if( pad->GetDrillSize().x != 0 )
                {
                    if( pad->GetAttribute() == PAD_HOLE_NOT_PLATED )
                        m_notplatedPadsHoleCount++;
                    else
                        m_platedPadsHoleCount++;
                }
            }
            else
            {
                if( pad->GetDrillSize().x != 0 && pad->GetDrillSize().y != 0 )
                {
                    if( pad->GetAttribute() == PAD_HOLE_NOT_PLATED )
                        m_notplatedPadsHoleCount++;
                    else
                        m_platedPadsHoleCount++;
                }
            }
        }
    }

    // Display hole counts:
    msg = m_PlatedPadsCountInfoMsg->GetLabel();
    msg << wxT( " " ) << m_platedPadsHoleCount;
    m_PlatedPadsCountInfoMsg->SetLabel( msg );

    msg = m_NotPlatedPadsCountInfoMsg->GetLabel();
    msg << wxT( " " ) << m_notplatedPadsHoleCount;
    m_NotPlatedPadsCountInfoMsg->SetLabel( msg );

    msg = m_ThroughViasInfoMsg->GetLabel();
    msg << wxT( " " ) << m_throughViasCount;
    m_ThroughViasInfoMsg->SetLabel( msg );

    msg = m_MicroViasInfoMsg->GetLabel();
    msg << wxT( " " ) << m_microViasCount;
    m_MicroViasInfoMsg->SetLabel( msg );

    msg = m_BuriedViasInfoMsg->GetLabel();
    msg << wxT( " " ) << m_blindOrBuriedViasCount;
    m_BuriedViasInfoMsg->SetLabel( msg );

    // Output directory
    m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );
}


void DIALOG_GENDRILL::UpdateConfig()
{
    SetParams();

    m_config->Write( ZerosFormatKey, m_ZerosFormat );
    m_config->Write( MirrorKey, m_Mirror );
    m_config->Write( MinimalHeaderKey, m_MinimalHeader );
    m_config->Write( MergePTHNPTHKey, m_Merge_PTH_NPTH );
    m_config->Write( UnitDrillInchKey, m_UnitDrillIsInch );
    m_config->Write( DrillOriginIsAuxAxisKey, m_DrillOriginIsAuxAxis );
    m_config->Write( DrillMapFileTypeKey, m_mapFileType );
}


void DIALOG_GENDRILL::OnSelDrillUnitsSelected( wxCommandEvent& event )
{
    UpdatePrecisionOptions();
}

void DIALOG_GENDRILL::OnGenMapFile( wxCommandEvent& event )
{
    GenDrillAndMapFiles( false, true);
}


void DIALOG_GENDRILL::OnGenDrillFile( wxCommandEvent& event )
{
    GenDrillAndMapFiles(true, false);
}


void DIALOG_GENDRILL::OnCancelClick( wxCommandEvent& event )
{
    UpdateConfig();                 // Save drill options:
    EndModal( wxID_CANCEL );        // Process the default cancel event (close dialog)
}


void DIALOG_GENDRILL::OnSelZerosFmtSelected( wxCommandEvent& event )
{
    UpdatePrecisionOptions();
}


void DIALOG_GENDRILL::UpdatePrecisionOptions()
{
    if( m_Choice_Unit->GetSelection()== 1 )     // Units = inches
        m_staticTextPrecision->SetLabel( precisionListForInches.GetPrecisionString() );
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
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxFileName  fn( m_outputDirectoryName->GetValue() );
    wxString    path;

    if( fn.IsRelative() )
        path = wxGetCwd() + fn.GetPathSeparator() + m_outputDirectoryName->GetValue();
    else
        path = m_outputDirectoryName->GetValue();

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName      dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxMessageDialog dialog( this, _( "Use a relative path? " ),
                            _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    if( dialog.ShowModal() == wxID_YES )
    {
        wxString boardFilePath = ( (wxFileName) m_parent->GetBoard()->GetFileName() ).GetPath();

        if( !dirName.MakeRelativeTo( boardFilePath ) )
            wxMessageBox( _( "Cannot make path relative.  The target volume is different from board file volume!" ),
                          _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


void DIALOG_GENDRILL::SetParams()
{
    wxString msg;

    // Set output directory and replace backslashes with forward ones
    wxString dirStr;
    dirStr = m_outputDirectoryName->GetValue();
    dirStr.Replace( wxT( "\\" ), wxT( "/" ) );
    m_plotOpts.SetOutputDirectory( dirStr );

    m_mapFileType = m_Choice_Drill_Map->GetSelection();

    m_UnitDrillIsInch = (m_Choice_Unit->GetSelection() == 0) ? false : true;
    m_MinimalHeader   = m_Check_Minimal->IsChecked();
    m_Mirror = m_Check_Mirror->IsChecked();
    m_Merge_PTH_NPTH = m_Check_Merge_PTH_NPTH->IsChecked();
    m_ZerosFormat = m_Choice_Zeros_Format->GetSelection();
    m_DrillOriginIsAuxAxis = m_Choice_Drill_Offset->GetSelection();

    if( m_Choice_Drill_Offset->GetSelection() == 0 )
        m_FileDrillOffset = wxPoint( 0, 0 );
    else
        m_FileDrillOffset = m_parent->GetAuxOrigin();

    if( m_UnitDrillIsInch )
        m_Precision = precisionListForInches;
    else
        m_Precision = precisionListForMetric;

    m_board->SetPlotOptions( m_plotOpts );
}


void DIALOG_GENDRILL::GenDrillAndMapFiles(bool aGenDrill, bool aGenMap)
{
    wxString   layer_extend;              /* added to the  Board FileName to
                                           * create FullFileName (= Board
                                           * FileName + layer pair names) */
    wxString   msg;
    bool       hasBuriedVias = false;  /* If true, drill files are created
                                        * layer pair by layer pair for
                                        * buried vias */
    int        layer1 = LAYER_N_BACK;
    int        layer2 = LAYER_N_FRONT;
    bool       gen_through_holes = true;
    bool       gen_NPTH_holes    = false;

    wxString   currentWD = ::wxGetCwd();

    UpdateConfig(); // set params and Save drill options

    m_parent->ClearMsgPanel();

    if( m_microViasCount || m_blindOrBuriedViasCount )
        hasBuriedVias = true;

    EXCELLON_WRITER excellonWriter( m_parent->GetBoard(),
                                    m_FileDrillOffset );
    excellonWriter.SetFormat( !m_UnitDrillIsInch,
                              (EXCELLON_WRITER::zeros_fmt) m_ZerosFormat,
                              m_Precision.m_lhs, m_Precision.m_rhs );
    excellonWriter.SetOptions( m_Mirror, m_MinimalHeader, m_FileDrillOffset, m_Merge_PTH_NPTH );

    wxFileName fn;

    for( ; ; )
    {
        excellonWriter.BuildHolesList( layer1, layer2, gen_through_holes ? false : true,
                                       gen_NPTH_holes, m_Merge_PTH_NPTH );

        if( excellonWriter.GetHolesCount() > 0 ) // has holes?
        {
            fn = m_parent->GetBoard()->GetFileName();
            layer_extend.Empty();

            if( gen_NPTH_holes )
            {
                layer_extend << wxT( "-NPTH" );
            }
            else if( !gen_through_holes )
            {
                if( layer1 == LAYER_N_BACK )
                    layer_extend << wxT( "-back" );
                else
                    layer_extend << wxT( "-inner" ) << layer1;

                if( layer2 == LAYER_N_FRONT )
                    layer_extend << wxT( "-front" );
                else
                    layer_extend << wxT( "-inner" ) << layer2;
            }

            fn.SetName( fn.GetName() + layer_extend );
            wxString defaultPath = m_plotOpts.GetOutputDirectory();

            if( defaultPath.IsEmpty() )
                defaultPath = ::wxGetCwd();

            fn.SetPath( defaultPath );

            if( aGenDrill )
            {
                fn.SetExt( DrillFileExtension );
                wxString fullFilename = fn.GetFullPath();

                FILE* file = wxFopen( fullFilename, wxT( "w" ) );

                if( file == 0 )
                {
                    msg.Printf( _( "** Unable to create %s **\n" ),
                                GetChars( fullFilename ) );
                    m_messagesBox->AppendText( msg );
                    break;
                }
                else
                {
                    msg.Printf( _( "Plot: %s OK\n" ), GetChars( fullFilename ) );
                    m_messagesBox->AppendText( msg );
                }

                excellonWriter.CreateDrillFile( file );
            }

            if( aGenMap )
            {
                const PlotFormat filefmt[6] =
                {   // Keep these format ids in the same order than m_Choice_Drill_Map choices
                    PLOT_FORMAT_HPGL, PLOT_FORMAT_POST, PLOT_FORMAT_GERBER,
                    PLOT_FORMAT_DXF, PLOT_FORMAT_SVG, PLOT_FORMAT_PDF
                };
                unsigned choice = (unsigned) m_Choice_Drill_Map->GetSelection();

                if( choice >= m_Choice_Drill_Map->GetCount() )
                    choice = 1;

                fn.SetExt( wxEmptyString ); // Will be added by GenDrillMap
                wxString fullfilename = fn.GetFullPath() + wxT( "-drl_map" );

                GenDrillMap( fullfilename, excellonWriter, filefmt[choice] );
            }
        }

        if( gen_NPTH_holes )    // The last drill file was created
            break;

        if( !hasBuriedVias )
            gen_NPTH_holes = true;
        else
        {
            if(  gen_through_holes )
                layer2 = layer1 + 1;    // prepare generation of first layer pair
            else
            {
                if( layer2 >= LAYER_N_FRONT )    // no more layer pair to consider
                {
                    layer1 = LAYER_N_BACK;
                    layer2 = LAYER_N_FRONT;
                    gen_NPTH_holes = true;
                    continue;
                }

                layer1++;
                layer2++;                      // use next layer pair

                if( layer2 == m_parent->GetBoard()->GetCopperLayerCount() - 1 )
                    layer2 = LAYER_N_FRONT;         // the last layer is always the
                                                    // Front layer
            }

            gen_through_holes = false;
        }
    }

    ::wxSetWorkingDirectory( currentWD );
}


void DIALOG_GENDRILL::OnGenReportFile( wxCommandEvent& event )
{
    UpdateConfig(); // set params and Save drill options

    wxFileName fn = m_parent->GetBoard()->GetFileName();

    fn.SetName( fn.GetName() + wxT( "-drl" ) );
    fn.SetExt( ReportFileExtension );

    wxString defaultPath = m_plotOpts.GetOutputDirectory();

    if( defaultPath.IsEmpty() )
        defaultPath = ::wxGetCwd();

    wxFileDialog dlg( this, _( "Save Drill Report File" ), defaultPath,
                      fn.GetFullName(), wxGetTranslation( ReportFileWildcard ),
                      wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    EXCELLON_WRITER excellonWriter( m_parent->GetBoard(),
                                    m_FileDrillOffset );
    excellonWriter.SetFormat( !m_UnitDrillIsInch,
                              (EXCELLON_WRITER::zeros_fmt) m_ZerosFormat,
                              m_Precision.m_lhs, m_Precision.m_rhs );
    excellonWriter.SetOptions( m_Mirror, m_MinimalHeader, m_FileDrillOffset, m_Merge_PTH_NPTH );

    bool success = excellonWriter.GenDrillReportFile( dlg.GetPath() );

    wxString   msg;

    if( ! success )
    {
        msg.Printf(  _( "** Unable to create %s **\n" ), GetChars( dlg.GetPath() ) );
        m_messagesBox->AppendText( msg );
    }
    else
    {
        msg.Printf( _( "Report file %s created\n" ), GetChars( dlg.GetPath() ) );
        m_messagesBox->AppendText( msg );
    }
}


// Generate the drill map of the board
void DIALOG_GENDRILL::GenDrillMap( const wxString aFullFileNameWithoutExt,
                                   EXCELLON_WRITER& aExcellonWriter,
                                   PlotFormat     format )
{
    wxString   ext, wildcard;

    /* Init extension */
    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        ext = HPGL_PLOTTER::GetDefaultFileExtension();
        wildcard = _( "HPGL plot files (.plt)|*.plt" );
        break;

    case PLOT_FORMAT_POST:
        ext = PS_PLOTTER::GetDefaultFileExtension();
        wildcard = PSFileWildcard;
        break;

    case PLOT_FORMAT_GERBER:
        ext = GERBER_PLOTTER::GetDefaultFileExtension();
        wildcard = _( "Gerber files (.pho)|*.pho" );
        break;

    case PLOT_FORMAT_DXF:
        ext = DXF_PLOTTER::GetDefaultFileExtension();
        wildcard = _( "DXF files (.dxf)|*.dxf" );
        break;

    case PLOT_FORMAT_SVG:
        ext = SVG_PLOTTER::GetDefaultFileExtension();
        wildcard = SVGFileWildcard;
        break;

    case PLOT_FORMAT_PDF:
        ext = PDF_PLOTTER::GetDefaultFileExtension();
        wildcard = PdfFileWildcard;
        break;

    default:
        wxLogMessage( wxT( "DIALOG_GENDRILL::GenDrillMap() error, fmt % unknown" ), format );
        return;
    }

    // Add file name extension
    wxString fullFilename = aFullFileNameWithoutExt;
    fullFilename << wxT(".") << ext;

    bool success = aExcellonWriter.GenDrillMapFile( fullFilename,
                                                    m_parent->GetPageSettings(),
                                                    format );

    wxString   msg;

    if( ! success )
    {
        msg.Printf( _( "** Unable to create %s **\n" ), GetChars( fullFilename ) );
        m_messagesBox->AppendText( msg );
        return;
    }
    else
    {
        msg.Printf( _( "Plot: %s OK\n" ), GetChars( fullFilename ) );
        m_messagesBox->AppendText( msg );
    }
}
