/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/geometry/geometry.h>

#include <utility>

#include <board.h>
#include <core/kicad_algo.h>
#include <embedded_files.h>
#include <geometry/shape_utils.h>
#include <footprint.h>
#include <mmh3_hash.h>
#include <pcb_field.h>
#include <pcb_shape.h>


static bool CourtyardEqualPredicate( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b )
{
    // The courtyards get a tiny epsilon to handle polygonisaton errors
    const int courtyardEpsilon = pcbIUScale.mmToIU( 0.005 );

    if( a.OutlineCount() != b.OutlineCount() )
        return false;

    // We can only use this predicate on single-outline polys unless we do more work.
    // Because we don't know the sub-outlines are in the same order.
    BOOST_REQUIRE( a.OutlineCount() == 1 );

    return KI_TEST::ChainsAreCyclicallyEqual( a.Outline( 0 ), b.Outline( 0 ), courtyardEpsilon );
};


BOOST_AUTO_TEST_SUITE( Footprint )


BOOST_AUTO_TEST_CASE( FootprintCourtyardAndHull )
{
    // Footprint courtyards are cached internally. Some operations manipulate the
    // cache efficiently, some rebuild it. In any case, the courtyard should always
    // be consistent with the geometry of the footprint. Same for the bounding hull.

    BOARD     board;
    FOOTPRINT fp( &board );

    const int lineW = pcbIUScale.mmToIU( 0.05 );
    const int courtyardH = pcbIUScale.mmToIU( 1.0 );
    const int courtyardW = pcbIUScale.mmToIU( 2.0 );

    const std::vector<VECTOR2I> courtyardPoints = {
        { 0, 0 },
        { courtyardW, 0 },
        { courtyardW, courtyardH },
        { 0, courtyardH },
    };

    const auto assertCourtyardMatches = [&]( PCB_LAYER_ID layer, const SHAPE_LINE_CHAIN& aExpectedCourtyard )
    {
        const SHAPE_POLY_SET& courtyard = fp.GetCourtyard( layer );

        BOOST_REQUIRE( courtyard.OutlineCount() == 1 );
        BOOST_CHECK_PREDICATE( CourtyardEqualPredicate, ( courtyard.Outline( 0 ) )( aExpectedCourtyard ) );
    };

    const auto assertNoCourtyard = [&]( PCB_LAYER_ID layer )
    {
        const SHAPE_POLY_SET& courtyard = fp.GetCourtyard( layer );

        BOOST_TEST( courtyard.OutlineCount() == 0 );
    };

    const auto assertHullMatch = [&]( const SHAPE_LINE_CHAIN& aExpectedHull )
    {
        const SHAPE_POLY_SET& hull = fp.GetBoundingHull();

        BOOST_REQUIRE( hull.OutlineCount() == 1 );
        BOOST_CHECK_PREDICATE( KI_TEST::ChainsAreCyclicallyEqual, ( hull.Outline( 0 ) )(aExpectedHull) ( 0 ) );
    };

    {
        std::unique_ptr<PCB_SHAPE> courtyardPoly = std::make_unique<PCB_SHAPE>( &fp, SHAPE_T::POLY );
        courtyardPoly->SetLayer( F_CrtYd );
        courtyardPoly->SetPolyPoints( courtyardPoints );
        courtyardPoly->SetWidth( lineW );

        fp.Add( courtyardPoly.release() );
    }

    // We'll modify this in lock-step with the footprint
    SHAPE_LINE_CHAIN expectedCourtyard( courtyardPoints, true );
    // The hull is hard to calculate - we'll take the initial one as a given
    SHAPE_LINE_CHAIN expectedHull = fp.GetBoundingHull().Outline( 0 );

    BOOST_TEST_CONTEXT( "Initial courtyard" )
    {
        assertCourtyardMatches( F_CrtYd, expectedCourtyard );
        assertNoCourtyard( B_CrtYd );
    }

    const VECTOR2I moveVector = VECTOR2I( courtyardW, 0 );

    fp.Move( moveVector );
    expectedCourtyard.Move( moveVector );
    expectedHull.Move( moveVector );

    BOOST_TEST_CONTEXT( "Moved courtyard" )
    {
        assertCourtyardMatches( F_CrtYd, expectedCourtyard );
        assertNoCourtyard( B_CrtYd );
        assertHullMatch( expectedHull );
    }

    fp.Rotate( VECTOR2I( 0, 0 ), EDA_ANGLE( 90.0 ) );
    expectedCourtyard.Rotate( EDA_ANGLE( 90.0 ), VECTOR2I( 0, 0 ) );
    expectedHull.Rotate( EDA_ANGLE( 90.0 ), VECTOR2I( 0, 0 ) );

    BOOST_TEST_CONTEXT( "Rotated courtyard" )
    {
        assertCourtyardMatches( F_CrtYd, expectedCourtyard );
        assertNoCourtyard( B_CrtYd );
        assertHullMatch( expectedHull );
    }

    fp.Flip( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );
    expectedCourtyard.Mirror( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );
    expectedHull.Mirror( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );

    const BOX2I flippedExpectedBox = BOX2I::ByCorners( VECTOR2I( -courtyardW, 0 ), VECTOR2I( 0, courtyardH ) );

    BOOST_TEST_CONTEXT( "Flipped courtyard" )
    {
        assertCourtyardMatches( B_CrtYd, expectedCourtyard );
        assertNoCourtyard( F_CrtYd );
        assertHullMatch( expectedHull );
    }
}


