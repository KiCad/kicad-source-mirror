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

#include <board.h>
#include <board_design_settings.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_barcode.h>
#include <pcb_dimension.h>
#include <pcb_marker.h>
#include <pcb_point.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>
#include <zone.h>

#include <filesystem>


BOOST_AUTO_TEST_SUITE( FootprintTransformSync )


static void CHECK_TRANSFORM_MATCHES_LEGACY( const FOOTPRINT& aFp )
{
    BOOST_CHECK_EQUAL( aFp.GetTransform().GetTranslate().x, aFp.GetPosition().x );
    BOOST_CHECK_EQUAL( aFp.GetTransform().GetTranslate().y, aFp.GetPosition().y );
    BOOST_CHECK_EQUAL( aFp.GetTransform().GetRotate().AsDegrees(), aFp.GetOrientation().AsDegrees() );
    BOOST_CHECK_EQUAL( aFp.GetTransform().GetScaleX(), 1.0 );
    BOOST_CHECK_EQUAL( aFp.GetTransform().GetScaleY(), 1.0 );
}


BOOST_AUTO_TEST_CASE( DefaultIsIdentity )
{
    FOOTPRINT fp( nullptr );
    BOOST_CHECK( fp.GetTransform().IsIdentity() );
    CHECK_TRANSFORM_MATCHES_LEGACY( fp );
}


BOOST_AUTO_TEST_CASE( SetPositionUpdatesTransform )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 1234, -5678 ) );
    CHECK_TRANSFORM_MATCHES_LEGACY( fp );
}


BOOST_AUTO_TEST_CASE( SetOrientationUpdatesTransform )
{
    FOOTPRINT fp( nullptr );
    fp.SetOrientation( EDA_ANGLE( 45.0, DEGREES_T ) );
    CHECK_TRANSFORM_MATCHES_LEGACY( fp );
}


BOOST_AUTO_TEST_CASE( MoveUpdatesTransform )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 100, 200 ) );
    fp.Move( VECTOR2I( 50, -25 ) );
    CHECK_TRANSFORM_MATCHES_LEGACY( fp );
    BOOST_CHECK_EQUAL( fp.GetTransform().GetTranslate().x, 150 );
    BOOST_CHECK_EQUAL( fp.GetTransform().GetTranslate().y, 175 );
}


BOOST_AUTO_TEST_CASE( PadEffectiveShapeFollowsFootprintScale )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PAD* pad = new PAD( &fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 1000000, 1000000 ) );
    pad->SetPosition( VECTOR2I( 5000000, 0 ) );
    pad->SetLayerSet( PAD::SMDMask() );
    fp.Add( pad, ADD_MODE::APPEND );

    const VECTOR2I         posBefore = pad->ShapePos( F_Cu );
    std::shared_ptr<SHAPE> shapeBefore = pad->GetEffectiveShape( F_Cu, FLASHING::ALWAYS_FLASHED );
    const BOX2I            bboxBefore = shapeBefore->BBox();

    BOOST_CHECK_EQUAL( posBefore.x, 5000000 );
    BOOST_CHECK_EQUAL( bboxBefore.GetCenter().x, 5000000 );
    BOOST_CHECK_EQUAL( bboxBefore.GetSize().x, 1000000 );

    fp.SetTransformScale( 2.0, 1.0 );

    const VECTOR2I         posAfter = pad->ShapePos( F_Cu );
    std::shared_ptr<SHAPE> shapeAfter = pad->GetEffectiveShape( F_Cu, FLASHING::ALWAYS_FLASHED );
    const BOX2I            bboxAfter = shapeAfter->BBox();

    BOOST_CHECK_EQUAL( posAfter.x, 10000000 );
    BOOST_CHECK_EQUAL( bboxAfter.GetCenter().x, 10000000 );
    BOOST_CHECK_EQUAL( bboxAfter.GetSize().x, 1500000 );
}


BOOST_AUTO_TEST_CASE( PadBBoxFollowsFootprintMove )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PAD* pad = new PAD( &fp );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 1000000, 500000 ) );
    pad->SetPosition( VECTOR2I( 5000000, 0 ) );
    fp.Add( pad, ADD_MODE::APPEND );

    BOX2I before = pad->GetBoundingBox();
    BOOST_CHECK_EQUAL( before.GetCenter().x, 5000000 );

    fp.SetPosition( VECTOR2I( 10000000, 0 ) );

    BOX2I after = pad->GetBoundingBox();
    BOOST_CHECK_EQUAL( after.GetCenter().x, 15000000 );
}


BOOST_AUTO_TEST_CASE( PadBBoxFollowsFootprintRotation )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PAD* pad = new PAD( &fp );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 1000000, 500000 ) );
    pad->SetPosition( VECTOR2I( 5000000, 0 ) );
    fp.Add( pad, ADD_MODE::APPEND );

    BOX2I before = pad->GetBoundingBox();
    BOOST_CHECK_EQUAL( before.GetCenter().x, 5000000 );
    BOOST_CHECK_EQUAL( before.GetCenter().y, 0 );

    fp.SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    BOX2I after = pad->GetBoundingBox();
    BOOST_CHECK_EQUAL( after.GetCenter().x, 0 );
    BOOST_CHECK_EQUAL( after.GetCenter().y, -5000000 );
}


BOOST_AUTO_TEST_CASE( RotateUpdatesTransform )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 100, 0 ) );
    fp.Rotate( VECTOR2I( 0, 0 ), EDA_ANGLE( 90.0, DEGREES_T ) );
    CHECK_TRANSFORM_MATCHES_LEGACY( fp );
}


BOOST_AUTO_TEST_CASE( SetLayerUpdatesFlipped )
{
    FOOTPRINT fp( nullptr );
    BOOST_CHECK( !fp.IsFlipped() );

    fp.SetLayer( B_Cu );
    BOOST_CHECK( fp.IsFlipped() );

    fp.SetLayer( F_Cu );
    BOOST_CHECK( !fp.IsFlipped() );
}


BOOST_AUTO_TEST_CASE( CopyPreservesTransform )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 500, 700 ) );
    fp.SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp.SetLayer( B_Cu );

    FOOTPRINT copy( fp );
    CHECK_TRANSFORM_MATCHES_LEGACY( copy );
    BOOST_CHECK( copy.IsFlipped() );
}


// xform.Apply(libPos) must equal the pad's board position.
static void CHECK_TRANSFORM_PAD_INVARIANT( const FOOTPRINT& aFp )
{
    for( const PAD* pad : aFp.Pads() )
    {
        VECTOR2I libPos = pad->GetFPRelativePosition();
        VECTOR2I expected = aFp.GetTransform().Apply( libPos );
        VECTOR2I actual = pad->GetPosition();

        BOOST_CHECK_MESSAGE( std::abs( expected.x - actual.x ) <= 1 && std::abs( expected.y - actual.y ) <= 1,
                             "pad " << pad->GetNumber() << " expected ( " << expected.x << ", " << expected.y << " )"
                                    << " actual ( " << actual.x << ", " << actual.y << " )" );
    }
}


// Stored lib pos and the derived FP-relative pos must agree within 1 IU.
static void CHECK_PAD_LIBPOS_MIRROR( const FOOTPRINT& aFp )
{
    for( const PAD* pad : aFp.Pads() )
    {
        VECTOR2I derived = pad->GetFPRelativePosition();
        VECTOR2I stored = pad->GetLibraryPosition();

        BOOST_CHECK_MESSAGE( std::abs( derived.x - stored.x ) <= 1 && std::abs( derived.y - stored.y ) <= 1,
                             "pad " << pad->GetNumber() << " m_libPos ( " << stored.x << ", " << stored.y << " )"
                                    << " != GetFPRelativePosition ( " << derived.x << ", " << derived.y << " )" );
    }
}


struct BOARD_FIXTURE
{
    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( PadTransformInvariantOnLoadedBoard, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    for( const FOOTPRINT* fp : m_board->Footprints() )
        CHECK_TRANSFORM_PAD_INVARIANT( *fp );
}


BOOST_FIXTURE_TEST_CASE( PadTransformInvariantAfterMutation, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        fp->Move( VECTOR2I( 1234, -567 ) );
        CHECK_TRANSFORM_PAD_INVARIANT( *fp );

        fp->SetOrientation( fp->GetOrientation() + EDA_ANGLE( 17.5, DEGREES_T ) );
        CHECK_TRANSFORM_PAD_INVARIANT( *fp );

        fp->Flip( fp->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
        CHECK_TRANSFORM_PAD_INVARIANT( *fp );
    }
}


BOOST_FIXTURE_TEST_CASE( PadLibPosMirrorOnLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    for( const FOOTPRINT* fp : m_board->Footprints() )
        CHECK_PAD_LIBPOS_MIRROR( *fp );
}


BOOST_FIXTURE_TEST_CASE( PadLibPosMirrorAfterMutation, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        fp->Move( VECTOR2I( 1234, -567 ) );
        CHECK_PAD_LIBPOS_MIRROR( *fp );

        fp->SetOrientation( fp->GetOrientation() + EDA_ANGLE( 17.5, DEGREES_T ) );
        CHECK_PAD_LIBPOS_MIRROR( *fp );

        fp->Flip( fp->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
        CHECK_PAD_LIBPOS_MIRROR( *fp );
    }
}


BOOST_FIXTURE_TEST_CASE( UndoRedoRestoresFootprintAndPadState, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();

    VECTOR2I     origPos = fp->GetPosition();
    EDA_ANGLE    origOrient = fp->GetOrientation();
    PCB_LAYER_ID origLayer = fp->GetLayer();

    // Capture pad positions by number. Iteration order is not stable across swap.
    std::map<wxString, VECTOR2I> origPadByNumber;
    for( const PAD* pad : fp->Pads() )
        origPadByNumber[pad->GetNumber()] = pad->GetPosition();

    std::unique_ptr<FOOTPRINT> snapshot( static_cast<FOOTPRINT*>( fp->Clone() ) );

    fp->Move( VECTOR2I( 1234, -567 ) );
    fp->SetOrientation( fp->GetOrientation() + EDA_ANGLE( 45.0, DEGREES_T ) );
    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK( fp->GetPosition() != origPos );

    fp->SwapItemData( snapshot.get() );

    BOOST_CHECK_EQUAL( fp->GetPosition().x, origPos.x );
    BOOST_CHECK_EQUAL( fp->GetPosition().y, origPos.y );
    BOOST_CHECK_EQUAL( fp->GetOrientation().AsDegrees(), origOrient.AsDegrees() );
    BOOST_CHECK_EQUAL( fp->GetLayer(), origLayer );

    for( const PAD* pad : fp->Pads() )
    {
        VECTOR2I p = pad->GetPosition();
        VECTOR2I orig = origPadByNumber[pad->GetNumber()];
        BOOST_CHECK_MESSAGE( std::abs( p.x - orig.x ) <= 1 && std::abs( p.y - orig.y ) <= 1,
                             "pad " << pad->GetNumber() << " after undo at ( " << p.x << ", " << p.y << " )"
                                    << " expected ( " << orig.x << ", " << orig.y << " )" );
    }

    CHECK_TRANSFORM_PAD_INVARIANT( *fp );
    CHECK_PAD_LIBPOS_MIRROR( *fp );
}


// Stored lib start and the derived FP-relative pos must agree within 1 IU.
static void CHECK_SHAPE_LIBPOS_MIRROR( const FOOTPRINT& aFp )
{
    for( const BOARD_ITEM* item : aFp.GraphicalItems() )
    {
        if( item->Type() != PCB_SHAPE_T )
            continue;

        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );

        if( shape->GetShape() != SHAPE_T::SEGMENT && shape->GetShape() != SHAPE_T::RECTANGLE
            && shape->GetShape() != SHAPE_T::CIRCLE )
        {
            continue;
        }

        VECTOR2I libStartFromShape = shape->GetFPRelativePosition();
        VECTOR2I libStartStored = shape->GetLibraryStart();

        BOOST_CHECK_MESSAGE( std::abs( libStartFromShape.x - libStartStored.x ) <= 1
                                     && std::abs( libStartFromShape.y - libStartStored.y ) <= 1,
                             "shape m_libStart ( " << libStartStored.x << ", " << libStartStored.y << " ) "
                                                   << "!= GetFPRelativePosition ( " << libStartFromShape.x << ", "
                                                   << libStartFromShape.y << " )" );
    }
}


BOOST_FIXTURE_TEST_CASE( ShapeLibPosMirrorOnLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    for( const FOOTPRINT* fp : m_board->Footprints() )
        CHECK_SHAPE_LIBPOS_MIRROR( *fp );
}


BOOST_FIXTURE_TEST_CASE( ShapeLibPosMirrorAfterMutation, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        fp->Move( VECTOR2I( 1234, -567 ) );
        CHECK_SHAPE_LIBPOS_MIRROR( *fp );

        fp->SetOrientation( fp->GetOrientation() + EDA_ANGLE( 17.5, DEGREES_T ) );
        CHECK_SHAPE_LIBPOS_MIRROR( *fp );

        fp->Flip( fp->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
        CHECK_SHAPE_LIBPOS_MIRROR( *fp );
    }
}


BOOST_FIXTURE_TEST_CASE( SetTransformScaleDoublesPadOffsets, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    // Capture each pad's offset from the footprint anchor before scaling.
    std::map<wxString, VECTOR2I> origOffsets;
    VECTOR2I                     anchor = fp->GetPosition();

    for( const PAD* pad : fp->Pads() )
        origOffsets[pad->GetNumber()] = pad->GetPosition() - anchor;

    fp->SetTransformScale( 2.0, 2.0 );

    BOOST_CHECK_CLOSE( fp->GetTransform().GetScaleX(), 2.0, 1e-9 );
    BOOST_CHECK_CLOSE( fp->GetTransform().GetScaleY(), 2.0, 1e-9 );

    // Anchor doesn't move under SetTransformScale.
    BOOST_CHECK_EQUAL( fp->GetPosition().x, anchor.x );
    BOOST_CHECK_EQUAL( fp->GetPosition().y, anchor.y );

    // Each pad's offset from the anchor should have doubled.
    for( const PAD* pad : fp->Pads() )
    {
        VECTOR2I offset = pad->GetPosition() - anchor;
        VECTOR2I orig = origOffsets[pad->GetNumber()];

        BOOST_CHECK_MESSAGE( std::abs( offset.x - 2 * orig.x ) <= 1 && std::abs( offset.y - 2 * orig.y ) <= 1,
                             "pad " << pad->GetNumber() << " offset ( " << offset.x << ", " << offset.y << " ) "
                                    << "expected ( " << 2 * orig.x << ", " << 2 * orig.y << " )" );
    }
}


BOOST_FIXTURE_TEST_CASE( SetTransformScaleRebakeShapes, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    VECTOR2I anchor = fp->GetPosition();

    // Capture each segment's anchor-relative start offset before scaling.
    std::vector<VECTOR2I> origStartOffsets;

    for( const BOARD_ITEM* item : fp->GraphicalItems() )
    {
        if( item->Type() != PCB_SHAPE_T )
            continue;

        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );

        if( shape->GetShape() != SHAPE_T::SEGMENT )
            continue;

        origStartOffsets.push_back( shape->GetStart() - anchor );
    }

    fp->SetTransformScale( 2.0, 2.0 );

    size_t i = 0;
    for( const BOARD_ITEM* item : fp->GraphicalItems() )
    {
        if( item->Type() != PCB_SHAPE_T )
            continue;

        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );

        if( shape->GetShape() != SHAPE_T::SEGMENT )
            continue;

        VECTOR2I offset = shape->GetStart() - anchor;
        VECTOR2I orig = origStartOffsets[i++];

        BOOST_CHECK_MESSAGE( std::abs( offset.x - 2 * orig.x ) <= 1 && std::abs( offset.y - 2 * orig.y ) <= 1,
                             "shape start offset ( " << offset.x << ", " << offset.y << " ) "
                                                     << "expected ( " << 2 * orig.x << ", " << 2 * orig.y << " )" );
    }
}


BOOST_FIXTURE_TEST_CASE( ScalePersistsAcrossSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetTransformScale( 2.0, 3.0 );

    VECTOR2I                     origAnchor = fp->GetPosition();
    std::map<wxString, VECTOR2I> origPadPositions;
    for( const PAD* pad : fp->Pads() )
        origPadPositions[pad->GetNumber()] = pad->GetPosition();

    const std::filesystem::path savePath = std::filesystem::temp_directory_path() / "scale_roundtrip.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    BOOST_CHECK_CLOSE( fp2->GetTransform().GetScaleX(), 2.0, 1e-9 );
    BOOST_CHECK_CLOSE( fp2->GetTransform().GetScaleY(), 3.0, 1e-9 );
    BOOST_CHECK_EQUAL( fp2->GetPosition().x, origAnchor.x );
    BOOST_CHECK_EQUAL( fp2->GetPosition().y, origAnchor.y );

    for( const PAD* pad : fp2->Pads() )
    {
        VECTOR2I p = pad->GetPosition();
        VECTOR2I orig = origPadPositions[pad->GetNumber()];

        BOOST_CHECK_MESSAGE( std::abs( p.x - orig.x ) <= 1 && std::abs( p.y - orig.y ) <= 1,
                             "pad " << pad->GetNumber() << " after reload at ( " << p.x << ", " << p.y
                                    << " ) expected ( " << orig.x << ", " << orig.y << " )" );
    }
}


BOOST_FIXTURE_TEST_CASE( ScaleSurvivesUndoRedo, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    std::map<wxString, VECTOR2I> origPadPositions;
    for( const PAD* pad : fp->Pads() )
        origPadPositions[pad->GetNumber()] = pad->GetPosition();

    std::unique_ptr<FOOTPRINT> snapshot( static_cast<FOOTPRINT*>( fp->Clone() ) );

    fp->SetTransformScale( 2.0, 2.0 );

    BOOST_CHECK_CLOSE( fp->GetTransform().GetScaleX(), 2.0, 1e-9 );

    fp->SwapItemData( snapshot.get() );

    BOOST_CHECK_CLOSE( fp->GetTransform().GetScaleX(), 1.0, 1e-9 );
    BOOST_CHECK_CLOSE( fp->GetTransform().GetScaleY(), 1.0, 1e-9 );

    for( const PAD* pad : fp->Pads() )
    {
        VECTOR2I p = pad->GetPosition();
        VECTOR2I orig = origPadPositions[pad->GetNumber()];

        BOOST_CHECK_MESSAGE( std::abs( p.x - orig.x ) <= 1 && std::abs( p.y - orig.y ) <= 1,
                             "pad " << pad->GetNumber() << " after undo at ( " << p.x << ", " << p.y << " ) expected ( "
                                    << orig.x << ", " << orig.y << " )" );
    }
}


BOOST_AUTO_TEST_CASE( SetTransformScaleScalesZoneOutline )
{
    // Footprint with a single keepout zone outline (a 2 mm square 1 mm off the
    // anchor). Scaling the footprint should scale the outline.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 1000000, 500000 ) ); // 1mm, 0.5mm

    ZONE* zone = new ZONE( &fp );

    SHAPE_POLY_SET poly;
    poly.NewOutline();
    VECTOR2I anchor = fp.GetPosition();
    poly.Append( anchor + VECTOR2I( 1000000, 1000000 ) );
    poly.Append( anchor + VECTOR2I( 3000000, 1000000 ) );
    poly.Append( anchor + VECTOR2I( 3000000, 3000000 ) );
    poly.Append( anchor + VECTOR2I( 1000000, 3000000 ) );

    *zone->Outline() = poly;
    fp.Add( zone, ADD_MODE::APPEND );

    // Capture original board-frame offsets for each vertex.
    SHAPE_POLY_SET        origBoard = zone->GetBoardOutline();
    std::vector<VECTOR2I> origOffsets;
    for( int i = 0; i < origBoard.TotalVertices(); ++i )
        origOffsets.push_back( origBoard.CVertex( i ) - anchor );

    fp.SetTransformScale( 2.0, 2.0 );

    SHAPE_POLY_SET scaledBoard = zone->GetBoardOutline();
    BOOST_REQUIRE_EQUAL( scaledBoard.TotalVertices(), (int) origOffsets.size() );

    for( int i = 0; i < scaledBoard.TotalVertices(); ++i )
    {
        VECTOR2I offset = scaledBoard.CVertex( i ) - anchor;
        BOOST_CHECK_MESSAGE( std::abs( offset.x - 2 * origOffsets[i].x ) <= 1
                                     && std::abs( offset.y - 2 * origOffsets[i].y ) <= 1,
                             "vertex " << i << " offset ( " << offset.x << ", " << offset.y << " ) expected ( "
                                       << 2 * origOffsets[i].x << ", " << 2 * origOffsets[i].y << " )" );
    }
}


BOOST_FIXTURE_TEST_CASE( SetTransformScaleScalesPadAndDrillSize, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    std::map<wxString, VECTOR2I> origSizes;
    std::map<wxString, VECTOR2I> origDrills;
    for( const PAD* pad : fp->Pads() )
    {
        origSizes[pad->GetNumber()] = pad->GetSize( PADSTACK::ALL_LAYERS );
        origDrills[pad->GetNumber()] = pad->GetDrillSize();
    }

    fp->SetTransformScale( 2.0, 2.0 );

    for( const PAD* pad : fp->Pads() )
    {
        VECTOR2I size = pad->GetSize( PADSTACK::ALL_LAYERS );
        VECTOR2I drill = pad->GetDrillSize();
        VECTOR2I origSize = origSizes[pad->GetNumber()];
        VECTOR2I origDrill = origDrills[pad->GetNumber()];

        BOOST_CHECK_EQUAL( size.x, 2 * origSize.x );
        BOOST_CHECK_EQUAL( size.y, 2 * origSize.y );
        BOOST_CHECK_EQUAL( drill.x, 2 * origDrill.x );
        BOOST_CHECK_EQUAL( drill.y, 2 * origDrill.y );
    }
}


BOOST_FIXTURE_TEST_CASE( SetTransformScaleScalesTextSize, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    BOOST_REQUIRE( !fp->GetFields().empty() );

    PCB_FIELD* refField = fp->GetFields().front();
    VECTOR2I   origSize = refField->GetTextSize();
    int        origThickness = refField->GetTextThickness();

    fp->SetTransformScale( 2.0, 2.0 );

    BOOST_CHECK_EQUAL( refField->GetTextSize().x, 2 * origSize.x );
    BOOST_CHECK_EQUAL( refField->GetTextSize().y, 2 * origSize.y );
    BOOST_CHECK_EQUAL( refField->GetTextThickness(), 2 * origThickness );
}


BOOST_FIXTURE_TEST_CASE( SetTransformScaleInvalidatesTextBBoxCache, BOARD_FIXTURE )
{
    // GetTextBox caches per line. SetTransformScale must clear that cache so
    // the next read sees the new size.
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );
    BOOST_REQUIRE( !fp->GetFields().empty() );

    PCB_FIELD* refField = fp->GetFields().front();

    // Prime the bbox cache.
    BOX2I origBox = refField->GetTextBox( nullptr );
    BOOST_REQUIRE( origBox.GetWidth() > 0 && origBox.GetHeight() > 0 );

    fp->SetTransformScale( 2.0, 2.0 );

    BOX2I scaledBox = refField->GetTextBox( nullptr );

    // Roughly 2x in each dimension. Allow 10% difference for text metrics. A stale
    // cache would return origBox.
    BOOST_CHECK_GT( scaledBox.GetWidth(), origBox.GetWidth() );
    BOOST_CHECK_GT( scaledBox.GetHeight(), origBox.GetHeight() );
    BOOST_CHECK_CLOSE( (double) scaledBox.GetWidth(), 2.0 * origBox.GetWidth(), 10.0 );
    BOOST_CHECK_CLOSE( (double) scaledBox.GetHeight(), 2.0 * origBox.GetHeight(), 10.0 );
}


