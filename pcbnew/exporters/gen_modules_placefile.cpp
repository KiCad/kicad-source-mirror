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
#include <trigo.h>
#include <pgm_base.h>
#include <build_version.h>
#include <macros.h>
#include <reporter.h>

#include <class_board.h>
#include <class_module.h>
#include <class_drawsegment.h>
#include <legacy_plugin.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <pcb_plot_params.h>
#include <wildcards_and_files_ext.h>
#include <kiface_i.h>

#include <dialog_gen_module_position_file_base.h>
/*
 * The format of the kicad place file is:
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
 *      # Ref    Val                  Package         PosX       PosY        Rot     Side
 *      C123     0,1uF/50V        SM0603              1.6024    -2.6280     180.0    Front
 *      C124     0,1uF/50V        SM0603              1.6063    -2.7579     180.0    Front
 *      C125     0,1uF/50V        SM0603              1.6010    -2.8310     180.0    Front
 *      ## End
 */

#define PLACEFILE_UNITS_KEY wxT( "PlaceFileUnits" )
#define PLACEFILE_OPT_KEY   wxT( "PlaceFileOpts" )


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
        initDialog();

        GetSizer()->SetSizeHints(this);
        Centre();
    }

private:
    PCB_EDIT_FRAME* m_parent;
    PCB_PLOT_PARAMS m_plotOpts;
    wxConfigBase* m_config;

    static int m_unitsOpt;
    static int m_fileOpt;

    void initDialog();
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event );
    void OnOKButton( wxCommandEvent& event );

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

    void AddMessage( const wxString & aMessage )
    {
        m_messagesBox->AppendText( aMessage );
    }
};


// Static members to remember choices
int DIALOG_GEN_MODULE_POSITION::m_unitsOpt = 0;
int DIALOG_GEN_MODULE_POSITION::m_fileOpt = 0;

// Use standard board side name. do not translate them,
// they are keywords in place file
const wxString frontSideName = wxT( "top" );
const wxString backSideName = wxT( "bottom" );

void DIALOG_GEN_MODULE_POSITION::initDialog()
{
    m_config = Kiface().KifaceSettings();
    m_config->Read( PLACEFILE_UNITS_KEY, &m_unitsOpt, 1 );
    m_config->Read( PLACEFILE_OPT_KEY, &m_fileOpt, 0 );

    // Output directory
    m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );
    m_radioBoxUnits->SetSelection( m_unitsOpt );
    m_radioBoxFilesCount->SetSelection( m_fileOpt );

    m_sdbSizerButtonsOK->SetDefault();
}

void DIALOG_GEN_MODULE_POSITION::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxFileName  fn( m_outputDirectoryName->GetValue() );
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

    m_config->Write( PLACEFILE_UNITS_KEY, m_unitsOpt );
    m_config->Write( PLACEFILE_OPT_KEY, m_fileOpt );

    // Set output directory and replace backslashes with forward ones
    // (Keep unix convention in cfg files)
    wxString dirStr;
    dirStr = m_outputDirectoryName->GetValue();
    dirStr.Replace( wxT( "\\" ), wxT( "/" ) );

    m_plotOpts.SetOutputDirectory( dirStr );

    m_parent->SetPlotSettings( m_plotOpts );

    CreateFiles();
}


