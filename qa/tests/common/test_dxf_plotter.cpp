/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <boost/test/unit_test.hpp>

#include <string>

#include <wx/filename.h>
#include <wx/ffile.h>

#include <plotters/plotter_dxf.h>
#include <qa_utils/pdf_test_utils.h>


BOOST_AUTO_TEST_SUITE( DxfPlotter )


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24405
// Locks in the R2000-mandated structure AutoCAD requires when we use the 420
// true-color group on LAYER records.
BOOST_AUTO_TEST_CASE( R2000HandlesAndTableSkeleton )
{
    DXF_PLOTTER            plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );

    wxString dxfPath = wxFileName::CreateTempFileName( wxT( "kicad_dxf_r2000" ) );
    BOOST_REQUIRE( !dxfPath.IsEmpty() );
    BOOST_TEST_MESSAGE( "DXF output: " << dxfPath.ToStdString() );
    BOOST_REQUIRE( plotter.OpenFile( dxfPath ) );

    plotter.SetViewport( VECTOR2I( 0, 0 ), 2540.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ) ) );

    // LINE exercises the inline emit path; CIRCLE exercises emitEntityHandle.
    plotter.MoveTo( VECTOR2I( 0, 0 ) );
    plotter.FinishTo( VECTOR2I( 1000000, 1000000 ) );
    plotter.Circle( VECTOR2I( 2000000, 0 ), 500000, FILL_T::NO_FILL, 0 );

    BOOST_REQUIRE( plotter.EndPlot() );

    wxFFile file( dxfPath, wxT( "rb" ) );
    BOOST_REQUIRE( file.IsOpened() );

    wxFileOffset len = file.Length();
    BOOST_REQUIRE_GT( len, 0 );

    std::string buffer;
    buffer.resize( static_cast<size_t>( len ) );
    BOOST_REQUIRE_EQUAL( file.Read( buffer.data(), len ), static_cast<size_t>( len ) );
    file.Close();

    // AC1018 makes the 420 true-color tag legal; $HANDSEED must exist for strict
    // R2000+ readers.
    BOOST_CHECK_MESSAGE( buffer.find( "$ACADVER\n  1\nAC1018\n" ) != std::string::npos,
                         "Expected $ACADVER = AC1018 in DXF header" );
    BOOST_CHECK_MESSAGE( buffer.find( "$HANDSEED\n" ) != std::string::npos,
                         "Expected $HANDSEED in DXF header" );

    // APPID/ACAD is required by R2000+.
    BOOST_CHECK_MESSAGE( buffer.find( "TABLE\n  2\nAPPID\n" ) != std::string::npos,
                         "Expected APPID table in TABLES section" );
    BOOST_CHECK_MESSAGE( buffer.find( "APPID\n" ) != std::string::npos
                                 && buffer.find( "\n  2\nACAD\n" ) != std::string::npos,
                         "Expected ACAD entry in APPID table" );

    // Canonical *Model_Space BLOCK_RECORD plus the BLOCKS section that backs it.
    BOOST_CHECK_MESSAGE( buffer.find( "TABLE\n  2\nBLOCK_RECORD\n" ) != std::string::npos,
                         "Expected BLOCK_RECORD table" );
    BOOST_CHECK_MESSAGE( buffer.find( "*Model_Space" ) != std::string::npos,
                         "Expected *Model_Space layout block" );
    BOOST_CHECK_MESSAGE( buffer.find( "SECTION\n  2\nBLOCKS\n" ) != std::string::npos,
                         "Expected BLOCKS section" );

    // Every entity carries handle (5), owner (330) and AcDbEntity.  LINE additionally
    // has the AcDbLine subclass and a 6/linetype inside the AcDbEntity scope.
    BOOST_CHECK_MESSAGE( buffer.find( "0\nLINE\n" ) != std::string::npos,
                         "Expected LINE entity in output" );
    BOOST_CHECK_MESSAGE( buffer.find( "100\nAcDbEntity\n" ) != std::string::npos,
                         "Expected AcDbEntity subclass marker on entities" );
    BOOST_CHECK_MESSAGE( buffer.find( "100\nAcDbLine\n" ) != std::string::npos,
                         "Expected AcDbLine subclass marker on LINE entity" );
    BOOST_CHECK_MESSAGE( buffer.find( "100\nAcDbCircle\n" ) != std::string::npos,
                         "Expected AcDbCircle subclass marker on CIRCLE entity" );

    // Strict R2000+ readers require a 74 group after each 49 in complex linetypes.
    BOOST_CHECK_MESSAGE( buffer.find( " 49\n1.25\n 74\n0\n" ) != std::string::npos,
                         "Expected 74 group code following 49 in DASHDOT pattern" );

    // Spec page 59 mandates three empty layout blocks (*Model_Space, *Paper_Space,
    // *Paper_Space0); each BLOCK_RECORD entry has a 340 pointer to its LAYOUT.
    BOOST_CHECK_MESSAGE( buffer.find( "\n*Paper_Space0\n" ) != std::string::npos,
                         "Expected *Paper_Space0 layout (spec mandates three empty blocks)" );
    BOOST_CHECK_MESSAGE( buffer.find( "100\nAcDbBlockTableRecord\n  2\n*Paper_Space0\n340\n" )
                                 != std::string::npos,
                         "Expected 340 LAYOUT pointer in *Paper_Space0 BLOCK_RECORD" );

    // OBJECTS section with the root NOD, ACAD_LAYOUT/ACAD_GROUP and three LAYOUT
    // objects that close the 330 back-loop to each BLOCK_RECORD.
    BOOST_CHECK_MESSAGE( buffer.find( "SECTION\n  2\nOBJECTS\n" ) != std::string::npos,
                         "Expected OBJECTS section" );
    BOOST_CHECK_MESSAGE( buffer.find( "ACAD_LAYOUT\n350\n" ) != std::string::npos,
                         "Expected ACAD_LAYOUT entry in root dictionary" );
    BOOST_CHECK_MESSAGE( buffer.find( "ACAD_GROUP\n350\n" ) != std::string::npos,
                         "Expected ACAD_GROUP entry in root dictionary" );
    BOOST_CHECK_MESSAGE( buffer.find( "0\nLAYOUT\n" ) != std::string::npos,
                         "Expected LAYOUT objects in OBJECTS section" );
    BOOST_CHECK_MESSAGE( buffer.find( "100\nAcDbLayout\n  1\nModel\n" ) != std::string::npos,
                         "Expected Model layout object" );
    BOOST_CHECK_MESSAGE( buffer.find( "100\nAcDbLayout\n  1\nLayout1\n" ) != std::string::npos,
                         "Expected Layout1 (Paper_Space) layout object" );
    BOOST_CHECK_MESSAGE( buffer.find( "100\nAcDbLayout\n  1\nLayout2\n" ) != std::string::npos,
                         "Expected Layout2 (Paper_Space0) layout object" );

    // Every LAYER's 390 must resolve to a real plot-style object; without the
    // ACDBPLACEHOLDER "Normal" the LAYER table fails to load.
    BOOST_CHECK_MESSAGE( buffer.find( "ACAD_PLOTSTYLENAME\n350\n" ) != std::string::npos,
                         "Expected ACAD_PLOTSTYLENAME entry in root dictionary" );
    BOOST_CHECK_MESSAGE( buffer.find( "ACDBDICTIONARYWDFLT\n" ) != std::string::npos,
                         "Expected ACDBDICTIONARYWDFLT for ACAD_PLOTSTYLENAME" );
    BOOST_CHECK_MESSAGE( buffer.find( "ACDBPLACEHOLDER\n" ) != std::string::npos,
                         "Expected ACDBPLACEHOLDER (the 'Normal' plot style)" );
    BOOST_CHECK_MESSAGE( buffer.find( "  6\nCONTINUOUS\n390\n" ) != std::string::npos,
                         "Expected 390 plot-style handle on every LAYER record" );

    // VPORT/VIEW/UCS must exist (empty is OK).  DIMSTYLE needs a Standard entry
    // whose handle uses group code 105 instead of 5 (spec page 35).
    BOOST_CHECK_MESSAGE( buffer.find( "TABLE\n  2\nVPORT\n" ) != std::string::npos,
                         "Expected VPORT table" );
    BOOST_CHECK_MESSAGE( buffer.find( "TABLE\n  2\nVIEW\n" ) != std::string::npos,
                         "Expected VIEW table" );
    BOOST_CHECK_MESSAGE( buffer.find( "TABLE\n  2\nUCS\n" ) != std::string::npos,
                         "Expected UCS table" );
    BOOST_CHECK_MESSAGE( buffer.find( "TABLE\n  2\nDIMSTYLE\n" ) != std::string::npos,
                         "Expected DIMSTYLE table" );
    BOOST_CHECK_MESSAGE( buffer.find( "  0\nDIMSTYLE\n105\n" ) != std::string::npos,
                         "Expected DIMSTYLE record handle on group code 105 (not 5)" );
    BOOST_CHECK_MESSAGE( buffer.find( "AcDbDimStyleTableRecord\n  2\nStandard\n" )
                                 != std::string::npos,
                         "Expected Standard DIMSTYLE entry" );

    // Default layer "0" must exist in the LAYER table even if no entity uses it.
    BOOST_CHECK_MESSAGE( buffer.find( "AcDbLayerTableRecord\n  2\n0\n" ) != std::string::npos,
                         "Expected default layer \"0\" entry in LAYER table" );

    // ByBlock and ByLayer are required LTYPE entries.
    BOOST_CHECK_MESSAGE( buffer.find( "AcDbLinetypeTableRecord\n  2\nByBlock\n" )
                                 != std::string::npos,
                         "Expected ByBlock linetype entry" );
    BOOST_CHECK_MESSAGE( buffer.find( "AcDbLinetypeTableRecord\n  2\nByLayer\n" )
                                 != std::string::npos,
                         "Expected ByLayer linetype entry" );

    // AutoCAD requires group 2 (printer name) in AcDbPlotSettings.  ODA File
    // Converter inserts "none_device" on load when it's missing.
    BOOST_CHECK_MESSAGE( buffer.find( "AcDbPlotSettings\n  1\n\n  2\nnone_device\n" )
                                 != std::string::npos,
                         "Expected group 2 (none_device) in AcDbPlotSettings" );

    // ModelType (1024) in AcDbPlotSettings 70 is only valid on the Model layout.
    BOOST_CHECK_MESSAGE( buffer.find( "AcDbLayout\n  1\nModel\n" ) != std::string::npos,
                         "Expected Model layout (precondition for the next check)" );
    BOOST_CHECK_MESSAGE( buffer.find( " 70\n1024\n" ) != std::string::npos,
                         "Expected ModelType (1024) flag on the Model layout" );

    MaybeRemoveFile( dxfPath );
}


BOOST_AUTO_TEST_SUITE_END()
