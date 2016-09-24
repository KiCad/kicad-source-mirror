/**
 * @file gen_modules_placefile.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <wxPcbStruct.h>
#include <pgm_base.h>
#include <build_version.h>
#include <macros.h>
#include <reporter.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <wildcards_and_files_ext.h>
#include <kiface_i.h>
#include <wx_html_report_panel.h>


#include <dialog_gen_module_position_file_base.h>
/*
 * The ASCII format of the kicad place file is:
 *      ### Module positions - created on 04/12/2012 15:24:24 ###
 *      ### Printed by Pcbnew version pcbnew (2012-11-30 BZR 3828)-testing
 *      ## Unit = inches, Angle = deg.
 * or
 *      ## Unit = mm, Angle = deg.
 *      ## Side : top
 * or
 *      ## Side : bottom
 * or
 *      ## Side : all
 *      # Ref    Val              Package             PosX       PosY        Rot     Side
 *      C123     0,1uF/50V        SM0603              1.6024    -2.6280     180.0    Front
 *      C124     0,1uF/50V        SM0603              1.6063    -2.7579     180.0    Front
 *      C125     0,1uF/50V        SM0603              1.6010    -2.8310     180.0    Front
 *      ## End
 */

#define PLACEFILE_UNITS_KEY  wxT( "PlaceFileUnits" )
#define PLACEFILE_OPT_KEY    wxT( "PlaceFileOpts" )
#define PLACEFILE_FORMAT_KEY wxT( "PlaceFileFormat" )


#define PCB_BACK_SIDE 0
#define PCB_FRONT_SIDE 1
#define PCB_BOTH_SIDES 2

class LIST_MOD      // An helper class used to build a list of useful footprints.
{
public:
    MODULE*       m_Module;         // Link to the actual footprint
    wxString      m_Reference;      // Its schematic reference
    wxString      m_Value;          // Its schematic value
    LAYER_NUM     m_Layer;          // its side (B_Cu, or F_Cu)
};


/**
 * The dialog to create footprint position files,
 * and choose options (one or 2 files, units and force all SMD footprints in list)
 */
class DIALOG_GEN_MODULE_POSITION : public DIALOG_GEN_MODULE_POSITION_BASE
{
public:
    DIALOG_GEN_MODULE_POSITION( PCB_EDIT_FRAME * aParent ):
        DIALOG_GEN_MODULE_POSITION_BASE( aParent ),
        m_parent( aParent ),
        m_plotOpts( aParent->GetPlotSettings() )
    {
        m_reporter = &m_messagesPanel->Reporter();
        initDialog();

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
    void OnOKButton( wxCommandEvent& event ) override;

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
        return m_radioBoxForceSmd->GetSelection() == 1;
    }
};


// Static members to remember choices
int DIALOG_GEN_MODULE_POSITION::m_unitsOpt = 0;
int DIALOG_GEN_MODULE_POSITION::m_fileOpt = 0;
int DIALOG_GEN_MODULE_POSITION::m_fileFormat = 0;

// Use standard board side name. do not translate them,
// they are keywords in place file
const wxString frontSideName = wxT( "top" );
const wxString backSideName = wxT( "bottom" );

void DIALOG_GEN_MODULE_POSITION::initDialog()
{
    m_config = Kiface().KifaceSettings();
    m_config->Read( PLACEFILE_UNITS_KEY, &m_unitsOpt, 1 );
    m_config->Read( PLACEFILE_OPT_KEY, &m_fileOpt, 0 );
    m_config->Read( PLACEFILE_FORMAT_KEY, &m_fileFormat, 0 );

    // Output directory
    m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );
    m_radioBoxUnits->SetSelection( m_unitsOpt );
    m_radioBoxFilesCount->SetSelection( m_fileOpt );
    m_rbFormat->SetSelection( m_fileFormat );

    m_sdbSizerButtonsOK->SetDefault();
}

void DIALOG_GEN_MODULE_POSITION::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxString    path = Prj().AbsolutePath( m_outputDirectoryName->GetValue() );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxMessageDialog dialog( this, _( "Use a relative path? "),
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

void DIALOG_GEN_MODULE_POSITION::OnOKButton( wxCommandEvent& event )
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

    // the dialog is not closed here.
}


