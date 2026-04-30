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

#include <plotters/plotter_gerber.h>
#include <qa_utils/pdf_test_utils.h>


BOOST_AUTO_TEST_SUITE( GerberPlotter )


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24131
//
// A zero-length thick segment (start == end) must produce a single flash of the
// segment's aperture, not a stroked outline of a phantom 0-width aperture.
// The pre-fix code delegated to PLOTTER::ThickSegment with width set to the
// DO_NOT_SET_LINE_WIDTH sentinel (-2). PLOTTER::ThickSegment then called
// Circle( start, -2, FILLED, 0 ), which the GERBER_PLOTTER turned into a tiny
// polyArc with line width 0, producing an ADD11C,0 aperture and ~76 D01
// commands sampling a degenerate radius.
BOOST_AUTO_TEST_CASE( ZeroLengthSegmentEmitsSingleFlash )
{
    GERBER_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );

    wxString gbrPath = wxFileName::CreateTempFileName( wxT( "kicad_gbr_zero_seg" ) );
    BOOST_REQUIRE( !gbrPath.IsEmpty() );
    BOOST_TEST_MESSAGE( "Gerber output: " << gbrPath.ToStdString() );
    BOOST_REQUIRE( plotter.OpenFile( gbrPath ) );

    // PCB internal units: 1 IU = 1 nm, so IUs per decimil = 2540.
    plotter.SetViewport( VECTOR2I( 0, 0 ), 2540.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ) ) );

    const int width = 2 * 1000000; // 2 mm in nm
    const VECTOR2I origin( 0, 0 );

    plotter.ThickSegment( origin, origin, width, nullptr );
    BOOST_REQUIRE( plotter.EndPlot() );

    // Slurp the resulting gerber file.
    wxFFile file( gbrPath, wxT( "rb" ) );
    BOOST_REQUIRE( file.IsOpened() );
    wxFileOffset len = file.Length();
    BOOST_REQUIRE_GT( len, 0 );

    std::string buffer;
    buffer.resize( static_cast<size_t>( len ) );
    BOOST_REQUIRE_EQUAL( file.Read( buffer.data(), len ), static_cast<size_t>( len ) );
    file.Close();

    // The 2 mm aperture must be present.
    BOOST_CHECK_MESSAGE( buffer.find( "C,2.000000" ) != std::string::npos,
                         "Expected 2.000000 mm circle aperture in gerber output" );

    // No phantom 0 mm aperture should leak out.
    BOOST_CHECK_MESSAGE( buffer.find( "C,0.000000" ) == std::string::npos,
                         "Spurious 0.000000 mm aperture present (issue 24131 regression)" );

    // No circular interpolation commands either; the degenerate point should be
    // a flash, not an arc.
    BOOST_CHECK_MESSAGE( buffer.find( "G02" ) == std::string::npos
                                 && buffer.find( "G03" ) == std::string::npos,
                         "Unexpected circular interpolation in zero-length segment plot" );

    // The pre-fix output had about 76 D01 commands. The expected v9.0.9 output is
    // exactly one D02 (move) immediately followed by one D01 (flash) at the origin.
    int d01Count = CountOccurrences( buffer, "D01*" );
    BOOST_CHECK_EQUAL( d01Count, 1 );
    BOOST_CHECK_MESSAGE( buffer.find( "X0Y0D02*" ) != std::string::npos
                                 && buffer.find( "X0Y0D01*" ) != std::string::npos,
                         "Expected single zero-length segment flash at origin" );

    MaybeRemoveFile( gbrPath );
}


BOOST_AUTO_TEST_SUITE_END()