BOOST_FIXTURE_TEST_CASE( SetTransformScaleScalesShapeLineWidth, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    PCB_SHAPE* shape = nullptr;

    for( BOARD_ITEM* item : fp->GraphicalItems() )
    {
        if( item->Type() == PCB_SHAPE_T )
        {
            shape = static_cast<PCB_SHAPE*>( item );
            break;
        }
    }

    BOOST_REQUIRE( shape );

    int origWidth = shape->GetStroke().GetWidth();

    fp->SetTransformScale( 2.0, 2.0 );

    BOOST_CHECK_EQUAL( shape->GetStroke().GetWidth(), 2 * origWidth );
}


BOOST_FIXTURE_TEST_CASE( DRCFlagsScaledFootprintWithPads, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );
    BOOST_REQUIRE( !fp->Pads().empty() );

    fp->SetTransformScale( 2.0, 2.0 );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Silence unrelated DRC checks so we can read the violation list cleanly.
    for( int code = DRCE_FIRST; code <= DRCE_LAST; ++code )
    {
        if( code != DRCE_FOOTPRINT_SCALED_WITH_PADS )
            bds.m_DRCSeverities[code] = SEVERITY::RPT_SEVERITY_IGNORE;
    }

    bds.m_DRCSeverities[DRCE_FOOTPRINT_SCALED_WITH_PADS] = SEVERITY::RPT_SEVERITY_WARNING;

    std::vector<int> violations;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                violations.push_back( aItem->GetErrorCode() );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bool found = false;
    for( int code : violations )
    {
        if( code == DRCE_FOOTPRINT_SCALED_WITH_PADS )
        {
            found = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( found, "expected DRCE_FOOTPRINT_SCALED_WITH_PADS for a 2x scaled "
                                "footprint with pads; got "
                                        << violations.size() << " violations" );
}


BOOST_FIXTURE_TEST_CASE( DRCSilentForUnscaledFootprintWithPads, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    for( int code = DRCE_FIRST; code <= DRCE_LAST; ++code )
    {
        if( code != DRCE_FOOTPRINT_SCALED_WITH_PADS )
            bds.m_DRCSeverities[code] = SEVERITY::RPT_SEVERITY_IGNORE;
    }

    bds.m_DRCSeverities[DRCE_FOOTPRINT_SCALED_WITH_PADS] = SEVERITY::RPT_SEVERITY_WARNING;

    int scaledFlagged = 0;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_FOOTPRINT_SCALED_WITH_PADS )
                    scaledFlagged++;
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_CHECK_EQUAL( scaledFlagged, 0 );
}


BOOST_FIXTURE_TEST_CASE( RescaleAroundPointKeepsCenterFixed, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    // Center is offset from the anchor so the translate change is non-trivial.
    VECTOR2I center( fp->GetPosition().x + 10000, fp->GetPosition().y + 5000 );

    fp->RescaleAroundPoint( center, 2.0, 2.0 );

    VECTOR2I expectedAnchor( center.x + 2 * ( fp->GetTransform().GetTranslate().x - center.x ) / 2,
                             center.y + 2 * ( fp->GetTransform().GetTranslate().y - center.y ) / 2 );

    BOOST_CHECK_CLOSE( fp->GetTransform().GetScaleX(), 2.0, 1e-9 );
    BOOST_CHECK_CLOSE( fp->GetTransform().GetScaleY(), 2.0, 1e-9 );

    // Pad invariants still hold after the rescale.
    CHECK_TRANSFORM_PAD_INVARIANT( *fp );
    CHECK_PAD_LIBPOS_MIRROR( *fp );
}


BOOST_FIXTURE_TEST_CASE( RescaleAroundPointMovesAnchorByExpectedDelta, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    VECTOR2I origAnchor = fp->GetPosition();
    VECTOR2I center( origAnchor.x - 50000, origAnchor.y + 30000 );
    double   sx = 3.0;
    double   sy = 3.0;

    fp->RescaleAroundPoint( center, sx, sy );

    // new_anchor = center + s * (old_anchor - center)
    VECTOR2I expected( KiROUND( center.x + sx * ( origAnchor.x - center.x ) ),
                       KiROUND( center.y + sy * ( origAnchor.y - center.y ) ) );

    BOOST_CHECK_EQUAL( fp->GetPosition().x, expected.x );
    BOOST_CHECK_EQUAL( fp->GetPosition().y, expected.y );
}


BOOST_FIXTURE_TEST_CASE( RescaleAroundPointComposesScale, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    VECTOR2I origAnchor = fp->GetPosition();
    VECTOR2I center( origAnchor.x + 12345, origAnchor.y - 6789 );

    fp->RescaleAroundPoint( center, 2.0, 2.0 );
    fp->RescaleAroundPoint( center, 0.5, 0.5 );

    // Scale composes back to identity within float tolerance.
    BOOST_CHECK_CLOSE( fp->GetTransform().GetScaleX(), 1.0, 1e-9 );
    BOOST_CHECK_CLOSE( fp->GetTransform().GetScaleY(), 1.0, 1e-9 );

    // Anchor returns close to the original (rounding may shift by a few IU).
    BOOST_CHECK_MESSAGE( std::abs( fp->GetPosition().x - origAnchor.x ) <= 2
                                 && std::abs( fp->GetPosition().y - origAnchor.y ) <= 2,
                         "anchor drifted: now ( " << fp->GetPosition().x << ", " << fp->GetPosition().y
                                                  << " ) expected ( " << origAnchor.x << ", " << origAnchor.y << " )" );
}


BOOST_FIXTURE_TEST_CASE( RatsnestFollowsScaledPads, BOARD_FIXTURE )
{
    // Ratsnest endpoints are cached at connectivity build time. After scaling
    // the footprint, recalc must rebuild against the new pad positions.
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    std::shared_ptr<CONNECTIVITY_DATA> conn = m_board->GetConnectivity();
    BOOST_REQUIRE( conn );

    conn->RecalculateRatsnest();

    FOOTPRINT* targetFp = nullptr;
    PAD*       targetPad = nullptr;

    // Find a pad with at least one ratsnest edge.
    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( !conn->GetRatsnestForPad( pad ).empty() )
            {
                targetFp = fp;
                targetPad = pad;
                break;
            }
        }

        if( targetPad )
            break;
    }

    if( !targetPad )
    {
        BOOST_TEST_MESSAGE( "issue18 has no unconnected pads, skipping" );
        return;
    }

    targetFp->SetTransformScale( 2.0, 2.0 );
    conn->RecalculateRatsnest();

    VECTOR2I newPadPos = targetPad->GetPosition();
    auto     edges = conn->GetRatsnestForPad( targetPad );
    BOOST_REQUIRE( !edges.empty() );

    // At least one edge must touch the new pad position.
    bool found = false;

    for( const CN_EDGE& e : edges )
    {
        if( e.GetSourcePos() == newPadPos || e.GetTargetPos() == newPadPos )
        {
            found = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( found, "no ratsnest edge originates at the scaled pad position ( " << newPadPos.x << ", "
                                                                                            << newPadPos.y << " )" );
}


BOOST_FIXTURE_TEST_CASE( SetTransformScaleInvalidatesBoundingBoxCache, BOARD_FIXTURE )
{
    // After SetTransformScale, the bounding box cache must drop so the next
    // read rebuilds from the scaled geometry.
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    // Prime the cache and capture original size relative to the anchor.
    BOX2I    before = fp->GetBoundingBox();
    VECTOR2I anchor = fp->GetPosition();
    int      origWidth = before.GetWidth();
    int      origHeight = before.GetHeight();
    BOOST_REQUIRE( origWidth > 0 && origHeight > 0 );

    fp->SetTransformScale( 2.0, 2.0 );

    BOX2I after = fp->GetBoundingBox();

    // Roughly 2x in each dimension. Allow 5% slop for text stroke rounding.
    BOOST_CHECK_CLOSE( (double) after.GetWidth(), 2.0 * origWidth, 5.0 );
    BOOST_CHECK_CLOSE( (double) after.GetHeight(), 2.0 * origHeight, 5.0 );

    // Anchor stays put under SetTransformScale.
    BOOST_CHECK_EQUAL( fp->GetPosition().x, anchor.x );
    BOOST_CHECK_EQUAL( fp->GetPosition().y, anchor.y );
}


BOOST_FIXTURE_TEST_CASE( SetTransformScaleInvalidatesCourtyardCache, BOARD_FIXTURE )
{
    // After scale, GetCourtyard must rebuild from the scaled coords.
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = nullptr;

    for( FOOTPRINT* candidate : m_board->Footprints() )
    {
        if( !candidate->GetCourtyard( F_CrtYd ).IsEmpty() )
        {
            fp = candidate;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( fp, "no footprint with a front courtyard in issue18" );

    SHAPE_POLY_SET before = fp->GetCourtyard( F_CrtYd );
    double         areaBefore = before.Area();
    BOOST_REQUIRE( areaBefore > 0.0 );

    fp->SetTransformScale( 2.0, 2.0 );

    SHAPE_POLY_SET after = fp->GetCourtyard( F_CrtYd );
    double         areaAfter = after.Area();

    // 2x linear scale gives 4x area. Allow 1% drift for polygon Inflate offsets.
    BOOST_CHECK_CLOSE( areaAfter, 4.0 * areaBefore, 1.0 );
}


BOOST_AUTO_TEST_CASE( SetTransformScaleInvalidatesZoneFill )
{
    // After scale, a pre-filled zone must be marked for refill and the cached
    // fill cleared.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    ZONE* zone = new ZONE( &fp );

    SHAPE_POLY_SET outline;
    outline.NewOutline();
    outline.Append( VECTOR2I( 0, 0 ) );
    outline.Append( VECTOR2I( 1000000, 0 ) );
    outline.Append( VECTOR2I( 1000000, 1000000 ) );
    outline.Append( VECTOR2I( 0, 1000000 ) );
    *zone->Outline() = outline;

    // Pretend the zone has been filled and is up to date.
    zone->SetIsFilled( true );
    zone->SetNeedRefill( false );

    fp.Add( zone, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 2.0 );

    BOOST_CHECK( zone->NeedRefill() );
    BOOST_CHECK( !zone->IsFilled() );
}


BOOST_AUTO_TEST_CASE( MultiFootprintScaleSelectionCenterIsBBoxCenter )
{
    FOOTPRINT a( nullptr );
    FOOTPRINT b( nullptr );

    a.SetPosition( VECTOR2I( 0, 0 ) );
    b.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* aShape = new PCB_SHAPE( &a, SHAPE_T::RECTANGLE );
    aShape->SetLayer( F_SilkS );
    aShape->SetStart( VECTOR2I( -1000000, -1000000 ) );
    aShape->SetEnd( VECTOR2I( 1000000, 1000000 ) );
    a.Add( aShape, ADD_MODE::APPEND );

    PCB_SHAPE* bShape = new PCB_SHAPE( &b, SHAPE_T::RECTANGLE );
    bShape->SetLayer( F_SilkS );
    bShape->SetStart( VECTOR2I( 10000000, 10000000 ) );
    bShape->SetEnd( VECTOR2I( 20000000, 15000000 ) );
    b.Add( bShape, ADD_MODE::APPEND );

    BOX2I selBBox;
    selBBox.Merge( a.GetBoundingBox() );
    selBBox.Merge( b.GetBoundingBox() );
    const VECTOR2I bboxCenter = selBBox.GetCenter();
    const VECTOR2I anchorMean( ( a.GetPosition().x + b.GetPosition().x ) / 2,
                               ( a.GetPosition().y + b.GetPosition().y ) / 2 );

    BOOST_REQUIRE( bboxCenter != anchorMean );

    a.RescaleAroundPoint( bboxCenter, 2.0, 2.0 );
    b.RescaleAroundPoint( bboxCenter, 2.0, 2.0 );

    BOOST_CHECK_EQUAL( a.GetPosition().x, bboxCenter.x + 2 * ( 0 - bboxCenter.x ) );
    BOOST_CHECK_EQUAL( a.GetPosition().y, bboxCenter.y + 2 * ( 0 - bboxCenter.y ) );
    BOOST_CHECK_EQUAL( b.GetPosition().x, bboxCenter.x + 2 * ( 0 - bboxCenter.x ) );
    BOOST_CHECK_EQUAL( b.GetPosition().y, bboxCenter.y + 2 * ( 0 - bboxCenter.y ) );
}


BOOST_AUTO_TEST_CASE( MultiFootprintScaleAroundSelectionCenter )
{
    // Scale 2x around the midpoint of two footprints. Each anchor must move to
    // center + 2 * (orig - center).
    FOOTPRINT a( nullptr );
    FOOTPRINT b( nullptr );

    a.SetPosition( VECTOR2I( 0, 0 ) );
    b.SetPosition( VECTOR2I( 1000, 500 ) );

    VECTOR2I center( ( 0 + 1000 ) / 2, ( 0 + 500 ) / 2 );

    auto applyOnSelection = [&]( double sx, double sy )
    {
        double relSxA = sx / a.GetScaleX();
        double relSyA = sy / a.GetScaleY();
        a.RescaleAroundPoint( center, relSxA, relSyA );

        double relSxB = sx / b.GetScaleX();
        double relSyB = sy / b.GetScaleY();
        b.RescaleAroundPoint( center, relSxB, relSyB );
    };

    applyOnSelection( 2.0, 2.0 );

    BOOST_CHECK_EQUAL( a.GetPosition().x, center.x + 2 * ( 0 - center.x ) );
    BOOST_CHECK_EQUAL( a.GetPosition().y, center.y + 2 * ( 0 - center.y ) );
    BOOST_CHECK_EQUAL( b.GetPosition().x, center.x + 2 * ( 1000 - center.x ) );
    BOOST_CHECK_EQUAL( b.GetPosition().y, center.y + 2 * ( 500 - center.y ) );

    BOOST_CHECK_CLOSE( a.GetScaleX(), 2.0, 1e-9 );
    BOOST_CHECK_CLOSE( a.GetScaleY(), 2.0, 1e-9 );
    BOOST_CHECK_CLOSE( b.GetScaleX(), 2.0, 1e-9 );
    BOOST_CHECK_CLOSE( b.GetScaleY(), 2.0, 1e-9 );
}


BOOST_FIXTURE_TEST_CASE( ScaleXSetterPreservesOtherAxis, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetTransformScale( 1.5, 0.75 );

    fp->SetScaleX( 3.0 );

    BOOST_CHECK_CLOSE( fp->GetScaleX(), 3.0, 1e-9 );
    BOOST_CHECK_CLOSE( fp->GetScaleY(), 0.75, 1e-9 );
    CHECK_TRANSFORM_PAD_INVARIANT( *fp );
}


BOOST_FIXTURE_TEST_CASE( ScaleYSetterPreservesOtherAxis, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetTransformScale( 1.5, 0.75 );

    fp->SetScaleY( 4.0 );

    BOOST_CHECK_CLOSE( fp->GetScaleX(), 1.5, 1e-9 );
    BOOST_CHECK_CLOSE( fp->GetScaleY(), 4.0, 1e-9 );
    CHECK_TRANSFORM_PAD_INVARIANT( *fp );
}


BOOST_FIXTURE_TEST_CASE( PromotedArcSerializesAsLibArc, BOARD_FIXTURE )
{
    // An ARC promoted to ELLIPSE_ARC by non-uniform scale must still write as fp_arc.
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    VECTOR2I anchor = fp->GetPosition();

    PCB_SHAPE* arc = new PCB_SHAPE( fp, SHAPE_T::ARC );
    arc->SetLayer( F_SilkS );
    arc->SetArcGeometry( anchor + VECTOR2I( 1000000, 0 ), anchor + VECTOR2I( 707107, 707107 ),
                         anchor + VECTOR2I( 0, 1000000 ) );
    fp->Add( arc, ADD_MODE::APPEND );

    fp->SetTransformScale( 2.0, 1.0 );
    BOOST_REQUIRE_EQUAL( static_cast<int>( arc->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE_ARC ) );

    const std::filesystem::path savePath = std::filesystem::temp_directory_path() / "promoted_arc_roundtrip.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    bool foundArc = false;

    for( const BOARD_ITEM* item : fp2->GraphicalItems() )
    {
        if( item->Type() != PCB_SHAPE_T )
            continue;

        const PCB_SHAPE* s = static_cast<const PCB_SHAPE*>( item );

        if( s->GetLibraryShape() == SHAPE_T::ARC )
        {
            foundArc = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( foundArc, "promoted arc did not round-trip as a lib-frame arc" );
}


BOOST_FIXTURE_TEST_CASE( PromotedCircleSerializesAsLibCircle, BOARD_FIXTURE )
{
    // A CIRCLE promoted to ELLIPSE by non-uniform scale must still write as
    // fp_circle. The on-disk form follows the lib frame.
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    PCB_SHAPE* circle = new PCB_SHAPE( fp, SHAPE_T::CIRCLE );
    circle->SetLayer( F_SilkS );
    circle->SetStart( fp->GetPosition() );
    circle->SetEnd( fp->GetPosition() + VECTOR2I( 1000000, 0 ) );
    fp->Add( circle, ADD_MODE::APPEND );

    fp->SetTransformScale( 2.0, 1.0 );
    BOOST_REQUIRE_EQUAL( static_cast<int>( circle->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE ) );

    const std::filesystem::path savePath =
            std::filesystem::temp_directory_path() / "promoted_circle_roundtrip.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    bool foundCircle = false;

    for( const BOARD_ITEM* item : fp2->GraphicalItems() )
    {
        if( item->Type() != PCB_SHAPE_T )
            continue;

        const PCB_SHAPE* s = static_cast<const PCB_SHAPE*>( item );

        if( s->GetLibraryShape() == SHAPE_T::CIRCLE )
        {
            foundCircle = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( foundCircle, "promoted circle did not round-trip as a lib-frame circle" );
}


BOOST_FIXTURE_TEST_CASE( NativeEllipseRoundTripsThroughRotatedFootprint, BOARD_FIXTURE )
{
    // Native ellipse must survive save and load with a rotated parent FP.
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetOrientation( ANGLE_0 );

    PCB_SHAPE* ellipse = new PCB_SHAPE( fp, SHAPE_T::ELLIPSE );
    ellipse->SetLayer( F_SilkS );
    ellipse->SetEllipseCenter( fp->GetPosition() + VECTOR2I( 1000000, 0 ) );
    ellipse->SetEllipseMajorRadius( 500000 );
    ellipse->SetEllipseMinorRadius( 250000 );
    ellipse->SetEllipseRotation( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp->Add( ellipse, ADD_MODE::APPEND );

    const VECTOR2I  libCenter = ellipse->GetLibraryEllipseCenter();
    const int       libMajor = ellipse->GetLibraryEllipseMajorRadius();
    const int       libMinor = ellipse->GetLibraryEllipseMinorRadius();
    const EDA_ANGLE libRotation = ellipse->GetLibraryEllipseRotation();

    BOOST_CHECK_EQUAL( libCenter.x, 1000000 );
    BOOST_CHECK_EQUAL( libCenter.y, 0 );
    BOOST_CHECK_EQUAL( libMajor, 500000 );
    BOOST_CHECK_EQUAL( libMinor, 250000 );
    BOOST_CHECK_CLOSE( libRotation.AsDegrees(), 30.0, 0.01 );

    fp->SetOrientation( EDA_ANGLE( 60.0, DEGREES_T ) );

    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseCenter().x, libCenter.x );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseCenter().y, libCenter.y );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseMajorRadius(), libMajor );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseMinorRadius(), libMinor );
    BOOST_CHECK_CLOSE( ellipse->GetLibraryEllipseRotation().AsDegrees(),
                       libRotation.AsDegrees(), 0.01 );

    const std::filesystem::path savePath =
            std::filesystem::temp_directory_path() / "native_ellipse_rotated_roundtrip.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    const PCB_SHAPE* ellipse2 = nullptr;

    for( const BOARD_ITEM* item : fp2->GraphicalItems() )
    {
        if( item->Type() != PCB_SHAPE_T )
            continue;

        const PCB_SHAPE* s = static_cast<const PCB_SHAPE*>( item );

        if( s->GetLibraryShape() == SHAPE_T::ELLIPSE )
        {
            ellipse2 = s;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( ellipse2, "native ellipse not found after reload" );

    BOOST_CHECK_EQUAL( ellipse2->GetLibraryEllipseCenter().x, libCenter.x );
    BOOST_CHECK_EQUAL( ellipse2->GetLibraryEllipseCenter().y, libCenter.y );
    BOOST_CHECK_EQUAL( ellipse2->GetLibraryEllipseMajorRadius(), libMajor );
    BOOST_CHECK_EQUAL( ellipse2->GetLibraryEllipseMinorRadius(), libMinor );
    BOOST_CHECK_CLOSE( ellipse2->GetLibraryEllipseRotation().AsDegrees(),
                       libRotation.AsDegrees(), 0.01 );
}


BOOST_AUTO_TEST_CASE( NativeEllipseRebakesUnderUniformScale )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* ellipse = new PCB_SHAPE( &fp, SHAPE_T::ELLIPSE );
    ellipse->SetEllipseCenter( VECTOR2I( 1000000, 0 ) );
    ellipse->SetEllipseMajorRadius( 500000 );
    ellipse->SetEllipseMinorRadius( 250000 );
    ellipse->SetEllipseRotation( ANGLE_0 );
    fp.Add( ellipse, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 2.0 );

    BOOST_CHECK_EQUAL( static_cast<int>( ellipse->GetShape() ),
                       static_cast<int>( SHAPE_T::ELLIPSE ) );
    BOOST_CHECK_EQUAL( ellipse->GetEllipseCenter().x, 2000000 );
    BOOST_CHECK_EQUAL( ellipse->GetEllipseMajorRadius(), 1000000 );
    BOOST_CHECK_EQUAL( ellipse->GetEllipseMinorRadius(), 500000 );

    // Lib values should be unchanged.
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseCenter().x, 1000000 );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseMajorRadius(), 500000 );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseMinorRadius(), 250000 );
}


BOOST_FIXTURE_TEST_CASE( SizesAreStableAcrossSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );
    BOOST_REQUIRE( !fp->Pads().empty() );

    PAD*           pad = *fp->Pads().begin();
    const VECTOR2I baselineSize = pad->GetSize( PADSTACK::ALL_LAYERS );

    fp->SetTransformScale( 2.0, 1.0 );
    const VECTOR2I scaledSize = pad->GetSize( PADSTACK::ALL_LAYERS );
    BOOST_CHECK_EQUAL( scaledSize.x, baselineSize.x * 2 );
    BOOST_CHECK_EQUAL( scaledSize.y, baselineSize.y );

    const std::filesystem::path savePath = std::filesystem::temp_directory_path() / "scale_size_roundtrip_a.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );
    PAD* pad2 = *fp2->Pads().begin();
    BOOST_CHECK_EQUAL( pad2->GetSize( PADSTACK::ALL_LAYERS ).x, scaledSize.x );
    BOOST_CHECK_EQUAL( pad2->GetSize( PADSTACK::ALL_LAYERS ).y, scaledSize.y );

    const std::filesystem::path savePath2 = std::filesystem::temp_directory_path() / "scale_size_roundtrip_b.kicad_pcb";

    KI_TEST::DumpBoardToFile( *reloaded, savePath2.string() );
    std::unique_ptr<BOARD> reloaded2 = KI_TEST::ReadBoardFromFileOrStream( savePath2.string() );
    BOOST_REQUIRE( reloaded2 );

    FOOTPRINT* fp3 = *reloaded2->Footprints().begin();
    BOOST_REQUIRE( fp3 );
    PAD* pad3 = *fp3->Pads().begin();
    BOOST_CHECK_EQUAL( pad3->GetSize( PADSTACK::ALL_LAYERS ).x, scaledSize.x );
    BOOST_CHECK_EQUAL( pad3->GetSize( PADSTACK::ALL_LAYERS ).y, scaledSize.y );
}


BOOST_AUTO_TEST_CASE( PolyShapeLibMirrorDoesNotDriftOnTranslate )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE*     poly = new PCB_SHAPE( &fp, SHAPE_T::POLY );
    SHAPE_POLY_SET pts;
    pts.NewOutline();
    pts.Append( VECTOR2I( 0, 0 ) );
    pts.Append( VECTOR2I( 1000000, 0 ) );
    pts.Append( VECTOR2I( 1000000, 1000000 ) );
    pts.Append( VECTOR2I( 0, 1000000 ) );
    poly->SetPolyShape( pts );
    fp.Add( poly, ADD_MODE::APPEND );

    const VECTOR2I libStart0 = poly->GetLibraryStart();
    const VECTOR2I libEnd0 = poly->GetLibraryEnd();

    fp.SetPosition( VECTOR2I( 5000000, 3000000 ) );
    fp.SetPosition( VECTOR2I( 10000000, 6000000 ) );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    BOOST_CHECK_EQUAL( poly->GetLibraryStart().x, libStart0.x );
    BOOST_CHECK_EQUAL( poly->GetLibraryStart().y, libStart0.y );
    BOOST_CHECK_EQUAL( poly->GetLibraryEnd().x, libEnd0.x );
    BOOST_CHECK_EQUAL( poly->GetLibraryEnd().y, libEnd0.y );
}


