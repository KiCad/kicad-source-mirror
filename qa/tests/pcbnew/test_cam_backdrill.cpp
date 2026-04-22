/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <board.h>
#include <pcb_shape.h>
#include <pcbnew/exporters/gendrill_excellon_writer.h>
#include <pcbnew/exporters/gendrill_gerber_writer.h>
#include <pcbnew/pcb_io/odbpp/pcb_io_odbpp.h>
#include <pcbnew/pcb_track.h>
#include <base_units.h>

#include <map>

#include <core/utf8.h>

#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>


namespace
{
wxFileName MakeTempDir()
{
    wxFileName tempDir( wxFileName::GetTempDir(), wxEmptyString );
    tempDir.AppendDir( wxString::Format( "kicad-backdrill-%llu",
                                         static_cast<unsigned long long>( wxGetUTCTime() ) ) );
    BOOST_REQUIRE( tempDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    return tempDir;
}
} // anonymous namespace


BOOST_AUTO_TEST_CASE( BackdrillCamOutputs )
{
    wxFileName tempDir = MakeTempDir();
    wxFileName boardFile( tempDir.GetFullPath(), wxT( "backdrill_board.kicad_pcb" ) );

    BOARD board;
    board.SetCopperLayerCount( 6 );
    board.SetFileName( boardFile.GetFullPath() );

    auto via = new PCB_VIA( &board );
    via->SetPosition( VECTOR2I( 0, 0 ) );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetDrill( pcbIUScale.mmToIU( 0.30 ) );
    via->SetWidth( pcbIUScale.mmToIU( 0.60 ) );
    via->SetSecondaryDrillSize( pcbIUScale.mmToIU( 0.20 ) );
    via->SetSecondaryDrillStartLayer( F_Cu );
    via->SetSecondaryDrillEndLayer( In3_Cu );
    via->SetFrontPostMachiningMode( PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK );
    via->SetFrontPostMachiningSize( pcbIUScale.mmToIU( 0.60 ) );
    via->SetFrontPostMachiningDepth( pcbIUScale.mmToIU( 0.15 ) );
    via->SetFrontPostMachiningAngle( 900 );
    board.Add( via );

    EXCELLON_WRITER excellon( &board );
    excellon.SetOptions( false, false, VECTOR2I( 0, 0 ), false );
    excellon.SetFormat( true );
    BOOST_REQUIRE( excellon.CreateDrillandMapFilesSet( tempDir.GetFullPath(), true, false, nullptr ) );

    wxFileName excellonFile( tempDir.GetFullPath(), wxT( "backdrill_board_Backdrills_Drill_1_4.drl" ) );
    BOOST_REQUIRE( excellonFile.FileExists() );

    wxFFile excellonStream( excellonFile.GetFullPath(), wxT( "rb" ) );
    wxString excellonContents;
    BOOST_REQUIRE( excellonStream.ReadAll( &excellonContents ) );
    BOOST_CHECK( excellonContents.Contains( wxT( "TF.FileFunction,NonPlated,1,4,Blind" ) ) );
    BOOST_CHECK( excellonContents.Contains( wxT( "; Backdrill" ) ) );
    BOOST_CHECK( excellonContents.Contains( wxT( "post-machining" ) ) );

    wxFileName layerPairFile( tempDir.GetFullPath(), wxT( "backdrill_board-front-in3-backdrill.drl" ) );
    BOOST_REQUIRE( layerPairFile.FileExists() );

    wxFFile layerPairStream( layerPairFile.GetFullPath(), wxT( "rb" ) );
    wxString layerPairContents;
    BOOST_REQUIRE( layerPairStream.ReadAll( &layerPairContents ) );
    BOOST_CHECK( layerPairContents.Contains( wxT( "; backdrill" ) ) );

    wxFileName pthFile( tempDir.GetFullPath(), wxT( "backdrill_board-PTH.drl" ) );
    BOOST_REQUIRE( pthFile.FileExists() );

    wxFFile pthStream( pthFile.GetFullPath(), wxT( "rb" ) );
    wxString pthContents;
    BOOST_REQUIRE( pthStream.ReadAll( &pthContents ) );
    BOOST_CHECK( pthContents.Contains( wxT( "; Post-machining front countersink dia 0.600mm depth 0.150mm angle 90deg" ) ) );

    GERBER_WRITER gerber( &board );
    gerber.SetOptions( VECTOR2I( 0, 0 ) );
    gerber.SetFormat( 6 );
    BOOST_REQUIRE( gerber.CreateDrillandMapFilesSet( tempDir.GetFullPath(), true, false, false, nullptr ) );

    wxFileName gerberFile( tempDir.GetFullPath(), wxT( "backdrill_board_Backdrills_Drill_1_4-drl.gbr" ) );
    BOOST_REQUIRE( gerberFile.FileExists() );

    wxFFile gerberStream( gerberFile.GetFullPath(), wxT( "rb" ) );
    wxString gerberContents;
    BOOST_REQUIRE( gerberStream.ReadAll( &gerberContents ) );
    BOOST_CHECK( gerberContents.Contains( wxT( "%TA.AperFunction,BackDrill*%" ) ) );
    BOOST_CHECK( gerberContents.Contains( wxT( "%TF.FileFunction,NonPlated,1,4,Blind,Drill*%" ) ) );

    wxFileName gerberLayerPairFile( tempDir.GetFullPath(),
                                    wxT( "backdrill_board-front-in3-backdrill-drl.gbr" ) );
    BOOST_REQUIRE( gerberLayerPairFile.FileExists() );

    wxFFile gerberLayerPairStream( gerberLayerPairFile.GetFullPath(), wxT( "rb" ) );
    wxString gerberLayerPairContents;
    BOOST_REQUIRE( gerberLayerPairStream.ReadAll( &gerberLayerPairContents ) );
    BOOST_CHECK( gerberLayerPairContents.Contains( wxT( "%TF.FileFunction,NonPlated,1,4,Blind,Drill*%" ) ) );
    BOOST_CHECK( gerberLayerPairContents.Contains( wxT( "%TA.AperFunction,BackDrill*%" ) ) );

    wxFileName odbRoot( tempDir.GetFullPath(), wxEmptyString );
    odbRoot.AppendDir( wxT( "odb_out" ) );
    BOOST_REQUIRE( odbRoot.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    PCB_IO_ODBPP odbExporter;
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["sigfig"] = "4";
    BOOST_REQUIRE_NO_THROW( odbExporter.SaveBoard( odbRoot.GetFullPath(), &board, &props ) );

    wxFileName drill1Dir( odbRoot.GetFullPath(), wxEmptyString );
    drill1Dir.AppendDir( wxT( "steps" ) );
    drill1Dir.AppendDir( wxT( "pcb" ) );
    drill1Dir.AppendDir( wxT( "layers" ) );
    drill1Dir.AppendDir( wxT( "drill1" ) );
    BOOST_REQUIRE( drill1Dir.DirExists() );

    wxFileName toolsFile( drill1Dir.GetFullPath(), wxT( "tools" ) );
    BOOST_REQUIRE( toolsFile.FileExists() );

    wxFFile toolsStream( toolsFile.GetFullPath(), wxT( "rb" ) );
    wxString toolsContents;
    BOOST_REQUIRE( toolsStream.ReadAll( &toolsContents ) );
    BOOST_CHECK( toolsContents.Contains( wxT( "TYPE=NON_PLATED" ) ) );
    BOOST_CHECK( toolsContents.Contains( wxT( "TYPE2=BLIND" ) ) );

    wxFileName matrixFile( odbRoot.GetFullPath(), wxEmptyString );
    matrixFile.AppendDir( wxT( "matrix" ) );
    matrixFile.SetFullName( wxT( "matrix" ) );
    BOOST_REQUIRE( matrixFile.FileExists() );

    wxFFile matrixStream( matrixFile.GetFullPath(), wxT( "rb" ) );
    wxString matrixContents;
    BOOST_REQUIRE( matrixStream.ReadAll( &matrixContents ) );
    BOOST_CHECK( matrixContents.Contains( wxT( "ADD_TYPE=BACKDRILL" ) ) );

    matrixStream.Close();
    toolsStream.Close();
    gerberStream.Close();
    gerberLayerPairStream.Close();
    excellonStream.Close();
    layerPairStream.Close();
    pthStream.Close();

    wxFileName::Rmdir( odbRoot.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
    wxFileName::Rmdir( tempDir.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23914
// Only front-side (secondary) backdrill holes were exported; back-side (tertiary)
// backdrill operations never produced a drill file.
BOOST_AUTO_TEST_CASE( FrontAndBackBackdrillCamOutputs )
{
    wxFileName tempDir = MakeTempDir();
    wxFileName boardFile( tempDir.GetFullPath(), wxT( "backdrill_pair_board.kicad_pcb" ) );

    BOARD board;
    board.SetCopperLayerCount( 6 );
    board.SetFileName( boardFile.GetFullPath() );

    auto topVia = new PCB_VIA( &board );
    topVia->SetPosition( VECTOR2I( 0, 0 ) );
    topVia->SetLayerPair( F_Cu, B_Cu );
    topVia->SetDrill( pcbIUScale.mmToIU( 0.30 ) );
    topVia->SetWidth( pcbIUScale.mmToIU( 0.60 ) );
    topVia->SetSecondaryDrillSize( pcbIUScale.mmToIU( 0.40 ) );
    topVia->SetSecondaryDrillStartLayer( F_Cu );
    topVia->SetSecondaryDrillEndLayer( In1_Cu );
    board.Add( topVia );

    auto bottomVia = new PCB_VIA( &board );
    bottomVia->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 5.0 ), 0 ) );
    bottomVia->SetLayerPair( F_Cu, B_Cu );
    bottomVia->SetDrill( pcbIUScale.mmToIU( 0.30 ) );
    bottomVia->SetWidth( pcbIUScale.mmToIU( 0.60 ) );
    bottomVia->SetTertiaryDrillSize( pcbIUScale.mmToIU( 0.40 ) );
    bottomVia->SetTertiaryDrillStartLayer( B_Cu );
    bottomVia->SetTertiaryDrillEndLayer( In3_Cu );
    board.Add( bottomVia );

    EXCELLON_WRITER excellon( &board );
    excellon.SetOptions( false, false, VECTOR2I( 0, 0 ), false );
    excellon.SetFormat( true );
    BOOST_REQUIRE( excellon.CreateDrillandMapFilesSet( tempDir.GetFullPath(), true, false,
                                                       nullptr ) );

    wxFileName topBackdrillFile( tempDir.GetFullPath(),
                                 wxT( "backdrill_pair_board_Backdrills_Drill_1_2.drl" ) );
    BOOST_CHECK_MESSAGE( topBackdrillFile.FileExists(),
                         "Front-side backdrill drill file should be produced" );

    // Start=B_Cu (UI index 6) drilled toward In3_Cu (UI index 4) in a 6-layer board
    wxFileName bottomBackdrillFile( tempDir.GetFullPath(),
                                    wxT( "backdrill_pair_board_Backdrills_Drill_6_4.drl" ) );
    BOOST_CHECK_MESSAGE( bottomBackdrillFile.FileExists(),
                         "Back-side (tertiary) backdrill drill file should be produced" );

    if( bottomBackdrillFile.FileExists() )
    {
        wxFFile stream( bottomBackdrillFile.GetFullPath(), wxT( "rb" ) );
        wxString contents;
        BOOST_REQUIRE( stream.ReadAll( &contents ) );
        BOOST_CHECK( contents.Contains( wxT( "; Backdrill" ) ) );
        stream.Close();
    }

    wxFileName::Rmdir( tempDir.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
}


// Stronger coverage for https://gitlab.com/kicad/code/kicad/-/issues/23914
// A single via can carry both a front-side (secondary) and a back-side
// (tertiary) backdrill. Both drill files must be produced.
BOOST_AUTO_TEST_CASE( DualBackdrillSameViaCamOutputs )
{
    wxFileName tempDir = MakeTempDir();
    wxFileName boardFile( tempDir.GetFullPath(), wxT( "dual_backdrill_board.kicad_pcb" ) );

    BOARD board;
    board.SetCopperLayerCount( 6 );
    board.SetFileName( boardFile.GetFullPath() );

    auto via = new PCB_VIA( &board );
    via->SetPosition( VECTOR2I( 0, 0 ) );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetDrill( pcbIUScale.mmToIU( 0.30 ) );
    via->SetWidth( pcbIUScale.mmToIU( 0.60 ) );
    via->SetSecondaryDrillSize( pcbIUScale.mmToIU( 0.40 ) );
    via->SetSecondaryDrillStartLayer( F_Cu );
    via->SetSecondaryDrillEndLayer( In1_Cu );
    via->SetTertiaryDrillSize( pcbIUScale.mmToIU( 0.40 ) );
    via->SetTertiaryDrillStartLayer( B_Cu );
    via->SetTertiaryDrillEndLayer( In3_Cu );
    board.Add( via );

    EXCELLON_WRITER excellon( &board );
    excellon.SetOptions( false, false, VECTOR2I( 0, 0 ), false );
    excellon.SetFormat( true );
    BOOST_REQUIRE( excellon.CreateDrillandMapFilesSet( tempDir.GetFullPath(), true, false,
                                                       nullptr ) );

    wxFileName topBackdrillFile( tempDir.GetFullPath(),
                                 wxT( "dual_backdrill_board_Backdrills_Drill_1_2.drl" ) );
    BOOST_CHECK( topBackdrillFile.FileExists() );

    wxFileName bottomBackdrillFile( tempDir.GetFullPath(),
                                    wxT( "dual_backdrill_board_Backdrills_Drill_6_4.drl" ) );
    BOOST_CHECK( bottomBackdrillFile.FileExists() );

    wxFileName::Rmdir( tempDir.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23451
// GERBER_WRITER::SetFormat() precision was not passed to the plotter; output always used 4.6.
BOOST_AUTO_TEST_CASE( GerberDrillPrecision )
{
    wxFileName tempDir = MakeTempDir();
    wxFileName boardFile( tempDir.GetFullPath(), wxT( "precision_board.kicad_pcb" ) );

    BOARD board;
    board.SetCopperLayerCount( 2 );
    board.SetFileName( boardFile.GetFullPath() );

    auto via = new PCB_VIA( &board );
    via->SetPosition( VECTOR2I( 0, 0 ) );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetDrill( pcbIUScale.mmToIU( 0.30 ) );
    via->SetWidth( pcbIUScale.mmToIU( 0.60 ) );
    board.Add( via );

    // Verify precision 5 produces "Fmt 4.5" in the file header
    GERBER_WRITER gerber5( &board );
    gerber5.SetOptions( VECTOR2I( 0, 0 ) );
    gerber5.SetFormat( 5 );
    BOOST_REQUIRE( gerber5.CreateDrillandMapFilesSet( tempDir.GetFullPath(), true, false, false, nullptr ) );

    wxFileName gerberFile5( tempDir.GetFullPath(), wxT( "precision_board-PTH-drl.gbr" ) );
    BOOST_REQUIRE( gerberFile5.FileExists() );

    wxFFile gerberStream5( gerberFile5.GetFullPath(), wxT( "rb" ) );
    wxString gerberContents5;
    BOOST_REQUIRE( gerberStream5.ReadAll( &gerberContents5 ) );
    BOOST_CHECK_MESSAGE( gerberContents5.Contains( wxT( "Fmt 4.5" ) ),
                         "Expected 'Fmt 4.5' in gerber header with precision=5" );
    BOOST_CHECK( !gerberContents5.Contains( wxT( "Fmt 4.6" ) ) );
    gerberStream5.Close();

    // Verify precision 6 produces "Fmt 4.6" in the file header
    GERBER_WRITER gerber6( &board );
    gerber6.SetOptions( VECTOR2I( 0, 0 ) );
    gerber6.SetFormat( 6 );
    BOOST_REQUIRE( gerber6.CreateDrillandMapFilesSet( tempDir.GetFullPath(), true, false, false, nullptr ) );

    wxFFile gerberStream6( gerberFile5.GetFullPath(), wxT( "rb" ) );
    wxString gerberContents6;
    BOOST_REQUIRE( gerberStream6.ReadAll( &gerberContents6 ) );
    BOOST_CHECK_MESSAGE( gerberContents6.Contains( wxT( "Fmt 4.6" ) ),
                         "Expected 'Fmt 4.6' in gerber header with precision=6" );
    BOOST_CHECK( !gerberContents6.Contains( wxT( "Fmt 4.5" ) ) );
    gerberStream6.Close();

    wxFileName::Rmdir( tempDir.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23005
// GenDrillReportFile crashed when aReporter was null (the default)
BOOST_AUTO_TEST_CASE( DrillReportNullReporter )
{
    wxFileName tempDir = MakeTempDir();
    wxFileName boardFile( tempDir.GetFullPath(), wxT( "test_board.kicad_pcb" ) );

    BOARD board;
    board.SetCopperLayerCount( 2 );
    board.SetFileName( boardFile.GetFullPath() );

    wxFileName reportFile( tempDir.GetFullPath(), wxT( "test_board-drl.rpt" ) );

    // Valid path with null reporter should succeed without crashing
    EXCELLON_WRITER excellon( &board );
    BOOST_CHECK( excellon.GenDrillReportFile( reportFile.GetFullPath() ) );
    BOOST_CHECK( reportFile.FileExists() );

    GERBER_WRITER gerber( &board );
    BOOST_CHECK( gerber.GenDrillReportFile( reportFile.GetFullPath() ) );

    // Invalid path with null reporter should return false without crashing
    EXCELLON_WRITER excellon2( &board );
    BOOST_CHECK( !excellon2.GenDrillReportFile( wxT( "/nonexistent/path/report.rpt" ) ) );

    GERBER_WRITER gerber2( &board );
    BOOST_CHECK( !gerber2.GenDrillReportFile( wxT( "/nonexistent/path/report.rpt" ) ) );

    wxFileName::Rmdir( tempDir.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23289
// GenDrillReportFile crashed with SIGSEGV when the board had drills because
// printToolSummary() passed integer literal 0 instead of the FILE* to fmt::print()
BOOST_AUTO_TEST_CASE( DrillReportWithTools )
{
    wxFileName tempDir = MakeTempDir();
    wxFileName boardFile( tempDir.GetFullPath(), wxT( "test_board_with_drills.kicad_pcb" ) );

    BOARD board;
    board.SetCopperLayerCount( 2 );
    board.SetFileName( boardFile.GetFullPath() );

    auto via = new PCB_VIA( &board );
    via->SetPosition( VECTOR2I( 0, 0 ) );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetDrill( pcbIUScale.mmToIU( 0.30 ) );
    via->SetWidth( pcbIUScale.mmToIU( 0.60 ) );
    board.Add( via );

    wxFileName reportFile( tempDir.GetFullPath(), wxT( "test_board_with_drills-drl.rpt" ) );

    EXCELLON_WRITER excellon( &board );
    excellon.SetOptions( false, false, VECTOR2I( 0, 0 ), false );
    excellon.SetFormat( true );
    BOOST_CHECK_NO_THROW( excellon.GenDrillReportFile( reportFile.GetFullPath() ) );
    BOOST_CHECK( reportFile.FileExists() );

    wxFFile reportStream( reportFile.GetFullPath(), wxT( "rb" ) );
    wxString reportContents;
    BOOST_REQUIRE( reportStream.ReadAll( &reportContents ) );
    BOOST_CHECK( reportContents.Contains( wxT( "T1" ) ) );
    BOOST_CHECK( reportContents.Contains( wxT( "0.300mm" ) ) );
    reportStream.Close();

    wxFileName::Rmdir( tempDir.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24014
// A non-filled PCB_SHAPE rectangle on F.SilkS was emitted as a donut_rc symbol with a
// corner radius smaller than half the line width.  Many ODB++ viewers reject the
// resulting degenerate symbol, so the outline appeared to be missing.  The exporter now
// emits the four outline edges as line features, matching how a rectangle built from
// individual line segments is exported.
BOOST_AUTO_TEST_CASE( OdbPpUnfilledRectangleOnSilk )
{
    wxFileName tempDir = MakeTempDir();
    wxFileName boardFile( tempDir.GetFullPath(), wxT( "silk_rect_board.kicad_pcb" ) );

    BOARD board;
    board.SetCopperLayerCount( 2 );
    board.SetFileName( boardFile.GetFullPath() );

    // Non-filled rectangle on F.SilkS
    PCB_SHAPE* rect = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( pcbIUScale.mmToIU( 10.0 ), pcbIUScale.mmToIU( 10.0 ) ) );
    rect->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 30.0 ), pcbIUScale.mmToIU( 20.0 ) ) );
    rect->SetLayer( F_SilkS );
    rect->SetFilled( false );
    rect->SetWidth( pcbIUScale.mmToIU( 0.15 ) );
    board.Add( rect );

    wxFileName odbRoot( tempDir.GetFullPath(), wxEmptyString );
    odbRoot.AppendDir( wxT( "odb_out" ) );
    BOOST_REQUIRE( odbRoot.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    PCB_IO_ODBPP                odbExporter;
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["sigfig"] = "4";
    BOOST_REQUIRE_NO_THROW( odbExporter.SaveBoard( odbRoot.GetFullPath(), &board, &props ) );

    wxFileName silkFeatures( odbRoot.GetFullPath(), wxT( "features" ) );
    silkFeatures.AppendDir( wxT( "steps" ) );
    silkFeatures.AppendDir( wxT( "pcb" ) );
    silkFeatures.AppendDir( wxT( "layers" ) );
    silkFeatures.AppendDir( wxT( "f.silkscreen" ) );
    BOOST_REQUIRE( silkFeatures.FileExists() );

    wxFFile silkStream( silkFeatures.GetFullPath(), wxT( "rb" ) );
    wxString silkContents;
    BOOST_REQUIRE( silkStream.ReadAll( &silkContents ) );
    silkStream.Close();

    // Four ODB++ line ("L ...") features describe the rectangle outline.
    int lineCount = 0;
    wxStringTokenizer lines( silkContents, wxT( "\n" ) );

    while( lines.HasMoreTokens() )
    {
        if( lines.GetNextToken().StartsWith( wxT( "L " ) ) )
            lineCount++;
    }

    BOOST_CHECK_EQUAL( lineCount, 4 );

    // The degenerate donut_rc symbol should not appear anymore.
    BOOST_CHECK( !silkContents.Contains( wxT( "donut_rc" ) ) );

    wxFileName::Rmdir( odbRoot.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
    wxFileName::Rmdir( tempDir.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
}
