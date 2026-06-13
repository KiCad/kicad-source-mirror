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

#include <diff_merge/diff_renderer_plotter.h>
#include <diff_merge/diff_scene.h>

#include <base_units.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <regex>
#include <string>


using namespace KICAD_DIFF;


namespace
{

// Tiny RAII helper: makes a unique tmp path with the requested extension,
// deletes the file on destruction so tests don't leak artifacts.
struct ScopedTmpFile
{
    explicit ScopedTmpFile( const wxString& aExt )
    {
        wxString tmp = wxStandardPaths::Get().GetTempDir();
        static int counter = 0;
        wxString   name = wxString::Format( wxS( "kicad_render_%d_%d.%s" ), static_cast<int>( wxGetProcessId() ),
                                            ++counter, aExt );
        m_path = tmp + wxFILE_SEP_PATH + name;
    }

    ~ScopedTmpFile()
    {
        if( !m_path.IsEmpty() && wxFileExists( m_path ) )
            wxRemoveFile( m_path );
    }

    const wxString& Path() const { return m_path; }

    wxString m_path;
};


// Read the first @p aBytes bytes of a file into a std::string so test can
// pin header bytes for format-sniffing.
std::string readHeaderBytes( const wxString& aPath, size_t aBytes )
{
    wxFile f( aPath );

    if( !f.IsOpened() )
        return {};

    std::string buf( aBytes, '\0' );
    ssize_t got = f.Read( buf.data(), aBytes );

    if( got <= 0 )
        return {};

    buf.resize( static_cast<size_t>( got ) );
    return buf;
}


// A DIFF_SCENE with no shapes and a zero-extent documentBBox -- both
// renderers should emit a tiny empty placeholder PNG/SVG rather than
// failing.
DIFF_SCENE makeEmptyScene()
{
    DIFF_SCENE scene;
    return scene;
}


// A DIFF_SCENE with one shape per category so the renderer walks all four
// PAINT_ORDER entries.
DIFF_SCENE makePopulatedScene()
{
    DIFF_SCENE scene;
    scene.documentBBox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 1000000, 1000000 ) );

    auto add = [&]( std::vector<SCENE_SHAPE>& aBucket, const BOX2I& bb,
                    const KIGFX::COLOR4D& col )
    {
        SCENE_SHAPE s;
        s.bbox  = bb;
        s.color = col;
        aBucket.push_back( s );
    };

    add( scene.modifiedShapes, BOX2I( VECTOR2I( 100000, 100000 ),
                                      VECTOR2I( 100000, 100000 ) ),
         KIGFX::COLOR4D( 1.0, 1.0, 0.0, 1.0 ) );
    add( scene.addedShapes,    BOX2I( VECTOR2I( 200000, 100000 ),
                                      VECTOR2I( 100000, 100000 ) ),
         KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 ) );
    add( scene.removedShapes,  BOX2I( VECTOR2I( 100000, 200000 ),
                                      VECTOR2I( 100000, 100000 ) ),
         KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 ) );
    add( scene.conflictShapes, BOX2I( VECTOR2I( 200000, 200000 ),
                                      VECTOR2I( 100000, 100000 ) ),
         KIGFX::COLOR4D( 1.0, 0.0, 1.0, 1.0 ) );

    return scene;
}

// A DIFF_SCENE carrying one square added-shape of a known internal-unit
// extent, tagged with the supplied document kind. Used to observe how the
// plotter renderer maps internal units to device (mm) coordinates per kind.
DIFF_SCENE makeSingleSquareScene( KICAD_DIFF::DOC_KIND aKind, int aSideIU )
{
    DIFF_SCENE scene;
    scene.docKind      = aKind;
    scene.documentBBox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( aSideIU, aSideIU ) );

    SCENE_SHAPE s;
    s.bbox  = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( aSideIU, aSideIU ) );
    s.color = KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 );
    scene.addedShapes.push_back( s );

    return scene;
}