BOOST_AUTO_TEST_CASE( PolyShapeScalesWithFootprint )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE*     poly = new PCB_SHAPE( &fp, SHAPE_T::POLY );
    SHAPE_POLY_SET pts;
    pts.NewOutline();
    pts.Append( VECTOR2I( 0, 0 ) );
    pts.Append( VECTOR2I( 1000000, 0 ) );
    pts.Append( VECTOR2I( 1000000, 1000000 ) );
    pts.Append( VECTOR2I( 0, 1000000 ) );
    poly->SetPolyShape( pts );
    fp.Add( poly, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.0 );

    const SHAPE_POLY_SET& out = poly->GetPolyShape();
    BOOST_REQUIRE( out.OutlineCount() == 1 );
    BOOST_CHECK_EQUAL( out.Outline( 0 ).CPoint( 1 ).x, 2000000 );
    BOOST_CHECK_EQUAL( out.Outline( 0 ).CPoint( 2 ).x, 2000000 );
    BOOST_CHECK_EQUAL( out.Outline( 0 ).CPoint( 2 ).y, 1000000 );
}


BOOST_AUTO_TEST_CASE( BezierShapeScalesWithFootprint )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* bez = new PCB_SHAPE( &fp, SHAPE_T::BEZIER );
    bez->SetStart( VECTOR2I( 0, 0 ) );
    bez->SetBezierC1( VECTOR2I( 1000000, 0 ) );
    bez->SetBezierC2( VECTOR2I( 1000000, 1000000 ) );
    bez->SetEnd( VECTOR2I( 2000000, 1000000 ) );
    bez->RebuildBezierToSegmentsPointsList();
    fp.Add( bez, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.0 );

    BOOST_CHECK_EQUAL( bez->GetStart().x, 0 );
    BOOST_CHECK_EQUAL( bez->GetBezierC1().x, 2000000 );
    BOOST_CHECK_EQUAL( bez->GetBezierC2().x, 2000000 );
    BOOST_CHECK_EQUAL( bez->GetBezierC2().y, 1000000 );
    BOOST_CHECK_EQUAL( bez->GetEnd().x, 4000000 );

    // Polyline cache must be rebuilt after scale
    const std::vector<VECTOR2I>& pts = bez->GetBezierPoints();
    BOOST_REQUIRE( pts.size() >= 2 );

    int maxX = 0;
    for( const VECTOR2I& p : pts )
        maxX = std::max( maxX, p.x );

    BOOST_CHECK_GT( maxX, 3000000 );
}


BOOST_AUTO_TEST_CASE( RectangleSurvivesScaleRotateRoundTrip )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* rect = new PCB_SHAPE( &fp, SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( -1000000, -500000 ) );
    rect->SetEnd( VECTOR2I( 1000000, 500000 ) );
    fp.Add( rect, ADD_MODE::APPEND );

    const VECTOR2I libStart0 = rect->GetLibraryStart();
    const VECTOR2I libEnd0 = rect->GetLibraryEnd();

    fp.SetTransformScale( 2.0, 1.0 );
    fp.SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp.SetTransformScale( 3.0, 1.0 );
    fp.SetOrientation( EDA_ANGLE( 0.0, DEGREES_T ) );
    fp.SetTransformScale( 1.0, 1.0 );

    BOOST_CHECK( rect->GetLibraryShape() == SHAPE_T::RECTANGLE );
    BOOST_CHECK( rect->GetShape() == SHAPE_T::RECTANGLE );
    BOOST_CHECK_EQUAL( rect->GetLibraryStart().x, libStart0.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryStart().y, libStart0.y );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().x, libEnd0.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().y, libEnd0.y );
    BOOST_CHECK_EQUAL( rect->GetStart().x, libStart0.x );
    BOOST_CHECK_EQUAL( rect->GetStart().y, libStart0.y );
    BOOST_CHECK_EQUAL( rect->GetEnd().x, libEnd0.x );
    BOOST_CHECK_EQUAL( rect->GetEnd().y, libEnd0.y );
}


BOOST_AUTO_TEST_CASE( RectangleSwapsDimsAt90 )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* rect = new PCB_SHAPE( &fp, SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( -1000000, -500000 ) );
    rect->SetEnd( VECTOR2I( 1000000, 500000 ) );
    fp.Add( rect, ADD_MODE::APPEND );

    fp.SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    BOOST_CHECK( rect->GetShape() == SHAPE_T::RECTANGLE );
    const int w = std::abs( rect->GetEnd().x - rect->GetStart().x );
    const int h = std::abs( rect->GetEnd().y - rect->GetStart().y );
    BOOST_CHECK_EQUAL( w, 1000000 );
    BOOST_CHECK_EQUAL( h, 2000000 );
}


BOOST_AUTO_TEST_CASE( RectangleMorphsToPolyUnderNonCardinalRotation )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* rect = new PCB_SHAPE( &fp, SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( -1000000, -500000 ) );
    rect->SetEnd( VECTOR2I( 1000000, 500000 ) );
    fp.Add( rect, ADD_MODE::APPEND );

    fp.SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );

    BOOST_CHECK( rect->GetLibraryShape() == SHAPE_T::RECTANGLE );
    BOOST_CHECK( rect->GetShape() == SHAPE_T::POLY );
    BOOST_CHECK_EQUAL( rect->GetPolyShape().Outline( 0 ).PointCount(), 4 );
    BOOST_CHECK_EQUAL( rect->GetLibraryStart().x, -1000000 );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().x, 1000000 );
}


BOOST_AUTO_TEST_CASE( RectangleSurvivesIncrementalRotateAroundExternalCenter )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 152000000, 105500000 ) );

    PCB_SHAPE* rect = new PCB_SHAPE( &fp, SHAPE_T::RECTANGLE );
    rect->SetStart( fp.GetPosition() + VECTOR2I( -1680000, -950000 ) );
    rect->SetEnd( fp.GetPosition() + VECTOR2I( 1680000, 950000 ) );
    fp.Add( rect, ADD_MODE::APPEND );

    const VECTOR2I libStart0 = rect->GetLibraryStart();
    const VECTOR2I libEnd0 = rect->GetLibraryEnd();

    const VECTOR2I rotCenter( 150000000, 100000000 );
    fp.Rotate( rotCenter, EDA_ANGLE( 30.0, DEGREES_T ) );
    fp.Rotate( rotCenter, EDA_ANGLE( 30.0, DEGREES_T ) );

    BOOST_CHECK_EQUAL( rect->GetLibraryStart().x, libStart0.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryStart().y, libStart0.y );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().x, libEnd0.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().y, libEnd0.y );
}


BOOST_FIXTURE_TEST_CASE( NativeEllipseFlipSurvivesNonUniformScale, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetOrientation( ANGLE_0 );
    fp->SetTransformScale( 1.0, 1.0 );

    PCB_SHAPE* ellipse = new PCB_SHAPE( fp, SHAPE_T::ELLIPSE_ARC );
    ellipse->SetLayer( F_SilkS );
    ellipse->SetEllipseCenter( fp->GetPosition() + VECTOR2I( 1000000, 0 ) );
    ellipse->SetEllipseMajorRadius( 800000 );
    ellipse->SetEllipseMinorRadius( 400000 );
    ellipse->SetEllipseRotation( EDA_ANGLE( 30.0, DEGREES_T ) );
    ellipse->SetEllipseStartAngle( EDA_ANGLE( 10.0, DEGREES_T ) );
    ellipse->SetEllipseEndAngle( EDA_ANGLE( 100.0, DEGREES_T ) );
    fp->Add( ellipse, ADD_MODE::APPEND );

    fp->SetTransformScale( 2.0, 1.0 );

    const VECTOR2I  libCenterBefore = ellipse->GetLibraryEllipseCenter();
    const int       libMajorBefore = ellipse->GetLibraryEllipseMajorRadius();
    const int       libMinorBefore = ellipse->GetLibraryEllipseMinorRadius();
    const EDA_ANGLE libRotBefore = ellipse->GetLibraryEllipseRotation();
    const EDA_ANGLE libStartBefore = ellipse->GetLibraryEllipseStartAngle();
    const EDA_ANGLE libEndBefore = ellipse->GetLibraryEllipseEndAngle();

    ellipse->Flip( fp->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseMajorRadius(), libMajorBefore );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseMinorRadius(), libMinorBefore );
    BOOST_CHECK_CLOSE( ellipse->GetLibraryEllipseRotation().AsDegrees(), ( -libRotBefore ).AsDegrees(), 1e-6 );
    BOOST_CHECK_CLOSE( ellipse->GetLibraryEllipseStartAngle().AsDegrees(), ( ANGLE_180 - libEndBefore ).AsDegrees(),
                       1e-6 );
    BOOST_CHECK_CLOSE( ellipse->GetLibraryEllipseEndAngle().AsDegrees(), ( ANGLE_180 - libStartBefore ).AsDegrees(),
                       1e-6 );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseCenter().x, -libCenterBefore.x );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseCenter().y, libCenterBefore.y );

    ellipse->Flip( fp->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseMajorRadius(), libMajorBefore );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseMinorRadius(), libMinorBefore );
    BOOST_CHECK_CLOSE( ellipse->GetLibraryEllipseRotation().AsDegrees(), libRotBefore.AsDegrees(), 1e-6 );
    BOOST_CHECK_CLOSE( ellipse->GetLibraryEllipseStartAngle().AsDegrees(), libStartBefore.AsDegrees(), 1e-6 );
    BOOST_CHECK_CLOSE( ellipse->GetLibraryEllipseEndAngle().AsDegrees(), libEndBefore.AsDegrees(), 1e-6 );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseCenter().x, libCenterBefore.x );
    BOOST_CHECK_EQUAL( ellipse->GetLibraryEllipseCenter().y, libCenterBefore.y );
}


BOOST_AUTO_TEST_CASE( NativeEllipseRebakesUnderNonUniformScaleAxisAligned )
{
    // Axis-aligned lib ellipse: non-uniform scale just stretches each axis.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* ellipse = new PCB_SHAPE( &fp, SHAPE_T::ELLIPSE );
    ellipse->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    ellipse->SetEllipseMajorRadius( 1000000 );
    ellipse->SetEllipseMinorRadius( 500000 );
    ellipse->SetEllipseRotation( ANGLE_0 );
    fp.Add( ellipse, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.0 );

    BOOST_CHECK_EQUAL( ellipse->GetEllipseMajorRadius(), 2000000 );
    BOOST_CHECK_EQUAL( ellipse->GetEllipseMinorRadius(), 500000 );
    BOOST_CHECK_SMALL( ellipse->GetEllipseRotation().AsDegrees(), 1e-6 );
}


BOOST_AUTO_TEST_CASE( NativeEllipseSignUnderNonUniformScaleAndParentRotation )
{
    // Lib ellipse 1 x 0.5 mm at 0 deg, parent rotated 30 deg, scaled (2, 1).
    // Expect 2 x 0.5 mm at -30 deg on the board.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* ellipse = new PCB_SHAPE( &fp, SHAPE_T::ELLIPSE );
    ellipse->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    ellipse->SetEllipseMajorRadius( 1000000 );
    ellipse->SetEllipseMinorRadius( 500000 );
    ellipse->SetEllipseRotation( ANGLE_0 );
    fp.Add( ellipse, ADD_MODE::APPEND );

    fp.SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp.SetTransformScale( 2.0, 1.0 );

    BOOST_CHECK_CLOSE( static_cast<double>( ellipse->GetEllipseMajorRadius() ), 2000000.0, 0.1 );
    BOOST_CHECK_CLOSE( static_cast<double>( ellipse->GetEllipseMinorRadius() ),  500000.0, 0.1 );

    // An ellipse rotated 180 deg looks the same, so normalize to (-90, 90].
    double rotDeg = ellipse->GetEllipseRotation().AsDegrees();
    while( rotDeg > 90.0 )   rotDeg -= 180.0;
    while( rotDeg <= -90.0 ) rotDeg += 180.0;
    BOOST_CHECK_CLOSE( rotDeg, -30.0, 0.1 );
}


BOOST_AUTO_TEST_CASE( NativeEllipseContinuousAtUniformLimit )
{
    // Bumping Sy by 5 percent from the uniform case must not jump the rotation
    // or the radii.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* ellipse = new PCB_SHAPE( &fp, SHAPE_T::ELLIPSE );
    ellipse->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    ellipse->SetEllipseMajorRadius( 1000000 );
    ellipse->SetEllipseMinorRadius( 500000 );
    ellipse->SetEllipseRotation( ANGLE_0 );
    fp.Add( ellipse, ADD_MODE::APPEND );

    fp.SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp.SetTransformScale( 2.0, 2.0 );
    const double major_u = ellipse->GetEllipseMajorRadius();
    const double minor_u = ellipse->GetEllipseMinorRadius();
    const double rot_u   = ellipse->GetEllipseRotation().AsDegrees();

    fp.SetTransformScale( 2.0, 2.1 );
    const double major_n = ellipse->GetEllipseMajorRadius();
    const double minor_n = ellipse->GetEllipseMinorRadius();
    const double rot_n   = ellipse->GetEllipseRotation().AsDegrees();

    BOOST_CHECK_CLOSE( major_n, major_u, 1.0 );
    BOOST_CHECK_CLOSE( minor_n, minor_u, 6.0 );

    double diffDeg = rot_n - rot_u;
    while( diffDeg > 90.0 )   diffDeg -= 180.0;
    while( diffDeg <= -90.0 ) diffDeg += 180.0;
    BOOST_CHECK_SMALL( diffDeg, 1.0 );
}


BOOST_AUTO_TEST_CASE( NativeEllipseTessellatesUnderNonUniformScaleRotated )
{
    // A rotated lib ellipse under non-uniform scale is not a standard ellipse.
    // Tessellate to POLY while keeping lib as ELLIPSE so the round trip works.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* ellipse = new PCB_SHAPE( &fp, SHAPE_T::ELLIPSE );
    ellipse->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    ellipse->SetEllipseMajorRadius( 1000000 );
    ellipse->SetEllipseMinorRadius( 500000 );
    ellipse->SetEllipseRotation( EDA_ANGLE( 45.0, DEGREES_T ) );
    fp.Add( ellipse, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.0 );

    BOOST_CHECK_EQUAL( static_cast<int>( ellipse->GetShape() ), static_cast<int>( SHAPE_T::POLY ) );
    BOOST_CHECK_EQUAL( static_cast<int>( ellipse->GetLibraryShape() ),
                       static_cast<int>( SHAPE_T::ELLIPSE ) );
    BOOST_REQUIRE( ellipse->IsPolyShapeValid() );
    BOOST_CHECK( ellipse->GetPolyShape().OutlineCount() == 1 );
    BOOST_CHECK( ellipse->GetPolyShape().Outline( 0 ).PointCount() > 8 );

    // Expected bbox after Sx=2, Sy=1 on a 1 x 0.5 mm ellipse at 45 deg:
    // ~3.162 mm wide, ~1.581 mm tall.
    const BOX2I bbox = ellipse->GetPolyShape().BBox();
    BOOST_CHECK_CLOSE( static_cast<double>( bbox.GetWidth() ), 3162277.0, 1.0 );
    BOOST_CHECK_CLOSE( static_cast<double>( bbox.GetHeight() ), 1581139.0, 1.0 );

    // Restore uniform scale, the native ellipse comes back.
    fp.SetTransformScale( 1.0, 1.0 );

    BOOST_CHECK_EQUAL( static_cast<int>( ellipse->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE ) );
    BOOST_CHECK_EQUAL( ellipse->GetEllipseMajorRadius(), 1000000 );
    BOOST_CHECK_EQUAL( ellipse->GetEllipseMinorRadius(), 500000 );
}


// Pad primitive lib coords are pad-local and untouched by the parent FP transform, but
// the runtime geometry derives through the footprint scale like every other shape.
BOOST_AUTO_TEST_CASE( PadPrimitiveScalesRuntimeKeepsLibPadLocal )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PAD* pad = new PAD( &fp );
    pad->SetPosition( VECTOR2I( 0, 0 ) );
    fp.Add( pad, ADD_MODE::APPEND );

    PCB_SHAPE* prim = new PCB_SHAPE( pad, SHAPE_T::CIRCLE );
    prim->SetStart( VECTOR2I( 0, 0 ) );
    prim->SetEnd( VECTOR2I( 1000000, 0 ) );
    pad->AddPrimitive( PADSTACK::ALL_LAYERS, prim );

    BOOST_CHECK_EQUAL( prim->GetEnd().x, 1000000 );
    BOOST_CHECK_EQUAL( prim->GetLibraryEnd().x, 1000000 );

    fp.SetTransformScale( 2.0, 2.0 );

    BOOST_CHECK_EQUAL( prim->GetEnd().x, 2000000 );        // runtime scales
    BOOST_CHECK_EQUAL( prim->GetLibraryEnd().x, 1000000 ); // lib stays pad-local
}


BOOST_AUTO_TEST_CASE( PadPrimitiveCircleBecomesEllipseUnderNonUniformScale )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PAD* pad = new PAD( &fp );
    pad->SetPosition( VECTOR2I( 0, 0 ) );
    fp.Add( pad, ADD_MODE::APPEND );

    PCB_SHAPE* prim = new PCB_SHAPE( pad, SHAPE_T::CIRCLE );
    prim->SetStart( VECTOR2I( 0, 0 ) );
    prim->SetEnd( VECTOR2I( 1000000, 0 ) );
    pad->AddPrimitive( PADSTACK::ALL_LAYERS, prim );

    fp.SetTransformScale( 2.0, 1.0 );

    BOOST_CHECK_EQUAL( static_cast<int>( prim->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE ) );
    BOOST_CHECK_EQUAL( static_cast<int>( prim->GetLibraryShape() ), static_cast<int>( SHAPE_T::CIRCLE ) );
}


// work_items/24732: a custom pad primitive is pad-local, so a flip must mirror it
// about the pad anchor regardless of where the parent footprint sits on the board.
BOOST_AUTO_TEST_CASE( PadPrimitiveFlipStaysPadLocal )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 50000000, 30000000 ) ); // deliberately off origin
    board.Add( fp );

    PAD* pad = new PAD( fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
    pad->SetPosition( fp->GetPosition() );
    fp->Add( pad, ADD_MODE::APPEND );

    PCB_SHAPE* prim = new PCB_SHAPE( pad, SHAPE_T::SEGMENT );
    prim->SetStart( VECTOR2I( 0, 0 ) );
    prim->SetEnd( VECTOR2I( 1000000, 500000 ) );
    pad->AddPrimitive( PADSTACK::ALL_LAYERS, prim );

    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    // Pad-local primitive mirrors about (0,0): x stays, y negates.
    BOOST_CHECK_EQUAL( prim->GetStart().x, 0 );
    BOOST_CHECK_EQUAL( prim->GetStart().y, 0 );
    BOOST_CHECK_EQUAL( prim->GetEnd().x, 1000000 );
    BOOST_CHECK_EQUAL( prim->GetEnd().y, -500000 );

    // Effective and lib coords stay 1:1 for pad-local primitives.
    BOOST_CHECK_EQUAL( prim->GetLibraryEnd().x, prim->GetEnd().x );
    BOOST_CHECK_EQUAL( prim->GetLibraryEnd().y, prim->GetEnd().y );
}


BOOST_AUTO_TEST_CASE( CirclePadStaysCircularUnderNonUniformScale )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PAD* pad = new PAD( &fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 1000000, 1000000 ) );
    pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
    pad->SetDrillSize( VECTOR2I( 400000, 400000 ) );
    fp.Add( pad, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.0 );

    const VECTOR2I size = pad->GetSize( PADSTACK::ALL_LAYERS );
    BOOST_CHECK_EQUAL( size.x, size.y );
    BOOST_CHECK_EQUAL( size.x, 1500000 );

    const VECTOR2I drill = pad->GetDrillSize();
    BOOST_CHECK_EQUAL( drill.x, drill.y );
    BOOST_CHECK_EQUAL( drill.x, 600000 );
}


