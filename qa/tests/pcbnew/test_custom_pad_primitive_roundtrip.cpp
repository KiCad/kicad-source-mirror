/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_point.h>
#include <pcb_shape.h>
#include <geometry/eda_angle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include <pcbnew_utils/board_file_utils.h>

#include <filesystem>
#include <memory>


BOOST_AUTO_TEST_SUITE( CustomPadPrimitiveRoundTrip )


namespace
{

/// Temporary file holder that removes the file (and its directory) on destruction.
class TEMP_FILE_HOLDER
{
public:
    TEMP_FILE_HOLDER( const std::string& aPrefix, const std::string& aSuffix )
    {
        std::filesystem::path dir = std::filesystem::temp_directory_path() / aPrefix;
        std::filesystem::create_directories( dir );
        m_path = dir / ( "board" + aSuffix );
    }

    ~TEMP_FILE_HOLDER()
    {
        std::error_code ec;
        std::filesystem::remove( m_path, ec );
        std::filesystem::remove( m_path.parent_path(), ec );
    }

    const std::filesystem::path& Path() const { return m_path; }

private:
    std::filesystem::path m_path;
};


int mm( double aMillimetres )
{
    return pcbIUScale.mmToIU( aMillimetres );
}


PCB_SHAPE* findPrimitive( const PAD* aPad, SHAPE_T aShape )
{
    for( const std::shared_ptr<PCB_SHAPE>& prim : aPad->GetPrimitives( PADSTACK::ALL_LAYERS ) )
    {
        if( prim->GetShape() == aShape )
            return prim.get();
    }

    return nullptr;
}


void checkPoly( const SHAPE_POLY_SET& aExpected, const SHAPE_POLY_SET& aGot )
{
    BOOST_REQUIRE_EQUAL( aExpected.OutlineCount(), aGot.OutlineCount() );

    const SHAPE_LINE_CHAIN& expOutline = aExpected.COutline( 0 );
    const SHAPE_LINE_CHAIN& gotOutline = aGot.COutline( 0 );

    BOOST_REQUIRE_EQUAL( expOutline.PointCount(), gotOutline.PointCount() );

    for( int ii = 0; ii < expOutline.PointCount(); ++ii )
    {
        BOOST_CHECK_EQUAL( gotOutline.CPoint( ii ).x, expOutline.CPoint( ii ).x );
        BOOST_CHECK_EQUAL( gotOutline.CPoint( ii ).y, expOutline.CPoint( ii ).y );
    }
}

} // namespace


/**
 * A custom pad's bezier and polygon primitives must survive a save/reload unchanged,
 * even when the parent footprint carries a non-trivial transform (rotated + non-uniform
 * scale). Pad primitives are stored in raw library coordinates and are not baked by the
 * footprint transform, so the writer must emit those raw coordinates: applying the
 * footprint transform on save (but reading raw on load) corrupts the round-trip.
 */