// Read an entire text file into a std::string.
std::string readWholeFile( const wxString& aPath )
{
    wxFile f( aPath );

    if( !f.IsOpened() )
        return {};

    wxString content;

    if( !f.ReadAll( &content ) )
        return {};

    return std::string( content.utf8_str() );
}


// Largest width attribute (in mm) among the <rect> elements in an SVG body.
// The diff renderer emits each scene shape as a <rect>; the biggest one is the
// document bbox shape, whose mm width is iuWidth * iuScale-derived factor.
double largestSvgRectWidthMM( const std::string& aSvg )
{
    std::regex  rectWidth( "<rect[^>]*width=\"([0-9.]+)\"" );
    double      best = 0.0;
    auto        begin = std::sregex_iterator( aSvg.begin(), aSvg.end(), rectWidth );
    auto        end = std::sregex_iterator();

    for( auto it = begin; it != end; ++it )
        best = std::max( best, std::stod( ( *it )[1].str() ) );

    return best;
}

} // namespace


BOOST_AUTO_TEST_SUITE( RendererOutput )


// A schematic-kind scene must be sized with the schematic internal-unit scale,
// not the PCB one. The two scales differ by SCH_IU_PER_MM vs PCB_IU_PER_MM
// (1e4 vs 1e6), so a fixed internal-unit square renders 100x larger in device
// (mm) coordinates under the correct schematic scale. This pins the bug where
// computeViewport hard-coded the PCB scale for every document type.
BOOST_AUTO_TEST_CASE( SvgSchematicSceneUsesSchematicScale )
{
    constexpr int oneMillimetreSchIU = 10000; // SCH_IU_PER_MM

    ScopedTmpFile          schOut( wxS( "svg" ) );
    PLOTTER_RENDER_OPTIONS opts;

    DIFF_SCENE schScene = makeSingleSquareScene( KICAD_DIFF::DOC_KIND::SCH, oneMillimetreSchIU );
    BOOST_REQUIRE( RenderSceneToSvg( schScene, schOut.Path(), opts ) );

    double schWidthMM = largestSvgRectWidthMM( readWholeFile( schOut.Path() ) );

    // 10000 schematic IU is exactly 1 mm. Allow a small tolerance for the 5%
    // viewport margin and SVG coordinate rounding.
    BOOST_CHECK_GT( schWidthMM, 0.5 );

    // The same internal-unit square interpreted with the PCB scale would render
    // ~100x smaller (10000 PCB IU = 0.01 mm). Confirm the schematic render is
    // nowhere near that erroneous size.
    ScopedTmpFile pcbOut( wxS( "svg" ) );
    DIFF_SCENE    pcbScene = makeSingleSquareScene( KICAD_DIFF::DOC_KIND::PCB, oneMillimetreSchIU );
    BOOST_REQUIRE( RenderSceneToSvg( pcbScene, pcbOut.Path(), opts ) );

    double pcbWidthMM = largestSvgRectWidthMM( readWholeFile( pcbOut.Path() ) );

    BOOST_CHECK_GT( pcbWidthMM, 0.0 );

    // A fixed internal-unit box renders mm-wide in inverse proportion to the
    // scale's IU_PER_MM, so the schematic render must be (PCB_IU_PER_MM /
    // SCH_IU_PER_MM) = 100x the PCB render of the same box.
    const double ratio = schWidthMM / pcbWidthMM;
    BOOST_CHECK_CLOSE( ratio, pcbIUScale.IU_PER_MM / schIUScale.IU_PER_MM, 5.0 );
}