BOOST_FIXTURE_TEST_CASE( CirclePadRoundTripsAcrossSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetOrientation( ANGLE_0 );
    fp->SetTransformScale( 1.0, 1.0 );

    PAD* pad = new PAD( fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 1000000, 1000000 ) );
    pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
    pad->SetDrillSize( VECTOR2I( 400000, 400000 ) );
    pad->SetPosition( fp->GetPosition() );
    pad->SetLayerSet( PAD::PTHMask() );
    pad->SetNumber( wxT( "88" ) );
    fp->Add( pad, ADD_MODE::APPEND );

    fp->SetTransformScale( 2.0, 1.0 );

    const std::filesystem::path savePath = std::filesystem::temp_directory_path() / "circle_pad_roundtrip.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    PAD* pad2 = nullptr;

    for( PAD* p : fp2->Pads() )
    {
        if( p->GetNumber() == wxT( "88" ) )
        {
            pad2 = p;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( pad2, "circle pad not found after reload" );

    const VECTOR2I size2 = pad2->GetSize( PADSTACK::ALL_LAYERS );
    BOOST_CHECK_EQUAL( size2.x, size2.y );
    BOOST_CHECK_EQUAL( size2.x, 1500000 );

    const VECTOR2I drill2 = pad2->GetDrillSize();
    BOOST_CHECK_EQUAL( drill2.x, drill2.y );
    BOOST_CHECK_EQUAL( drill2.x, 600000 );
}


// Pad primitives stay pad-local across save / load. The parent FP scale does not bake
// into primitive coords because the pad applies its own transform when composing the
// effective shape.
BOOST_FIXTURE_TEST_CASE( PadPrimitiveRoundTripsAcrossSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetOrientation( ANGLE_0 );
    fp->SetTransformScale( 1.0, 1.0 );

    PAD* pad = new PAD( fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
    pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 500000, 500000 ) );
    pad->SetPosition( fp->GetPosition() );
    pad->SetLayerSet( PAD::SMDMask() );
    pad->SetNumber( wxT( "99" ) );
    fp->Add( pad, ADD_MODE::APPEND );

    PCB_SHAPE* prim = new PCB_SHAPE( pad, SHAPE_T::CIRCLE );
    prim->SetStart( VECTOR2I( 0, 0 ) );
    prim->SetEnd( VECTOR2I( 1000000, 0 ) );
    prim->SetWidth( 100000 );
    prim->SetFilled( false );
    pad->AddPrimitive( PADSTACK::ALL_LAYERS, prim );

    fp->SetTransformScale( 2.0, 1.0 );

    // Runtime morphs to ELLIPSE under the non-uniform scale, lib stays a pad-local CIRCLE.
    BOOST_REQUIRE_EQUAL( static_cast<int>( prim->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE ) );
    BOOST_REQUIRE_EQUAL( static_cast<int>( prim->GetLibraryShape() ), static_cast<int>( SHAPE_T::CIRCLE ) );

    const std::filesystem::path savePath = std::filesystem::temp_directory_path() / "pad_prim_roundtrip.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    PAD* pad2 = nullptr;

    for( PAD* p : fp2->Pads() )
    {
        if( p->GetNumber() == wxT( "99" ) )
        {
            pad2 = p;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( pad2, "custom pad not found after reload" );
    BOOST_REQUIRE_EQUAL( pad2->GetPrimitives( PADSTACK::ALL_LAYERS ).size(), 1u );

    const std::shared_ptr<PCB_SHAPE>& prim2 = pad2->GetPrimitives( PADSTACK::ALL_LAYERS ).front();

    // The file stores pad-local lib coords, so the reloaded primitive is a CIRCLE in lib
    // space, re-derived to an ELLIPSE by the persisted non-uniform footprint scale.
    BOOST_CHECK_EQUAL( static_cast<int>( prim2->GetLibraryShape() ), static_cast<int>( SHAPE_T::CIRCLE ) );
    BOOST_CHECK_EQUAL( static_cast<int>( prim2->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE ) );

    const VECTOR2I libDelta = prim2->GetLibraryEnd() - prim2->GetLibraryStart();
    BOOST_CHECK_EQUAL( libDelta.EuclideanNorm(), 1000000 );
}


// Syncing lib coords from runtime (as a GUI edit commit does) must not bake the footprint
// scale into a pad primitive's lib geometry, or it double-scales on the next reload.
BOOST_AUTO_TEST_CASE( PadPrimitiveSyncKeepsLibUnscaled )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    board.Add( fp );
    fp->SetPosition( VECTOR2I( 0, 0 ) );

    PAD* pad = new PAD( fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
    pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 500000, 500000 ) );
    pad->SetLayerSet( PAD::SMDMask() );

    std::vector<VECTOR2I> poly = {
        { -1000000, -1000000 }, { 1000000, -1000000 }, { 1000000, 1000000 }, { -1000000, 1000000 }
    };
    pad->AddPrimitivePoly( PADSTACK::ALL_LAYERS, poly, 0, true );
    fp->Add( pad, ADD_MODE::APPEND );

    const std::shared_ptr<PCB_SHAPE>& prim = pad->GetPrimitives( PADSTACK::ALL_LAYERS ).front();
    const BOX2I                       libBefore = prim->GetLibraryPolyShape().BBox();

    fp->SetTransformScale( 2.0, 1.5 );

    // A GUI edit commit syncs lib from runtime; lib must stay pad-local (unscaled).
    prim->EndEdit();

    const BOX2I libAfter = prim->GetLibraryPolyShape().BBox();

    BOOST_CHECK_EQUAL( libAfter.GetWidth(), libBefore.GetWidth() );
    BOOST_CHECK_EQUAL( libAfter.GetHeight(), libBefore.GetHeight() );
}


// A polygon custom-pad primitive must not double-scale across save / load: the file stores
// pad-local lib coords, so the runtime polygon size has to match before and after a reload.
BOOST_FIXTURE_TEST_CASE( PadPolyPrimitiveRoundTripsAcrossSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetOrientation( ANGLE_0 );
    fp->SetTransformScale( 1.0, 1.0 );

    PAD* pad = new PAD( fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
    pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 500000, 500000 ) );
    pad->SetPosition( fp->GetPosition() );
    pad->SetLayerSet( PAD::SMDMask() );
    pad->SetNumber( wxT( "77" ) );
    fp->Add( pad, ADD_MODE::APPEND );

    std::vector<VECTOR2I> poly = {
        { -1000000, -1000000 }, { 1000000, -1000000 }, { 1000000, 1000000 }, { -1000000, 1000000 }
    };
    pad->AddPrimitivePoly( PADSTACK::ALL_LAYERS, poly, 0, true );

    fp->SetTransformScale( 2.0, 1.5 );

    const BOX2I before = pad->GetBoundingBox();

    const std::filesystem::path savePath = std::filesystem::temp_directory_path() / "pad_poly_prim_roundtrip.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    PAD* pad2 = nullptr;

    for( PAD* p : fp2->Pads() )
    {
        if( p->GetNumber() == wxT( "77" ) )
        {
            pad2 = p;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( pad2, "custom poly pad not found after reload" );

    const BOX2I after = pad2->GetBoundingBox();

    BOOST_CHECK_EQUAL( after.GetWidth(), before.GetWidth() );
    BOOST_CHECK_EQUAL( after.GetHeight(), before.GetHeight() );
}


BOOST_AUTO_TEST_CASE( NonUniformScalePromotesCircleToEllipse )
{
    // Non-uniform scale promotes a CIRCLE to ELLIPSE on the board side.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* circle = new PCB_SHAPE( &fp, SHAPE_T::CIRCLE );
    circle->SetStart( VECTOR2I( 0, 0 ) );
    circle->SetEnd( VECTOR2I( 1000000, 0 ) ); // radius 1mm
    fp.Add( circle, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.0 );

    BOOST_CHECK_EQUAL( static_cast<int>( circle->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE ) );
    BOOST_CHECK_EQUAL( circle->GetEllipseMajorRadius(), 2000000 );
    BOOST_CHECK_EQUAL( circle->GetEllipseMinorRadius(), 1000000 );
    BOOST_CHECK_EQUAL( circle->GetEllipseRotation().AsDegrees(), 0.0 );
    BOOST_CHECK_EQUAL( circle->GetEllipseCenter().x, 0 );
    BOOST_CHECK_EQUAL( circle->GetEllipseCenter().y, 0 );
}


BOOST_AUTO_TEST_CASE( CircleToEllipseRotatesWithParent )
{
    // Morphed ellipse rotation must follow the parent's rotation direction.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_SHAPE* circle = new PCB_SHAPE( &fp, SHAPE_T::CIRCLE );
    circle->SetStart( VECTOR2I( 0, 0 ) );
    circle->SetEnd( VECTOR2I( 1000000, 0 ) );
    fp.Add( circle, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.0 );
    fp.SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );

    // Parent rotated +30 in screen frame so ellipse math rotation = -30 deg.
    BOOST_CHECK_CLOSE( circle->GetEllipseRotation().AsDegrees(), -30.0, 0.001 );
}


BOOST_AUTO_TEST_CASE( NonUniformScaleSwapsAxesWhenMinorWouldExceedMajor )
{
    // sx < sy: swap axes and rotate 90 deg so major >= minor.
    FOOTPRINT fp( nullptr );

    PCB_SHAPE* circle = new PCB_SHAPE( &fp, SHAPE_T::CIRCLE );
    circle->SetStart( VECTOR2I( 0, 0 ) );
    circle->SetEnd( VECTOR2I( 1000000, 0 ) );
    fp.Add( circle, ADD_MODE::APPEND );

    fp.SetTransformScale( 1.0, 3.0 );

    BOOST_CHECK_EQUAL( static_cast<int>( circle->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE ) );
    BOOST_CHECK_EQUAL( circle->GetEllipseMajorRadius(), 3000000 );
    BOOST_CHECK_EQUAL( circle->GetEllipseMinorRadius(), 1000000 );
    BOOST_CHECK_EQUAL( circle->GetEllipseRotation().AsDegrees(), 90.0 );
}


BOOST_AUTO_TEST_CASE( NonUniformScalePromotesArcToEllipseArc )
{
    // Non-uniform scale promotes ARC to ELLIPSE_ARC, axes scaled by sx/sy.
    FOOTPRINT fp( nullptr );

    PCB_SHAPE* arc = new PCB_SHAPE( &fp, SHAPE_T::ARC );

    // Quarter arc on a 1 mm circle at origin, (1,0) -> (0,1).
    arc->SetArcGeometry( VECTOR2I( 1000000, 0 ), VECTOR2I( 707107, 707107 ), VECTOR2I( 0, 1000000 ) );
    fp.Add( arc, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.0 );

    BOOST_CHECK_EQUAL( static_cast<int>( arc->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE_ARC ) );
    BOOST_CHECK_EQUAL( arc->GetEllipseMajorRadius(), 2000000 );
    BOOST_CHECK_EQUAL( arc->GetEllipseMinorRadius(), 1000000 );
    BOOST_CHECK_EQUAL( arc->GetEllipseRotation().AsDegrees(), 0.0 );
    BOOST_CHECK_EQUAL( arc->GetEllipseCenter().x, 0 );
    BOOST_CHECK_EQUAL( arc->GetEllipseCenter().y, 0 );
    BOOST_CHECK_CLOSE( arc->GetEllipseStartAngle().AsDegrees(), 0.0, 1e-6 );
    BOOST_CHECK_CLOSE( arc->GetEllipseEndAngle().AsDegrees(), 90.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( UniformRescaleRestoresArc )
{
    // Going back to a uniform scale must restore SHAPE_T::ARC.
    FOOTPRINT fp( nullptr );

    PCB_SHAPE* arc = new PCB_SHAPE( &fp, SHAPE_T::ARC );
    arc->SetArcGeometry( VECTOR2I( 1000000, 0 ), VECTOR2I( 707107, 707107 ), VECTOR2I( 0, 1000000 ) );
    fp.Add( arc, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.0 );
    BOOST_REQUIRE_EQUAL( static_cast<int>( arc->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE_ARC ) );

    fp.SetTransformScale( 2.0, 2.0 );
    BOOST_CHECK_EQUAL( static_cast<int>( arc->GetShape() ), static_cast<int>( SHAPE_T::ARC ) );
}


BOOST_AUTO_TEST_CASE( UniformRescaleRestoresCircle )
{
    // Going back to uniform scale must restore SHAPE_T::CIRCLE, not leave a
    // degenerate ellipse with major == minor.
    FOOTPRINT fp( nullptr );

    PCB_SHAPE* circle = new PCB_SHAPE( &fp, SHAPE_T::CIRCLE );
    circle->SetStart( VECTOR2I( 0, 0 ) );
    circle->SetEnd( VECTOR2I( 1000000, 0 ) );
    fp.Add( circle, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.0 );
    BOOST_REQUIRE_EQUAL( static_cast<int>( circle->GetShape() ), static_cast<int>( SHAPE_T::ELLIPSE ) );

    fp.SetTransformScale( 2.0, 2.0 );
    BOOST_CHECK_EQUAL( static_cast<int>( circle->GetShape() ), static_cast<int>( SHAPE_T::CIRCLE ) );
}


// Lib-frame square outline used as the zone shape for the tests below.
static void seedSquareLibOutline( ZONE* aZone, int aHalfSide = 1000000 )
{
    SHAPE_POLY_SET poly;
    poly.NewOutline();
    poly.Append( VECTOR2I( -aHalfSide, -aHalfSide ) );
    poly.Append( VECTOR2I( aHalfSide, -aHalfSide ) );
    poly.Append( VECTOR2I( aHalfSide, aHalfSide ) );
    poly.Append( VECTOR2I( -aHalfSide, aHalfSide ) );
    *aZone->Outline() = poly;
}


BOOST_AUTO_TEST_CASE( ZoneOutlineFollowsFootprintTransform )
{
    // FP with translate, rotate, and non-uniform scale. Each vertex of the
    // board outline must equal xform.Apply(lib vertex).
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 5000000, 3000000 ) );
    fp.SetOrientation( EDA_ANGLE( 45.0, DEGREES_T ) );

    ZONE* zone = new ZONE( &fp );
    seedSquareLibOutline( zone );
    fp.Add( zone, ADD_MODE::APPEND );

    fp.SetTransformScale( 2.0, 1.5 );

    SHAPE_POLY_SET libOutline = zone->GetLibraryOutline();
    SHAPE_POLY_SET boardOutline = zone->GetBoardOutline();
    BOOST_REQUIRE_EQUAL( libOutline.TotalVertices(), boardOutline.TotalVertices() );

    for( int i = 0; i < libOutline.TotalVertices(); ++i )
    {
        VECTOR2I expected = fp.GetTransform().Apply( libOutline.CVertex( i ) );
        VECTOR2I actual = boardOutline.CVertex( i );

        BOOST_CHECK_MESSAGE( std::abs( expected.x - actual.x ) <= 1 && std::abs( expected.y - actual.y ) <= 1,
                             "vertex " << i << " expected ( " << expected.x << ", " << expected.y << " ) actual ( "
                                       << actual.x << ", " << actual.y << " )" );
    }
}


BOOST_AUTO_TEST_CASE( ZoneOutlineSurvivesFootprintFlip )
{
    // Each board-frame vertex must mirror about the flip axis. Flip routes
    // through board->FlipLayer so the FP needs a parent board.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 5000000 ) );

    ZONE* zone = new ZONE( fp );
    seedSquareLibOutline( zone );
    fp->Add( zone, ADD_MODE::APPEND );
    board.Add( fp );

    SHAPE_POLY_SET        before = zone->GetBoardOutline();
    std::vector<VECTOR2I> beforeVerts;
    for( int i = 0; i < before.TotalVertices(); ++i )
        beforeVerts.push_back( before.CVertex( i ) );

    const VECTOR2I flipCentre = fp->GetPosition();
    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    SHAPE_POLY_SET after = zone->GetBoardOutline();
    BOOST_REQUIRE_EQUAL( after.TotalVertices(), (int) beforeVerts.size() );

    for( int i = 0; i < after.TotalVertices(); ++i )
    {
        VECTOR2I expected( beforeVerts[i].x, 2 * flipCentre.y - beforeVerts[i].y );
        VECTOR2I actual = after.CVertex( i );

        BOOST_CHECK_MESSAGE( std::abs( expected.x - actual.x ) <= 1 && std::abs( expected.y - actual.y ) <= 1,
                             "vertex " << i << " expected ( " << expected.x << ", " << expected.y << " ) actual ( "
                                       << actual.x << ", " << actual.y << " )" );
    }
}


BOOST_AUTO_TEST_CASE( ZoneHitTestRespectsFootprintTransform )
{
    // HitTestForCorner / HitTestForEdge go through InverseApply, this guards
    // against anyone reverting that path.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 2000000, -1000000 ) );
    fp.SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );

    ZONE* zone = new ZONE( &fp );
    zone->SetIsRuleArea( true );
    seedSquareLibOutline( zone );
    fp.Add( zone, ADD_MODE::APPEND );

    // Lib origin sits at the centre of the square outline so it's inside.
    VECTOR2I boardInside = fp.GetTransform().Apply( VECTOR2I( 0, 0 ) );

    BOOST_CHECK( zone->HitTestFilledArea( F_Cu, boardInside, 0 ) );

    // A lib vertex maps to a board corner. Corner hit-test must find it.
    VECTOR2I boardCorner = fp.GetTransform().Apply( VECTOR2I( 1000000, 1000000 ) );
    BOOST_CHECK( zone->HitTestForCorner( boardCorner, pcbIUScale.mmToIU( 0.1 ) ) );
}


BOOST_AUTO_TEST_CASE( ZoneMoveTranslatesInBoardFrame )
{
    // ZONE::Move takes a board-frame offset. For an FP-child zone, the
    // offset must land as a board-frame shift on every vertex.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 1000000, 0 ) );
    fp.SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    ZONE* zone = new ZONE( &fp );
    seedSquareLibOutline( zone );
    fp.Add( zone, ADD_MODE::APPEND );

    SHAPE_POLY_SET        before = zone->GetBoardOutline();
    std::vector<VECTOR2I> beforeVerts;
    for( int i = 0; i < before.TotalVertices(); ++i )
        beforeVerts.push_back( before.CVertex( i ) );

    const VECTOR2I offset( 500000, 250000 );
    zone->Move( offset );

    SHAPE_POLY_SET after = zone->GetBoardOutline();
    BOOST_REQUIRE_EQUAL( after.TotalVertices(), (int) beforeVerts.size() );

    for( int i = 0; i < after.TotalVertices(); ++i )
    {
        VECTOR2I expected = beforeVerts[i] + offset;
        VECTOR2I actual = after.CVertex( i );

        BOOST_CHECK_MESSAGE( std::abs( expected.x - actual.x ) <= 1 && std::abs( expected.y - actual.y ) <= 1,
                             "vertex " << i << " expected ( " << expected.x << ", " << expected.y << " ) actual ( "
                                       << actual.x << ", " << actual.y << " )" );
    }
}


BOOST_AUTO_TEST_CASE( ZoneRotateAboutBoardCenter )
{
    // ZONE::Rotate takes a board-frame center and angle. Every vertex of the
    // board outline must rotate about the supplied center.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 3000000, 0 ) );

    ZONE* zone = new ZONE( &fp );
    seedSquareLibOutline( zone );
    fp.Add( zone, ADD_MODE::APPEND );

    SHAPE_POLY_SET        before = zone->GetBoardOutline();
    std::vector<VECTOR2I> beforeVerts;
    for( int i = 0; i < before.TotalVertices(); ++i )
        beforeVerts.push_back( before.CVertex( i ) );

    const VECTOR2I  centre( 0, 0 );
    const EDA_ANGLE angle( 90.0, DEGREES_T );
    zone->Rotate( centre, angle );

    SHAPE_POLY_SET after = zone->GetBoardOutline();
    BOOST_REQUIRE_EQUAL( after.TotalVertices(), (int) beforeVerts.size() );

    for( int i = 0; i < after.TotalVertices(); ++i )
    {
        VECTOR2I expected = beforeVerts[i];
        RotatePoint( expected, centre, angle );
        VECTOR2I actual = after.CVertex( i );

        BOOST_CHECK_MESSAGE( std::abs( expected.x - actual.x ) <= 1 && std::abs( expected.y - actual.y ) <= 1,
                             "vertex " << i << " expected ( " << expected.x << ", " << expected.y << " ) actual ( "
                                       << actual.x << ", " << actual.y << " )" );
    }
}


BOOST_AUTO_TEST_CASE( ZoneBoundingBoxIsBoardFrameForFPChild )
{
    // Bbox must come out in board frame, not lib frame.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 50000000, 30000000 ) );

    ZONE* zone = new ZONE( &fp );
    seedSquareLibOutline( zone );
    fp.Add( zone, ADD_MODE::APPEND );

    BOX2I bbox = zone->GetBoundingBox();

    BOOST_CHECK_MESSAGE( bbox.GetCenter().x > 49000000 && bbox.GetCenter().x < 51000000 && bbox.GetCenter().y > 29000000
                                 && bbox.GetCenter().y < 31000000,
                         "bbox centre ( " << bbox.GetCenter().x << ", " << bbox.GetCenter().y
                                          << " ) expected near board ( 50000000, 30000000 )" );
}


BOOST_AUTO_TEST_CASE( CachedZoneBoundingBoxIsBoardFrameForFPChild )
{
    // Footprint-owned zone: the cached fast-path box must be the board-frame outline, not the
    // raw lib-frame outline, or DRC/culling gets a box at the wrong place.
    BOARD board;

    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 50000000, 30000000 ) );
    board.Add( fp, ADD_MODE::APPEND );

    ZONE* zone = new ZONE( fp );
    seedSquareLibOutline( zone );
    fp->Add( zone, ADD_MODE::APPEND );

    zone->CacheBoundingBox();
    BOX2I bbox = zone->GetBoundingBox();

    BOOST_CHECK_MESSAGE( bbox.GetCenter().x > 49000000 && bbox.GetCenter().x < 51000000
                                 && bbox.GetCenter().y > 29000000 && bbox.GetCenter().y < 31000000,
                         "cached bbox centre ( " << bbox.GetCenter().x << ", " << bbox.GetCenter().y
                                          << " ) expected near board ( 50000000, 30000000 )" );
}


BOOST_AUTO_TEST_CASE( ZoneSmoothedPolyForFPRuleAreaIsBoardFrame )
{
    // Keepout knockout source must be board frame so it carves at the FP position.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 50000000, 30000000 ) );

    ZONE* zone = new ZONE( &fp );
    zone->SetIsRuleArea( true );
    seedSquareLibOutline( zone );
    fp.Add( zone, ADD_MODE::APPEND );

    SHAPE_POLY_SET smoothed;
    zone->TransformSmoothedOutlineToPolygon( smoothed, 0, ARC_HIGH_DEF, ERROR_OUTSIDE, nullptr );

    BOX2I bbox = smoothed.BBox();
    BOOST_CHECK_MESSAGE( bbox.GetCenter().x > 49000000 && bbox.GetCenter().x < 51000000 && bbox.GetCenter().y > 29000000
                                 && bbox.GetCenter().y < 31000000,
                         "smoothed-poly centre ( " << bbox.GetCenter().x << ", " << bbox.GetCenter().y
                                                   << " ) expected near board ( 50000000, 30000000 )" );
}


BOOST_AUTO_TEST_CASE( ZoneEffectiveShapeForFPRuleAreaIsBoardFrame )
{
    // DRC reads this shape, it must be board frame.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 50000000, 30000000 ) );

    ZONE* zone = new ZONE( &fp );
    zone->SetIsRuleArea( true );
    seedSquareLibOutline( zone );
    fp.Add( zone, ADD_MODE::APPEND );

    std::shared_ptr<SHAPE> shape = zone->GetEffectiveShape( F_Cu );
    BOOST_REQUIRE( shape );

    BOX2I bbox = shape->BBox();
    BOOST_CHECK_MESSAGE( bbox.GetCenter().x > 49000000 && bbox.GetCenter().x < 51000000 && bbox.GetCenter().y > 29000000
                                 && bbox.GetCenter().y < 31000000,
                         "effective shape centre ( " << bbox.GetCenter().x << ", " << bbox.GetCenter().y
                                                     << " ) expected near board ( 50000000, 30000000 )" );
}


BOOST_AUTO_TEST_CASE( FootprintFlipPreservesChildRectUnderRotation )
{
    // Double flip must restore the rectangle, including under FP rotation.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE* rect = new PCB_SHAPE( fp, SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( 1000000, 1000000 ) );
    rect->SetEnd( VECTOR2I( 5000000, 3000000 ) );
    fp->Add( rect, ADD_MODE::APPEND );

    VECTOR2I libStartBefore = rect->GetLibraryStart();
    VECTOR2I libEndBefore = rect->GetLibraryEnd();

    const VECTOR2I flipCentre = fp->GetPosition();
    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( rect->GetLibraryStart() != libStartBefore || rect->GetLibraryEnd() != libEndBefore,
                         "lib coords did not change after one flip" );

    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( rect->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().y, libEndBefore.y );
}