BOOST_AUTO_TEST_CASE( BezierAndPolyOnTransformedFootprint )
{
    auto board = std::make_unique<BOARD>();
    auto fp = std::make_unique<FOOTPRINT>( board.get() );

    fp->SetReference( "U1" );

    // A non-cardinal rotation and an anisotropic scale make any spurious transform on
    // the primitives obvious.
    fp->SetPosition( VECTOR2I( mm( 50.0 ), mm( 25.0 ) ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp->SetTransformScale( 2.0, 1.5 );

    auto pad = std::make_unique<PAD>( fp.get() );
    pad->SetNumber( "1" );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetLayerSet( LSET( { F_Cu } ) );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( mm( 1.0 ), mm( 1.0 ) ) );
    pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );

    // Bezier primitive with deliberately asymmetric control points.
    auto* bezier = new PCB_SHAPE( pad.get() );
    bezier->SetShape( SHAPE_T::BEZIER );
    bezier->SetStart( VECTOR2I( mm( 0.0 ), mm( 0.0 ) ) );
    bezier->SetBezierC1( VECTOR2I( mm( 1.0 ), mm( 2.0 ) ) );
    bezier->SetBezierC2( VECTOR2I( mm( 3.0 ), mm( -1.0 ) ) );
    bezier->SetEnd( VECTOR2I( mm( 4.0 ), mm( 0.5 ) ) );
    bezier->SetWidth( mm( 0.1 ) );
    pad->AddPrimitive( PADSTACK::ALL_LAYERS, bezier );

    // Polygon primitive with an asymmetric (non-rectangular) outline.
    SHAPE_LINE_CHAIN chain;
    chain.Append( VECTOR2I( mm( 1.0 ), mm( 0.0 ) ) );
    chain.Append( VECTOR2I( mm( 3.0 ), mm( 1.0 ) ) );
    chain.Append( VECTOR2I( mm( 2.0 ), mm( 4.0 ) ) );
    chain.Append( VECTOR2I( mm( -1.0 ), mm( 2.0 ) ) );
    chain.SetClosed( true );

    SHAPE_POLY_SET polySet;
    polySet.AddOutline( chain );

    auto* poly = new PCB_SHAPE( pad.get() );
    poly->SetShape( SHAPE_T::POLY );
    poly->SetPolyShape( polySet );
    poly->SetWidth( mm( 0.1 ) );
    pad->AddPrimitive( PADSTACK::ALL_LAYERS, poly );

    // Capture the in-memory (pre-save) geometry as the round-trip reference.
    const VECTOR2I       expBezierStart = bezier->GetLibraryStart();
    const VECTOR2I       expBezierC1 = bezier->GetLibraryBezierC1();
    const VECTOR2I       expBezierC2 = bezier->GetLibraryBezierC2();
    const VECTOR2I       expBezierEnd = bezier->GetLibraryEnd();
    const int            expBezierWidth = bezier->GetWidth();
    const SHAPE_POLY_SET expPoly = poly->GetLibraryPolyShape();
    const int            expPolyWidth = poly->GetWidth();

    fp->Add( pad.release(), ADD_MODE::APPEND, true );
    board->Add( fp.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_custom_pad_primitive_roundtrip", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fpFound = reloaded->Footprints().empty() ? nullptr : reloaded->Footprints().front();
    BOOST_REQUIRE_MESSAGE( fpFound, "Footprint not found in reloaded board" );
    BOOST_REQUIRE_MESSAGE( !fpFound->Pads().empty(), "Pad not found in reloaded footprint" );

    PAD* padFound = fpFound->Pads().front();

    PCB_SHAPE* bezierFound = findPrimitive( padFound, SHAPE_T::BEZIER );
    BOOST_REQUIRE_MESSAGE( bezierFound, "Bezier primitive not found in reloaded pad" );

    BOOST_CHECK_EQUAL( bezierFound->GetLibraryStart().x, expBezierStart.x );
    BOOST_CHECK_EQUAL( bezierFound->GetLibraryStart().y, expBezierStart.y );
    BOOST_CHECK_EQUAL( bezierFound->GetLibraryBezierC1().x, expBezierC1.x );
    BOOST_CHECK_EQUAL( bezierFound->GetLibraryBezierC1().y, expBezierC1.y );
    BOOST_CHECK_EQUAL( bezierFound->GetLibraryBezierC2().x, expBezierC2.x );
    BOOST_CHECK_EQUAL( bezierFound->GetLibraryBezierC2().y, expBezierC2.y );
    BOOST_CHECK_EQUAL( bezierFound->GetLibraryEnd().x, expBezierEnd.x );
    BOOST_CHECK_EQUAL( bezierFound->GetLibraryEnd().y, expBezierEnd.y );
    BOOST_CHECK_EQUAL( bezierFound->GetWidth(), expBezierWidth );

    PCB_SHAPE* polyFound = findPrimitive( padFound, SHAPE_T::POLY );
    BOOST_REQUIRE_MESSAGE( polyFound, "Polygon primitive not found in reloaded pad" );
    BOOST_REQUIRE( polyFound->IsPolyShapeValid() );

    checkPoly( expPoly, polyFound->GetLibraryPolyShape() );
    BOOST_CHECK_EQUAL( polyFound->GetWidth(), expPolyWidth );
}


BOOST_AUTO_TEST_CASE( ScaledFootprintRoundTripStable )
{
    auto board = std::make_unique<BOARD>();
    auto fp = std::make_unique<FOOTPRINT>( board.get() );
    fp->SetReference( "U1" );
    fp->SetPosition( VECTOR2I( mm( 50.0 ), mm( 50.0 ) ) );
    fp->SetTransformScale( 2.0, 1.5 );

    auto pad = std::make_unique<PAD>( fp.get() );
    pad->SetNumber( "1" );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetLayerSet( LSET( { F_Cu } ) );
    pad->SetLibSize( PADSTACK::ALL_LAYERS, VECTOR2I( mm( 1.0 ), mm( 1.0 ) ) );
    fp->Add( pad.release(), ADD_MODE::APPEND, true );
    board->Add( fp.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_scaled_fp_roundtrip", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* r = reloaded->Footprints().front();
    BOOST_CHECK_CLOSE( r->GetScaleX(), 2.0, 1e-6 );
    BOOST_CHECK_CLOSE( r->GetScaleY(), 1.5, 1e-6 );
}


BOOST_AUTO_TEST_CASE( PadPositionAndOrientationRoundTripStable )
{
    auto board = std::make_unique<BOARD>();
    auto fp = std::make_unique<FOOTPRINT>( board.get() );
    fp->SetReference( "U1" );
    fp->SetPosition( VECTOR2I( mm( 50.0 ), mm( 50.0 ) ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp->SetTransformScale( 2.0, 1.5 );

    auto pad = std::make_unique<PAD>( fp.get() );
    pad->SetNumber( "1" );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetLayerSet( LSET( { F_Cu } ) );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    pad->SetLibSize( PADSTACK::ALL_LAYERS, VECTOR2I( mm( 1.0 ), mm( 0.5 ) ) );
    pad->SetPosition( VECTOR2I( mm( 53.0 ), mm( 51.0 ) ) );
    pad->SetOrientation( EDA_ANGLE( 75.0, DEGREES_T ) );
    PAD* padPtr = pad.get();
    fp->Add( pad.release(), ADD_MODE::APPEND, true );
    board->Add( fp.release(), ADD_MODE::APPEND, true );

    const VECTOR2I  posBefore = padPtr->GetPosition();
    const EDA_ANGLE orientBefore = padPtr->GetOrientation();

    TEMP_FILE_HOLDER tmp( "kicad_qa_pad_pos_orient_roundtrip", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PAD* r = reloaded->Footprints().front()->Pads().front();
    BOOST_CHECK_EQUAL( r->GetPosition().x, posBefore.x );
    BOOST_CHECK_EQUAL( r->GetPosition().y, posBefore.y );
    BOOST_CHECK_CLOSE( r->GetOrientation().AsDegrees(), orientBefore.AsDegrees(), 1e-6 );
}


BOOST_AUTO_TEST_CASE( PadDeltaRoundTripStable )
{
    auto board = std::make_unique<BOARD>();
    auto fp = std::make_unique<FOOTPRINT>( board.get() );
    fp->SetReference( "U1" );
    fp->SetPosition( VECTOR2I( mm( 50.0 ), mm( 50.0 ) ) );
    fp->SetTransformScale( 2.0, 1.5 );

    auto pad = std::make_unique<PAD>( fp.get() );
    pad->SetNumber( "1" );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetLayerSet( LSET( { F_Cu } ) );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::TRAPEZOID );
    pad->SetLibSize( PADSTACK::ALL_LAYERS, VECTOR2I( mm( 1.0 ), mm( 1.0 ) ) );
    pad->SetDelta( PADSTACK::ALL_LAYERS, VECTOR2I( mm( 0.4 ), mm( 0.0 ) ) );
    PAD* padPtr = pad.get();
    fp->Add( pad.release(), ADD_MODE::APPEND, true );
    board->Add( fp.release(), ADD_MODE::APPEND, true );

    const VECTOR2I before = padPtr->GetDelta( PADSTACK::ALL_LAYERS );

    TEMP_FILE_HOLDER tmp( "kicad_qa_pad_delta_roundtrip", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PAD* r = reloaded->Footprints().front()->Pads().front();
    BOOST_TEST_MESSAGE( "delta before: " << before.x << "," << before.y
                                         << " after: " << r->GetDelta( PADSTACK::ALL_LAYERS ).x << ","
                                         << r->GetDelta( PADSTACK::ALL_LAYERS ).y );
    BOOST_CHECK_EQUAL( r->GetDelta( PADSTACK::ALL_LAYERS ).x, before.x );
    BOOST_CHECK_EQUAL( r->GetDelta( PADSTACK::ALL_LAYERS ).y, before.y );
}


BOOST_AUTO_TEST_CASE( PadOffsetRoundTripStable )
{
    auto board = std::make_unique<BOARD>();
    auto fp = std::make_unique<FOOTPRINT>( board.get() );
    fp->SetReference( "U1" );
    fp->SetPosition( VECTOR2I( mm( 50.0 ), mm( 50.0 ) ) );
    fp->SetTransformScale( 2.0, 1.5 );

    auto pad = std::make_unique<PAD>( fp.get() );
    pad->SetNumber( "1" );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetLayerSet( LSET( { F_Cu } ) );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    pad->SetLibSize( PADSTACK::ALL_LAYERS, VECTOR2I( mm( 1.0 ), mm( 1.0 ) ) );
    pad->SetLibOffset( PADSTACK::ALL_LAYERS, VECTOR2I( mm( 0.3 ), mm( 0.2 ) ) );
    PAD* padPtr = pad.get();
    fp->Add( pad.release(), ADD_MODE::APPEND, true );
    board->Add( fp.release(), ADD_MODE::APPEND, true );

    const VECTOR2I before = padPtr->GetOffset( PADSTACK::ALL_LAYERS );

    TEMP_FILE_HOLDER tmp( "kicad_qa_pad_offset_roundtrip", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PAD* r = reloaded->Footprints().front()->Pads().front();
    BOOST_TEST_MESSAGE( "offset before: " << before.x << "," << before.y
                                          << " after: " << r->GetOffset( PADSTACK::ALL_LAYERS ).x << ","
                                          << r->GetOffset( PADSTACK::ALL_LAYERS ).y );
    BOOST_CHECK_EQUAL( r->GetOffset( PADSTACK::ALL_LAYERS ).x, before.x );
    BOOST_CHECK_EQUAL( r->GetOffset( PADSTACK::ALL_LAYERS ).y, before.y );
}


BOOST_AUTO_TEST_CASE( FootprintPointRoundTripStable )
{
    auto board = std::make_unique<BOARD>();
    auto fp = std::make_unique<FOOTPRINT>( board.get() );
    fp->SetReference( "U1" );
    fp->SetPosition( VECTOR2I( mm( 50.0 ), mm( 50.0 ) ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp->SetTransformScale( 2.0, 1.5 );

    auto pt = std::make_unique<PCB_POINT>( fp.get() );
    pt->SetLayer( F_Cu );
    pt->SetPosition( VECTOR2I( mm( 55.0 ), mm( 52.0 ) ) );
    PCB_POINT* ptPtr = pt.get();
    fp->Add( pt.release(), ADD_MODE::APPEND, true );
    board->Add( fp.release(), ADD_MODE::APPEND, true );

    const VECTOR2I before = ptPtr->GetPosition();

    TEMP_FILE_HOLDER tmp( "kicad_qa_fp_point_roundtrip", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* r = reloaded->Footprints().front();
    BOOST_REQUIRE( !r->Points().empty() );
    PCB_POINT* rp = r->Points().front();
    BOOST_TEST_MESSAGE( "point before: " << before.x << "," << before.y << " after: " << rp->GetPosition().x << ","
                                         << rp->GetPosition().y );
    BOOST_CHECK_EQUAL( rp->GetPosition().x, before.x );
    BOOST_CHECK_EQUAL( rp->GetPosition().y, before.y );
}


BOOST_AUTO_TEST_CASE( RotatedPadSizeRoundTripStable )
{
    auto board = std::make_unique<BOARD>();
    auto fp = std::make_unique<FOOTPRINT>( board.get() );
    fp->SetReference( "U1" );
    fp->SetPosition( VECTOR2I( mm( 50.0 ), mm( 50.0 ) ) );
    fp->SetTransformScale( 2.0, 1.5 );

    auto pad = std::make_unique<PAD>( fp.get() );
    pad->SetNumber( "1" );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetLayerSet( LSET( { F_Cu } ) );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    pad->SetLibSize( PADSTACK::ALL_LAYERS, VECTOR2I( mm( 1.0 ), mm( 0.5 ) ) );
    pad->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    PAD* padPtr = pad.get();
    fp->Add( pad.release(), ADD_MODE::APPEND, true );
    board->Add( fp.release(), ADD_MODE::APPEND, true );

    const VECTOR2I boardSizeBefore = padPtr->GetSize( PADSTACK::ALL_LAYERS );

    TEMP_FILE_HOLDER tmp( "kicad_qa_rotated_pad_roundtrip", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PAD* r = reloaded->Footprints().front()->Pads().front();
    BOOST_CHECK_EQUAL( r->GetSize( PADSTACK::ALL_LAYERS ).x, boardSizeBefore.x );
    BOOST_CHECK_EQUAL( r->GetSize( PADSTACK::ALL_LAYERS ).y, boardSizeBefore.y );
}


// A footprint X-scale must stretch a pad along the footprint's X axis whatever the pad's
// own orientation, so the scale is conjugated into the pad frame (a 90deg pad swaps X/Y).
BOOST_AUTO_TEST_CASE( PadSizeScalesAlongFootprintAxisRegardlessOfPadOrientation )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    fp->SetOrientation( ANGLE_0 );
    board.Add( fp );

    auto makePad = [&]( const EDA_ANGLE& aRelOrient )
    {
        PAD* pad = new PAD( fp );
        pad->SetAttribute( PAD_ATTRIB::SMD );
        pad->SetLayerSet( LSET( { F_Cu } ) );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( mm( 1.0 ), mm( 0.5 ) ) );
        pad->SetOrientation( aRelOrient );
        fp->Add( pad );
        return pad;
    };

    PAD* pad0 = makePad( ANGLE_0 );
    PAD* pad90 = makePad( EDA_ANGLE( 90.0, DEGREES_T ) );

    const int xBefore0 = pad0->GetBoundingBox().GetWidth();
    const int xBefore90 = pad90->GetBoundingBox().GetWidth();

    fp->SetTransformScale( 2.0, 1.0 );

    BOOST_CHECK_EQUAL( pad0->GetSize( PADSTACK::ALL_LAYERS ).x, mm( 2.0 ) );
    BOOST_CHECK_EQUAL( pad0->GetSize( PADSTACK::ALL_LAYERS ).y, mm( 0.5 ) );
    BOOST_CHECK_EQUAL( pad90->GetSize( PADSTACK::ALL_LAYERS ).x, mm( 1.0 ) );
    BOOST_CHECK_EQUAL( pad90->GetSize( PADSTACK::ALL_LAYERS ).y, mm( 1.0 ) );

    // Both pads double their board-X extent, regardless of orientation.
    BOOST_CHECK_EQUAL( pad0->GetBoundingBox().GetWidth(), 2 * xBefore0 );
    BOOST_CHECK_EQUAL( pad90->GetBoundingBox().GetWidth(), 2 * xBefore90 );
}


BOOST_AUTO_TEST_SUITE_END()