// Empty scene must still produce a valid PNG file -- the renderer falls back
// to a minimal placeholder so the CLI exit code (success) matches "no changes".
BOOST_AUTO_TEST_CASE( PngEmptyScenePlaceholder )
{
    ScopedTmpFile out( wxS( "png" ) );
    PLOTTER_RENDER_OPTIONS opts;
    opts.dpi = 96;

    DIFF_SCENE scene = makeEmptyScene();
    BOOST_REQUIRE( RenderSceneToPng( scene, out.Path(), opts ) );
    BOOST_REQUIRE( wxFileExists( out.Path() ) );

    std::string header = readHeaderBytes( out.Path(), 8 );
    BOOST_REQUIRE_GE( header.size(), 8u );

    // PNG magic: 89 50 4E 47 0D 0A 1A 0A
    BOOST_CHECK_EQUAL( static_cast<unsigned char>( header[0] ), 0x89u );
    BOOST_CHECK_EQUAL( static_cast<unsigned char>( header[1] ), 0x50u );
    BOOST_CHECK_EQUAL( static_cast<unsigned char>( header[2] ), 0x4Eu );
    BOOST_CHECK_EQUAL( static_cast<unsigned char>( header[3] ), 0x47u );
}


// Populated scene produces a PNG larger than the empty-scene placeholder.
BOOST_AUTO_TEST_CASE( PngPopulatedSceneIsLargerThanPlaceholder )
{
    ScopedTmpFile out( wxS( "png" ) );
    PLOTTER_RENDER_OPTIONS opts;
    opts.pixelWidth  = 64;
    opts.pixelHeight = 64;
    opts.dpi         = 96;

    DIFF_SCENE scene = makePopulatedScene();
    BOOST_REQUIRE( RenderSceneToPng( scene, out.Path(), opts ) );
    BOOST_REQUIRE( wxFileExists( out.Path() ) );

    wxFileName fn( out.Path() );
    wxULongLong size = fn.GetSize();

    // Empty-scene placeholder is 67 bytes (see writeEmptyPng in
    // diff_renderer_plotter.cpp).  A populated render must be bigger.
    BOOST_CHECK_GT( size.GetValue(), 67u );

    // Verify PNG header bytes too.
    std::string header = readHeaderBytes( out.Path(), 8 );
    BOOST_CHECK_EQUAL( static_cast<unsigned char>( header[0] ), 0x89u );
    BOOST_CHECK_EQUAL( static_cast<unsigned char>( header[1] ), 0x50u );
}


BOOST_AUTO_TEST_CASE( SvgEmptyScenePlaceholder )
{
    ScopedTmpFile out( wxS( "svg" ) );
    PLOTTER_RENDER_OPTIONS opts;

    DIFF_SCENE scene = makeEmptyScene();
    BOOST_REQUIRE( RenderSceneToSvg( scene, out.Path(), opts ) );
    BOOST_REQUIRE( wxFileExists( out.Path() ) );

    std::string content = readHeaderBytes( out.Path(), 256 );
    BOOST_REQUIRE( !content.empty() );

    // The empty SVG placeholder starts with an XML declaration.
    BOOST_CHECK( content.find( "<?xml" ) == 0 );
    BOOST_CHECK( content.find( "<svg" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( SvgPopulatedSceneEmitsSvgRoot )
{
    ScopedTmpFile out( wxS( "svg" ) );
    PLOTTER_RENDER_OPTIONS opts;
    opts.pixelWidth  = 64;
    opts.pixelHeight = 64;

    DIFF_SCENE scene = makePopulatedScene();
    BOOST_REQUIRE( RenderSceneToSvg( scene, out.Path(), opts ) );
    BOOST_REQUIRE( wxFileExists( out.Path() ) );

    // Verify the output is structurally an SVG document by looking for the
    // <svg ...> root element.  Read up to 4 KiB so headers + opening root
    // tag fit comfortably.
    std::string content = readHeaderBytes( out.Path(), 4096 );
    BOOST_REQUIRE( !content.empty() );
    BOOST_CHECK( content.find( "<svg" ) != std::string::npos );

    // Populated SVG must be larger than the empty placeholder (which is
    // ~80 bytes).
    wxFileName fn( out.Path() );
    BOOST_CHECK_GT( fn.GetSize().GetValue(), 80u );
}


BOOST_AUTO_TEST_SUITE_END()