BOOST_AUTO_TEST_CASE( FootprintFlipPreservesChildSegmentUnderRotation )
{
    // Double flip must restore the segment endpoints.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE* seg = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
    seg->SetStart( VECTOR2I( 1000000, 1000000 ) );
    seg->SetEnd( VECTOR2I( 5000000, 3000000 ) );
    fp->Add( seg, ADD_MODE::APPEND );

    VECTOR2I libStartBefore = seg->GetLibraryStart();
    VECTOR2I libEndBefore = seg->GetLibraryEnd();

    const VECTOR2I flipCentre = fp->GetPosition();
    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( seg->GetLibraryStart() != libStartBefore || seg->GetLibraryEnd() != libEndBefore,
                         "lib coords did not change after one flip" );

    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( seg->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( seg->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( seg->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( seg->GetLibraryEnd().y, libEndBefore.y );
}


BOOST_AUTO_TEST_CASE( FootprintFlipPreservesChildCircleUnderRotation )
{
    // Double flip must restore the circle.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE* circle = new PCB_SHAPE( fp, SHAPE_T::CIRCLE );
    circle->SetStart( VECTOR2I( 2000000, 2000000 ) );
    circle->SetEnd( VECTOR2I( 3000000, 2000000 ) );
    fp->Add( circle, ADD_MODE::APPEND );

    VECTOR2I libStartBefore = circle->GetLibraryStart();
    VECTOR2I libEndBefore = circle->GetLibraryEnd();

    const VECTOR2I flipCentre = fp->GetPosition();
    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( circle->GetLibraryStart() != libStartBefore || circle->GetLibraryEnd() != libEndBefore,
                         "lib coords did not change after one flip" );

    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( circle->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( circle->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( circle->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( circle->GetLibraryEnd().y, libEndBefore.y );
}


BOOST_AUTO_TEST_CASE( FootprintFlipPreservesChildArcUnderRotation )
{
    // Double flip must restore arc endpoints and mid.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE* arc = new PCB_SHAPE( fp, SHAPE_T::ARC );
    arc->SetArcGeometry( VECTOR2I( 1000000, 0 ), VECTOR2I( 707107, 707107 ), VECTOR2I( 0, 1000000 ) );
    fp->Add( arc, ADD_MODE::APPEND );

    VECTOR2I libStartBefore = arc->GetLibraryStart();
    VECTOR2I libEndBefore = arc->GetLibraryEnd();
    VECTOR2I libMidBefore = arc->GetLibraryArcMid();

    const VECTOR2I flipCentre = fp->GetPosition();
    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( arc->GetLibraryStart() != libStartBefore || arc->GetLibraryEnd() != libEndBefore
                                 || arc->GetLibraryArcMid() != libMidBefore,
                         "lib coords did not change after one flip" );

    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( arc->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( arc->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( arc->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( arc->GetLibraryEnd().y, libEndBefore.y );
    BOOST_CHECK_EQUAL( arc->GetLibraryArcMid().x, libMidBefore.x );
    BOOST_CHECK_EQUAL( arc->GetLibraryArcMid().y, libMidBefore.y );
}


BOOST_AUTO_TEST_CASE( FootprintFlipPreservesPCBTextUnderRotation )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_TEXT* text = new PCB_TEXT( fp );
    text->SetText( wxT( "X" ) );
    text->SetTextPos( VECTOR2I( 2000000, 1000000 ) );
    fp->Add( text, ADD_MODE::APPEND );

    VECTOR2I boardPosBefore = text->GetTextPos();

    const VECTOR2I flipCentre = fp->GetPosition();
    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( text->GetTextPos() != boardPosBefore, "text position did not change after one flip" );

    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( text->GetTextPos().x, boardPosBefore.x );
    BOOST_CHECK_EQUAL( text->GetTextPos().y, boardPosBefore.y );
}


BOOST_FIXTURE_TEST_CASE( TableInRotatedFootprintSurvivesSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    PCB_TABLE* table = new PCB_TABLE( fp, pcbIUScale.mmToIU( 0.1 ) );
    table->SetColCount( 2 );

    const int colW = pcbIUScale.mmToIU( 20.0 );
    const int rowH = pcbIUScale.mmToIU( 5.0 );
    table->SetColWidth( 0, colW );
    table->SetColWidth( 1, colW );
    table->SetRowHeight( 0, rowH );
    table->SetRowHeight( 1, rowH );

    for( int i = 0; i < 4; ++i )
    {
        PCB_TABLECELL* cell = new PCB_TABLECELL( table );
        cell->SetStart( VECTOR2I( ( i % 2 ) * colW, ( i / 2 ) * rowH ) );
        cell->SetEnd( VECTOR2I( ( i % 2 + 1 ) * colW, ( i / 2 + 1 ) * rowH ) );
        cell->SetText( wxString::Format( "c%d", i ) );
        table->AddCell( cell );
    }

    fp->Add( table, ADD_MODE::APPEND );

    fp->SetPosition( VECTOR2I( 12345678, -7654321 ) );

    const double angles[] = { 90.0, 180.0, 270.0 };

    for( double deg : angles )
    {
        BOOST_TEST_CONTEXT( "FP orient " << deg )
        {
            fp->SetOrientation( EDA_ANGLE( deg, DEGREES_T ) );

            std::vector<VECTOR2I> startsBefore, endsBefore;
            for( PCB_TABLECELL* cell : table->GetCells() )
            {
                startsBefore.push_back( cell->GetStart() );
                endsBefore.push_back( cell->GetEnd() );
            }

            const std::filesystem::path savePath =
                    std::filesystem::temp_directory_path() / "table_in_rotated_fp.kicad_pcb";

            KI_TEST::DumpBoardToFile( *m_board, savePath.string() );
            std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
            BOOST_REQUIRE( reloaded );

            FOOTPRINT* fp2 = *reloaded->Footprints().begin();
            BOOST_REQUIRE( fp2 );

            PCB_TABLE* table2 = nullptr;
            for( BOARD_ITEM* item : fp2->GraphicalItems() )
            {
                if( PCB_TABLE* t = dynamic_cast<PCB_TABLE*>( item ) )
                {
                    table2 = t;
                    break;
                }
            }

            BOOST_REQUIRE( table2 );
            BOOST_REQUIRE_EQUAL( (int) table2->GetCells().size(), 4 );

            for( int i = 0; i < 4; ++i )
            {
                PCB_TABLECELL* cell = table2->GetCells()[i];
                BOOST_CHECK_MESSAGE( std::abs( cell->GetStart().x - startsBefore[i].x ) <= 1
                                             && std::abs( cell->GetStart().y - startsBefore[i].y ) <= 1,
                                     "cell " << i << " start ( " << cell->GetStart().x << ", " << cell->GetStart().y
                                             << " ) expected ( " << startsBefore[i].x << ", " << startsBefore[i].y
                                             << " )" );
                BOOST_CHECK_MESSAGE( std::abs( cell->GetEnd().x - endsBefore[i].x ) <= 1
                                             && std::abs( cell->GetEnd().y - endsBefore[i].y ) <= 1,
                                     "cell " << i << " end ( " << cell->GetEnd().x << ", " << cell->GetEnd().y
                                             << " ) expected ( " << endsBefore[i].x << ", " << endsBefore[i].y
                                             << " )" );
            }
        }
    }
}


BOOST_FIXTURE_TEST_CASE( TableRotatedInFootprintThenFootprintRotatedRoundTrips, BOARD_FIXTURE )
{
    struct Case
    {
        VECTOR2I pos;
        double   angle;
    };

    const std::vector<Case> cases = {
        { VECTOR2I( 12345678, -7654321 ), 0.0 },    { VECTOR2I( 12345678, -7654321 ), 90.0 },
        { VECTOR2I( 12345678, -7654321 ), 180.0 },  { VECTOR2I( 12345678, -7654321 ), 270.0 },
        { VECTOR2I( -12345678, -7654321 ), 90.0 },  { VECTOR2I( -12345678, 7654321 ), 90.0 },
        { VECTOR2I( 12345678, 7654321 ), 90.0 },    { VECTOR2I( 12345678, 7654321 ), 270.0 },
        { VECTOR2I( -12345678, -7654321 ), 270.0 },
    };

    for( const Case& tc : cases )
    {
        BOOST_TEST_CONTEXT( "fp pos ( " << tc.pos.x << ", " << tc.pos.y << " ) orient " << tc.angle )
        {
            const double fpAngle = tc.angle;
            KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

            FOOTPRINT* fp = *m_board->Footprints().begin();
            BOOST_REQUIRE( fp );

            fp->SetPosition( VECTOR2I( 0, 0 ) );
            fp->SetOrientation( ANGLE_0 );

            PCB_TABLE* table = new PCB_TABLE( fp, pcbIUScale.mmToIU( 0.1 ) );
            table->SetColCount( 2 );

            const int colW = pcbIUScale.mmToIU( 20.0 );
            const int rowH = pcbIUScale.mmToIU( 5.0 );
            table->SetColWidth( 0, colW );
            table->SetColWidth( 1, colW );
            table->SetRowHeight( 0, rowH );
            table->SetRowHeight( 1, rowH );

            for( int i = 0; i < 4; ++i )
            {
                PCB_TABLECELL* cell = new PCB_TABLECELL( table );
                cell->SetStart( VECTOR2I( ( i % 2 ) * colW, ( i / 2 ) * rowH ) );
                cell->SetEnd( VECTOR2I( ( i % 2 + 1 ) * colW, ( i / 2 + 1 ) * rowH ) );
                cell->SetText( wxString::Format( "c%d", i ) );
                table->AddCell( cell );
            }

            fp->Add( table, ADD_MODE::APPEND );

            table->Rotate( VECTOR2I( 0, 0 ), EDA_ANGLE( 90.0, DEGREES_T ) );

            fp->SetPosition( tc.pos );
            fp->SetOrientation( EDA_ANGLE( fpAngle, DEGREES_T ) );

            std::vector<VECTOR2I> startsBefore, endsBefore;
            std::vector<double>   anglesBefore;
            for( PCB_TABLECELL* cell : table->GetCells() )
            {
                startsBefore.push_back( cell->GetStart() );
                endsBefore.push_back( cell->GetEnd() );
                anglesBefore.push_back( cell->GetTextAngle().AsDegrees() );
            }

            const std::filesystem::path savePath =
                    std::filesystem::temp_directory_path() / "table_rotated_in_fp_then_fp_rotated.kicad_pcb";

            KI_TEST::DumpBoardToFile( *m_board, savePath.string() );
            std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
            BOOST_REQUIRE( reloaded );

            FOOTPRINT* fp2 = *reloaded->Footprints().begin();
            BOOST_REQUIRE( fp2 );

            PCB_TABLE* table2 = nullptr;
            for( BOARD_ITEM* item : fp2->GraphicalItems() )
            {
                if( PCB_TABLE* t = dynamic_cast<PCB_TABLE*>( item ) )
                {
                    table2 = t;
                    break;
                }
            }

            BOOST_REQUIRE( table2 );
            BOOST_REQUIRE_EQUAL( (int) table2->GetCells().size(), 4 );

            for( int i = 0; i < 4; ++i )
            {
                PCB_TABLECELL* cell = table2->GetCells()[i];
                BOOST_CHECK_MESSAGE( std::abs( cell->GetStart().x - startsBefore[i].x ) <= 1
                                             && std::abs( cell->GetStart().y - startsBefore[i].y ) <= 1,
                                     "cell " << i << " start ( " << cell->GetStart().x << ", " << cell->GetStart().y
                                             << " ) expected ( " << startsBefore[i].x << ", " << startsBefore[i].y
                                             << " )" );
                BOOST_CHECK_MESSAGE( std::abs( cell->GetEnd().x - endsBefore[i].x ) <= 1
                                             && std::abs( cell->GetEnd().y - endsBefore[i].y ) <= 1,
                                     "cell " << i << " end ( " << cell->GetEnd().x << ", " << cell->GetEnd().y
                                             << " ) expected ( " << endsBefore[i].x << ", " << endsBefore[i].y
                                             << " )" );
                BOOST_CHECK_CLOSE( cell->GetTextAngle().AsDegrees(), anglesBefore[i], 1e-6 );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( TableInRotatedFootprintCellsAreVisuallyRotated )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_TABLE* table = new PCB_TABLE( fp, pcbIUScale.mmToIU( 0.1 ) );
    table->SetColCount( 2 );

    const int colW = pcbIUScale.mmToIU( 20.0 );
    const int rowH = pcbIUScale.mmToIU( 5.0 );
    table->SetColWidth( 0, colW );
    table->SetColWidth( 1, colW );
    table->SetRowHeight( 0, rowH );
    table->SetRowHeight( 1, rowH );

    for( int i = 0; i < 4; ++i )
    {
        PCB_TABLECELL* cell = new PCB_TABLECELL( &board );
        cell->SetStart( VECTOR2I( ( i % 2 ) * colW, ( i / 2 ) * rowH ) );
        cell->SetEnd( VECTOR2I( ( i % 2 + 1 ) * colW, ( i / 2 + 1 ) * rowH ) );
        table->AddCell( cell );
    }

    table->Normalize();
    fp->Add( table, ADD_MODE::APPEND );

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    for( PCB_TABLECELL* cell : table->GetCells() )
    {
        std::vector<VECTOR2I> corners = cell->GetCorners();
        BOOST_REQUIRE_EQUAL( corners.size(), 4u );

        BOX2I visual;
        for( const VECTOR2I& p : corners )
            visual.Merge( p );

        BOOST_CHECK_MESSAGE( std::abs( visual.GetWidth() - rowH ) <= pcbIUScale.mmToIU( 0.2 ),
                             "visual width " << visual.GetWidth() << " expected " << rowH );
        BOOST_CHECK_MESSAGE( std::abs( visual.GetHeight() - colW ) <= pcbIUScale.mmToIU( 0.2 ),
                             "visual height " << visual.GetHeight() << " expected " << colW );
    }
}


BOOST_AUTO_TEST_CASE( TableRotates90Cleanly )
{
    BOARD      board;
    PCB_TABLE* table = new PCB_TABLE( &board, pcbIUScale.mmToIU( 0.1 ) );
    table->SetColCount( 2 );

    const int colW = pcbIUScale.mmToIU( 20.0 );
    const int rowH = pcbIUScale.mmToIU( 5.0 );
    table->SetColWidth( 0, colW );
    table->SetColWidth( 1, colW );
    table->SetRowHeight( 0, rowH );
    table->SetRowHeight( 1, rowH );

    for( int i = 0; i < 4; ++i )
    {
        PCB_TABLECELL* cell = new PCB_TABLECELL( &board );
        cell->SetStart( VECTOR2I( ( i % 2 ) * colW, ( i / 2 ) * rowH ) );
        cell->SetEnd( VECTOR2I( ( i % 2 + 1 ) * colW, ( i / 2 + 1 ) * rowH ) );
        table->AddCell( cell );
    }

    table->Normalize();
    board.Add( table );

    table->Rotate( VECTOR2I( 0, 0 ), EDA_ANGLE( 90.0, DEGREES_T ) );

    std::set<std::pair<int, int>> centers;
    for( PCB_TABLECELL* cell : table->GetCells() )
    {
        int w = std::abs( cell->GetEnd().x - cell->GetStart().x );
        int h = std::abs( cell->GetEnd().y - cell->GetStart().y );

        BOOST_CHECK_MESSAGE( ( w == colW && h == rowH ) || ( w == rowH && h == colW ),
                             "cell dims " << w << " x " << h << " expected " << colW << "x" << rowH << " or rotated" );

        std::vector<VECTOR2I> corners = cell->GetCorners();
        BOOST_REQUIRE_EQUAL( corners.size(), 4u );

        BOX2I visual;
        for( const VECTOR2I& p : corners )
            visual.Merge( p );

        BOOST_CHECK_MESSAGE( std::abs( visual.GetWidth() - rowH ) <= pcbIUScale.mmToIU( 0.2 ),
                             "visual width " << visual.GetWidth() << " expected " << rowH );
        BOOST_CHECK_MESSAGE( std::abs( visual.GetHeight() - colW ) <= pcbIUScale.mmToIU( 0.2 ),
                             "visual height " << visual.GetHeight() << " expected " << colW );

        VECTOR2I vc = visual.GetCenter();
        centers.insert( { vc.x, vc.y } );
    }

    BOOST_CHECK_MESSAGE( centers.size() == 4, "cells overlapped, got " << centers.size() << " unique centers" );
}


BOOST_AUTO_TEST_CASE( TextBoxInFootprintKeepsRectangleShape )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_TEXTBOX* tb = new PCB_TEXTBOX( fp );
    tb->SetText( wxT( "Hello" ) );
    tb->SetStart( VECTOR2I( 0, 0 ) );
    tb->SetEnd( VECTOR2I( 20000000, 5000000 ) );
    fp->Add( tb, ADD_MODE::APPEND );

    int origWidth = tb->GetEnd().x - tb->GetStart().x;
    int origHeight = tb->GetEnd().y - tb->GetStart().y;

    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );

    BOOST_CHECK_MESSAGE( tb->GetLibraryShape() == SHAPE_T::RECTANGLE,
                         "lib shape changed to " << static_cast<int>( tb->GetLibraryShape() ) );
    BOOST_CHECK_MESSAGE( tb->GetShape() == SHAPE_T::RECTANGLE,
                         "runtime shape changed to " << static_cast<int>( tb->GetShape() ) );

    int newWidth = tb->GetEnd().x - tb->GetStart().x;
    int newHeight = tb->GetEnd().y - tb->GetStart().y;

    BOOST_CHECK_MESSAGE( newWidth == origWidth, "width changed from " << origWidth << " to " << newWidth );
    BOOST_CHECK_MESSAGE( newHeight == origHeight, "height changed from " << origHeight << " to " << newHeight );
}


BOOST_FIXTURE_TEST_CASE( PCBTextInRotatedFootprintSurvivesSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetPosition( VECTOR2I( 12345678, -7654321 ) );
    fp->SetOrientation( EDA_ANGLE( 37.5, DEGREES_T ) );

    PCB_TEXT* text = new PCB_TEXT( fp );
    text->SetText( wxT( "drift probe" ) );
    text->SetTextPos( fp->GetPosition() + VECTOR2I( 2000000, 1000000 ) );
    text->SetTextAngle( EDA_ANGLE( 30.0, DEGREES_T ) );
    text->SetLayer( F_SilkS );
    fp->Add( text, ADD_MODE::APPEND );

    VECTOR2I  posBefore = text->GetTextPos();
    EDA_ANGLE angleBefore = text->GetTextAngle();

    auto findProbe = []( BOARD& aBoard ) -> PCB_TEXT*
    {
        FOOTPRINT* fp = *aBoard.Footprints().begin();
        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( PCB_TEXT* t = dynamic_cast<PCB_TEXT*>( item ) )
            {
                if( t->GetText() == wxT( "drift probe" ) )
                    return t;
            }
        }
        return nullptr;
    };

    BOARD*                      currentBoard = m_board.get();
    std::unique_ptr<BOARD>      reloadedHolder;
    const std::filesystem::path savePathA =
            std::filesystem::temp_directory_path() / "pcb_text_in_rotated_fp_a.kicad_pcb";
    const std::filesystem::path savePathB =
            std::filesystem::temp_directory_path() / "pcb_text_in_rotated_fp_b.kicad_pcb";

    for( int cycle = 0; cycle < 3; ++cycle )
    {
        const std::filesystem::path& path = ( cycle % 2 == 0 ) ? savePathA : savePathB;

        KI_TEST::DumpBoardToFile( *currentBoard, path.string() );
        reloadedHolder = KI_TEST::ReadBoardFromFileOrStream( path.string() );
        BOOST_REQUIRE( reloadedHolder );
        currentBoard = reloadedHolder.get();

        PCB_TEXT* probe = findProbe( *currentBoard );
        BOOST_REQUIRE( probe );

        BOOST_CHECK_MESSAGE( std::abs( probe->GetTextPos().x - posBefore.x ) <= 1
                                     && std::abs( probe->GetTextPos().y - posBefore.y ) <= 1,
                             "cycle " << cycle << " text pos ( " << probe->GetTextPos().x << ", "
                                      << probe->GetTextPos().y << " ) expected ( " << posBefore.x << ", " << posBefore.y
                                      << " )" );
        BOOST_CHECK_CLOSE( probe->GetTextAngle().AsDegrees(), angleBefore.AsDegrees(), 1e-6 );
    }
}


BOOST_FIXTURE_TEST_CASE( PCBFieldInRotatedFootprintSurvivesSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetPosition( VECTOR2I( 12345678, -7654321 ) );
    fp->SetOrientation( EDA_ANGLE( 37.5, DEGREES_T ) );

    PCB_FIELD& ref = fp->Reference();
    ref.SetTextPos( fp->GetPosition() + VECTOR2I( 3000000, -2000000 ) );
    ref.SetTextAngle( EDA_ANGLE( 25.0, DEGREES_T ) );

    VECTOR2I  refPosBefore = ref.GetTextPos();
    EDA_ANGLE refAngleBefore = ref.GetTextAngle();

    const std::filesystem::path savePath = std::filesystem::temp_directory_path() / "pcb_field_in_rotated_fp.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    PCB_FIELD& ref2 = fp2->Reference();

    BOOST_CHECK_MESSAGE( std::abs( ref2.GetTextPos().x - refPosBefore.x ) <= 1
                                 && std::abs( ref2.GetTextPos().y - refPosBefore.y ) <= 1,
                         "ref pos after reload ( " << ref2.GetTextPos().x << ", " << ref2.GetTextPos().y
                                                   << " ) expected ( " << refPosBefore.x << ", " << refPosBefore.y
                                                   << " )" );
    BOOST_CHECK_CLOSE( ref2.GetTextAngle().AsDegrees(), refAngleBefore.AsDegrees(), 1e-6 );
}


BOOST_FIXTURE_TEST_CASE( DimensionAndBarcodeInScaledFootprintSurviveSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetPosition( VECTOR2I( 12345678, -7654321 ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );

    PCB_DIM_ALIGNED* dim = new PCB_DIM_ALIGNED( fp, PCB_DIM_ALIGNED_T );
    dim->SetStart( fp->GetPosition() + VECTOR2I( 1000000, 0 ) );
    dim->SetEnd( fp->GetPosition() + VECTOR2I( 5000000, 0 ) );
    fp->Add( dim, ADD_MODE::APPEND );

    PCB_BARCODE* bc = new PCB_BARCODE( fp );
    bc->SetPosition( fp->GetPosition() + VECTOR2I( 2000000, 3000000 ) );
    fp->Add( bc, ADD_MODE::APPEND );

    PCB_TEXT* text = new PCB_TEXT( fp );
    text->SetText( wxT( "scale probe" ) );
    text->SetTextPos( fp->GetPosition() + VECTOR2I( 4000000, -2000000 ) );
    text->SetLayer( F_SilkS );
    fp->Add( text, ADD_MODE::APPEND );

    PCB_TEXTBOX* tb = new PCB_TEXTBOX( fp );
    tb->SetText( wxT( "scale probe box" ) );
    tb->SetStart( fp->GetPosition() + VECTOR2I( 1000000, -3000000 ) );
    tb->SetEnd( fp->GetPosition() + VECTOR2I( 4000000, -1500000 ) );
    tb->SetLayer( F_SilkS );
    fp->Add( tb, ADD_MODE::APPEND );

    fp->SetTransformScale( 2.0, 1.5 );

    VECTOR2I dimStartBefore = dim->GetStart();
    VECTOR2I dimEndBefore = dim->GetEnd();
    VECTOR2I bcPosBefore = bc->GetPosition();
    VECTOR2I textPosBefore = text->GetTextPos();
    VECTOR2I tbStartBefore = tb->GetStart();
    VECTOR2I tbEndBefore = tb->GetEnd();

    const std::filesystem::path savePath =
            std::filesystem::temp_directory_path() / "dim_barcode_in_scaled_fp.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    PCB_DIM_ALIGNED* dim2 = nullptr;
    PCB_BARCODE*     bc2 = nullptr;
    PCB_TEXT*        text2 = nullptr;
    PCB_TEXTBOX*     tb2 = nullptr;
    for( BOARD_ITEM* item : fp2->GraphicalItems() )
    {
        if( PCB_DIM_ALIGNED* d = dynamic_cast<PCB_DIM_ALIGNED*>( item ) )
            dim2 = d;
        else if( PCB_BARCODE* b = dynamic_cast<PCB_BARCODE*>( item ) )
            bc2 = b;
        else if( PCB_TEXTBOX* x = dynamic_cast<PCB_TEXTBOX*>( item ) )
            tb2 = x;
        else if( PCB_TEXT* t = dynamic_cast<PCB_TEXT*>( item ) )
        {
            if( t->GetText() == wxT( "scale probe" ) )
                text2 = t;
        }
    }

    BOOST_REQUIRE( dim2 );
    BOOST_REQUIRE( bc2 );
    BOOST_REQUIRE( text2 );
    BOOST_REQUIRE( tb2 );

    BOOST_CHECK_MESSAGE( std::abs( dim2->GetStart().x - dimStartBefore.x ) <= 1
                                 && std::abs( dim2->GetStart().y - dimStartBefore.y ) <= 1,
                         "dim start after reload ( " << dim2->GetStart().x << ", " << dim2->GetStart().y
                                                     << " ) expected ( " << dimStartBefore.x << ", " << dimStartBefore.y
                                                     << " )" );
    BOOST_CHECK_MESSAGE( std::abs( dim2->GetEnd().x - dimEndBefore.x ) <= 1
                                 && std::abs( dim2->GetEnd().y - dimEndBefore.y ) <= 1,
                         "dim end after reload ( " << dim2->GetEnd().x << ", " << dim2->GetEnd().y << " ) expected ( "
                                                   << dimEndBefore.x << ", " << dimEndBefore.y << " )" );
    BOOST_CHECK_MESSAGE( std::abs( bc2->GetPosition().x - bcPosBefore.x ) <= 1
                                 && std::abs( bc2->GetPosition().y - bcPosBefore.y ) <= 1,
                         "barcode pos after reload ( " << bc2->GetPosition().x << ", " << bc2->GetPosition().y
                                                       << " ) expected ( " << bcPosBefore.x << ", " << bcPosBefore.y
                                                       << " )" );
    BOOST_CHECK_MESSAGE( std::abs( text2->GetTextPos().x - textPosBefore.x ) <= 1
                                 && std::abs( text2->GetTextPos().y - textPosBefore.y ) <= 1,
                         "text pos after reload ( " << text2->GetTextPos().x << ", " << text2->GetTextPos().y
                                                    << " ) expected ( " << textPosBefore.x << ", " << textPosBefore.y
                                                    << " )" );
    BOOST_CHECK_MESSAGE( std::abs( tb2->GetStart().x - tbStartBefore.x ) <= 1
                                 && std::abs( tb2->GetStart().y - tbStartBefore.y ) <= 1,
                         "textbox start after reload ( " << tb2->GetStart().x << ", " << tb2->GetStart().y
                                                         << " ) expected ( " << tbStartBefore.x << ", "
                                                         << tbStartBefore.y << " )" );
    BOOST_CHECK_MESSAGE( std::abs( tb2->GetEnd().x - tbEndBefore.x ) <= 1
                                 && std::abs( tb2->GetEnd().y - tbEndBefore.y ) <= 1,
                         "textbox end after reload ( " << tb2->GetEnd().x << ", " << tb2->GetEnd().y << " ) expected ( "
                                                       << tbEndBefore.x << ", " << tbEndBefore.y << " )" );
}


BOOST_AUTO_TEST_CASE( PCBTextSizeFollowsFootprintScale )
{
    // GetTextSize must return the lib size scaled by the parent transform.
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 0, 0 ) );

    PCB_TEXT* text = new PCB_TEXT( &fp );
    text->SetText( wxT( "X" ) );
    text->SetTextSize( VECTOR2I( 1000000, 500000 ) );
    fp.Add( text, ADD_MODE::APPEND );

    BOOST_CHECK_EQUAL( text->GetTextSize().x, 1000000 );
    BOOST_CHECK_EQUAL( text->GetTextSize().y, 500000 );

    fp.SetTransformScale( 2.0, 1.5 );

    BOOST_CHECK_EQUAL( text->GetTextSize().x, 2000000 );
    BOOST_CHECK_EQUAL( text->GetTextSize().y, 750000 );
}


BOOST_AUTO_TEST_CASE( FootprintRotateRoundTripPreservesChildRect )
{
    // Rotate by X then by -X must restore the child rect lib coords.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE* rect = new PCB_SHAPE( fp, SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( 1000000, 1000000 ) );
    rect->SetEnd( VECTOR2I( 5000000, 3000000 ) );
    fp->Add( rect, ADD_MODE::APPEND );

    VECTOR2I libStartBefore = rect->GetLibraryStart();
    VECTOR2I libEndBefore = rect->GetLibraryEnd();

    fp->Rotate( fp->GetPosition(), EDA_ANGLE( 45.0, DEGREES_T ) );
    fp->Rotate( fp->GetPosition(), EDA_ANGLE( -45.0, DEGREES_T ) );

    BOOST_CHECK_EQUAL( rect->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().y, libEndBefore.y );
}


BOOST_AUTO_TEST_CASE( FootprintSetOrientationRoundTripPreservesChildRect )
{
    // SetOrientation to a new angle and back must restore lib coords.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE* rect = new PCB_SHAPE( fp, SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( 1000000, 1000000 ) );
    rect->SetEnd( VECTOR2I( 5000000, 3000000 ) );
    fp->Add( rect, ADD_MODE::APPEND );

    VECTOR2I libStartBefore = rect->GetLibraryStart();
    VECTOR2I libEndBefore = rect->GetLibraryEnd();

    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    BOOST_CHECK_EQUAL( rect->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().y, libEndBefore.y );
}


BOOST_AUTO_TEST_CASE( FootprintSetOrientationNoOpPreservesChildRect )
{
    // SetOrientation with no actual change must not touch lib coords.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE* rect = new PCB_SHAPE( fp, SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( 1000000, 1000000 ) );
    rect->SetEnd( VECTOR2I( 5000000, 3000000 ) );
    fp->Add( rect, ADD_MODE::APPEND );

    VECTOR2I libStartBefore = rect->GetLibraryStart();
    VECTOR2I libEndBefore = rect->GetLibraryEnd();

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    BOOST_CHECK_EQUAL( rect->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().y, libEndBefore.y );
}


BOOST_AUTO_TEST_CASE( FootprintFlipPreservesChildPolyUnderRotation )
{
    // Single flip must mirror, double flip must restore. The single-flip check
    // catches a no-op POLY path that would round-trip trivially.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE*     poly = new PCB_SHAPE( fp, SHAPE_T::POLY );
    SHAPE_POLY_SET pset;
    pset.NewOutline();
    pset.Append( VECTOR2I( 1000000, 1000000 ) );
    pset.Append( VECTOR2I( 5000000, 1000000 ) );
    pset.Append( VECTOR2I( 3000000, 4000000 ) );
    poly->SetPolyShape( pset );
    fp->Add( poly, ADD_MODE::APPEND );

    SHAPE_POLY_SET polyBefore = poly->GetPolyShape();

    const VECTOR2I flipCentre = fp->GetPosition();
    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    SHAPE_POLY_SET polyAfter1 = poly->GetPolyShape();
    BOOST_REQUIRE_EQUAL( polyAfter1.TotalVertices(), polyBefore.TotalVertices() );

    bool anyVertexChanged = false;

    for( int i = 0; i < polyBefore.TotalVertices(); ++i )
    {
        if( polyBefore.CVertex( i ) != polyAfter1.CVertex( i ) )
            anyVertexChanged = true;
    }

    BOOST_CHECK_MESSAGE( anyVertexChanged, "polygon vertices did not change after one flip" );

    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    SHAPE_POLY_SET polyAfter2 = poly->GetPolyShape();
    BOOST_REQUIRE_EQUAL( polyAfter2.TotalVertices(), polyBefore.TotalVertices() );

    for( int i = 0; i < polyBefore.TotalVertices(); ++i )
    {
        VECTOR2I before = polyBefore.CVertex( i );
        VECTOR2I after = polyAfter2.CVertex( i );

        BOOST_CHECK_MESSAGE( std::abs( before.x - after.x ) <= 1 && std::abs( before.y - after.y ) <= 1,
                             "vertex " << i << " before ( " << before.x << ", " << before.y << " ) after ( " << after.x
                                       << ", " << after.y << " )" );
    }
}


BOOST_AUTO_TEST_CASE( FootprintFlipPreservesChildBezierUnderRotation )
{
    // Double flip must restore the bezier endpoints and control points.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE* bezier = new PCB_SHAPE( fp, SHAPE_T::BEZIER );
    bezier->SetStart( VECTOR2I( 1000000, 1000000 ) );
    bezier->SetEnd( VECTOR2I( 5000000, 3000000 ) );
    bezier->SetBezierC1( VECTOR2I( 2000000, 4000000 ) );
    bezier->SetBezierC2( VECTOR2I( 4000000, 0 ) );
    fp->Add( bezier, ADD_MODE::APPEND );

    VECTOR2I libStartBefore = bezier->GetLibraryStart();
    VECTOR2I libEndBefore = bezier->GetLibraryEnd();
    VECTOR2I libC1Before = bezier->GetLibraryBezierC1();
    VECTOR2I libC2Before = bezier->GetLibraryBezierC2();

    const VECTOR2I flipCentre = fp->GetPosition();
    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( bezier->GetLibraryStart() != libStartBefore || bezier->GetLibraryEnd() != libEndBefore
                                 || bezier->GetLibraryBezierC1() != libC1Before
                                 || bezier->GetLibraryBezierC2() != libC2Before,
                         "lib coords did not change after one flip" );

    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( bezier->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( bezier->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( bezier->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( bezier->GetLibraryEnd().y, libEndBefore.y );
    BOOST_CHECK_EQUAL( bezier->GetLibraryBezierC1().x, libC1Before.x );
    BOOST_CHECK_EQUAL( bezier->GetLibraryBezierC1().y, libC1Before.y );
    BOOST_CHECK_EQUAL( bezier->GetLibraryBezierC2().x, libC2Before.x );
    BOOST_CHECK_EQUAL( bezier->GetLibraryBezierC2().y, libC2Before.y );
}


BOOST_AUTO_TEST_CASE( FootprintFlipPreservesChildPointUnderRotation )
{
    // Double flip must restore a PCB_POINT position.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    board.Add( fp );

    PCB_POINT* point = new PCB_POINT( fp );
    point->SetPosition( VECTOR2I( fp->GetPosition().x + 2000000, fp->GetPosition().y + 1000000 ) );
    fp->Add( point, ADD_MODE::APPEND );

    VECTOR2I boardPosBefore = point->GetPosition();

    const VECTOR2I flipCentre = fp->GetPosition();
    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( point->GetPosition() != boardPosBefore, "board position did not change after one flip" );

    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( point->GetPosition().x, boardPosBefore.x );
    BOOST_CHECK_EQUAL( point->GetPosition().y, boardPosBefore.y );
}


BOOST_AUTO_TEST_CASE( FlipRoundTripUnderNonUniformScaleDivergedRect )
{
    // Non-uniform scale makes m_shape diverge from m_libShape (POLY vs RECTANGLE).
    // Double flip must still restore lib coords.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    board.Add( fp );

    PCB_SHAPE* rect = new PCB_SHAPE( fp, SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( 1000000, 1000000 ) );
    rect->SetEnd( VECTOR2I( 5000000, 3000000 ) );
    fp->Add( rect, ADD_MODE::APPEND );

    fp->SetTransformScale( 2.0, 1.0 );

    BOOST_REQUIRE_EQUAL( (int) rect->GetLibraryShape(), (int) SHAPE_T::RECTANGLE );

    VECTOR2I libStartBefore = rect->GetLibraryStart();
    VECTOR2I libEndBefore = rect->GetLibraryEnd();

    const VECTOR2I flipCentre = fp->GetPosition();
    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( rect->GetLibraryStart() != libStartBefore || rect->GetLibraryEnd() != libEndBefore,
                         "lib coords did not change after one flip" );

    fp->Flip( flipCentre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( rect->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( rect->GetLibraryEnd().y, libEndBefore.y );
}


BOOST_AUTO_TEST_CASE( BezierControlPointsFollowFootprintMove )
{
    // Bezier control points must track the parent FP transform on a move.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_SHAPE* bezier = new PCB_SHAPE( fp, SHAPE_T::BEZIER );
    bezier->SetStart( VECTOR2I( 1000000, 1000000 ) );
    bezier->SetEnd( VECTOR2I( 5000000, 3000000 ) );
    bezier->SetBezierC1( VECTOR2I( 2000000, 4000000 ) );
    bezier->SetBezierC2( VECTOR2I( 4000000, 0 ) );
    fp->Add( bezier, ADD_MODE::APPEND );

    VECTOR2I c1Before = bezier->GetBezierC1();
    VECTOR2I c2Before = bezier->GetBezierC2();

    fp->SetPosition( VECTOR2I( 10000000, 5000000 ) );

    BOOST_CHECK_EQUAL( bezier->GetBezierC1().x, c1Before.x + 10000000 );
    BOOST_CHECK_EQUAL( bezier->GetBezierC1().y, c1Before.y + 5000000 );
    BOOST_CHECK_EQUAL( bezier->GetBezierC2().x, c2Before.x + 10000000 );
    BOOST_CHECK_EQUAL( bezier->GetBezierC2().y, c2Before.y + 5000000 );
}


BOOST_AUTO_TEST_CASE( BezierControlPointsFollowFootprintRotate )
{
    // Bezier control points must follow the parent FP rotation.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_SHAPE* bezier = new PCB_SHAPE( fp, SHAPE_T::BEZIER );
    bezier->SetStart( VECTOR2I( 1000000, 0 ) );
    bezier->SetEnd( VECTOR2I( 0, 1000000 ) );
    bezier->SetBezierC1( VECTOR2I( 2000000, 0 ) );
    bezier->SetBezierC2( VECTOR2I( 0, 2000000 ) );
    fp->Add( bezier, ADD_MODE::APPEND );

    VECTOR2I c1Before = bezier->GetBezierC1();
    VECTOR2I c2Before = bezier->GetBezierC2();

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    BOOST_CHECK_MESSAGE( bezier->GetBezierC1() != c1Before, "C1 did not follow rotation" );
    BOOST_CHECK_MESSAGE( bezier->GetBezierC2() != c2Before, "C2 did not follow rotation" );
}


BOOST_AUTO_TEST_CASE( BezierBoardCoordsCorrectAfterRotation )
{
    // FOOTPRINT::SetOrientation must not double-rotate bezier control points.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_SHAPE* bezier = new PCB_SHAPE( fp, SHAPE_T::BEZIER );
    bezier->SetStart( VECTOR2I( 0, 0 ) );
    bezier->SetBezierC1( VECTOR2I( 1000000, 0 ) );
    bezier->SetBezierC2( VECTOR2I( 1000000, 1000000 ) );
    bezier->SetEnd( VECTOR2I( 2000000, 1000000 ) );
    fp->Add( bezier, ADD_MODE::APPEND );

    VECTOR2I libC1 = bezier->GetLibraryBezierC1();
    VECTOR2I libC2 = bezier->GetLibraryBezierC2();

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    const TRANSFORM_TRS& xf = fp->GetTransform();
    VECTOR2I             expectedC1 = xf.Apply( libC1 );
    VECTOR2I             expectedC2 = xf.Apply( libC2 );

    BOOST_CHECK_MESSAGE( std::abs( bezier->GetBezierC1().x - expectedC1.x ) <= 1
                                 && std::abs( bezier->GetBezierC1().y - expectedC1.y ) <= 1,
                         "C1 expected ( " << expectedC1.x << ", " << expectedC1.y << " ) actual ( "
                                          << bezier->GetBezierC1().x << ", " << bezier->GetBezierC1().y << " )" );
    BOOST_CHECK_MESSAGE( std::abs( bezier->GetBezierC2().x - expectedC2.x ) <= 1
                                 && std::abs( bezier->GetBezierC2().y - expectedC2.y ) <= 1,
                         "C2 expected ( " << expectedC2.x << ", " << expectedC2.y << " ) actual ( "
                                          << bezier->GetBezierC2().x << ", " << bezier->GetBezierC2().y << " )" );
}


BOOST_AUTO_TEST_CASE( BezierFromParserHasCorrectBoardCoords )
{
    // Mimic the parser path. After the manual lib-bake the board values must
    // equal Apply(lib).
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE* bezier = new PCB_SHAPE( fp, SHAPE_T::BEZIER );

    // Parser-style: feed in lib-frame coords through the setters.
    const VECTOR2I libStart( 1000000, 1000000 );
    const VECTOR2I libC1( 2000000, 4000000 );
    const VECTOR2I libC2( 4000000, 0 );
    const VECTOR2I libEnd( 5000000, 3000000 );

    bezier->SetStart( libStart );
    bezier->SetBezierC1( libC1 );
    bezier->SetBezierC2( libC2 );
    bezier->SetEnd( libEnd );

    // Manual lib-bake mimicking the parser fix-up.
    bezier->OverrideLibCoords( libStart, libEnd );
    bezier->OverrideLibBezier( libC1, libC2 );
    bezier->RebakeFromLib();

    fp->Add( bezier, ADD_MODE::APPEND );

    const TRANSFORM_TRS& xf = fp->GetTransform();

    auto checkBoard = [&]( const char* name, const VECTOR2I& lib, const VECTOR2I& board )
    {
        VECTOR2I expected = xf.Apply( lib );
        BOOST_CHECK_MESSAGE( std::abs( expected.x - board.x ) <= 1 && std::abs( expected.y - board.y ) <= 1,
                             name << " expected ( " << expected.x << ", " << expected.y << " ) actual ( " << board.x
                                  << ", " << board.y << " )" );
    };

    checkBoard( "start", libStart, bezier->GetStart() );
    checkBoard( "C1", libC1, bezier->GetBezierC1() );
    checkBoard( "C2", libC2, bezier->GetBezierC2() );
    checkBoard( "end", libEnd, bezier->GetEnd() );
}


BOOST_AUTO_TEST_CASE( PolyFromParserHasCorrectBoardCoords )
{
    // After the manual lib-to-board bake the polygon must land at the FP position.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    board.Add( fp );

    PCB_SHAPE* poly = new PCB_SHAPE( fp, SHAPE_T::POLY );

    std::vector<VECTOR2I> libVerts = {
        VECTOR2I( 1000000, 1000000 ),
        VECTOR2I( 5000000, 1000000 ),
        VECTOR2I( 3000000, 4000000 ),
    };

    SHAPE_POLY_SET pset;
    pset.NewOutline();
    for( const VECTOR2I& v : libVerts )
        pset.Append( v );

    poly->SetPolyShape( pset );

    // Mimic the parser fix-up that transforms m_poly from lib to board.
    const TRANSFORM_TRS& xform = fp->GetTransform();
    SHAPE_POLY_SET&      mpoly = poly->GetPolyShape();
    for( auto it = mpoly.IterateWithHoles(); it; it++ )
        mpoly.SetVertex( it.GetIndex(), xform.Apply( *it ) );

    fp->Add( poly, ADD_MODE::APPEND );

    SHAPE_POLY_SET result = poly->GetPolyShape();
    BOOST_REQUIRE_EQUAL( result.TotalVertices(), (int) libVerts.size() );

    for( int i = 0; i < (int) libVerts.size(); ++i )
    {
        VECTOR2I expected = xform.Apply( libVerts[i] );
        VECTOR2I actual = result.CVertex( i );

        BOOST_CHECK_MESSAGE( std::abs( expected.x - actual.x ) <= 1 && std::abs( expected.y - actual.y ) <= 1,
                             "vertex " << i << " expected ( " << expected.x << ", " << expected.y << " ) actual ( "
                                       << actual.x << ", " << actual.y << " )" );
    }
}


BOOST_AUTO_TEST_CASE( PolyFollowsFootprintRotate )
{
    // POLY vertices must follow the parent FP rotation.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_SHAPE*     poly = new PCB_SHAPE( fp, SHAPE_T::POLY );
    SHAPE_POLY_SET pset;
    pset.NewOutline();
    pset.Append( VECTOR2I( 1000000, 0 ) );
    pset.Append( VECTOR2I( 0, 1000000 ) );
    pset.Append( VECTOR2I( -1000000, 0 ) );
    poly->SetPolyShape( pset );
    fp->Add( poly, ADD_MODE::APPEND );

    SHAPE_POLY_SET before = poly->GetPolyShape();

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    SHAPE_POLY_SET after = poly->GetPolyShape();
    BOOST_REQUIRE_EQUAL( after.TotalVertices(), before.TotalVertices() );

    bool anyMoved = false;
    for( int i = 0; i < before.TotalVertices(); ++i )
    {
        if( before.CVertex( i ) != after.CVertex( i ) )
            anyMoved = true;
    }

    BOOST_CHECK_MESSAGE( anyMoved, "polygon vertices did not move after rotation" );

    // Round-trip: rotate back to 0 must restore the original positions.
    fp->SetOrientation( EDA_ANGLE( 0.0, DEGREES_T ) );

    SHAPE_POLY_SET restored = poly->GetPolyShape();
    BOOST_REQUIRE_EQUAL( restored.TotalVertices(), before.TotalVertices() );

    for( int i = 0; i < before.TotalVertices(); ++i )
    {
        VECTOR2I b = before.CVertex( i );
        VECTOR2I r = restored.CVertex( i );

        BOOST_CHECK_MESSAGE( std::abs( b.x - r.x ) <= 1 && std::abs( b.y - r.y ) <= 1,
                             "vertex " << i << " before ( " << b.x << ", " << b.y << " ) restored ( " << r.x << ", "
                                       << r.y << " )" );
    }
}


BOOST_AUTO_TEST_CASE( PolyFollowsFootprintMove )
{
    // POLY vertices must move with the parent FP.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_SHAPE*     poly = new PCB_SHAPE( fp, SHAPE_T::POLY );
    SHAPE_POLY_SET pset;
    pset.NewOutline();
    pset.Append( VECTOR2I( 1000000, 1000000 ) );
    pset.Append( VECTOR2I( 5000000, 1000000 ) );
    pset.Append( VECTOR2I( 3000000, 4000000 ) );
    poly->SetPolyShape( pset );
    fp->Add( poly, ADD_MODE::APPEND );

    SHAPE_POLY_SET before = poly->GetPolyShape();

    const VECTOR2I delta( 10000000, 5000000 );
    fp->SetPosition( fp->GetPosition() + delta );

    SHAPE_POLY_SET after = poly->GetPolyShape();
    BOOST_REQUIRE_EQUAL( after.TotalVertices(), before.TotalVertices() );

    for( int i = 0; i < before.TotalVertices(); ++i )
    {
        VECTOR2I expected = before.CVertex( i ) + delta;
        VECTOR2I actual = after.CVertex( i );

        BOOST_CHECK_MESSAGE( std::abs( expected.x - actual.x ) <= 1 && std::abs( expected.y - actual.y ) <= 1,
                             "vertex " << i << " expected ( " << expected.x << ", " << expected.y << " ) actual ( "
                                       << actual.x << ", " << actual.y << " )" );
    }
}


BOOST_AUTO_TEST_CASE( PadOrientationLibFrameFollowsFootprintRotate )
{
    // Pad orientation is stored FP-relative. When the FP rotates, the
    // absolute orientation must change but the FP-relative one must stay.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PAD* pad = new PAD( fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 1000000, 1000000 ) );
    pad->SetPosition( VECTOR2I( 5000000, 0 ) );
    pad->SetFPRelativeOrientation( EDA_ANGLE( 45.0, DEGREES_T ) );
    pad->SetLayerSet( PAD::SMDMask() );
    fp->Add( pad, ADD_MODE::APPEND );

    BOOST_CHECK_CLOSE( pad->GetOrientation().AsDegrees(), 45.0, 1e-6 );
    BOOST_CHECK_CLOSE( pad->GetFPRelativeOrientation().AsDegrees(), 45.0, 1e-6 );

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    // Absolute orientation has followed the FP.
    BOOST_CHECK_CLOSE( pad->GetOrientation().AsDegrees(), 135.0, 1e-6 );
    // FP-relative orientation has not.
    BOOST_CHECK_CLOSE( pad->GetFPRelativeOrientation().AsDegrees(), 45.0, 1e-6 );

    fp->SetOrientation( EDA_ANGLE( 0.0, DEGREES_T ) );
    BOOST_CHECK_CLOSE( pad->GetOrientation().AsDegrees(), 45.0, 1e-6 );
    BOOST_CHECK_CLOSE( pad->GetFPRelativeOrientation().AsDegrees(), 45.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( PCBTextAngleLibFrameFollowsFootprintRotate )
{
    // Text angle is stored FP-relative. When the FP rotates, the absolute
    // angle must change but the FP-relative one must stay.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_TEXT* text = new PCB_TEXT( fp );
    text->SetText( wxT( "X" ) );
    text->SetTextPos( VECTOR2I( 1000000, 0 ) );
    text->SetTextAngle( EDA_ANGLE( 45.0, DEGREES_T ) );
    fp->Add( text, ADD_MODE::APPEND );

    BOOST_CHECK_CLOSE( text->GetTextAngle().AsDegrees(), 45.0, 1e-6 );

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    BOOST_CHECK_CLOSE( text->GetTextAngle().AsDegrees(), 135.0, 1e-6 );

    fp->SetOrientation( EDA_ANGLE( 0.0, DEGREES_T ) );
    BOOST_CHECK_CLOSE( text->GetTextAngle().AsDegrees(), 45.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( PCBTextBoxAngleLibFrameFollowsFootprintRotate )
{
    // PCB_TEXTBOX angle is stored FP-relative. FP rotation updates the
    // absolute, lib stays.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_TEXTBOX* tb = new PCB_TEXTBOX( fp );
    tb->SetStart( VECTOR2I( 0, 0 ) );
    tb->SetEnd( VECTOR2I( 2000000, 1000000 ) );
    tb->SetText( wxT( "X" ) );
    tb->SetTextAngle( EDA_ANGLE( 45.0, DEGREES_T ) );
    fp->Add( tb, ADD_MODE::APPEND );

    BOOST_CHECK_CLOSE( tb->GetTextAngle().AsDegrees(), 45.0, 1e-6 );

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    BOOST_CHECK_CLOSE( tb->GetTextAngle().AsDegrees(), 135.0, 1e-6 );

    fp->SetOrientation( EDA_ANGLE( 0.0, DEGREES_T ) );
    BOOST_CHECK_CLOSE( tb->GetTextAngle().AsDegrees(), 45.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( PCBTextBoxAngleLibFrameSurvivesFlipRoundTrip )
{
    // Double flip must restore both the absolute angle and the lib angle. If
    // PCB_TEXTBOX::Flip / Mirror forgets to keep m_libTextAngle in sync, the
    // second flip would compute the wrong absolute on the way back.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 5000000 ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    board.Add( fp );

    PCB_TEXTBOX* tb = new PCB_TEXTBOX( fp );
    tb->SetStart( VECTOR2I( 0, 0 ) );
    tb->SetEnd( VECTOR2I( 2000000, 1000000 ) );
    tb->SetText( wxT( "X" ) );
    tb->SetTextAngle( EDA_ANGLE( 15.0, DEGREES_T ) );
    fp->Add( tb, ADD_MODE::APPEND );

    EDA_ANGLE absBefore = tb->GetTextAngle();

    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( tb->GetTextAngle() != absBefore, "text angle did not change after one flip" );

    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_CLOSE( tb->GetTextAngle().AsDegrees(), absBefore.AsDegrees(), 1e-6 );
}


BOOST_AUTO_TEST_CASE( DimensionFollowsFootprintScale )
{
    // PCB_DIMENSION inside an FP: start and end must scale with the parent
    // FP transform.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_DIM_ALIGNED* dim = new PCB_DIM_ALIGNED( fp, PCB_DIM_ALIGNED_T );
    dim->SetStart( VECTOR2I( 0, 0 ) );
    dim->SetEnd( VECTOR2I( 10000000, 0 ) );
    fp->Add( dim, ADD_MODE::APPEND );

    fp->SetTransformScale( 2.0, 1.0 );

    BOOST_CHECK_EQUAL( dim->GetStart().x, 0 );
    BOOST_CHECK_EQUAL( dim->GetEnd().x, 20000000 );
}


BOOST_AUTO_TEST_CASE( DimensionAngleLibFrameFollowsFootprintRotate )
{
    // FP rotation updates the absolute text angle, lib value stays.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_DIM_ALIGNED* dim = new PCB_DIM_ALIGNED( fp, PCB_DIM_ALIGNED_T );
    dim->SetKeepTextAligned( false );
    dim->SetStart( VECTOR2I( 0, 0 ) );
    dim->SetEnd( VECTOR2I( 10000000, 0 ) );
    dim->SetTextAngle( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp->Add( dim, ADD_MODE::APPEND );

    BOOST_CHECK_CLOSE( dim->GetTextAngle().AsDegrees(), 30.0, 1e-6 );

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    BOOST_CHECK_CLOSE( dim->GetTextAngle().AsDegrees(), 120.0, 1e-6 );

    fp->SetOrientation( EDA_ANGLE( 0.0, DEGREES_T ) );
    BOOST_CHECK_CLOSE( dim->GetTextAngle().AsDegrees(), 30.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( DimensionLibCoordsSurviveFootprintRotate )
{
    // Lib coords stay invariant under FP rotation.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 30000000, 20000000 ) );
    board.Add( fp );

    PCB_DIM_ALIGNED* dim = new PCB_DIM_ALIGNED( fp, PCB_DIM_ALIGNED_T );
    dim->SetStart( VECTOR2I( 1000000, 2000000 ) );
    dim->SetEnd( VECTOR2I( 5000000, 3000000 ) );
    fp->Add( dim, ADD_MODE::APPEND );

    VECTOR2I libStartBefore = dim->GetLibraryStart();
    VECTOR2I libEndBefore = dim->GetLibraryEnd();

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    fp->SetOrientation( EDA_ANGLE( 0.0, DEGREES_T ) );

    BOOST_CHECK_EQUAL( dim->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( dim->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( dim->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( dim->GetLibraryEnd().y, libEndBefore.y );
}


BOOST_AUTO_TEST_CASE( BarcodeFollowsFootprintRotate )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_BARCODE* bc = new PCB_BARCODE( fp );
    bc->SetPosition( VECTOR2I( 5000000, 0 ) );
    bc->SetOrientation( 30.0 );
    fp->Add( bc, ADD_MODE::APPEND );

    BOOST_CHECK_EQUAL( bc->GetLibraryPos().x, 5000000 );
    BOOST_CHECK_EQUAL( bc->GetLibraryPos().y, 0 );
    BOOST_CHECK_CLOSE( bc->GetLibraryAngle().AsDegrees(), 30.0, 1e-6 );

    fp->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );

    // Lib stays the same. Board angle picks up the FP rotation.
    BOOST_CHECK_EQUAL( bc->GetLibraryPos().x, 5000000 );
    BOOST_CHECK_EQUAL( bc->GetLibraryPos().y, 0 );
    BOOST_CHECK_CLOSE( bc->GetLibraryAngle().AsDegrees(), 30.0, 1e-6 );
    BOOST_CHECK_CLOSE( bc->GetAngle().AsDegrees(), 120.0, 1e-6 );

    fp->SetOrientation( EDA_ANGLE( 0.0, DEGREES_T ) );
    BOOST_CHECK_CLOSE( bc->GetAngle().AsDegrees(), 30.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( BarcodeFollowsFootprintMove )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );
    board.Add( fp );

    PCB_BARCODE* bc = new PCB_BARCODE( fp );
    bc->SetPosition( VECTOR2I( 3000000, 2000000 ) );
    fp->Add( bc, ADD_MODE::APPEND );

    VECTOR2I libBefore = bc->GetLibraryPos();

    fp->SetPosition( VECTOR2I( 10000000, 5000000 ) );

    BOOST_CHECK_EQUAL( bc->GetLibraryPos().x, libBefore.x );
    BOOST_CHECK_EQUAL( bc->GetLibraryPos().y, libBefore.y );
    BOOST_CHECK_EQUAL( bc->GetPosition().x, 13000000 );
    BOOST_CHECK_EQUAL( bc->GetPosition().y, 7000000 );
}


BOOST_AUTO_TEST_CASE( BarcodeFlipPreservesBoardAngleDelta )
{
    // Single flip should change the visible angle by +180 for TOP_BOTTOM,
    // 0 for LEFT_RIGHT, regardless of FP rotation.
    auto checkFlip = [&]( double aFpOrientDeg, FLIP_DIRECTION aDir, double aExpectedDelta )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        fp->SetPosition( VECTOR2I( 10000000, 5000000 ) );
        fp->SetOrientation( EDA_ANGLE( aFpOrientDeg, DEGREES_T ) );
        board.Add( fp );

        PCB_BARCODE* bc = new PCB_BARCODE( fp );
        bc->SetPosition( fp->GetPosition() + VECTOR2I( 2000000, 1000000 ) );
        bc->SetOrientation( 0.0 );
        fp->Add( bc, ADD_MODE::APPEND );

        EDA_ANGLE before = bc->GetAngle();

        fp->Flip( fp->GetPosition(), aDir );

        EDA_ANGLE after = bc->GetAngle();
        EDA_ANGLE expected = ( before + EDA_ANGLE( aExpectedDelta, DEGREES_T ) ).Normalize();
        EDA_ANGLE actual = after.Normalize();

        BOOST_CHECK_CLOSE( actual.AsDegrees(), expected.AsDegrees(), 1e-6 );
    };

    checkFlip( 0.0, FLIP_DIRECTION::TOP_BOTTOM, 180.0 );
    checkFlip( 30.0, FLIP_DIRECTION::TOP_BOTTOM, 180.0 );
    checkFlip( 90.0, FLIP_DIRECTION::TOP_BOTTOM, 180.0 );
    checkFlip( 0.0, FLIP_DIRECTION::LEFT_RIGHT, 0.0 );
    checkFlip( 30.0, FLIP_DIRECTION::LEFT_RIGHT, 0.0 );
}


BOOST_AUTO_TEST_CASE( BarcodeSurvivesFootprintFlipRoundTrip )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 20000000, 10000000 ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    board.Add( fp );

    PCB_BARCODE* bc = new PCB_BARCODE( fp );
    bc->SetPosition( fp->GetPosition() + VECTOR2I( 2000000, 1000000 ) );
    bc->SetOrientation( 15.0 );
    fp->Add( bc, ADD_MODE::APPEND );

    VECTOR2I  libPosBefore = bc->GetLibraryPos();
    EDA_ANGLE libAngleBefore = bc->GetLibraryAngle();
    VECTOR2I  posBefore = bc->GetPosition();
    EDA_ANGLE angleBefore = bc->GetAngle();

    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( bc->GetPosition() != posBefore, "barcode pos did not move on first flip" );

    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( bc->GetLibraryPos().x, libPosBefore.x );
    BOOST_CHECK_EQUAL( bc->GetLibraryPos().y, libPosBefore.y );
    BOOST_CHECK_CLOSE( bc->GetLibraryAngle().AsDegrees(), libAngleBefore.AsDegrees(), 1e-6 );
    BOOST_CHECK_EQUAL( bc->GetPosition().x, posBefore.x );
    BOOST_CHECK_EQUAL( bc->GetPosition().y, posBefore.y );
    BOOST_CHECK_CLOSE( bc->GetAngle().AsDegrees(), angleBefore.AsDegrees(), 1e-6 );
}


BOOST_AUTO_TEST_CASE( DimensionLibCoordsSurviveFootprintFlipRoundTrip )
{
    // Double flip restores lib coords and board caches for dimensions.
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 20000000, 10000000 ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    board.Add( fp );

    PCB_DIM_ALIGNED* dim = new PCB_DIM_ALIGNED( fp, PCB_DIM_ALIGNED_T );
    dim->SetKeepTextAligned( false );
    dim->SetStart( VECTOR2I( 1000000, 2000000 ) );
    dim->SetEnd( VECTOR2I( 5000000, 3000000 ) );
    dim->SetTextAngle( EDA_ANGLE( 15.0, DEGREES_T ) );
    fp->Add( dim, ADD_MODE::APPEND );

    VECTOR2I  libStartBefore = dim->GetLibraryStart();
    VECTOR2I  libEndBefore = dim->GetLibraryEnd();
    EDA_ANGLE absAngleBefore = dim->GetTextAngle();
    VECTOR2I  startBefore = dim->GetStart();
    VECTOR2I  endBefore = dim->GetEnd();

    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_MESSAGE( dim->GetStart() != startBefore, "dim start did not move on first flip" );

    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( dim->GetLibraryStart().x, libStartBefore.x );
    BOOST_CHECK_EQUAL( dim->GetLibraryStart().y, libStartBefore.y );
    BOOST_CHECK_EQUAL( dim->GetLibraryEnd().x, libEndBefore.x );
    BOOST_CHECK_EQUAL( dim->GetLibraryEnd().y, libEndBefore.y );
    BOOST_CHECK_CLOSE( dim->GetTextAngle().AsDegrees(), absAngleBefore.AsDegrees(), 1e-6 );
    BOOST_CHECK_EQUAL( dim->GetStart().x, startBefore.x );
    BOOST_CHECK_EQUAL( dim->GetStart().y, startBefore.y );
    BOOST_CHECK_EQUAL( dim->GetEnd().x, endBefore.x );
    BOOST_CHECK_EQUAL( dim->GetEnd().y, endBefore.y );
}


/**
 * A standalone rectangle rotated by a non-cardinal angle should keep that
 * orientation when saved and reloaded. The rectangle converts to a polygon
 * on rotation, which is the on-disk form that preserves the tilt.
 */
BOOST_FIXTURE_TEST_CASE( StandaloneRectangleNonCardinalRotationSurvivesSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    PCB_SHAPE* rect = new PCB_SHAPE( m_board.get(), SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( 0, 0 ) );
    rect->SetEnd( VECTOR2I( 10000000, 5000000 ) );
    rect->SetLayer( F_SilkS );
    m_board->Add( rect, ADD_MODE::APPEND );

    BOOST_REQUIRE( rect->GetShape() == SHAPE_T::RECTANGLE );

    rect->Rotate( VECTOR2I( 0, 0 ), EDA_ANGLE( 30.0, DEGREES_T ) );

    BOOST_CHECK( rect->GetShape() == SHAPE_T::POLY );
    BOOST_REQUIRE_EQUAL( rect->GetPolyShape().OutlineCount(), 1 );

    std::vector<VECTOR2I> savedCorners;
    for( const VECTOR2I& p : rect->GetPolyShape().Outline( 0 ).CPoints() )
        savedCorners.emplace_back( p );

    const std::filesystem::path savePath =
            std::filesystem::temp_directory_path() / "rect_noncardinal_roundtrip.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* reloadedRect = nullptr;
    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* s = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( s->GetShape() == SHAPE_T::POLY )
            {
                reloadedRect = s;
                break;
            }
        }
    }

    BOOST_REQUIRE( reloadedRect );
    BOOST_REQUIRE_EQUAL( reloadedRect->GetPolyShape().OutlineCount(), 1 );

    const auto& reloadedPoints = reloadedRect->GetPolyShape().Outline( 0 ).CPoints();
    BOOST_REQUIRE_EQUAL( (int) reloadedPoints.size(), (int) savedCorners.size() );

    for( size_t i = 0; i < savedCorners.size(); ++i )
    {
        BOOST_CHECK_EQUAL( reloadedPoints[i].x, savedCorners[i].x );
        BOOST_CHECK_EQUAL( reloadedPoints[i].y, savedCorners[i].y );
    }
}


BOOST_AUTO_TEST_CASE( MoveAnchorMovesPointsLikeOtherChildren )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    board.Add( fp );

    PAD* pad = new PAD( fp );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetLayerSet( PAD::SMDMask() );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 1000000, 1000000 ) );
    pad->SetPosition( VECTOR2I( 5000000, 2000000 ) );
    fp->Add( pad );

    PCB_POINT* pt = new PCB_POINT( fp );
    pt->SetLayer( F_Cu );
    pt->SetPosition( VECTOR2I( 3000000, 4000000 ) );
    fp->Add( pt );

    const VECTOR2I padToPointBefore = pt->GetPosition() - pad->GetPosition();

    fp->MoveAnchorPosition( VECTOR2I( 2000000, 1000000 ) );

    // The point must move with the rest of the footprint, keeping its spacing to the pad.
    const VECTOR2I padToPointAfter = pt->GetPosition() - pad->GetPosition();
    BOOST_CHECK_EQUAL( padToPointAfter.x, padToPointBefore.x );
    BOOST_CHECK_EQUAL( padToPointAfter.y, padToPointBefore.y );
}


BOOST_AUTO_TEST_CASE( FlipNonCardinalKeepsPadAndShapeCoincident )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 50000000, 50000000 ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    board.Add( fp );

    const VECTOR2I coincident( 53000000, 51000000 );

    PAD* pad = new PAD( fp );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetLayerSet( PAD::SMDMask() );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 1000000, 1000000 ) );
    pad->SetPosition( coincident );
    fp->Add( pad );

    PCB_SHAPE* seg = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
    seg->SetStart( coincident );
    seg->SetEnd( coincident + VECTOR2I( 1000000, 0 ) );
    seg->SetLayer( F_SilkS );
    fp->Add( seg );

    // Sanity: coincident before the flip.
    BOOST_REQUIRE_EQUAL( pad->GetPosition().x, seg->GetStart().x );
    BOOST_REQUIRE_EQUAL( pad->GetPosition().y, seg->GetStart().y );

    PCB_TEXT* txt = new PCB_TEXT( fp );
    txt->SetText( "X" );
    txt->SetTextPos( coincident );
    txt->SetLayer( F_SilkS );
    fp->Add( txt );

    PCB_POINT* pt = new PCB_POINT( fp );
    pt->SetLayer( F_Cu );
    pt->SetPosition( coincident );
    fp->Add( pt );

    BOOST_REQUIRE_EQUAL( txt->GetTextPos().x, seg->GetStart().x );
    BOOST_REQUIRE_EQUAL( pt->GetPosition().x, seg->GetStart().x );

    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    // All footprint children must still coincide after the flip.
    BOOST_CHECK_EQUAL( pad->GetPosition().x, seg->GetStart().x );
    BOOST_CHECK_EQUAL( pad->GetPosition().y, seg->GetStart().y );
    BOOST_CHECK_EQUAL( txt->GetTextPos().x, seg->GetStart().x );
    BOOST_CHECK_EQUAL( txt->GetTextPos().y, seg->GetStart().y );
    BOOST_CHECK_EQUAL( pt->GetPosition().x, seg->GetStart().x );
    BOOST_CHECK_EQUAL( pt->GetPosition().y, seg->GetStart().y );
}


BOOST_AUTO_TEST_CASE( FlipBarcodeAngleMatchesTextOnRotatedFootprint )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 50000000, 50000000 ) );
    fp->SetOrientation( EDA_ANGLE( 30.0, DEGREES_T ) );
    board.Add( fp );

    PCB_TEXT* txt = new PCB_TEXT( fp );
    txt->SetText( "X" );
    txt->SetLayer( F_SilkS );
    txt->SetTextAngle( EDA_ANGLE( 75.0, DEGREES_T ) );
    fp->Add( txt );

    PCB_BARCODE* bc = new PCB_BARCODE( fp );
    bc->SetText( "12345" );
    bc->SetWidth( 3000000 );
    bc->SetHeight( 3000000 );
    bc->SetTextSize( 1500000 );
    bc->SetLayer( F_SilkS );
    bc->SetOrientation( 75.0 );
    bc->AssembleBarcode();
    fp->Add( bc );

    // Same board angle before the flip.
    BOOST_REQUIRE_CLOSE( bc->GetOrientation(), txt->GetTextAngle().AsDegrees(), 1e-6 );

    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    // A barcode must reflect its angle on flip exactly like text does.
    EDA_ANGLE bcAngle( bc->GetOrientation(), DEGREES_T );
    EDA_ANGLE txtAngle = txt->GetTextAngle();
    bcAngle.Normalize();
    txtAngle.Normalize();
    BOOST_CHECK_CLOSE( bcAngle.AsDegrees(), txtAngle.AsDegrees(), 1e-6 );
}


// Build a footprint at the given placement with a pad, graphic, text and point all
// anchored at the same board point. Returns the footprint; child pointers via out-params.
static FOOTPRINT* buildCoincidentFootprint( BOARD& aBoard, const EDA_ANGLE& aOrient, double aScaleX, double aScaleY,
                                            const VECTOR2I& aCoincident, PAD*& aPad, PCB_SHAPE*& aSeg, PCB_TEXT*& aTxt,
                                            PCB_POINT*& aPt )
{
    FOOTPRINT* fp = new FOOTPRINT( &aBoard );
    fp->SetPosition( VECTOR2I( 50000000, 50000000 ) );
    fp->SetOrientation( aOrient );
    fp->SetTransformScale( aScaleX, aScaleY );
    aBoard.Add( fp );

    aPad = new PAD( fp );
    aPad->SetAttribute( PAD_ATTRIB::SMD );
    aPad->SetLayerSet( PAD::SMDMask() );
    aPad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 1000000, 1000000 ) );
    aPad->SetPosition( aCoincident );
    fp->Add( aPad );

    aSeg = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
    aSeg->SetStart( aCoincident );
    aSeg->SetEnd( aCoincident + VECTOR2I( 1000000, 0 ) );
    aSeg->SetLayer( F_SilkS );
    fp->Add( aSeg );

    aTxt = new PCB_TEXT( fp );
    aTxt->SetText( "X" );
    aTxt->SetTextPos( aCoincident );
    aTxt->SetLayer( F_SilkS );
    fp->Add( aTxt );

    aPt = new PCB_POINT( fp );
    aPt->SetLayer( F_Cu );
    aPt->SetPosition( aCoincident );
    fp->Add( aPt );

    return fp;
}


static void CHECK_ALL_COINCIDENT( PAD* aPad, PCB_SHAPE* aSeg, PCB_TEXT* aTxt, PCB_POINT* aPt, const std::string& aWhen )
{
    BOOST_CHECK_MESSAGE( aPad->GetPosition() == aSeg->GetStart(), aWhen << ": pad vs seg" );
    BOOST_CHECK_MESSAGE( aTxt->GetTextPos() == aSeg->GetStart(), aWhen << ": text vs seg" );
    BOOST_CHECK_MESSAGE( aPt->GetPosition() == aSeg->GetStart(), aWhen << ": point vs seg" );
}


BOOST_AUTO_TEST_CASE( RoundTripPreservesChildGeometryScaledRotated )
{
    auto       board = std::make_unique<BOARD>();
    PAD*       pad;
    PCB_SHAPE* seg;
    PCB_TEXT*  txt;
    PCB_POINT* pt;
    buildCoincidentFootprint( *board, EDA_ANGLE( 30.0, DEGREES_T ), 2.0, 1.5, VECTOR2I( 53000000, 51000000 ), pad, seg,
                              txt, pt );

    const VECTOR2I padPos = pad->GetPosition();
    const VECTOR2I segStart = seg->GetStart();
    const VECTOR2I txtPos = txt->GetTextPos();
    const VECTOR2I ptPos = pt->GetPosition();

    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "kicad_qa_xform_roundtrip.kicad_pcb";
    KI_TEST::DumpBoardToFile( *board, tmp.string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.string() );
    std::error_code        ec;
    std::filesystem::remove( tmp, ec );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* r = reloaded->Footprints().front();
    BOOST_REQUIRE( !r->Pads().empty() && !r->Points().empty() );
    PCB_SHAPE* rseg = nullptr;
    PCB_TEXT*  rtxt = nullptr;

    for( BOARD_ITEM* it : r->GraphicalItems() )
    {
        if( it->Type() == PCB_SHAPE_T )
            rseg = static_cast<PCB_SHAPE*>( it );
        else if( it->Type() == PCB_TEXT_T )
            rtxt = static_cast<PCB_TEXT*>( it );
    }

    BOOST_REQUIRE( rseg && rtxt );

    // Absolute identity: every child returns to its exact board position.
    BOOST_CHECK( r->Pads().front()->GetPosition() == padPos );
    BOOST_CHECK( rseg->GetStart() == segStart );
    BOOST_CHECK( rtxt->GetTextPos() == txtPos );
    BOOST_CHECK( r->Points().front()->GetPosition() == ptPos );
}


BOOST_AUTO_TEST_CASE( RotateAndMoveKeepChildrenCoincident )
{
    BOARD      board;
    PAD*       pad;
    PCB_SHAPE* seg;
    PCB_TEXT*  txt;
    PCB_POINT* pt;
    FOOTPRINT* fp = buildCoincidentFootprint( board, EDA_ANGLE( 30.0, DEGREES_T ), 2.0, 1.5,
                                              VECTOR2I( 53000000, 51000000 ), pad, seg, txt, pt );

    CHECK_ALL_COINCIDENT( pad, seg, txt, pt, "before" );
    fp->SetOrientation( fp->GetOrientation() + EDA_ANGLE( 17.0, DEGREES_T ) );
    CHECK_ALL_COINCIDENT( pad, seg, txt, pt, "after rotate" );
    fp->SetPosition( fp->GetPosition() + VECTOR2I( 1234567, -7654321 ) );
    CHECK_ALL_COINCIDENT( pad, seg, txt, pt, "after move" );
    fp->SetTransformScale( 1.3, 2.7 );
    CHECK_ALL_COINCIDENT( pad, seg, txt, pt, "after rescale" );
}


BOOST_AUTO_TEST_CASE( FlipLeftRightKeepsChildrenCoincident )
{
    BOARD      board;
    PAD*       pad;
    PCB_SHAPE* seg;
    PCB_TEXT*  txt;
    PCB_POINT* pt;
    FOOTPRINT* fp = buildCoincidentFootprint( board, EDA_ANGLE( 30.0, DEGREES_T ), 1.0, 1.0,
                                              VECTOR2I( 53000000, 51000000 ), pad, seg, txt, pt );

    CHECK_ALL_COINCIDENT( pad, seg, txt, pt, "before" );
    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
    CHECK_ALL_COINCIDENT( pad, seg, txt, pt, "after LEFT_RIGHT flip" );
}


BOOST_AUTO_TEST_CASE( CloneKeepsChildGeometryScaledRotated )
{
    BOARD      board;
    PAD*       pad;
    PCB_SHAPE* seg;
    PCB_TEXT*  txt;
    PCB_POINT* pt;
    FOOTPRINT* fp = buildCoincidentFootprint( board, EDA_ANGLE( 30.0, DEGREES_T ), 2.0, 1.5,
                                              VECTOR2I( 53000000, 51000000 ), pad, seg, txt, pt );

    const VECTOR2I ref = seg->GetStart();

    std::unique_ptr<FOOTPRINT> clone( static_cast<FOOTPRINT*>( fp->Clone() ) );

    BOOST_REQUIRE( !clone->Pads().empty() && !clone->Points().empty() );
    PCB_SHAPE* cseg = nullptr;
    PCB_TEXT*  ctxt = nullptr;

    for( BOARD_ITEM* it : clone->GraphicalItems() )
    {
        if( it->Type() == PCB_SHAPE_T )
            cseg = static_cast<PCB_SHAPE*>( it );
        else if( it->Type() == PCB_TEXT_T )
            ctxt = static_cast<PCB_TEXT*>( it );
    }

    BOOST_REQUIRE( cseg && ctxt );

    // The clone's children must sit at the same board geometry as the original.
    BOOST_CHECK( clone->Pads().front()->GetPosition() == ref );
    BOOST_CHECK( cseg->GetStart() == ref );
    BOOST_CHECK( ctxt->GetTextPos() == ref );
    BOOST_CHECK( clone->Points().front()->GetPosition() == ref );
}


BOOST_AUTO_TEST_CASE( DoubleFlipIsIdentityScaledRotated )
{
    for( FLIP_DIRECTION dir : { FLIP_DIRECTION::TOP_BOTTOM, FLIP_DIRECTION::LEFT_RIGHT } )
    {
        BOARD      board;
        PAD*       pad;
        PCB_SHAPE* seg;
        PCB_TEXT*  txt;
        PCB_POINT* pt;
        FOOTPRINT* fp = buildCoincidentFootprint( board, EDA_ANGLE( 30.0, DEGREES_T ), 2.0, 1.5,
                                                  VECTOR2I( 53000000, 51000000 ), pad, seg, txt, pt );

        const VECTOR2I padPos = pad->GetPosition();
        const VECTOR2I segStart = seg->GetStart();
        const VECTOR2I txtPos = txt->GetTextPos();
        const VECTOR2I ptPos = pt->GetPosition();

        fp->Flip( fp->GetPosition(), dir );
        fp->Flip( fp->GetPosition(), dir );

        BOOST_CHECK( pad->GetPosition() == padPos );
        BOOST_CHECK( seg->GetStart() == segStart );
        BOOST_CHECK( txt->GetTextPos() == txtPos );
        BOOST_CHECK( pt->GetPosition() == ptPos );
    }
}


// Stroke width and text size must survive save/reload under a footprint scale.
BOOST_AUTO_TEST_CASE( ScalarAttributesStableAcrossSaveLoadUnderScale )
{
    auto       board = std::make_unique<BOARD>();
    PAD*       pad;
    PCB_SHAPE* seg;
    PCB_TEXT*  txt;
    PCB_POINT* pt;
    FOOTPRINT* fp =
            buildCoincidentFootprint( *board, ANGLE_0, 2.0, 2.0, VECTOR2I( 53000000, 51000000 ), pad, seg, txt, pt );

    // Author board frame attributes while the 2x scale is active.
    seg->SetWidth( 400000 );
    txt->SetTextSize( VECTOR2I( 2000000, 1000000 ) );

    const int      segWidth = seg->GetStroke().GetWidth();
    const VECTOR2I txtSize = txt->GetTextSize();

    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "kicad_qa_scalar_roundtrip.kicad_pcb";
    KI_TEST::DumpBoardToFile( *board, tmp.string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.string() );
    std::error_code        ec;
    std::filesystem::remove( tmp, ec );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* r = reloaded->Footprints().front();
    PCB_SHAPE* rseg = nullptr;
    PCB_TEXT*  rtxt = nullptr;

    for( BOARD_ITEM* it : r->GraphicalItems() )
    {
        if( it->Type() == PCB_SHAPE_T )
            rseg = static_cast<PCB_SHAPE*>( it );
        else if( it->Type() == PCB_TEXT_T )
            rtxt = static_cast<PCB_TEXT*>( it );
    }

    BOOST_REQUIRE( rseg && rtxt );

    BOOST_CHECK_EQUAL( rseg->GetStroke().GetWidth(), segWidth );
    BOOST_CHECK_EQUAL( rtxt->GetTextSize().x, txtSize.x );
    BOOST_CHECK_EQUAL( rtxt->GetTextSize().y, txtSize.y );
}


// Ellipse radii must survive a rebake under non-uniform scale. Uniform is the control.
BOOST_AUTO_TEST_CASE( EllipseRadiusEditSurvivesRebakeUnderNonUniformScale )
{
    auto runCase = []( double aScaleX, double aScaleY )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        fp->SetPosition( VECTOR2I( 50000000, 50000000 ) );
        fp->SetTransformScale( aScaleX, aScaleY );
        board.Add( fp );

        PCB_SHAPE* ell = new PCB_SHAPE( fp, SHAPE_T::ELLIPSE );
        ell->SetLayer( F_SilkS );
        fp->Add( ell );

        ell->SetEllipseCenter( VECTOR2I( 50000000, 50000000 ) );
        ell->SetEllipseMajorRadius( 4000000 );
        ell->SetEllipseMinorRadius( 1500000 );

        const int majorBefore = ell->GetEllipseMajorRadius();
        const int minorBefore = ell->GetEllipseMinorRadius();

        // Any transform refresh rebakes children from the lib mirror.
        fp->SetPosition( fp->GetPosition() + VECTOR2I( 1000000, 0 ) );

        BOOST_CHECK_EQUAL( ell->GetEllipseMajorRadius(), majorBefore );
        BOOST_CHECK_EQUAL( ell->GetEllipseMinorRadius(), minorBefore );
    };

    runCase( 2.0, 2.0 ); // control: uniform scale keeps the radii
    runCase( 2.0, 1.0 ); // non-uniform: radii are lost on rebake
}


// The drawn glyph (used by rendering and DRC) must scale with the footprint.
BOOST_AUTO_TEST_CASE( ScaledTextGlyphShapeFollowsScale )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( 50000000, 50000000 ) );
    board.Add( fp );

    PCB_TEXT* txt = new PCB_TEXT( fp );
    txt->SetText( "H" );
    txt->SetTextPos( VECTOR2I( 50000000, 50000000 ) );
    txt->SetTextSize( VECTOR2I( 1000000, 1000000 ) );
    txt->SetLayer( F_SilkS );
    fp->Add( txt );

    const int maxError = pcbIUScale.mmToIU( 0.01 );

    SHAPE_POLY_SET glyph1x;
    txt->TransformTextToPolySet( glyph1x, 0, maxError, ERROR_INSIDE );
    const int height1x = glyph1x.BBox().GetHeight();

    fp->SetTransformScale( 2.0, 2.0 );

    SHAPE_POLY_SET glyph2x;
    txt->TransformTextToPolySet( glyph2x, 0, maxError, ERROR_INSIDE );
    const int height2x = glyph2x.BBox().GetHeight();

    BOOST_CHECK_GT( height2x, height1x * 3 / 2 );
}


// Text box size, thickness, and border stroke must survive save/reload under scale.
BOOST_AUTO_TEST_CASE( TextBoxScalarAttributesStableAcrossSaveLoadUnderScale )
{
    auto       board = std::make_unique<BOARD>();
    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    fp->SetPosition( VECTOR2I( 50000000, 50000000 ) );
    fp->SetTransformScale( 2.0, 2.0 );
    board->Add( fp );

    PCB_TEXTBOX* tb = new PCB_TEXTBOX( fp );
    tb->SetShape( SHAPE_T::RECTANGLE );
    tb->SetStart( VECTOR2I( 50000000, 50000000 ) );
    tb->SetEnd( VECTOR2I( 60000000, 55000000 ) );
    tb->SetText( wxT( "TB" ) );
    tb->SetLayer( F_SilkS );
    tb->SetTextSize( VECTOR2I( 2000000, 1000000 ) );
    tb->SetTextThickness( 300000 );
    tb->SetBorderEnabled( true );
    tb->SetWidth( 400000 );
    fp->Add( tb );

    const VECTOR2I txtSize = tb->GetTextSize();
    const int      txtThickness = tb->GetTextThickness();
    const int      borderWidth = tb->GetStroke().GetWidth();

    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "kicad_qa_textbox_roundtrip.kicad_pcb";
    KI_TEST::DumpBoardToFile( *board, tmp.string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.string() );
    std::error_code        ec;
    std::filesystem::remove( tmp, ec );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT*   fp2 = *reloaded->Footprints().begin();
    PCB_TEXTBOX* tb2 = nullptr;

    for( BOARD_ITEM* it : fp2->GraphicalItems() )
    {
        if( it->Type() == PCB_TEXTBOX_T )
            tb2 = static_cast<PCB_TEXTBOX*>( it );
    }

    BOOST_REQUIRE( tb2 );

    BOOST_CHECK_EQUAL( tb2->GetTextSize().x, txtSize.x );
    BOOST_CHECK_EQUAL( tb2->GetTextSize().y, txtSize.y );
    BOOST_CHECK_EQUAL( tb2->GetTextThickness(), txtThickness );
    BOOST_CHECK_EQUAL( tb2->GetStroke().GetWidth(), borderWidth );
}


// Build a 2x2 table. Call with the footprint at origin, unrotated, so the
// cell start/end land in lib coords.
static PCB_TABLE* buildLibTable( FOOTPRINT* aFp )
{
    PCB_TABLE* table = new PCB_TABLE( aFp, 0 );
    table->SetColCount( 2 );
    table->SetColWidth( 0, 16000000 );
    table->SetColWidth( 1, 18000000 );
    table->SetRowHeight( 0, 5000000 );
    table->SetRowHeight( 1, 5000000 );

    const VECTOR2I starts[4] = {
        { -4000000, 6000000 }, { 12000000, 6000000 }, { -4000000, 11000000 }, { 12000000, 11000000 }
    };
    const VECTOR2I ends[4] = {
        { 12000000, 11000000 }, { 30000000, 11000000 }, { 12000000, 16000000 }, { 30000000, 16000000 }
    };

    for( int ii = 0; ii < 4; ++ii )
    {
        PCB_TABLECELL* cell = new PCB_TABLECELL( table );
        cell->SetColSpan( 1 );
        cell->SetRowSpan( 1 );
        cell->SetText( wxT( "x" ) );
        cell->SetStart( starts[ii] );
        cell->SetEnd( ends[ii] );
        table->AddCell( cell );
    }

    return table;
}


// A 34x10 table fits in ~36mm. The bug blew it past 1 metre, so a loose bound catches it.
static void checkTableSane( PCB_TABLE* aTable, const VECTOR2I& aNearPos )
{
    BOX2I bbox = aTable->GetBoundingBox();

    BOOST_CHECK_MESSAGE( bbox.GetWidth() > 0 && bbox.GetWidth() < 50000000 && bbox.GetHeight() > 0
                                 && bbox.GetHeight() < 50000000,
                         "table bbox size out of range ( " << bbox.GetWidth() << ", " << bbox.GetHeight() << " )" );

    VECTOR2I center = bbox.GetCenter();
    BOOST_CHECK_MESSAGE( std::abs( center.x - aNearPos.x ) < 50000000 && std::abs( center.y - aNearPos.y ) < 50000000,
                         "table bbox center ( " << center.x << ", " << center.y << " ) far from footprint ( "
                                                << aNearPos.x << ", " << aNearPos.y << " )" );

    // Cells must stay a distinct grid, not collapse onto each other.
    BOOST_CHECK( aTable->GetCells()[0]->GetBoundingBox().GetCenter()
                 != aTable->GetCells()[3]->GetBoundingBox().GetCenter() );
}


// Rebaking a POLY cell must not transform its unseeded lib start/end, which used to overflow.
BOOST_AUTO_TEST_CASE( PolyTextBoxRebakeDoesNotOverflowOnStaleStartEnd )
{
    FOOTPRINT fp( nullptr );
    fp.SetPosition( VECTOR2I( 100000000, 60000000 ) );

    PCB_TEXTBOX* tb = new PCB_TEXTBOX( &fp );
    tb->SetShape( SHAPE_T::POLY );

    SHAPE_POLY_SET poly;
    poly.NewOutline();
    poly.Append( VECTOR2I( -4000000, 6000000 ) );
    poly.Append( VECTOR2I( 12000000, 6000000 ) );
    poly.Append( VECTOR2I( 12000000, 11000000 ) );
    poly.Append( VECTOR2I( -4000000, 11000000 ) );
    tb->SetPolyShape( poly );
    tb->OverrideLibPoly( poly );

    // Simulate the stale, unseeded lib start/end that loaded poly cells carry.
    tb->OverrideLibCoords( VECTOR2I( 2000000000, 2000000000 ), VECTOR2I( 2000000000, 2000000000 ) );
    fp.Add( tb, ADD_MODE::APPEND );

    // Rebake through the rotation. This used to convert the stale lib start/end and overflow.
    fp.SetOrientation( EDA_ANGLE( 33.0, DEGREES_T ) );

    // The polygon is rebaked to a sane, in-range size (16 x 5 mm rotated).
    const BOX2I polyBox = tb->GetPolyShape().BBox();
    BOOST_CHECK( polyBox.GetWidth() > 0 && polyBox.GetWidth() < 50000000 );
    BOOST_CHECK( polyBox.GetHeight() > 0 && polyBox.GetHeight() < 50000000 );
}


// A non-cardinally rotated table stays sane when the footprint is scaled in one axis.
BOOST_AUTO_TEST_CASE( RotatedTableSurvivesNonUniformScale )
{
    for( double angle : { 0.0, 90.0, 33.0, 217.5 } )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        board.Add( fp );
        fp->SetPosition( VECTOR2I( 0, 0 ) );

        PCB_TABLE* table = buildLibTable( fp );
        fp->Add( table, ADD_MODE::APPEND );

        fp->SetPosition( VECTOR2I( 101000000, 68000000 ) );
        fp->SetOrientation( EDA_ANGLE( angle, DEGREES_T ) );
        fp->SetTransformScale( 1.0, 2.0 );

        BOOST_TEST_CONTEXT( "orientation " << angle )
        checkTableSane( table, VECTOR2I( 101000000, 68000000 ) );
    }
}


BOOST_AUTO_TEST_CASE( TableInRotatedFootprintStaysSane )
{
    for( double angle : { 0.0, 90.0, 180.0, 270.0, 33.0, 217.5 } )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        board.Add( fp );
        fp->SetPosition( VECTOR2I( 0, 0 ) );

        PCB_TABLE* table = buildLibTable( fp );
        fp->Add( table, ADD_MODE::APPEND );

        fp->SetPosition( VECTOR2I( 101000000, 68000000 ) );
        fp->SetOrientation( EDA_ANGLE( angle, DEGREES_T ) );

        BOOST_TEST_CONTEXT( "orientation " << angle )
        checkTableSane( table, VECTOR2I( 101000000, 68000000 ) );
    }
}


// Non-cardinal rotation makes cells polygons, so compare the table bbox.
BOOST_FIXTURE_TEST_CASE( TableInNonCardinalRotatedFootprintSurvivesSaveLoad, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue18", m_board );

    FOOTPRINT* fp = *m_board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetPosition( VECTOR2I( 0, 0 ) );
    fp->SetOrientation( ANGLE_0 );

    PCB_TABLE* table = buildLibTable( fp );
    fp->Add( table, ADD_MODE::APPEND );

    fp->SetPosition( VECTOR2I( 12345678, -7654321 ) );
    fp->SetOrientation( EDA_ANGLE( 33.0, DEGREES_T ) );

    BOX2I bboxBefore = table->GetBoundingBox();

    const std::filesystem::path savePath =
            std::filesystem::temp_directory_path() / "table_in_rotated_scaled_fp.kicad_pcb";

    KI_TEST::DumpBoardToFile( *m_board, savePath.string() );
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    PCB_TABLE* table2 = nullptr;

    for( BOARD_ITEM* item : fp2->GraphicalItems() )
    {
        if( item->Type() == PCB_TABLE_T )
            table2 = static_cast<PCB_TABLE*>( item );
    }

    BOOST_REQUIRE( table2 );

    BOX2I bboxAfter = table2->GetBoundingBox();

    // Tolerance covers poly rounding at non-cardinal angles. The bug was off by tens of mm.
    const int tol = 300000;

    BOOST_CHECK_MESSAGE( std::abs( bboxAfter.GetOrigin().x - bboxBefore.GetOrigin().x ) <= tol
                                 && std::abs( bboxAfter.GetOrigin().y - bboxBefore.GetOrigin().y ) <= tol
                                 && std::abs( bboxAfter.GetWidth() - bboxBefore.GetWidth() ) <= tol
                                 && std::abs( bboxAfter.GetHeight() - bboxBefore.GetHeight() ) <= tol,
                         "table bbox after reload o( "
                                 << bboxAfter.GetOrigin().x << ", " << bboxAfter.GetOrigin().y << " ) sz( "
                                 << bboxAfter.GetWidth() << ", " << bboxAfter.GetHeight() << " ) expected o( "
                                 << bboxBefore.GetOrigin().x << ", " << bboxBefore.GetOrigin().y << " ) sz( "
                                 << bboxBefore.GetWidth() << ", " << bboxBefore.GetHeight() << " )" );
}


// Issue 24734: v10 tables in rotated footprints used to explode on load.
// Each table must land near its footprint with a bounded size.
BOOST_FIXTURE_TEST_CASE( LegacyEmbeddedTableInRotatedFootprintLoads, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "embedded_table_rotated_legacy_v10", m_board );

    int tableCount = 0;

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() != PCB_TABLE_T )
                continue;

            tableCount++;
            BOOST_TEST_CONTEXT( "footprint orientation " << fp->GetOrientation().AsDegrees() )
            checkTableSane( static_cast<PCB_TABLE*>( item ), fp->GetPosition() );
        }
    }

    BOOST_CHECK_EQUAL( tableCount, 5 );
}


