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

#include <map>
#include <memory>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <constraints/constraint_copy.h>
#include <constraints/pcb_constraint.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;


BOOST_AUTO_TEST_SUITE( ConstraintSolverDuplicate )


namespace
{
/// Clone a shape with a fresh UUID mimicking a user duplicate
PCB_SHAPE* duplicateShape( BOARD& aBoard, PCB_SHAPE* aOrig )
{
    PCB_SHAPE* dup = static_cast<PCB_SHAPE*>( aOrig->Clone() );
    dup->ResetUuid();
    aBoard.Add( dup );
    return dup;
}


const PCB_CONSTRAINT* findByType( const std::vector<PCB_CONSTRAINT*>& aClones, PCB_CONSTRAINT_TYPE aType )
{
    for( const PCB_CONSTRAINT* c : aClones )
    {
        if( c->GetConstraintType() == aType )
            return c;
    }

    return nullptr;
}
} // namespace


// Duplicate carries constraints fully inside the set re pointed at the copies
// a constraint touching an outside item is dropped
BOOST_AUTO_TEST_CASE( DuplicateCarriesFullyContainedConstraints )
{
    auto board = std::make_unique<BOARD>();

    PCB_SHAPE* seg1 = addSegment( *board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* seg2 = addSegment( *board, { 10 * MM, 0 }, { 10 * MM, 10 * MM } );
    PCB_SHAPE* arc = addArc( *board, { 0, 5 * MM }, { 2 * MM, 7 * MM }, { 4 * MM, 5 * MM } );

    // seg3 stays behind, so the constraint touching it must not travel with the duplicate.
    PCB_SHAPE* seg3 = addSegment( *board, { 0, 20 * MM }, { 10 * MM, 20 * MM } );

    // A coincident between the touching endpoints of the two duplicated segments.
    PCB_CONSTRAINT* coincident = addConstraint(
            *board, PCB_CONSTRAINT_TYPE::COINCIDENT,
            { { seg1->m_Uuid, CONSTRAINT_ANCHOR::END }, { seg2->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    // An internal constraint on the arc alone.
    PCB_CONSTRAINT* arcAngle =
            addConstraint( *board, PCB_CONSTRAINT_TYPE::ARC_ANGLE,
                           { { arc->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 90.0 );
    arcAngle->SetDriving( true );

    // A constraint reaching outside the duplicated set; it must be dropped.
    addConstraint( *board, PCB_CONSTRAINT_TYPE::PARALLEL,
                   { { seg1->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { seg3->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    PCB_SHAPE* dupSeg1 = duplicateShape( *board, seg1 );
    PCB_SHAPE* dupSeg2 = duplicateShape( *board, seg2 );
    PCB_SHAPE* dupArc = duplicateShape( *board, arc );

    std::map<KIID, KIID> idMap;
    idMap[seg1->m_Uuid] = dupSeg1->m_Uuid;
    idMap[seg2->m_Uuid] = dupSeg2->m_Uuid;
    idMap[arc->m_Uuid] = dupArc->m_Uuid;

    std::vector<PCB_CONSTRAINT*> clones = CloneFullySelectedConstraints( board->Constraints(), idMap );

    // Only the coincident and the arc-angle are fully contained; the parallel is dropped.
    BOOST_REQUIRE_EQUAL( clones.size(), 2 );
    BOOST_CHECK( findByType( clones, PCB_CONSTRAINT_TYPE::PARALLEL ) == nullptr );

    const PCB_CONSTRAINT* coincidentClone = findByType( clones, PCB_CONSTRAINT_TYPE::COINCIDENT );
    BOOST_REQUIRE( coincidentClone );
    BOOST_REQUIRE_EQUAL( coincidentClone->GetMembers().size(), 2 );

    // Members point at the duplicates, anchors preserved, and it is a distinct object.
    BOOST_CHECK( coincidentClone->m_Uuid != coincident->m_Uuid );
    BOOST_CHECK( coincidentClone->GetMembers()[0].m_item == dupSeg1->m_Uuid );
    BOOST_CHECK( coincidentClone->GetMembers()[0].m_anchor == CONSTRAINT_ANCHOR::END );
    BOOST_CHECK( coincidentClone->GetMembers()[1].m_item == dupSeg2->m_Uuid );
    BOOST_CHECK( coincidentClone->GetMembers()[1].m_anchor == CONSTRAINT_ANCHOR::START );

    const PCB_CONSTRAINT* arcAngleClone = findByType( clones, PCB_CONSTRAINT_TYPE::ARC_ANGLE );
    BOOST_REQUIRE( arcAngleClone );
    BOOST_REQUIRE_EQUAL( arcAngleClone->GetMembers().size(), 1 );

    BOOST_CHECK( arcAngleClone->m_Uuid != arcAngle->m_Uuid );
    BOOST_CHECK( arcAngleClone->GetMembers()[0].m_item == dupArc->m_Uuid );
    BOOST_CHECK( arcAngleClone->IsDriving() );
    BOOST_REQUIRE( arcAngleClone->GetValue().has_value() );
    BOOST_CHECK_CLOSE( arcAngleClone->GetValue().value(), 90.0, 1e-9 );

    for( PCB_CONSTRAINT* clone : clones )
        delete clone;
}


// A constraint entirely outside the set yields no clone even if the set is non empty
// map keys alone define the scope
BOOST_AUTO_TEST_CASE( DuplicateDropsConstraintOutsideScope )
{
    auto board = std::make_unique<BOARD>();

    PCB_SHAPE* seg1 = addSegment( *board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* seg2 = addSegment( *board, { 0, 5 * MM }, { 10 * MM, 5 * MM } );

    addConstraint( *board, PCB_CONSTRAINT_TYPE::PARALLEL,
                   { { seg1->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { seg2->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    // Duplicate an unrelated segment only; the parallel touches neither duplicated item.
    PCB_SHAPE* other = addSegment( *board, { 0, 30 * MM }, { 10 * MM, 30 * MM } );
    PCB_SHAPE* dupOther = duplicateShape( *board, other );

    std::map<KIID, KIID> idMap;
    idMap[other->m_Uuid] = dupOther->m_Uuid;

    std::vector<PCB_CONSTRAINT*> clones = CloneFullySelectedConstraints( board->Constraints(), idMap );

    BOOST_CHECK( clones.empty() );
}


// Group duplicate must pair each member with its own copy so constraints land on matching duplicates
// members iterate unordered so DeepDuplicate captures the mapping while cloning via its KIID map
BOOST_AUTO_TEST_CASE( GroupDuplicatePairsMembersForConstraintRemap )
{
    auto board = std::make_unique<BOARD>();

    // Distinct geometry so a swapped pairing is detectable.
    PCB_SHAPE* seg1 = addSegment( *board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* seg2 = addSegment( *board, { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    PCB_GROUP* group = new PCB_GROUP( board.get() );
    group->AddItem( seg1 );
    group->AddItem( seg2 );
    board->Add( group );

    addConstraint( *board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { seg1->m_Uuid, CONSTRAINT_ANCHOR::END }, { seg2->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    std::map<KIID, KIID> idMap;
    PCB_GROUP* dupGroup = group->DeepDuplicate( IGNORE_PARENT_GROUP, nullptr, &idMap );

    // Resolve each original to its paired duplicate and check geometry matches
    // fails if the two groups are walked in uncorrelated order
    auto dupeByOrig =
            [&]( const PCB_SHAPE* aOrig ) -> const PCB_SHAPE*
            {
                KIID dupId = idMap.at( aOrig->m_Uuid );

                for( EDA_ITEM* member : dupGroup->GetItems() )
                {
                    if( member->m_Uuid == dupId )
                        return static_cast<const PCB_SHAPE*>( member );
                }

                return nullptr;
            };

    const PCB_SHAPE* dupSeg1 = dupeByOrig( seg1 );
    const PCB_SHAPE* dupSeg2 = dupeByOrig( seg2 );

    BOOST_REQUIRE( dupSeg1 );
    BOOST_REQUIRE( dupSeg2 );
    BOOST_CHECK( dupSeg1->GetStart() == seg1->GetStart() && dupSeg1->GetEnd() == seg1->GetEnd() );
    BOOST_CHECK( dupSeg2->GetStart() == seg2->GetStart() && dupSeg2->GetEnd() == seg2->GetEnd() );

    std::vector<PCB_CONSTRAINT*> clones = CloneFullySelectedConstraints( board->Constraints(), idMap );

    BOOST_REQUIRE_EQUAL( clones.size(), 1 );

    const PCB_CONSTRAINT* coincidentClone = clones.front();
    BOOST_REQUIRE_EQUAL( coincidentClone->GetMembers().size(), 2 );
    BOOST_CHECK( coincidentClone->GetMembers()[0].m_item == dupSeg1->m_Uuid );
    BOOST_CHECK( coincidentClone->GetMembers()[1].m_item == dupSeg2->m_Uuid );

    for( PCB_CONSTRAINT* clone : clones )
        delete clone;

    // DeepDuplicate does not add its copies to the board, so the test owns them.
    for( EDA_ITEM* member : dupGroup->GetItems() )
        delete member;

    delete dupGroup;
}


// A footprint carries its own internal constraints and Duplicate resets every child UUID
// clone constraints must re point to the duplicate children not the original footprint
BOOST_AUTO_TEST_CASE( FootprintDuplicateRepointsInternalConstraints )
{
    auto board = std::make_unique<BOARD>();

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    board->Add( fp );

    auto addFpSegment =
            [&]( const VECTOR2I& aStart, const VECTOR2I& aEnd ) -> PCB_SHAPE*
            {
                PCB_SHAPE* seg = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
                seg->SetStart( aStart );
                seg->SetEnd( aEnd );
                fp->Add( seg );
                return seg;
            };

    PCB_SHAPE* a = addFpSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addFpSegment( { 0, 5 * MM }, { 10 * MM, 6 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( fp, PCB_CONSTRAINT_TYPE::PARALLEL );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    c->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    fp->Add( c );

    std::unique_ptr<FOOTPRINT> dupe( static_cast<FOOTPRINT*>( fp->Duplicate( IGNORE_PARENT_GROUP ) ) );

    BOOST_REQUIRE_EQUAL( dupe->Constraints().size(), 1 );

    std::set<KIID> dupeChildIds;
    dupe->RunOnChildren( [&]( BOARD_ITEM* aChild ) { dupeChildIds.insert( aChild->m_Uuid ); },
                         RECURSE_MODE::RECURSE );

    const PCB_CONSTRAINT* dupeConstraint = dupe->Constraints().front();
    BOOST_REQUIRE_EQUAL( dupeConstraint->GetMembers().size(), 2 );

    for( const CONSTRAINT_MEMBER& member : dupeConstraint->GetMembers() )
    {
        // The member must resolve inside the duplicate, never back to the original footprint.
        BOOST_CHECK( dupeChildIds.count( member.m_item ) > 0 );
        BOOST_CHECK( member.m_item != a->m_Uuid );
        BOOST_CHECK( member.m_item != b->m_Uuid );
    }
}


BOOST_AUTO_TEST_SUITE_END()