// Regression test for GitLab issue #24345.  Cloning a footprint with embedded files (3D models,
// fonts, etc.) must not duplicate the embedded payloads.  BOARD_NETLIST_UPDATER clones each
// footprint up to four times per "Update PCB from schematic" pass, so deep-copying embedded
// data per clone blows up memory on boards with hundreds of footprints carrying large
// (multi-megabyte) embedded 3D models.
BOOST_AUTO_TEST_CASE( FootprintCloneSharesEmbeddedFiles )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    auto* file = new EMBEDDED_FILES::EMBEDDED_FILE();
    file->name = wxS( "model.step" );
    file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::MODEL;

    // Keep the test payload small so the suite stays fast, but use enough data to exercise
    // the compression/encode path.
    std::string payload( 4096, 'k' );
    file->decompressedData.assign( payload.begin(), payload.end() );

    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.add( file->decompressedData );
    file->data_hash = hash.digest().ToString();

    BOOST_REQUIRE( EMBEDDED_FILES::CompressAndEncode( *file ) == EMBEDDED_FILES::RETURN_CODE::OK );

    fp.AddFile( file );

    EMBEDDED_FILES::EMBEDDED_FILE* originalFile = fp.GetEmbeddedFile( wxS( "model.step" ) );
    BOOST_REQUIRE( originalFile );

    // Clone the footprint several times, mimicking the BOARD_NETLIST_UPDATER undo-snapshot
    // pattern.  Each clone must reference the same payload, not allocate a fresh copy.
    std::vector<std::unique_ptr<FOOTPRINT>> clones;

    for( int ii = 0; ii < 4; ++ii )
    {
        clones.emplace_back( static_cast<FOOTPRINT*>( fp.Clone() ) );
        EMBEDDED_FILES::EMBEDDED_FILE* cloneFile =
                clones.back()->GetEmbeddedFile( wxS( "model.step" ) );
        BOOST_REQUIRE( cloneFile );
        BOOST_CHECK_EQUAL( cloneFile, originalFile );
    }

    // Releasing all clones must leave the source footprint's embedded file intact.
    clones.clear();
    BOOST_CHECK( fp.HasFile( wxS( "model.step" ) ) );
    BOOST_CHECK_EQUAL( fp.GetEmbeddedFile( wxS( "model.step" ) ), originalFile );
}


// Moving a footprint leaves the source with no fields at all, and the damage bounding box is
// computed for footprints in that state
BOOST_AUTO_TEST_CASE( FootprintBoundingBoxWithoutMandatoryFields )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    const int shapeSize = pcbIUScale.mmToIU( 3.0 );

    {
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &fp, SHAPE_T::RECTANGLE );
        shape->SetLayer( F_Cu );
        shape->SetStart( VECTOR2I( 0, 0 ) );
        shape->SetEnd( VECTOR2I( shapeSize, shapeSize ) );
        shape->SetWidth( pcbIUScale.mmToIU( 0.1 ) );

        fp.Add( shape.release() );
    }

    // Keep the annotations out of the bounding box so that dropping them cannot change it for
    // a legitimate reason.
    for( FIELD_T id : { FIELD_T::REFERENCE, FIELD_T::VALUE } )
        fp.GetField( id )->SetVisible( false );

    const BOX2I withFields = fp.GetBoundingBox();

    for( FIELD_T id : { FIELD_T::REFERENCE, FIELD_T::VALUE } )
    {
        std::unique_ptr<PCB_FIELD> field( fp.GetField( id ) );
        BOOST_REQUIRE( field );
        fp.Remove( field.get() );
    }

    const FOOTPRINT& constFp = fp;
    BOOST_REQUIRE( !constFp.GetField( FIELD_T::REFERENCE ) );
    BOOST_REQUIRE( !constFp.GetField( FIELD_T::VALUE ) );

    BOOST_CHECK( fp.GetBoundingBox() == withFields );

    // Text variables resolve against the same fields, and are read while the footprint is in
    // this state to draw it.
    wxString token = wxS( "VALUE" );
    BOOST_CHECK( fp.ResolveTextVar( &token ) );
    BOOST_CHECK( token.IsEmpty() );
}


