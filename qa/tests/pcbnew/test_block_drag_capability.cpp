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

#include <algorithm>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_field.h>
#include <pcb_track.h>
#include <drc/drc_item.h>
#include <pcb_marker.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_textbox.h>
#include <pcb_dimension.h>
#include <pcb_target.h>
#include <pcb_group.h>
#include <zone.h>
#include <pcb_generator.h>
#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>
#include <tools/block_drag_capability.h>


namespace
{

class TEST_GENERATOR : public PCB_GENERATOR
{
public:
    TEST_GENERATOR( BOARD_ITEM* aParent, const wxString& aGeneratorType ) :
            PCB_GENERATOR( aParent, F_Cu ),
            m_testGeneratorType( aGeneratorType )
    {
    }

    wxString GetGeneratorType() const override { return m_testGeneratorType; }

    void EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override {}
    bool Update( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override { return true; }
    void EditFinish( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override {}
    void EditCancel( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override {}
    void Remove( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override {}

    wxString GetPluralName() const override { return wxT( "Test Generators" ); }
    wxString GetCommitMessage() const override { return wxT( "Test Generator" ); }

private:
    wxString m_testGeneratorType;
};

} // namespace


BOOST_AUTO_TEST_SUITE( BlockDragCapabilityTests )


BOOST_AUTO_TEST_CASE( RouterItemsInBoardEditor )
{
    BOARD board;

    PCB_TRACK* track = new PCB_TRACK( &board );
    PCB_ARC*   arc = new PCB_ARC( &board );
    PCB_VIA*   via = new PCB_VIA( &board );
    FOOTPRINT* footprint = new FOOTPRINT( &board );
    PAD*       pad = new PAD( footprint );

    footprint->Add( pad );
    board.Add( track );
    board.Add( arc );
    board.Add( via );
    board.Add( footprint );

    BOOST_CHECK( BlockDragCapability( *track ) == BLOCK_DRAG_CAPABILITY::ROUTER );
    BOOST_CHECK( BlockDragCapability( *arc ) == BLOCK_DRAG_CAPABILITY::ROUTER );
    BOOST_CHECK( BlockDragCapability( *via ) == BLOCK_DRAG_CAPABILITY::ROUTER );
    BOOST_CHECK( BlockDragCapability( *footprint ) == BLOCK_DRAG_CAPABILITY::ROUTER );
    BOOST_CHECK( BlockDragCapability( *pad ) == BLOCK_DRAG_CAPABILITY::ROUTER );
}


BOOST_AUTO_TEST_CASE( FootprintHolderHasNoRouter )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    FOOTPRINT* footprint = new FOOTPRINT( &board );
    PAD*       pad = new PAD( footprint );
    PCB_TRACK* track = new PCB_TRACK( &board );

    footprint->Add( pad );
    board.Add( footprint );
    board.Add( track );

    BOOST_CHECK( BlockDragCapability( *pad ) == BLOCK_DRAG_CAPABILITY::RIGID );
    BOOST_CHECK( BlockDragCapability( *footprint ) == BLOCK_DRAG_CAPABILITY::UNSUPPORTED );
    BOOST_CHECK( BlockDragCapability( *track ) == BLOCK_DRAG_CAPABILITY::UNSUPPORTED );
}


BOOST_AUTO_TEST_CASE( ShapeKindsSplitDeformableFromRigid )
{
    BOARD board;

    PCB_SHAPE* segment = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_SHAPE* poly = new PCB_SHAPE( &board, SHAPE_T::POLY );
    PCB_SHAPE* rect = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
    PCB_SHAPE* arc = new PCB_SHAPE( &board, SHAPE_T::ARC );
    PCB_SHAPE* circle = new PCB_SHAPE( &board, SHAPE_T::CIRCLE );
    PCB_SHAPE* bezier = new PCB_SHAPE( &board, SHAPE_T::BEZIER );
    PCB_SHAPE* ellipse = new PCB_SHAPE( &board, SHAPE_T::ELLIPSE );
    PCB_SHAPE* ellipseArc = new PCB_SHAPE( &board, SHAPE_T::ELLIPSE_ARC );

    for( PCB_SHAPE* shape : { segment, poly, rect, arc, circle, bezier, ellipse, ellipseArc } )
        board.Add( shape );

    BOOST_CHECK( BlockDragCapability( *segment ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
    BOOST_CHECK( BlockDragCapability( *poly ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
    BOOST_CHECK( BlockDragCapability( *rect ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
    BOOST_CHECK( BlockDragCapability( *arc ) == BLOCK_DRAG_CAPABILITY::RIGID );
    BOOST_CHECK( BlockDragCapability( *circle ) == BLOCK_DRAG_CAPABILITY::RIGID );
    BOOST_CHECK( BlockDragCapability( *bezier ) == BLOCK_DRAG_CAPABILITY::RIGID );
    BOOST_CHECK( BlockDragCapability( *ellipse ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
    BOOST_CHECK( BlockDragCapability( *ellipseArc ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
}


BOOST_AUTO_TEST_CASE( ConstrainedShapeIsDeformable )
{
    BOARD board;

    PCB_SHAPE* constrained = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_SHAPE* free = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );

    board.Add( constrained );
    board.Add( free );

    PCB_CONSTRAINT* constraint = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::HORIZONTAL );
    constraint->AddMember( constrained->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( constraint );

    BOOST_CHECK( BlockDragIsConstraintMember( *constrained ) );
    BOOST_CHECK( !BlockDragIsConstraintMember( *free ) );
    BOOST_CHECK( BlockDragCapability( *constrained ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
    BOOST_CHECK( BlockDragCapability( *free ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
}


BOOST_AUTO_TEST_CASE( FootprintOwnedConstraintIsFound )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    FOOTPRINT* footprint = new FOOTPRINT( &board );
    PCB_SHAPE* shape = new PCB_SHAPE( footprint, SHAPE_T::SEGMENT );

    footprint->Add( shape );
    board.Add( footprint );

    PCB_CONSTRAINT* constraint = new PCB_CONSTRAINT( footprint, PCB_CONSTRAINT_TYPE::VERTICAL );
    constraint->AddMember( shape->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    footprint->Add( constraint );

    BOOST_CHECK( BlockDragIsConstraintMember( *shape ) );
    BOOST_CHECK( BlockDragCapability( *shape ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
}


BOOST_AUTO_TEST_CASE( FootprintChildShapeIsRigidInBoardEditor )
{
    BOARD board;

    FOOTPRINT* footprint = new FOOTPRINT( &board );
    PCB_SHAPE* childShape = new PCB_SHAPE( footprint, SHAPE_T::SEGMENT );

    footprint->Add( childShape );
    board.Add( footprint );

    BOOST_CHECK( BlockDragCapability( *childShape ) == BLOCK_DRAG_CAPABILITY::RIGID );
}


BOOST_AUTO_TEST_CASE( FootprintChildrenAreDeformableInFootprintEditor )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    FOOTPRINT* footprint = new FOOTPRINT( &board );
    PCB_SHAPE* childShape = new PCB_SHAPE( footprint, SHAPE_T::POLY );
    ZONE*      childZone = new ZONE( footprint );

    footprint->Add( childShape );
    footprint->Add( childZone );
    board.Add( footprint );

    BOOST_CHECK( BlockDragCapability( *childShape ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
    BOOST_CHECK( BlockDragCapability( *childZone ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
}


BOOST_AUTO_TEST_CASE( FootprintChildZoneIsRigidInBoardEditor )
{
    BOARD board;

    FOOTPRINT* footprint = new FOOTPRINT( &board );
    ZONE*      childZone = new ZONE( footprint );

    footprint->Add( childZone );
    board.Add( footprint );

    BOOST_CHECK( BlockDragCapability( *childZone ) == BLOCK_DRAG_CAPABILITY::RIGID );
}


BOOST_AUTO_TEST_CASE( RigidOnlyTypes )
{
    BOARD board;

    PCB_TEXT*        text = new PCB_TEXT( &board );
    PCB_TEXTBOX*     textbox = new PCB_TEXTBOX( &board );
    PCB_DIM_ALIGNED* dimension = new PCB_DIM_ALIGNED( &board );
    PCB_TARGET*      target = new PCB_TARGET( &board );
    PCB_TABLE*       table = new PCB_TABLE( &board, 0 );
    PCB_TABLECELL*   cell = new PCB_TABLECELL( table );

    board.Add( text );
    board.Add( textbox );
    board.Add( dimension );
    board.Add( target );
    board.Add( table );

    BOOST_CHECK( BlockDragCapability( *text ) == BLOCK_DRAG_CAPABILITY::RIGID );
    BOOST_CHECK( BlockDragCapability( *textbox ) == BLOCK_DRAG_CAPABILITY::RIGID );
    BOOST_CHECK( BlockDragCapability( *dimension ) == BLOCK_DRAG_CAPABILITY::RIGID );
    BOOST_CHECK( BlockDragCapability( *target ) == BLOCK_DRAG_CAPABILITY::RIGID );

    BOOST_CHECK( BlockDragCapability( *table ) == BLOCK_DRAG_CAPABILITY::RIGID );

    // A cell cannot travel on its own, the table owns its rectangle
    BOOST_CHECK( BlockDragCapability( *cell ) == BLOCK_DRAG_CAPABILITY::UNSUPPORTED );
}


BOOST_AUTO_TEST_CASE( GeneratorSafetyAllowList )
{
    BOARD board;

    TEST_GENERATOR* tuning = new TEST_GENERATOR( &board, wxS( "tuning_pattern" ) );
    TEST_GENERATOR* viaStack = new TEST_GENERATOR( &board, wxS( "via_stack" ) );
    TEST_GENERATOR* viaStitch = new TEST_GENERATOR( &board, wxS( "via_stitch" ) );
    TEST_GENERATOR* unknown = new TEST_GENERATOR( &board, wxS( "future_generator" ) );

    board.Add( tuning );
    board.Add( viaStack );
    board.Add( viaStitch );
    board.Add( unknown );

    BOOST_CHECK( BlockDragCapability( *tuning ) == BLOCK_DRAG_CAPABILITY::RIGID );
    BOOST_CHECK( BlockDragCapability( *viaStack ) == BLOCK_DRAG_CAPABILITY::RIGID );
    BOOST_CHECK( BlockDragCapability( *viaStitch ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
    BOOST_CHECK( BlockDragCapability( *unknown ) == BLOCK_DRAG_CAPABILITY::UNSUPPORTED );
}


BOOST_AUTO_TEST_CASE( GroupContainerMustBeExpanded )
{
    BOARD board;

    PCB_GROUP* group = new PCB_GROUP( &board );
    board.Add( group );

    BOOST_CHECK( BlockDragCapability( *group ) == BLOCK_DRAG_CAPABILITY::UNSUPPORTED );
}


BOOST_AUTO_TEST_CASE( GroupExpansionFlattensAndDeduplicates )
{
    BOARD board;

    PCB_SHAPE* shapeA = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_SHAPE* shapeB = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_SHAPE* nestedShape = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );

    PCB_GROUP* inner = new PCB_GROUP( &board );
    PCB_GROUP* outer = new PCB_GROUP( &board );

    board.Add( shapeA );
    board.Add( shapeB );
    board.Add( nestedShape );
    board.Add( inner );
    board.Add( outer );

    inner->AddItem( nestedShape );
    outer->AddItem( shapeA );
    outer->AddItem( shapeB );
    outer->AddItem( inner );

    // shapeA selected both directly and through the group
    std::vector<BOARD_ITEM*> selection{ shapeA, outer };
    std::vector<BOARD_ITEM*> expanded = BlockDragExpandSelection( selection );

    BOOST_REQUIRE_EQUAL( expanded.size(), 3 );
    BOOST_CHECK_EQUAL( std::count( expanded.begin(), expanded.end(), shapeA ), 1 );
    BOOST_CHECK_EQUAL( std::count( expanded.begin(), expanded.end(), shapeB ), 1 );
    BOOST_CHECK_EQUAL( std::count( expanded.begin(), expanded.end(), nestedShape ), 1 );

    for( BOARD_ITEM* item : expanded )
        BOOST_CHECK( item->Type() != PCB_GROUP_T );
}


BOOST_AUTO_TEST_CASE( MarkerIsDroppedFromTheExpansion )
{
    BOARD board;

    PCB_SHAPE*  shape = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_MARKER* marker = new PCB_MARKER( DRC_ITEM::Create( DRCE_CLEARANCE ), VECTOR2I( 0, 0 ) );

    board.Add( shape );
    board.Add( marker );

    std::vector<BOARD_ITEM*> expanded = BlockDragExpandSelection( { shape, marker } );

    BOOST_REQUIRE_EQUAL( expanded.size(), 1 );
    BOOST_CHECK( expanded.front() == shape );
}


// A footprint carries its own children, so a child that is selected next to its parent
// has to leave the moving set or it takes the displacement twice
BOOST_AUTO_TEST_CASE( FootprintChildIsDroppedWhenItsParentMovesToo )
{
    BOARD board;

    FOOTPRINT* footprint = new FOOTPRINT( &board );
    PAD*       pad = new PAD( footprint );
    PCB_SHAPE* silk = new PCB_SHAPE( footprint, SHAPE_T::SEGMENT );
    PCB_SHAPE* standalone = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );

    footprint->Add( pad );
    footprint->Add( silk );
    board.Add( footprint );
    board.Add( standalone );

    std::vector<BOARD_ITEM*> expanded =
            BlockDragExpandSelection( { &footprint->Reference(), pad, silk, footprint, standalone } );

    BOOST_REQUIRE_EQUAL( expanded.size(), 2 );
    BOOST_CHECK_EQUAL( std::count( expanded.begin(), expanded.end(), footprint ), 1 );
    BOOST_CHECK_EQUAL( std::count( expanded.begin(), expanded.end(), standalone ), 1 );
}


// The same child on its own still moves, because nothing else is carrying it
BOOST_AUTO_TEST_CASE( FootprintChildSurvivesWithoutItsParent )
{
    BOARD board;

    FOOTPRINT* footprint = new FOOTPRINT( &board );
    PCB_SHAPE* standalone = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );

    board.Add( footprint );
    board.Add( standalone );

    PCB_FIELD* reference = &footprint->Reference();

    std::vector<BOARD_ITEM*> expanded = BlockDragExpandSelection( { reference, standalone } );

    BOOST_REQUIRE_EQUAL( expanded.size(), 2 );
    BOOST_CHECK_EQUAL( std::count( expanded.begin(), expanded.end(), reference ), 1 );
}


// A group holding the footprint flattens to the footprint, which then has to drop the
// separately selected child just the same
BOOST_AUTO_TEST_CASE( FootprintChildIsDroppedWhenTheParentArrivesThroughAGroup )
{
    BOARD board;

    FOOTPRINT* footprint = new FOOTPRINT( &board );
    PCB_GROUP* group = new PCB_GROUP( &board );

    board.Add( footprint );
    board.Add( group );
    group->AddItem( footprint );

    std::vector<BOARD_ITEM*> expanded = BlockDragExpandSelection( { &footprint->Reference(), group } );

    BOOST_REQUIRE_EQUAL( expanded.size(), 1 );
    BOOST_CHECK( expanded.front() == footprint );
}


BOOST_AUTO_TEST_CASE( GeneratorIsNotExpanded )
{
    BOARD board;

    TEST_GENERATOR* generator = new TEST_GENERATOR( &board, wxS( "tuning_pattern" ) );
    PCB_TRACK*      memberTrack = new PCB_TRACK( &board );

    board.Add( generator );
    board.Add( memberTrack );
    generator->AddItem( memberTrack );

    std::vector<BOARD_ITEM*> selection{ generator };
    std::vector<BOARD_ITEM*> expanded = BlockDragExpandSelection( selection );

    BOOST_REQUIRE_EQUAL( expanded.size(), 1 );
    BOOST_CHECK( expanded.front() == generator );
}


BOOST_AUTO_TEST_CASE( ReferenceOnlyConstraintIsDeformable )
{
    BOARD board;

    PCB_SHAPE* measured = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    board.Add( measured );

    PCB_CONSTRAINT* reference = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::HORIZONTAL );
    reference->SetDriving( false );
    reference->AddMember( measured->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( reference );

    BOOST_CHECK( BlockDragIsConstraintMember( *measured ) );
    BOOST_CHECK( BlockDragCapability( *measured ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
}


BOOST_AUTO_TEST_CASE( MalformedConstraintIsDeformable )
{
    BOARD board;

    PCB_SHAPE* listed = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    board.Add( listed );

    // The other member never resolves to a real item, so the constraint is broken
    PCB_CONSTRAINT* dangling = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::PARALLEL );
    dangling->AddMember( listed->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    dangling->AddMember( KIID(), CONSTRAINT_ANCHOR::WHOLE );
    board.Add( dangling );

    BOOST_CHECK( BlockDragIsConstraintMember( *listed ) );
    BOOST_CHECK( BlockDragCapability( *listed ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
}


// Solve with the edited geometry held fixed. The constrained neighbour follows it, and
// the caller hears about it through the before-modify callback
BOOST_AUTO_TEST_CASE( HeldEditNeighborFollows )
{
    BOARD board;

    PCB_SHAPE* moved = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    moved->SetStart( VECTOR2I( 0, 0 ) );
    moved->SetEnd( VECTOR2I( 100000, 0 ) );

    PCB_SHAPE* follower = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    follower->SetStart( VECTOR2I( 100000, 0 ) );
    follower->SetEnd( VECTOR2I( 300000, 50000 ) );

    board.Add( moved );
    board.Add( follower );

    PCB_CONSTRAINT* coincident = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    coincident->AddMember( moved->m_Uuid, CONSTRAINT_ANCHOR::END );
    coincident->AddMember( follower->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( coincident );

    moved->SetStart( VECTOR2I( 10000, 10000 ) );
    moved->SetEnd( VECTOR2I( 110000, 10000 ) );

    std::vector<PCB_SHAPE*>  solverMoved;
    std::vector<BOARD_ITEM*> staged;

    bool solved = ReSolveShapeClustersHoldingEdited( &board, { moved }, &solverMoved,
                                                     [&]( BOARD_ITEM* aItem )
                                                     {
                                                         staged.push_back( aItem );
                                                     } );

    BOOST_REQUIRE( solved );
    BOOST_CHECK( moved->GetEnd() == VECTOR2I( 110000, 10000 ) );
    BOOST_CHECK_LE( ( follower->GetStart() - VECTOR2I( 110000, 10000 ) ).EuclideanNorm(), 5000.0 );

    BOOST_REQUIRE_EQUAL( solverMoved.size(), 1 );
    BOOST_CHECK( solverMoved.front() == follower );
    BOOST_CHECK( std::find( staged.begin(), staged.end(), follower ) != staged.end() );
}


// EDA_GROUP::AddItem asserts if a group is added to its own descendant, so the case
// to test is a selection naming both a group and a subgroup inside it
BOOST_AUTO_TEST_CASE( NestedGroupSelectedTwiceExpandsOnce )
{
    BOARD board;

    PCB_SHAPE* shapeA = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_SHAPE* shapeB = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_GROUP* inner = new PCB_GROUP( &board );
    PCB_GROUP* outer = new PCB_GROUP( &board );

    board.Add( shapeA );
    board.Add( shapeB );
    board.Add( inner );
    board.Add( outer );

    inner->AddItem( shapeB );
    outer->AddItem( shapeA );
    outer->AddItem( inner );

    std::vector<BOARD_ITEM*> expanded = BlockDragExpandSelection( { outer, inner } );

    BOOST_REQUIRE_EQUAL( expanded.size(), 2 );
    BOOST_CHECK_EQUAL( std::count( expanded.begin(), expanded.end(), shapeA ), 1 );
    BOOST_CHECK_EQUAL( std::count( expanded.begin(), expanded.end(), shapeB ), 1 );

    std::vector<BOARD_ITEM*> reversed = BlockDragExpandSelection( { inner, outer } );

    BOOST_REQUIRE_EQUAL( reversed.size(), 2 );
    BOOST_CHECK_EQUAL( std::count( reversed.begin(), reversed.end(), shapeA ), 1 );
    BOOST_CHECK_EQUAL( std::count( reversed.begin(), reversed.end(), shapeB ), 1 );
}


BOOST_AUTO_TEST_CASE( UnsupportedGroupMemberSurvivesExpansion )
{
    BOARD board;

    TEST_GENERATOR* unknown = new TEST_GENERATOR( &board, wxS( "future_generator" ) );
    PCB_SHAPE*      shape = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_GROUP*      group = new PCB_GROUP( &board );

    board.Add( unknown );
    board.Add( shape );
    board.Add( group );

    group->AddItem( unknown );
    group->AddItem( shape );

    std::vector<BOARD_ITEM*> expanded = BlockDragExpandSelection( { group } );

    // The unsupported member stays in the list, so the caller can refuse the whole block
    // and name it
    BOOST_REQUIRE_EQUAL( expanded.size(), 2 );
    BOOST_CHECK_EQUAL( std::count( expanded.begin(), expanded.end(), unknown ), 1 );
    BOOST_CHECK( BlockDragCapability( *unknown ) == BLOCK_DRAG_CAPABILITY::UNSUPPORTED );
}


BOOST_AUTO_TEST_CASE( ExpansionPreservesDirectItemOrder )
{
    BOARD board;

    PCB_SHAPE* first = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_SHAPE* member = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_SHAPE* last = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    PCB_GROUP* group = new PCB_GROUP( &board );

    board.Add( first );
    board.Add( member );
    board.Add( last );
    board.Add( group );

    group->AddItem( member );

    std::vector<BOARD_ITEM*> expanded = BlockDragExpandSelection( { first, group, last } );

    BOOST_REQUIRE_EQUAL( expanded.size(), 3 );
    BOOST_CHECK( expanded.front() == first );
    BOOST_CHECK( expanded[1] == member );
    BOOST_CHECK( expanded.back() == last );
}


BOOST_AUTO_TEST_SUITE_END()