bool DIALOG_GEN_MODULE_POSITION::CreateFiles()
{
    BOARD * brd = m_parent->GetBoard();
    wxFileName  fn;
    wxString    msg;
    bool singleFile = OneFileOnly();
    int fullcount = 0;

    // Count the footprints to place, do not yet create a file
    int fpcount = m_parent->DoGenFootprintsPositionFile( wxEmptyString, UnitsMM(),
                                                         ForceAllSmd(), 2 );
    if( fpcount == 0)
    {
        wxMessageBox( _( "No modules for automated placement." ) );
        return false;
    }

    // Create output directory if it does not exist (also transform it in
    // absolute form). Bail if it fails
    wxFileName  outputDir = wxFileName::DirName( m_plotOpts.GetOutputDirectory() );
    wxString    boardFilename = m_parent->GetBoard()->GetFileName();
    WX_TEXT_CTRL_REPORTER reporter( m_messagesBox );

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, &reporter ) )
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
    int side = 1;

    if( singleFile )
    {
        side = 2;
        fn.SetName( fn.GetName() + wxT( "-" ) + wxT("all") );
    }
    else
        fn.SetName( fn.GetName() + wxT( "-" ) + frontSideName );

    fn.SetExt( FootprintPlaceFileExtension );

    fpcount = m_parent->DoGenFootprintsPositionFile( fn.GetFullPath(), UnitsMM(),
                                                     ForceAllSmd(), side );
    if( fpcount < 0 )
    {
        msg.Printf( _( "Unable to create '%s'" ), GetChars( fn.GetFullPath() ) );
        wxMessageBox( msg );
        AddMessage( msg + wxT("\n") );
        return false;
    }

    if( singleFile  )
        msg.Printf( _( "Place file: '%s'\n" ), GetChars( fn.GetFullPath() ) );
    else
        msg.Printf( _( "Front side (top side) place file: '%s'\n" ),
                    GetChars( fn.GetFullPath() ) );

    AddMessage( msg );
    msg.Printf( _( "Footprint count %d\n" ), fpcount );
    AddMessage( msg );

    if( singleFile  )
        return true;

    // Create the Back or Bottom side placement file
    fullcount = fpcount;
    side = 0;
    fn = brd->GetFileName();
    fn.SetPath( outputDir.GetPath() );
    fn.SetName( fn.GetName() + wxT( "-" ) + backSideName );
    fn.SetExt( wxT( "pos" ) );

    fpcount = m_parent->DoGenFootprintsPositionFile( fn.GetFullPath(), UnitsMM(),
                                                    ForceAllSmd(), side );

    if( fpcount < 0 )
    {
        msg.Printf( _( "Unable to create '%s'" ), GetChars( fn.GetFullPath() ) );
        AddMessage( msg + wxT("\n") );
        wxMessageBox( msg );
        return false;
    }

    // Display results
    if( !singleFile )
    {
        msg.Printf( _( "Back side (bottom side) place file: '%s'\n" ), GetChars( fn.GetFullPath() ) );
        AddMessage( msg );
        msg.Printf( _( "Footprint count %d\n" ), fpcount );
        AddMessage( msg );
    }

    if( !singleFile )
    {
        fullcount += fpcount;
        msg.Printf( _( "Full footprint count %d\n" ), fullcount );
        AddMessage( msg );
    }

    return true;
}

// Defined values to write coordinates using inches or mm:
static const double conv_unit_inch = 0.001 / IU_PER_MILS ;      // units = INCHES
static const char unit_text_inch[] = "## Unit = inches, Angle = deg.\n";

static const double conv_unit_mm = 1.0 / IU_PER_MM;    // units = mm
static const char unit_text_mm[] = "## Unit = mm, Angle = deg.\n";

static wxPoint File_Place_Offset;  // Offset coordinates for generated file.