bool DIALOG_GEN_MODULE_POSITION::CreateFiles()
{
    BOARD * brd = m_parent->GetBoard();
    wxFileName  fn;
    wxString    msg;
    bool singleFile = OneFileOnly();
    bool useCSVfmt = m_fileFormat == 1;
    int fullcount = 0;

    // Count the footprints to place, do not yet create a file
    int fpcount = m_parent->DoGenFootprintsPositionFile( wxEmptyString, UnitsMM(),
                                                         ForceAllSmd(), PCB_BOTH_SIDES,
                                                         useCSVfmt );
    if( fpcount == 0)
    {
        wxMessageBox( _( "No footprint for automated placement." ) );
        return false;
    }

    // Create output directory if it does not exist (also transform it in
    // absolute path). Bail if it fails
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

    // Create the the Front or Top side placement file,
    // or the single file
    int side = PCB_FRONT_SIDE;

    if( singleFile )
    {
        side = PCB_BOTH_SIDES;
        fn.SetName( fn.GetName() + wxT( "-" ) + wxT("all") );
    }
    else
        fn.SetName( fn.GetName() + wxT( "-" ) + frontSideName );


    if( useCSVfmt )
    {
        fn.SetName( fn.GetName() + wxT( "-" ) + FootprintPlaceFileExtension );
        fn.SetExt( wxT( "csv" ) );
    }
    else
        fn.SetExt( FootprintPlaceFileExtension );

    fpcount = m_parent->DoGenFootprintsPositionFile( fn.GetFullPath(), UnitsMM(),
                                                     ForceAllSmd(), side, useCSVfmt );
    if( fpcount < 0 )
    {
        msg.Printf( _( "Unable to create '%s'." ), GetChars( fn.GetFullPath() ) );
        wxMessageBox( msg );
        m_reporter->Report( msg, REPORTER::RPT_ERROR );
        return false;
    }

    if( singleFile  )
        msg.Printf( _( "Place file: '%s'." ), GetChars( fn.GetFullPath() ) );
    else
        msg.Printf( _( "Front side (top side) place file: '%s'." ),
                    GetChars( fn.GetFullPath() ) );
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
    side = PCB_BACK_SIDE;
    fn = brd->GetFileName();
    fn.SetPath( outputDir.GetPath() );
    fn.SetName( fn.GetName() + wxT( "-" ) + backSideName );

    if( useCSVfmt )
    {
        fn.SetName( fn.GetName() + wxT( "-" ) + FootprintPlaceFileExtension );
        fn.SetExt( wxT( "csv" ) );
    }
    else
        fn.SetExt( FootprintPlaceFileExtension );

    fpcount = m_parent->DoGenFootprintsPositionFile( fn.GetFullPath(), UnitsMM(),
                                                    ForceAllSmd(), side, useCSVfmt );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Unable to create file '%s'." ), GetChars( fn.GetFullPath() ) );
        m_reporter->Report( msg, REPORTER::RPT_ERROR );
        wxMessageBox( msg );
        return false;
    }

    // Display results
    if( !singleFile )
    {
        msg.Printf( _( "Back side (bottom side) place file: '%s'." ), GetChars( fn.GetFullPath() ) );
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

// Defined values to write coordinates using inches or mm:
static const double conv_unit_inch = 0.001 / IU_PER_MILS ;      // units = INCHES
static const char unit_text_inch[] = "## Unit = inches, Angle = deg.\n";

static const double conv_unit_mm = 1.0 / IU_PER_MM;    // units = mm
static const char unit_text_mm[] = "## Unit = mm, Angle = deg.\n";

static wxPoint File_Place_Offset;  // Offset coordinates for generated file.


// Sort function use by GenereModulesPosition()
// sort is made by side (layer) top layer first
// then by reference increasing order
static bool sortFPlist( const LIST_MOD& ref, const LIST_MOD& tst )
{
    if( ref.m_Layer == tst.m_Layer )
        return StrNumCmp( ref.m_Reference, tst.m_Reference, 16 ) < 0;

    return ref.m_Layer > tst.m_Layer;
}


/**
 * Helper function HasNonSMDPins
 * returns true if the given module has any non smd pins, such as through hole
 * and therefore cannot be placed automatically.
 */
static bool HasNonSMDPins( MODULE* aModule )
{
    D_PAD* pad;

    for( pad = aModule->Pads();  pad;  pad = pad->Next() )
    {
        if( pad->GetAttribute() != PAD_ATTRIB_SMD )
            return true;
    }

    return false;
}

void PCB_EDIT_FRAME::GenFootprintsPositionFile( wxCommandEvent& event )
{
    DIALOG_GEN_MODULE_POSITION dlg( this );
    dlg.ShowModal();
}

/*
 * Creates a footprint position file
 * aSide = 0 -> Back (bottom) side)
 * aSide = 1 -> Front (top) side)
 * aSide = 2 -> both sides
 * if aFullFileName is empty, the file is not created, only the
 * count of footprints to place is returned
 */
int PCB_EDIT_FRAME::DoGenFootprintsPositionFile( const wxString& aFullFileName,
                                                 bool aUnitsMM,
                                                 bool aForceSmdItems, int aSide,
                                                 bool aFormatCSV )
{
    MODULE*     footprint;

    // Minimal text lengths:
    int lenRefText = 8;
    int lenValText = 8;
    int lenPkgText = 16;

    File_Place_Offset = GetAuxOrigin();

    // Calculating the number of useful footprints (CMS attribute, not VIRTUAL)
    int footprintCount = 0;

    // Select units:
    double conv_unit = aUnitsMM ? conv_unit_mm : conv_unit_inch;
    const char *unit_text = aUnitsMM ? unit_text_mm : unit_text_inch;

    // Build and sort the list of footprints alphabetically
    std::vector<LIST_MOD> list;
    list.reserve( footprintCount );

    for( footprint = GetBoard()->m_Modules; footprint; footprint = footprint->Next() )
    {
        if( aSide != PCB_BOTH_SIDES )
        {
            if( footprint->GetLayer() == B_Cu && aSide == PCB_FRONT_SIDE)
                continue;
            if( footprint->GetLayer() == F_Cu && aSide == PCB_BACK_SIDE)
                continue;
        }

        if( footprint->GetAttributes() & MOD_VIRTUAL )
        {
            DBG( printf( "skipping footprint %s because it's virtual\n",
                         TO_UTF8( footprint->GetReference() ) );)
            continue;
        }

        if( ( footprint->GetAttributes() & MOD_CMS ) == 0 )
        {
            if( aForceSmdItems )    // true to fix a bunch of mis-labeled footprints:
            {
                if( !HasNonSMDPins( footprint ) )
                {
                    // all footprint's pins are SMD, mark the part for pick and place
                    footprint->SetAttributes( footprint->GetAttributes() | MOD_CMS );
                    OnModify();
                }
                else
                {
                    DBG(printf( "skipping %s because its attribute is not CMS and it has non SMD pins\n",
                                TO_UTF8(footprint->GetReference()) ) );
                    continue;
                }
            }
            else
                continue;
        }

        footprintCount++;

        LIST_MOD item;
        item.m_Module    = footprint;
        item.m_Reference = footprint->GetReference();
        item.m_Value     = footprint->GetValue();
        item.m_Layer     = footprint->GetLayer();
        list.push_back( item );

        lenRefText = std::max( lenRefText, int(item.m_Reference.length()) );
        lenValText = std::max( lenValText, int(item.m_Value.length()) );
        lenPkgText = std::max( lenPkgText, int(item.m_Module->GetFPID().GetFootprintName().length()) );
    }

    if( aFullFileName.IsEmpty() )
        return footprintCount;

    FILE * file = wxFopen( aFullFileName, wxT( "wt" ) );
    if( file == NULL )
        return -1;

    if( list.size() > 1 )
        sort( list.begin(), list.end(), sortFPlist );

    // Switch the locale to standard C (needed to print floating point numbers)
    LOCALE_IO   toggle;

    if( aFormatCSV )
    {
        wxChar csv_sep = ',';

        // Set first line:;
        fprintf( file, "Ref%cVal%cPackage%cPosX%cPosY%cRot%cSide\n",
                 csv_sep, csv_sep, csv_sep, csv_sep, csv_sep, csv_sep );

        for( int ii = 0; ii < footprintCount; ii++ )
        {
            wxPoint  footprint_pos;
            footprint_pos  = list[ii].m_Module->GetPosition();
            footprint_pos -= File_Place_Offset;

            LAYER_NUM layer = list[ii].m_Module->GetLayer();
            wxASSERT( layer == F_Cu || layer == B_Cu );

            wxString line = list[ii].m_Reference;
            line << csv_sep;
            line << list[ii].m_Value;
            line << csv_sep;
            line << wxString( list[ii].m_Module->GetFPID().GetFootprintName() );
            line << csv_sep;

            line << wxString::Format( "%f%c%f%c%f",
                                    footprint_pos.x * conv_unit, csv_sep,
                                    // Keep the Y axis oriented from bottom to top,
                                    // ( change y coordinate sign )
                                    -footprint_pos.y * conv_unit, csv_sep,
                                    list[ii].m_Module->GetOrientation() / 10.0 );
            line << csv_sep;

            line << ( (layer == F_Cu ) ? frontSideName : backSideName );
            line << '\n';

            fputs( TO_UTF8( line ), file );
        }
    }
    else
    {
        // Write file header
        fprintf( file, "### Module positions - created on %s ###\n", TO_UTF8( DateAndTime() ) );

        wxString Title = Pgm().App().GetAppName() + wxT( " " ) + GetBuildVersion();
        fprintf( file, "### Printed by Pcbnew version %s\n", TO_UTF8( Title ) );

        fputs( unit_text, file );

        fputs( "## Side : ", file );

        if( aSide == PCB_BACK_SIDE )
            fputs( TO_UTF8( backSideName ), file );
        else if( aSide == PCB_FRONT_SIDE )
            fputs( TO_UTF8( frontSideName ), file );
        else
            fputs( "All", file );

        fputs( "\n", file );

        fprintf(file, "%-*s  %-*s  %-*s  %9.9s  %9.9s  %8.8s  %s\n",
                int(lenRefText), "# Ref",
                int(lenValText), "Val",
                int(lenPkgText), "Package",
                "PosX", "PosY", "Rot", "Side" );

        for( int ii = 0; ii < footprintCount; ii++ )
        {
            wxPoint  footprint_pos;
            footprint_pos  = list[ii].m_Module->GetPosition();
            footprint_pos -= File_Place_Offset;

            LAYER_NUM layer = list[ii].m_Module->GetLayer();
            wxASSERT( layer == F_Cu || layer == B_Cu );

            const wxString& ref = list[ii].m_Reference;
            const wxString& val = list[ii].m_Value;
            const wxString& pkg = list[ii].m_Module->GetFPID().GetFootprintName();

            fprintf(file, "%-*s  %-*s  %-*s  %9.4f  %9.4f  %8.4f  %s\n",
                    lenRefText, TO_UTF8( ref ),
                    lenValText, TO_UTF8( val ),
                    lenPkgText, TO_UTF8( pkg ),
                    footprint_pos.x * conv_unit,
                    // Keep the coordinates in the first quadrant,
                    // (i.e. change y sign
                    -footprint_pos.y * conv_unit,
                    list[ii].m_Module->GetOrientation() / 10.0,
                    (layer == F_Cu ) ? TO_UTF8( frontSideName ) : TO_UTF8( backSideName ));
        }

        // Write EOF
        fputs( "## End\n", file );
    }

    fclose( file );
    return footprintCount;
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

    bool unitMM = g_UserUnit != INCHES;
    bool success = DoGenFootprintsReport( fn.GetFullPath(), unitMM );

    wxString msg;
    if( success )
    {
        msg.Printf( _( "Footprint report file created:\n'%s'" ),
                    GetChars( fn.GetFullPath() ) );
        wxMessageBox( msg, _( "Footprint Report" ), wxICON_INFORMATION );
    }

    else
    {
        msg.Printf( _( "Unable to create '%s'" ), GetChars( fn.GetFullPath() ) );
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

    File_Place_Offset = wxPoint( 0, 0 );

    rptfile = wxFopen( aFullFilename, wxT( "wt" ) );

    if( rptfile == NULL )
        return false;

    // Select units:
    double conv_unit = aUnitsMM ? conv_unit_mm : conv_unit_inch;
    const char *unit_text = aUnitsMM ? unit_text_mm : unit_text_inch;

    LOCALE_IO   toggle;

    // Generate header file comments.)
    fprintf( rptfile, "## Footprint report - date %s\n", TO_UTF8( DateAndTime() ) );

    wxString Title = Pgm().App().GetAppName() + wxT( " " ) + GetBuildVersion();
    fprintf( rptfile, "## Created by Pcbnew version %s\n", TO_UTF8( Title ) );
    fputs( unit_text, rptfile );

    fputs( "\n$BeginDESCRIPTION\n", rptfile );

    EDA_RECT bbbox = GetBoard()->ComputeBoundingBox();

    fputs( "\n$BOARD\n", rptfile );

    fprintf( rptfile, "upper_left_corner %9.6f %9.6f\n",
             bbbox.GetX() * conv_unit,
             bbbox.GetY() * conv_unit );

    fprintf( rptfile, "lower_right_corner %9.6f %9.6f\n",
             bbbox.GetRight()  * conv_unit,
             bbbox.GetBottom() * conv_unit );

    fputs( "$EndBOARD\n\n", rptfile );

    for( MODULE* Module = GetBoard()->m_Modules;  Module;  Module = Module->Next() )
    {
        fprintf( rptfile, "$MODULE %s\n", EscapedUTF8( Module->GetReference() ).c_str() );

        fprintf( rptfile, "reference %s\n", EscapedUTF8( Module->GetReference() ).c_str() );
        fprintf( rptfile, "value %s\n", EscapedUTF8( Module->GetValue() ).c_str() );
        fprintf( rptfile, "footprint %s\n",
                 EscapedUTF8( FROM_UTF8( Module->GetFPID().Format().c_str() ) ).c_str() );

        msg = wxT( "attribut" );

        if( Module->GetAttributes() & MOD_VIRTUAL )
            msg += wxT( " virtual" );

        if( Module->GetAttributes() & MOD_CMS )
            msg += wxT( " smd" );

        if( ( Module->GetAttributes() & (MOD_VIRTUAL | MOD_CMS) ) == 0 )
            msg += wxT( " none" );

        msg += wxT( "\n" );
        fputs( TO_UTF8( msg ), rptfile );

        module_pos    = Module->GetPosition();
        module_pos.x -= File_Place_Offset.x;
        module_pos.y -= File_Place_Offset.y;

        fprintf( rptfile, "position %9.6f %9.6f  orientation %.2f\n",
                 module_pos.x * conv_unit,
                 module_pos.y * conv_unit,
                 Module->GetOrientation() / 10.0 );

        if( Module->GetLayer() == F_Cu )
            fputs( "layer front\n", rptfile );
        else if( Module->GetLayer() == B_Cu )
            fputs( "layer back\n", rptfile );
        else
            fputs( "layer other\n", rptfile );

        for( D_PAD* pad = Module->Pads(); pad != NULL; pad = pad->Next() )
        {
            fprintf( rptfile, "$PAD \"%s\"\n", TO_UTF8( pad->GetPadName() ) );
            int layer = 0;

            if( pad->GetLayerSet()[B_Cu] )
                layer = 1;

            if( pad->GetLayerSet()[F_Cu] )
                layer |= 2;

            static const char* layer_name[4] = { "nocopper", "back", "front", "both" };
            fprintf( rptfile, "Shape %s Layer %s\n", TO_UTF8( pad->ShowPadShape() ), layer_name[layer] );

            fprintf( rptfile, "position %9.6f %9.6f  size %9.6f %9.6f  orientation %.2f\n",
                     pad->GetPos0().x * conv_unit, pad->GetPos0().y * conv_unit,
                     pad->GetSize().x * conv_unit, pad->GetSize().y * conv_unit,
                     (pad->GetOrientation() - Module->GetOrientation()) / 10.0 );

            fprintf( rptfile, "drill %9.6f\n", pad->GetDrillSize().x * conv_unit );

            fprintf( rptfile, "shape_offset %9.6f %9.6f\n",
                     pad->GetOffset().x * conv_unit,
                     pad->GetOffset().y * conv_unit );

            fprintf( rptfile, "$EndPAD\n" );
        }

        fprintf( rptfile, "$EndMODULE  %s\n\n", TO_UTF8 (Module->GetReference() ) );
    }

    // Generate EOF.
    fputs( "$EndDESCRIPTION\n", rptfile );
    fclose( rptfile );

    return true;
}