// The properties dialogs write the whole grid back at once, and the mandatory fields have to
// come through it as the same objects
BOOST_AUTO_TEST_CASE( FootprintUpdateFieldsReusesMandatoryFields )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );

    board.Add( fp );

    fp->GetField( FIELD_T::VALUE )->SetText( wxS( "old value" ) );

    PCB_FIELD* userField = new PCB_FIELD( fp, FIELD_T::USER, wxS( "MPN" ) );
    userField->SetText( wxS( "PESD5V0S1BL" ) );
    fp->Add( userField );

    PCB_FIELD*  reference = fp->GetField( FIELD_T::REFERENCE );
    PCB_FIELD*  value = fp->GetField( FIELD_T::VALUE );
    const KIID  referenceId = reference->m_Uuid;
    const KIID  userFieldId = userField->m_Uuid;

    // What the grid hands back: the mandatory fields edited, the user field dropped and a
    // different one added
    std::vector<PCB_FIELD> newFields;

    for( FIELD_T id : { FIELD_T::REFERENCE, FIELD_T::VALUE, FIELD_T::DATASHEET,
                        FIELD_T::DESCRIPTION } )
    {
        newFields.push_back( *fp->GetField( id ) );
    }

    newFields[1].SetText( wxS( "new value" ) );
    newFields.emplace_back( fp, FIELD_T::USER, wxS( "LCSC" ) );

    std::vector<PCB_FIELD*> added;
    std::vector<PCB_FIELD*> detached;

    fp->UpdateFields( newFields, added, detached );

    BOOST_CHECK( fp->GetField( FIELD_T::REFERENCE ) == reference );
    BOOST_CHECK( fp->GetField( FIELD_T::VALUE ) == value );
    BOOST_CHECK_EQUAL( value->GetText(), wxS( "new value" ) );
    BOOST_CHECK( value->GetParent() == fp );

    BOOST_REQUIRE_EQUAL( added.size(), 1 );
    BOOST_CHECK_EQUAL( added.front()->GetName(), wxS( "LCSC" ) );
    BOOST_CHECK( added.front()->GetParent() == fp );

    BOOST_REQUIRE_EQUAL( detached.size(), 1 );
    BOOST_CHECK( detached.front() == userField );

    BOOST_CHECK_EQUAL( fp->GetFields().size(), newFields.size() );
    BOOST_CHECK( !alg::contains( fp->GetFields(), userField ) );

    // The board indexes items by KIID, so it has to learn about the new field and forget the
    // dropped one.  ResolveItem falls back to a linear scan, so the index itself is what tells
    // us the new field was registered.
    BOOST_CHECK( board.ResolveItem( referenceId, true ) == reference );
    BOOST_CHECK( added.front()->IsIndexedInBoard() );
    BOOST_CHECK( board.ResolveItem( userFieldId, true ) == nullptr );

    for( PCB_FIELD* field : detached )
        delete field;
}


// A grid write that only edits the mandatory fields adds and removes nothing, so nothing else
// bumps the board timestamp the bounding box cache is keyed on
BOOST_AUTO_TEST_CASE( FootprintUpdateFieldsInvalidatesGeometryCache )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );

    board.Add( fp );

    fp->GetField( FIELD_T::VALUE )->SetText( wxS( "V" ) );

    const BOX2I before = fp->GetBoundingBox();

    std::vector<PCB_FIELD> newFields;

    for( PCB_FIELD* field : fp->GetFields() )
        newFields.push_back( *field );

    for( PCB_FIELD& field : newFields )
    {
        if( field.GetId() == FIELD_T::VALUE )
            field.SetPosition( VECTOR2I( pcbIUScale.mmToIU( 50.0 ), 0 ) );
    }

    std::vector<PCB_FIELD*> added;
    std::vector<PCB_FIELD*> detached;

    fp->UpdateFields( newFields, added, detached );

    BOOST_REQUIRE( added.empty() );
    BOOST_REQUIRE( detached.empty() );
    BOOST_CHECK( fp->GetBoundingBox() != before );
}


// A footprint that has already lost a mandatory field gets it back as a new object, which the
// caller has to hand to the view itself
BOOST_AUTO_TEST_CASE( FootprintUpdateFieldsRestoresMissingMandatoryField )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    std::vector<PCB_FIELD> newFields;

    for( FIELD_T id : { FIELD_T::REFERENCE, FIELD_T::VALUE, FIELD_T::DATASHEET,
                        FIELD_T::DESCRIPTION } )
    {
        newFields.push_back( *fp.GetField( id ) );
    }

    std::unique_ptr<PCB_FIELD> orphan( fp.GetField( FIELD_T::VALUE ) );
    fp.Remove( orphan.get() );
    BOOST_REQUIRE( !std::as_const( fp ).GetField( FIELD_T::VALUE ) );

    std::vector<PCB_FIELD*> added;
    std::vector<PCB_FIELD*> detached;

    fp.UpdateFields( newFields, added, detached );

    BOOST_CHECK( detached.empty() );
    BOOST_REQUIRE_EQUAL( added.size(), 1 );
    BOOST_CHECK( added.front() == std::as_const( fp ).GetField( FIELD_T::VALUE ) );
    BOOST_CHECK( added.front() != orphan.get() );
}


BOOST_AUTO_TEST_SUITE_END()