static void WriteDrawSegmentPcb( DRAWSEGMENT* PtDrawSegment, FILE* rptfile,
                                 double aConvUnit );


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
        if( pad->GetAttribute() != PAD_SMD )
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
                                                 bool aForceSmdItems, int aSide )
{
    MODULE*     module;
    char        line[1024];

    File_Place_Offset = GetAuxOrigin();

    // Calculating the number of useful modules (CMS attribute, not VIRTUAL)
    int moduleCount = 0;

    for( module = GetBoard()->m_Modules;  module;  module = module->Next() )
    {
        if( aSide < 2 )
        {
            if( module->GetLayer() == B_Cu && aSide == 1)
                continue;
            if( module->GetLayer() == F_Cu && aSide == 0)
                continue;
        }

        if( module->GetAttributes() & MOD_VIRTUAL )
        {
            DBG( printf( "skipping module %s because it's virtual\n",
                       TO_UTF8( module->GetReference() ) );)
            continue;
        }

        if( ( module->GetAttributes() & MOD_CMS ) == 0 )
        {
            if( aForceSmdItems )    // true to fix a bunch of mis-labeled modules:
            {
                if( !HasNonSMDPins( module ) )
                {
                    // all module's pins are SMD, mark the part for pick and place
                    module->SetAttributes( module->GetAttributes() | MOD_CMS );
                    OnModify();
                }
                else
                {
                    DBG(printf( "skipping %s because its attribute is not CMS and it has non SMD pins\n",
                              TO_UTF8(module->GetReference()) ) );
                    continue;
                }
            }
            else
                continue;
        }

        moduleCount++;
    }

    if( aFullFileName.IsEmpty() )
        return moduleCount;

    FILE * file = wxFopen( aFullFileName, wxT( "wt" ) );
    if( file == NULL )
        return -1;

    // Select units:
    double conv_unit = aUnitsMM ? conv_unit_mm : conv_unit_inch;
    const char *unit_text = aUnitsMM ? unit_text_mm : unit_text_inch;

    // Build and sort the list of modules alphabetically
    std::vector<LIST_MOD> list;
    list.reserve(moduleCount);
    for( module = GetBoard()->m_Modules; module; module = module->Next() )
    {
        if( aSide < 2 )
        {
            if( module->GetLayer() == B_Cu && aSide == 1)
                continue;
            if( module->GetLayer() == F_Cu && aSide == 0)
                continue;
        }

        if( module->GetAttributes() & MOD_VIRTUAL )
            continue;

        if( (module->GetAttributes() & MOD_CMS) == 0 )
            continue;

        LIST_MOD item;
        item.m_Module    = module;
        item.m_Reference = module->GetReference();
        item.m_Value     = module->GetValue();
        item.m_Layer     = module->GetLayer();
        list.push_back( item );
    }

    if( list.size() > 1 )
        sort( list.begin(), list.end(), sortFPlist );

    // Switch the locale to standard C (needed to print floating point numbers)
    LOCALE_IO   toggle;

    // Write file header
    sprintf( line, "### Module positions - created on %s ###\n", TO_UTF8( DateAndTime() ) );
    fputs( line, file );

    wxString Title = Pgm().App().GetAppName() + wxT( " " ) + GetBuildVersion();
    sprintf( line, "### Printed by Pcbnew version %s\n", TO_UTF8( Title ) );
    fputs( line, file );

    fputs( unit_text, file );

    fputs( "## Side : ", file );

    if( aSide == 0 )
        fputs( TO_UTF8( backSideName ), file );
    else if( aSide == 1 )
        fputs( TO_UTF8( frontSideName ), file );
    else
        fputs( "All", file );

    fputs( "\n", file );

    fputs( "# Ref    Val                  Package         PosX       PosY        Rot     Side\n",
           file );

    for( int ii = 0; ii < moduleCount; ii++ )
    {
        wxPoint  module_pos;

        const wxString& ref = list[ii].m_Reference;
        const wxString& val = list[ii].m_Value;
        const wxString& pkg = list[ii].m_Module->GetFPID().GetFootprintName();

        sprintf( line, "%-8.8s %-16.16s %-16.16s",
                 TO_UTF8( ref ), TO_UTF8( val ), TO_UTF8( pkg ) );

        module_pos  = list[ii].m_Module->GetPosition();
        module_pos -= File_Place_Offset;

        char* text = line + strlen( line );
        /* Keep the coordinates in the first quadrant, like the gerbers
         * (i.e. change sign to y) */
        sprintf( text, " %9.4f  %9.4f  %8.1f    ",
                 module_pos.x * conv_unit,
                 -module_pos.y * conv_unit,
                 list[ii].m_Module->GetOrientation() / 10.0 );

        LAYER_NUM layer = list[ii].m_Module->GetLayer();

        fputs( line, file );

        wxASSERT( layer==F_Cu || layer==B_Cu );

        if( layer == F_Cu )
            fputs( TO_UTF8( frontSideName ), file );
        else if( layer == B_Cu )
            fputs( TO_UTF8( backSideName ), file );

        fputs( "\n", file );
    }

    // Write EOF
    fputs( "## End\n", file );

    fclose( file );
    return moduleCount;
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

    bool success = DoGenFootprintsReport( fn.GetFullPath(), false );

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
    D_PAD*   pad;
    char     line[1024];
    wxString fnFront, msg;
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
    sprintf( line, "## Module report - date %s\n", TO_UTF8( DateAndTime() ) );
    fputs( line, rptfile );

    wxString Title = Pgm().App().GetAppName() + wxT( " " ) + GetBuildVersion();
    sprintf( line, "## Created by Pcbnew version %s\n", TO_UTF8( Title ) );
    fputs( line, rptfile );
    fputs( unit_text, rptfile );

    fputs( "##\n", rptfile );
    fputs( "\n$BeginDESCRIPTION\n", rptfile );

    EDA_RECT bbbox = GetBoard()->ComputeBoundingBox();

    fputs( "\n$BOARD\n", rptfile );
    fputs( "unit INCH\n", rptfile );

    sprintf( line, "upper_left_corner %9.6f %9.6f\n",
             bbbox.GetX() * conv_unit,
             bbbox.GetY() * conv_unit );

    fputs( line, rptfile );

    sprintf( line, "lower_right_corner %9.6f %9.6f\n",
             bbbox.GetRight()  * conv_unit,
             bbbox.GetBottom() * conv_unit );
    fputs( line, rptfile );

    fputs( "$EndBOARD\n\n", rptfile );

    try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        LEGACY_PLUGIN* legacy = (LEGACY_PLUGIN*) (PLUGIN*) pi;

        legacy->SetFilePtr( rptfile );

        for( MODULE* Module = GetBoard()->m_Modules;  Module;  Module = Module->Next() )
        {
            sprintf( line, "$MODULE %s\n", EscapedUTF8( Module->GetReference() ).c_str() );
            fputs( line, rptfile );

            sprintf( line, "reference %s\n", EscapedUTF8( Module->GetReference() ).c_str() );
            fputs( line, rptfile );
            sprintf( line, "value %s\n", EscapedUTF8( Module->GetValue() ).c_str() );
            fputs( line, rptfile );
            sprintf( line, "footprint %s\n",
                     EscapedUTF8( FROM_UTF8( Module->GetFPID().Format().c_str() ) ).c_str() );
            fputs( line, rptfile );

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

            sprintf( line, "position %9.6f %9.6f\n",
                     module_pos.x * conv_unit,
                     module_pos.y * conv_unit );
            fputs( line, rptfile );

            sprintf( line, "orientation  %.2f\n", Module->GetOrientation() / 10.0 );

            if( Module->GetLayer() == F_Cu )
                strcat( line, "layer component\n" );
            else if( Module->GetLayer() == B_Cu )
                strcat( line, "layer copper\n" );
            else
                strcat( line, "layer other\n" );

            fputs( line, rptfile );

            legacy->SaveModule3D( Module );

            for( pad = Module->Pads(); pad != NULL; pad = pad->Next() )
            {
                fprintf( rptfile, "$PAD \"%s\"\n", TO_UTF8( pad->GetPadName() ) );
                sprintf( line, "position %9.6f %9.6f\n",
                         pad->GetPos0().x * conv_unit,
                         pad->GetPos0().y * conv_unit );
                fputs( line, rptfile );

                sprintf( line, "size %9.6f %9.6f\n",
                         pad->GetSize().x * conv_unit,
                         pad->GetSize().y * conv_unit );
                fputs( line, rptfile );

                sprintf( line, "drill %9.6f\n", pad->GetDrillSize().x * conv_unit );
                fputs( line, rptfile );

                sprintf( line, "shape_offset %9.6f %9.6f\n",
                         pad->GetOffset().x * conv_unit,
                         pad->GetOffset().y * conv_unit );
                fputs( line, rptfile );

                sprintf( line, "orientation  %.2f\n",
                         (pad->GetOrientation() - Module->GetOrientation()) / 10.0 );
                fputs( line, rptfile );

                static const char* shape_name[6] = { "???", "Circ", "Rect", "Oval", "Trap", "Spec" };

                sprintf( line, "Shape  %s\n", shape_name[pad->GetShape()] );
                fputs( line, rptfile );

                int layer = 0;

                if( pad->GetLayerSet()[B_Cu] )
                    layer = 1;

                if( pad->GetLayerSet()[F_Cu] )
                    layer |= 2;

                static const char* layer_name[4] = { "none", "back", "front", "both" };

                sprintf( line, "Layer  %s\n", layer_name[layer] );
                fputs( line, rptfile );
                fprintf( rptfile, "$EndPAD\n" );
            }

            fprintf( rptfile, "$EndMODULE  %s\n\n", TO_UTF8 (Module->GetReference() ) );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( NULL, ioe.errorText );
    }

    // Write board Edges
    EDA_ITEM* PtStruct;

    for( PtStruct = GetBoard()->m_Drawings; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        if( PtStruct->Type() != PCB_LINE_T )
            continue;

        if( ( (DRAWSEGMENT*) PtStruct )->GetLayer() != Edge_Cuts )
            continue;

        WriteDrawSegmentPcb( (DRAWSEGMENT*) PtStruct, rptfile, conv_unit );
    }

    // Generate EOF.
    fputs( "$EndDESCRIPTION\n", rptfile );
    fclose( rptfile );

    return true;
}


