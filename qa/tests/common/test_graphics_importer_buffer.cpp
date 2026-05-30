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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <eda_item.h>
#include <import_gfx/dxf_import_plugin.h>
#include <import_gfx/graphics_importer_buffer.h>
#include <base_units.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>


class TEST_GRAPHICS_IMPORTER : public GRAPHICS_IMPORTER
{
public:
    TEST_GRAPHICS_IMPORTER() : GRAPHICS_IMPORTER()
    {
        m_millimeterToIu = PCB_IU_PER_MM;
    }

    void AddLine( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                  const IMPORTED_STROKE& aStroke ) override
    {
        m_lines.push_back( { aStart, aEnd } );
        m_lineSourceLayers.push_back( m_currentSourceLayer );
    }

    bool CanImportSourceLayer( const wxString& aSourceLayer ) const override
    {
        if( m_enabledSourceLayers.empty() )
            return true;

        return std::find( m_enabledSourceLayers.begin(), m_enabledSourceLayers.end(), aSourceLayer )
               != m_enabledSourceLayers.end();
    }

    void SetCurrentSourceLayer( const wxString& aSourceLayer ) override { m_currentSourceLayer = aSourceLayer; }

    void AddCircle( const VECTOR2D& aCenter, double aRadius, const IMPORTED_STROKE& aStroke,
                    bool aFilled, const COLOR4D& aFillColor ) override {}

    void AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, const EDA_ANGLE& aAngle,
                 const IMPORTED_STROKE& aStroke ) override {}

    void AddPolygon( const std::vector<VECTOR2D>& aVertices, const IMPORTED_STROKE& aStroke,
                     bool aFilled, const COLOR4D& aFillColor ) override {}

    void AddText( const VECTOR2D& aOrigin, const wxString& aText, double aHeight, double aWidth,
                  double aThickness, double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                  GR_TEXT_V_ALIGN_T aVJustify, const COLOR4D& aColor ) override {}

    void AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                    const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                    const IMPORTED_STROKE& aStroke ) override {}

    void AddEllipse( const VECTOR2D& aCenter, double aMajorRadius, double aMinorRadius, const EDA_ANGLE& aRotation,
                     const IMPORTED_STROKE& aStroke, bool aFilled, const COLOR4D& aFillColor ) override
    {
    }

    void AddEllipseArc( const VECTOR2D& aCenter, double aMajorRadius, double aMinorRadius, const EDA_ANGLE& aRotation,
                        const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aEndAngle,
                        const IMPORTED_STROKE& aStroke ) override
    {
    }

    std::vector<std::pair<VECTOR2D, VECTOR2D>> m_lines;
    std::vector<wxString>                      m_lineSourceLayers;
    std::vector<wxString>                      m_enabledSourceLayers;
    wxString                                   m_currentSourceLayer;
};


BOOST_AUTO_TEST_SUITE( GraphicsImporterBuffer )


/**
 * Test that large coordinates that would overflow when converted to internal units
 * are automatically offset to fit within the valid coordinate range.
 *
 * This tests the fix for GitLab issue #9681 where DXF files with large coordinates
 * (e.g., around 2300mm which exceeds INT_MAX/1e6 ~= 2147mm) caused visual corruption.
 */
BOOST_AUTO_TEST_CASE( LargeCoordinatesAutoOffset )
{
    GRAPHICS_IMPORTER_BUFFER buffer;
    TEST_GRAPHICS_IMPORTER   importer;

    // Add a line with coordinates that would overflow INT32 when converted to IU
    // 2300mm * 1e6 IU/mm = 2.3e9 > INT32_MAX (2.147e9)
    VECTOR2D start( 2300.0, 1400.0 );
    VECTOR2D end( 2350.0, 1450.0 );

    IMPORTED_STROKE stroke( 0.1 );
    buffer.AddLine( start, end, stroke );

    // Import with no offset set (default is 0,0)
    BOOST_CHECK( importer.GetImportOffsetMM() == VECTOR2D( 0, 0 ) );

    buffer.ImportTo( importer );

    // After import, the offset should have been automatically adjusted
    // to move the coordinates into the valid range
    VECTOR2D offset = importer.GetImportOffsetMM();

    // The offset should be negative of the bounding box origin to bring coordinates near zero
    BOOST_CHECK_MESSAGE( offset.x < 0, "X offset should be negative to shift large coords down" );

    // Verify the resulting coordinates are within valid IU range
    if( !importer.m_lines.empty() )
    {
        VECTOR2D importedStart = importer.m_lines[0].first;
        VECTOR2D importedEnd = importer.m_lines[0].second;

        // After offset and scale, coordinates should be manageable
        double maxCoord = static_cast<double>( std::numeric_limits<int>::max() )
                          / importer.GetMillimeterToIuFactor();

        BOOST_CHECK_MESSAGE( ( importedStart.x + offset.x ) < maxCoord,
                             "Imported X coord should be within valid range" );
        BOOST_CHECK_MESSAGE( ( importedStart.y + offset.y ) < maxCoord,
                             "Imported Y coord should be within valid range" );
    }
}


/**
 * Test that normal coordinates (within valid range) are not modified.
 */