// A footprint scale must scale the whole table, not just the cell text.
BOOST_AUTO_TEST_CASE( TableScalesWithFootprint )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    board.Add( fp );
    fp->SetPosition( VECTOR2I( 0, 0 ) );

    PCB_TABLE* table = buildLibTable( fp );
    fp->Add( table, ADD_MODE::APPEND );

    BOX2I before = table->GetBoundingBox();

    fp->SetTransformScale( 2.0, 1.5 );

    BOX2I after = table->GetBoundingBox();

    const double wRatio = (double) after.GetWidth() / before.GetWidth();
    const double hRatio = (double) after.GetHeight() / before.GetHeight();

    BOOST_CHECK_MESSAGE( wRatio > 1.9 && wRatio < 2.1, "table width ratio " << wRatio << " expected ~2.0" );
    BOOST_CHECK_MESSAGE( hRatio > 1.4 && hRatio < 1.6, "table height ratio " << hRatio << " expected ~1.5" );
}


// Scaling a footprint must scale a custom pad's primitives, not just the anchor.
BOOST_AUTO_TEST_CASE( CustomPadShapeScalesWithFootprint )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    board.Add( fp );
    fp->SetPosition( VECTOR2I( 0, 0 ) );

    PAD* pad = new PAD( fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
    pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 500000, 500000 ) );
    pad->SetLayerSet( PAD::SMDMask() );

    std::vector<VECTOR2I> poly = {
        { -1000000, -1000000 }, { 1000000, -1000000 }, { 1000000, 1000000 }, { -1000000, 1000000 }
    };
    pad->AddPrimitivePoly( PADSTACK::ALL_LAYERS, poly, 0, true );
    fp->Add( pad, ADD_MODE::APPEND );

    BOX2I before = pad->GetBoundingBox();

    fp->SetTransformScale( 2.0, 1.5 );

    BOX2I after = pad->GetBoundingBox();

    const double wRatio = (double) after.GetWidth() / before.GetWidth();
    const double hRatio = (double) after.GetHeight() / before.GetHeight();

    BOOST_CHECK_MESSAGE( wRatio > 1.9 && wRatio < 2.1, "custom pad width ratio " << wRatio << " expected ~2.0" );
    BOOST_CHECK_MESSAGE( hRatio > 1.4 && hRatio < 1.6, "custom pad height ratio " << hRatio << " expected ~1.5" );
}


BOOST_AUTO_TEST_SUITE_END()