/* Output to rpt file a segment type from the PCB drawing.
 * The contours are of different types:
 * Segment
 * Circle
 * Arc
 */
void WriteDrawSegmentPcb( DRAWSEGMENT* PtDrawSegment, FILE* rptfile, double aConvUnit )
{
    double ux0, uy0, dx, dy;
    double radius, width;
    char   line[1024];

    ux0 = PtDrawSegment->GetStart().x * aConvUnit;
    uy0 = PtDrawSegment->GetStart().y * aConvUnit;

    dx = PtDrawSegment->GetEnd().x * aConvUnit;
    dy = PtDrawSegment->GetEnd().y * aConvUnit;

    width = PtDrawSegment->GetWidth() * aConvUnit;

    switch( PtDrawSegment->GetShape() )
    {
    case S_CIRCLE:
        radius = Distance( ux0, uy0, dx, dy );
        fprintf( rptfile, "$CIRCLE \n" );
        fprintf( rptfile, "centre %.6lf %.6lf\n", ux0, uy0 );
        fprintf( rptfile, "radius %.6lf\n", radius );
        fprintf( rptfile, "width %.6lf\n", width );
        fprintf( rptfile, "$EndCIRCLE \n" );
        break;

    case S_ARC:
        {
            int endx = PtDrawSegment->GetEnd().x;
            int endy = PtDrawSegment->GetEnd().y;

            radius = Distance( ux0, uy0, dx, dy );
            RotatePoint( &endx,
                         &endy,
                         PtDrawSegment->GetStart().x,
                         PtDrawSegment->GetStart().y,
                         PtDrawSegment->GetAngle() );

            fprintf( rptfile, "$ARC \n" );
            fprintf( rptfile, "centre %.6lf %.6lf\n", ux0, uy0 );
            fprintf( rptfile, "start %.6lf %.6lf\n",
                     endx * aConvUnit, endy * aConvUnit );
            fprintf( rptfile, "end %.6lf %.6lf\n", dx, dy );
            fprintf( rptfile, "width %.6lf\n", width );
            fprintf( rptfile, "$EndARC \n" );
        }
        break;

    default:
        sprintf( line, "$LINE \n" );
        fputs( line, rptfile );

        fprintf( rptfile, "start %.6lf %.6lf\n", ux0, uy0 );
        fprintf( rptfile, "end %.6lf %.6lf\n", dx, dy );
        fprintf( rptfile, "width %.6lf\n", width );
        fprintf( rptfile, "$EndLINE \n" );
        break;
    }
}