BOOST_AUTO_TEST_CASE( NormalCoordinatesNoOffset )
{
    GRAPHICS_IMPORTER_BUFFER buffer;
    TEST_GRAPHICS_IMPORTER   importer;

    // Add a line with normal coordinates that fit within the valid range
    // 100mm * 1e6 IU/mm = 1e8 << INT32_MAX
    VECTOR2D start( 100.0, 100.0 );
    VECTOR2D end( 200.0, 200.0 );

    IMPORTED_STROKE stroke( 0.1 );
    buffer.AddLine( start, end, stroke );

    buffer.ImportTo( importer );

    // Offset should remain at zero since coordinates are valid
    VECTOR2D offset = importer.GetImportOffsetMM();
    BOOST_CHECK_MESSAGE( offset == VECTOR2D( 0, 0 ),
                         "Normal coordinates should not trigger auto-offset" );
}


BOOST_AUTO_TEST_CASE( SourceLayersAreReportedAndCanBeSkipped )
{
    GRAPHICS_IMPORTER_BUFFER buffer;
    TEST_GRAPHICS_IMPORTER   importer;

    IMPORTED_STROKE stroke( 0.1 );

    buffer.SetCurrentSourceLayer( wxS( "Outline" ) );
    buffer.AddLine( VECTOR2D( 0.0, 0.0 ), VECTOR2D( 10.0, 0.0 ), stroke );

    buffer.SetCurrentSourceLayer( wxS( "Guide" ) );
    buffer.AddLine( VECTOR2D( 0.0, 1.0 ), VECTOR2D( 10.0, 1.0 ), stroke );

    std::vector<wxString> sourceLayers = buffer.GetSourceLayers();

    BOOST_CHECK( std::find( sourceLayers.begin(), sourceLayers.end(), wxS( "Outline" ) ) != sourceLayers.end() );
    BOOST_CHECK( std::find( sourceLayers.begin(), sourceLayers.end(), wxS( "Guide" ) ) != sourceLayers.end() );

    importer.m_enabledSourceLayers.push_back( wxS( "Outline" ) );

    buffer.ImportTo( importer );

    BOOST_REQUIRE_EQUAL( importer.m_lines.size(), 1 );
    BOOST_REQUIRE_EQUAL( importer.m_lineSourceLayers.size(), 1 );
    BOOST_CHECK_EQUAL( importer.m_lineSourceLayers[0], wxS( "Outline" ) );
}


BOOST_AUTO_TEST_CASE( DxfSourceLayersArePreserved )
{
    const char* dxf =
        "0\n"
        "SECTION\n"
        "2\n"
        "HEADER\n"
        "9\n"
        "$INSUNITS\n"
        "70\n"
        "4\n"
        "0\n"
        "ENDSEC\n"
        "0\n"
        "SECTION\n"
        "2\n"
        "TABLES\n"
        "0\n"
        "TABLE\n"
        "2\n"
        "LAYER\n"
        "70\n"
        "2\n"
        "0\n"
        "LAYER\n"
        "2\n"
        "Outline\n"
        "70\n"
        "0\n"
        "62\n"
        "7\n"
        "6\n"
        "Continuous\n"
        "0\n"
        "LAYER\n"
        "2\n"
        "Guide\n"
        "70\n"
        "0\n"
        "62\n"
        "7\n"
        "6\n"
        "Continuous\n"
        "0\n"
        "ENDTAB\n"
        "0\n"
        "ENDSEC\n"
        "0\n"
        "SECTION\n"
        "2\n"
        "ENTITIES\n"
        "0\n"
        "LINE\n"
        "8\n"
        "Outline\n"
        "10\n"
        "0\n"
        "20\n"
        "0\n"
        "11\n"
        "10\n"
        "21\n"
        "0\n"
        "0\n"
        "LINE\n"
        "8\n"
        "Guide\n"
        "10\n"
        "0\n"
        "20\n"
        "1\n"
        "11\n"
        "10\n"
        "21\n"
        "1\n"
        "0\n"
        "ENDSEC\n"
        "0\n"
        "EOF\n";

    wxString dxfPath = wxFileName::CreateTempFileName( wxS( "kicad_dxf_layers" ) );
    BOOST_REQUIRE( !dxfPath.IsEmpty() );

    {
        wxFFile file( dxfPath, wxT( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( dxf, strlen( dxf ) ), strlen( dxf ) );
    }

    DXF_IMPORT_PLUGIN      plugin;
    TEST_GRAPHICS_IMPORTER importer;

    plugin.SetUnit( DXF_IMPORT_UNITS::MM );
    plugin.SetImporter( &importer );

    BOOST_REQUIRE( plugin.Load( dxfPath ) );
    wxRemoveFile( dxfPath );

    std::vector<wxString> sourceLayers = plugin.GetSourceLayers();

    BOOST_CHECK( std::find( sourceLayers.begin(), sourceLayers.end(), wxS( "Outline" ) ) != sourceLayers.end() );
    BOOST_CHECK( std::find( sourceLayers.begin(), sourceLayers.end(), wxS( "Guide" ) ) != sourceLayers.end() );

    importer.m_enabledSourceLayers.push_back( wxS( "Outline" ) );

    BOOST_REQUIRE( plugin.Import() );

    BOOST_REQUIRE_EQUAL( importer.m_lines.size(), 1 );
    BOOST_REQUIRE_EQUAL( importer.m_lineSourceLayers.size(), 1 );
    BOOST_CHECK_EQUAL( importer.m_lineSourceLayers[0], wxS( "Outline" ) );
}


BOOST_AUTO_TEST_SUITE_END()
