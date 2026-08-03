/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3
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

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <gal/gal_display_options.h>
#include <gal/graphics_abstraction_layer.h>
#include <pad.h>
#include <pcb_painter.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <pcb_view.h>
#include <pcbnew_settings.h>
#include <tool/tool_manager.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_tool_base.h>


namespace
{
const int MM = pcbIUScale.mmToIU( 1.0 );


class HEADLESS_GAL : public KIGFX::GAL
{
public:
    explicit HEADLESS_GAL( KIGFX::GAL_DISPLAY_OPTIONS& aOptions ) :
            GAL( aOptions )
    {
    }
};


class TEST_PCB_GRID_HELPER : public PCB_GRID_HELPER
{
public:
    TEST_PCB_GRID_HELPER( TOOL_MANAGER* aToolMgr, MAGNETIC_SETTINGS* aMagneticSettings ) :
            PCB_GRID_HELPER( aToolMgr, aMagneticSettings )
    {
    }

    SNAP_MANAGER&                SnapManager() { return getSnapManager(); }
    int                          SnapRange() const { return computeSnapRanges( m_enableGrid ).range; }
    int                          SnapIn() const { return computeSnapRanges( m_enableGrid ).in; }
    int                          SnapOut() const { return computeSnapRanges( m_enableGrid ).out; }
    int                          MarkerTypes() const { return m_viewSnapPoint.GetSnapTypes(); }
    VECTOR2I                     MarkerPosition() const { return m_viewSnapPoint.GetPosition(); }
    const std::optional<ANCHOR>& SnapItem() const { return m_snapItem; }
};


struct PCB_SNAP_FIXTURE
{
    KIGFX::GAL_DISPLAY_OPTIONS            options;
    HEADLESS_GAL                          gal;
    KIGFX::PCB_VIEW                       view;
    KIGFX::PCB_PAINTER                    painter;
    BOARD                                 board;
    PCBNEW_SETTINGS                       settings;
    TOOL_MANAGER                          manager;
    MAGNETIC_SETTINGS                     magnetic;
    PCB_TOOL_BASE*                        tool = nullptr;
    std::unique_ptr<TEST_PCB_GRID_HELPER> helper;

    PCB_SNAP_FIXTURE() :
            gal( options ),
            painter( &gal, FRAME_PCB_EDITOR )
    {
        gal.SetGridSize( { MM, MM } );
        gal.SetGridOrigin( { 0, 0 } );
        gal.SetWorldUnitLength( 1e-9 / 0.0254 );
        gal.SetScreenSize( { 2000, 1200 } );
        view.SetGAL( &gal );
        view.SetPainter( &painter );
        view.SetCenter( { 2 * MM, 2 * MM } );
        view.SetScale( 10.0 );
        view.SetLayerVisible( LAYER_TRACKS, true );
        view.SetLayerVisible( LAYER_PADS, true );
        view.SetLayerVisible( LAYER_FOOTPRINTS_FR, true );
        view.SetLayerVisible( LAYER_FOOTPRINTS_BK, true );
        view.Redraw();

        manager.SetEnvironment( &board, &view, nullptr, &settings, nullptr );
        tool = new PCB_TOOL_BASE( "qa.PcbSnapTool" );
        manager.RegisterTool( tool );
        manager.InvokeTool( "qa.PcbSnapTool" );

        magnetic.pads = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
        magnetic.tracks = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
        magnetic.graphics = true;
        magnetic.allLayers = true;
        helper = std::make_unique<TEST_PCB_GRID_HELPER>( &manager, &magnetic );
    }

    PCB_SHAPE* AddSegment( const VECTOR2I& aStart, const VECTOR2I& aEnd, PCB_LAYER_ID aLayer = F_SilkS )
    {
        PCB_SHAPE* segment = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
        segment->SetStart( aStart );
        segment->SetEnd( aEnd );
        segment->SetLayer( aLayer );
        board.Add( segment );
        view.Add( segment );
        return segment;
    }

    PCB_TRACK* AddTrack( const VECTOR2I& aStart, const VECTOR2I& aEnd, PCB_LAYER_ID aLayer = F_Cu )
    {
        PCB_TRACK* track = new PCB_TRACK( &board );
        track->SetStart( aStart );
        track->SetEnd( aEnd );
        track->SetLayer( aLayer );
        track->SetWidth( 250000 );
        board.Add( track );
        view.Add( track );
        return track;
    }

    PAD* AddPad( const VECTOR2I& aPosition, PCB_LAYER_ID aLayer = F_Cu )
    {
        FOOTPRINT* footprint = new FOOTPRINT( &board );
        footprint->SetPosition( aPosition + VECTOR2I( 2 * MM, 2 * MM ) );
        footprint->SetLayer( aLayer == B_Cu ? B_Cu : F_Cu );

        PAD* pad = new PAD( footprint );
        pad->SetPosition( aPosition );
        pad->SetShape( aLayer, PAD_SHAPE::CIRCLE );
        pad->SetSize( aLayer, { MM, MM } );
        pad->SetLayerSet( LSET( { aLayer } ) );
        footprint->Add( pad );
        board.Add( footprint );
        view.Add( footprint );
        return pad;
    }

    std::vector<PAD*> AddFootprintPads( const std::vector<VECTOR2I>& aPositions )
    {
        FOOTPRINT* footprint = new FOOTPRINT( &board );
        footprint->SetPosition( aPositions.front() );
        footprint->SetLayer( F_Cu );

        std::vector<PAD*> pads;

        for( const VECTOR2I& position : aPositions )
        {
            PAD* pad = new PAD( footprint );
            pad->SetPosition( position );
            pad->SetShape( F_Cu, PAD_SHAPE::CIRCLE );
            pad->SetSize( F_Cu, { MM, MM } );
            pad->SetLayerSet( LSET( { F_Cu } ) );
            footprint->Add( pad );
            pads.push_back( pad );
        }

        board.Add( footprint );
        view.Add( footprint );
        return pads;
    }
};
} // namespace


BOOST_AUTO_TEST_SUITE( PcbSnapPipeline )


BOOST_FIXTURE_TEST_CASE( DisabledSnapUsesGridWhenEnabled, PCB_SNAP_FIXTURE )
{
    helper->SetSnap( false );
    helper->SetUseGrid( true );

    SNAP_RESULT result = helper->ResolveSnap( { 1400000, 1600000 }, LSET::AllLayersMask() );

    BOOST_CHECK_EQUAL( result.position, VECTOR2I( MM, 2 * MM ) );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), nullptr );
}


BOOST_FIXTURE_TEST_CASE( DisabledSnapUsesCursorWhenGridDisabled, PCB_SNAP_FIXTURE )
{
    helper->SetSnap( false );
    helper->SetUseGrid( false );
    const VECTOR2I cursor( 1400000, 1600000 );

    SNAP_RESULT result = helper->ResolveSnap( cursor, LSET::AllLayersMask() );

    BOOST_CHECK_EQUAL( result.position, cursor );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), nullptr );
}


BOOST_FIXTURE_TEST_CASE( TrackEndpointWinsOverGrid, PCB_SNAP_FIXTURE )
{
    PCB_TRACK*                                track = AddTrack( { 1410000, 1590000 }, { 4 * MM, 1590000 } );
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> visible;
    view.Query( BOX2I( { -10 * MM, -10 * MM }, { 20 * MM, 20 * MM } ), visible );

    BOOST_REQUIRE( view.IsVisible( track ) );
    BOOST_REQUIRE_GT( visible.size(), 0 );
    BOOST_REQUIRE_GT( helper->SnapRange(), 10000 );
    BOOST_REQUIRE_LT( track->ViewGetLOD( F_Cu, &view ), view.GetScale() );
    SNAP_RESULT result = helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) );

    BOOST_CHECK_EQUAL( result.position, track->GetStart() );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), track );
}


BOOST_FIXTURE_TEST_CASE( IntrinsicAnchorWinsOverConstructionLine, PCB_SNAP_FIXTURE )
{
    helper->SetSnapLineDirections( { { 1, 0 } } );
    helper->SetSnapLineOrigin( { 0, 1600000 } );
    const VECTOR2I cursor( 1400000, 1600000 );

    SNAP_RESULT guideResult = helper->ResolveSnap( cursor, LSET( { F_Cu } ) );
    BOOST_REQUIRE_EQUAL( guideResult.position, VECTOR2I( MM, 1600000 ) );

    PCB_TRACK*  track = AddTrack( { 1410000, 1590000 }, { 4 * MM, 1590000 } );
    SNAP_RESULT result = helper->ResolveSnap( cursor, LSET( { F_Cu } ) );
    BOOST_CHECK_EQUAL( result.position, track->GetStart() );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), track );
}


BOOST_FIXTURE_TEST_CASE( RetainedAnchorUsesSnapOutThreshold, PCB_SNAP_FIXTURE )
{
    const VECTOR2I anchor( 2 * MM, 2 * MM );
    PCB_TRACK*     track = AddTrack( anchor, anchor - VECTOR2I( 4 * MM, 0 ) );
    helper->SetSnapLine( false );
    helper->ResolveSnap( anchor, LSET( { F_Cu } ) );
    BOOST_REQUIRE_EQUAL( helper->GetSnapped(), track );

    SNAP_RESULT inside = helper->ResolveSnap( anchor + VECTOR2I( helper->SnapOut() - 1, 0 ), LSET( { F_Cu } ) );
    BOOST_CHECK_EQUAL( inside.position, anchor );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), track );

    SNAP_RESULT outside = helper->ResolveSnap( anchor + VECTOR2I( helper->SnapOut() + 1, 0 ), LSET( { F_Cu } ) );
    BOOST_CHECK_NE( outside.position, anchor );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), nullptr );
}


BOOST_FIXTURE_TEST_CASE( ReferenceOnlyPointUpdatesGuideWithoutSnapping, PCB_SNAP_FIXTURE )
{
    const VECTOR2I reference( 1410000, 1590000 );
    AddTrack( reference, { 4 * MM, 1590000 } );
    helper->SnapManager().SetReferenceOnlyPoints( { reference } );

    SNAP_RESULT result = helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) );

    BOOST_CHECK_NE( result.position, reference );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), nullptr );
    const OPT_VECTOR2I& origin = helper->SnapManager().GetSnapLineManager().GetSnapLineOrigin();
    BOOST_REQUIRE( origin );
    BOOST_CHECK_EQUAL( *origin, reference );
}


BOOST_FIXTURE_TEST_CASE( DefaultInferenceSnapsToElementWithGridEnabled, PCB_SNAP_FIXTURE )
{
    AddTrack( { 0, 1500000 }, { 4 * MM, 1500000 } );
    const VECTOR2I cursor( 1400000, 1510000 );

    helper->SetUseGrid( true );
    SNAP_RESULT result = helper->ResolveSnap( cursor, LSET( { F_Cu } ) );

    BOOST_CHECK_EQUAL( result.position, VECTOR2I( MM, 1500000 ) );
}


BOOST_FIXTURE_TEST_CASE( AlignmentFindsTargetFarAlongGuideDirection, PCB_SNAP_FIXTURE )
{
    PCB_SHAPE* moving = AddSegment( { 2 * MM, 2 * MM }, { 3 * MM, 2 * MM } );
    AddSegment( { 2 * MM, 40 * MM }, { 3 * MM, 40 * MM } );
    view.SetViewport( BOX2D( { 0, 0 }, { 50 * MM, 50 * MM } ) );
    settings.m_SnapInference.alignmentDistribution = true;

    SNAP_RESULT result = helper->ResolveSnap( moving->GetStart(), LSET( { F_SilkS } ), GRID_CURRENT, { moving },
                                              moving->GetStart() );

    BOOST_CHECK( std::any_of( result.accepted.begin(), result.accepted.end(),
                              []( const SNAP_STABLE_ID& aId )
                              {
                                  return aId.kind == SNAP_ID_KIND::BOUNDS_X;
                              } ) );
    BOOST_CHECK( std::any_of( result.guides.begin(), result.guides.end(),
                              []( const SNAP_GUIDE& aGuide )
                              {
                                  return std::abs( aGuide.end.y - aGuide.start.y ) > 30 * MM;
                              } ) );
}


BOOST_FIXTURE_TEST_CASE( EqualSpacingFindsTargetsFarAlongSpacingAxis, PCB_SNAP_FIXTURE )
{
    AddSegment( { 2 * MM, 2 * MM }, { 3 * MM, 3 * MM } );
    AddSegment( { 12 * MM, 2 * MM }, { 13 * MM, 3 * MM } );
    PCB_SHAPE* moving = AddSegment( { 22 * MM, 2 * MM }, { 23 * MM, 3 * MM } );
    view.SetViewport( BOX2D( { 0, 0 }, { 50 * MM, 50 * MM } ) );
    settings.m_SnapInference.alignmentDistribution = true;

    SNAP_RESULT result = helper->ResolveSnap( moving->GetStart(), LSET( { F_SilkS } ), GRID_CURRENT, { moving },
                                              moving->GetStart() );

    BOOST_CHECK( std::any_of( result.accepted.begin(), result.accepted.end(),
                              []( const SNAP_STABLE_ID& aId )
                              {
                                  return aId.kind == SNAP_ID_KIND::COPY_GAP_X;
                              } ) );
}


BOOST_FIXTURE_TEST_CASE( FootprintEditorSpacesPadsEvenly, PCB_SNAP_FIXTURE )
{
    std::vector<PAD*> pads = AddFootprintPads( { { 2 * MM, 2 * MM }, { 12 * MM, 2 * MM }, { 22 * MM, 2 * MM } } );
    view.SetViewport( BOX2D( { 0, 0 }, { 50 * MM, 50 * MM } ) );
    settings.m_SnapInference.alignmentDistribution = true;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    PAD*        moving = pads.back();
    SNAP_RESULT result = helper->ResolveSnap( moving->GetPosition(), LSET( { F_Cu } ), GRID_CURRENT, { moving },
                                              moving->GetPosition() );

    BOOST_CHECK( std::any_of( result.accepted.begin(), result.accepted.end(),
                              []( const SNAP_STABLE_ID& aId )
                              {
                                  return aId.kind == SNAP_ID_KIND::COPY_GAP_X
                                         || aId.kind == SNAP_ID_KIND::EQUAL_GAP_X;
                              } ) );
}


BOOST_FIXTURE_TEST_CASE( BoardKeepsPadsInsideTheirFootprint, PCB_SNAP_FIXTURE )
{
    std::vector<PAD*> pads = AddFootprintPads( { { 2 * MM, 2 * MM }, { 12 * MM, 2 * MM }, { 22 * MM, 2 * MM } } );
    view.SetViewport( BOX2D( { 0, 0 }, { 50 * MM, 50 * MM } ) );
    settings.m_SnapInference.alignmentDistribution = true;

    PAD*        moving = pads.back();
    SNAP_RESULT result = helper->ResolveSnap( moving->GetPosition(), LSET( { F_Cu } ), GRID_CURRENT, { moving },
                                              moving->GetPosition() );

    // One footprint collapses to one object, so its pads cannot form a spacing series.
    BOOST_CHECK( std::none_of( result.accepted.begin(), result.accepted.end(),
                               []( const SNAP_STABLE_ID& aId )
                               {
                                   return aId.kind == SNAP_ID_KIND::COPY_GAP_X
                                          || aId.kind == SNAP_ID_KIND::EQUAL_GAP_X;
                               } ) );
}


BOOST_FIXTURE_TEST_CASE( DraggedPadDoesNotAlignToItsOwnStartPosition, PCB_SNAP_FIXTURE )
{
    // A free pad's own footprint stays a layout object, so its pad list still reaches the pad
    // being dragged even though the skip list excluded it.
    std::vector<PAD*> pads = AddFootprintPads( { { 2 * MM, 2 * MM }, { 12 * MM, 9 * MM } } );
    view.SetViewport( BOX2D( { 0, 0 }, { 50 * MM, 50 * MM } ) );
    settings.m_SnapInference.alignmentDistribution = true;

    PAD*                     moving = pads.back();
    const VECTOR2I           origin = moving->GetPosition();
    std::vector<BOARD_ITEM*> dragged = { moving };

    // Grabbing a pad centre is what admits pad-centre alignment targets at all.
    helper->BestDragOrigin( origin, dragged );
    helper->SetUseGrid( false );

    const VECTOR2I cursor( 30 * MM, origin.y + 100000 );
    SNAP_RESULT    result = helper->ResolveSnap( cursor, LSET( { F_Cu } ), GRID_CURRENT, dragged, origin );

    BOOST_CHECK( result.position.y != origin.y );
}


BOOST_FIXTURE_TEST_CASE( LegacyFallbackSuppressesPointOnElementWhenGridEnabled, PCB_SNAP_FIXTURE )
{
    AddTrack( { 0, 1500000 }, { 4 * MM, 1500000 } );
    const VECTOR2I cursor( 1400000, 1510000 );
    settings.m_SnapInference.objectGeometry = false;

    helper->SetUseGrid( true );
    SNAP_RESULT gridResult = helper->ResolveSnap( cursor, LSET( { F_Cu } ) );
    BOOST_CHECK_EQUAL( gridResult.position, VECTOR2I( MM, 2 * MM ) );

    helper->SetUseGrid( false );
    SNAP_RESULT elementResult = helper->ResolveSnap( cursor, LSET( { F_Cu } ) );
    BOOST_CHECK_EQUAL( elementResult.position, VECTOR2I( cursor.x, 1500000 ) );
    BOOST_CHECK_EQUAL( helper->MarkerTypes(), POINT_TYPE::PT_ON_ELEMENT );
}


BOOST_FIXTURE_TEST_CASE( TrackMagnetismTogglesIndependently, PCB_SNAP_FIXTURE )
{
    const VECTOR2I anchor( 1410000, 1590000 );
    PCB_TRACK*     track = AddTrack( anchor, { 4 * MM, 1590000 }, B_Cu );
    magnetic.tracks = MAGNETIC_OPTIONS::NO_EFFECT;

    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, VECTOR2I( MM, 2 * MM ) );

    magnetic.tracks = MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL;
    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, VECTOR2I( MM, 2 * MM ) );
    magnetic.tracks = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
    magnetic.allLayers = false;
    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, VECTOR2I( MM, 2 * MM ) );

    magnetic.allLayers = true;
    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, anchor );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), track );
}


BOOST_FIXTURE_TEST_CASE( GraphicsMagnetismAndLayerMatchingToggleIndependently, PCB_SNAP_FIXTURE )
{
    const VECTOR2I anchor( 1410000, 1590000 );
    PCB_SHAPE*     segment = AddSegment( anchor, { 4 * MM, 1590000 }, B_SilkS );
    magnetic.graphics = false;
    magnetic.allLayers = true;

    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, VECTOR2I( MM, 2 * MM ) );

    magnetic.graphics = true;
    magnetic.allLayers = false;
    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, VECTOR2I( MM, 2 * MM ) );

    magnetic.allLayers = true;
    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, anchor );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), segment );
}


BOOST_FIXTURE_TEST_CASE( PadMagnetismTogglesIndependently, PCB_SNAP_FIXTURE )
{
    const VECTOR2I anchor( 1410000, 1590000 );
    PAD*           pad = AddPad( anchor, B_Cu );
    magnetic.pads = MAGNETIC_OPTIONS::NO_EFFECT;

    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, VECTOR2I( MM, 2 * MM ) );

    magnetic.pads = MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL;
    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, VECTOR2I( MM, 2 * MM ) );
    magnetic.pads = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
    magnetic.allLayers = false;
    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, VECTOR2I( MM, 2 * MM ) );

    magnetic.allLayers = true;
    BOOST_CHECK_EQUAL( helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) ).position, anchor );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), pad );
}


BOOST_FIXTURE_TEST_CASE( FeasibilityFallsBackToNextObjectCandidate, PCB_SNAP_FIXTURE )
{
    const VECTOR2I firstSelfPoint( 1410000, 1590000 );
    const VECTOR2I secondSelfPoint( 1420000, 1580000 );
    PCB_SHAPE*     moving = AddSegment( firstSelfPoint, secondSelfPoint );
    helper->SetPointEditProfile( true );
    helper->SetStationarySelfGeometry( { moving->GetStart(), moving->GetEnd() }, {} );
    helper->SetFeasibilityCallback(
            [firstSelfPoint]( const SNAP_SOURCE_CONTEXT& aContext, const std::vector<SNAP_CANDIDATE>& aTrial )
            {
                SNAP_RESULT result;
                result.position = aContext.sourcePoint;

                if( aTrial.empty() )
                    return result;

                const SNAP_CANDIDATE& candidate = aTrial.back();

                if( KiROUND( candidate.origin.x ) == firstSelfPoint.x )
                {
                    result.status = SNAP_RESULT_STATUS::INCOMPATIBLE;
                    return result;
                }

                result.position = KiROUND( candidate.origin );
                result.remainingDof = 0;
                return result;
            } );

    SNAP_RESULT result = helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_SilkS } ), GRID_CURRENT, { moving } );

    BOOST_CHECK_EQUAL( result.position, secondSelfPoint );
}


BOOST_FIXTURE_TEST_CASE( AcceptedAnchorDrivesPresentationPayload, PCB_SNAP_FIXTURE )
{
    const VECTOR2I anchor( 1410000, 1590000 );
    PCB_TRACK*     track = AddTrack( anchor, { 4 * MM, 1590000 } );

    SNAP_RESULT result = helper->ResolveSnap( { 1400000, 1600000 }, LSET( { F_Cu } ) );

    BOOST_REQUIRE_EQUAL( result.accepted.size(), 1 );
    BOOST_CHECK( result.accepted.front().kind == SNAP_ID_KIND::INTRINSIC_ANCHOR );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), track );
    BOOST_CHECK_EQUAL( helper->MarkerPosition(), anchor );
    BOOST_CHECK_EQUAL( helper->MarkerTypes(), POINT_TYPE::PT_END );

    std::vector<CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH> batches;
    helper->SnapManager().GetConstructionManager().GetPendingConstructionItems( batches );
    BOOST_REQUIRE_EQUAL( batches.size(), 1 );
    BOOST_REQUIRE_EQUAL( batches.front().size(), 1 );
    BOOST_CHECK_EQUAL( batches.front().front().Item, track );

    helper->ResolveSnap( anchor + VECTOR2I( 1, 1 ), LSET( { F_Cu } ) );
    batches.clear();
    helper->SnapManager().GetConstructionManager().GetPendingConstructionItems( batches );
    BOOST_REQUIRE_EQUAL( batches.size(), 1 );
    BOOST_REQUIRE_EQUAL( batches.front().size(), 1 );
    BOOST_CHECK_EQUAL( batches.front().front().Item, track );
}


BOOST_FIXTURE_TEST_CASE( CoincidentAcceptedIdSelectsOwningAnchorPayload, PCB_SNAP_FIXTURE )
{
    const VECTOR2I anchor( 1410000, 1590000 );
    settings.m_SnapInference.objectGeometry = false;
    settings.m_SnapInference.constructionExtensions = false;
    PCB_TRACK* retainedTrack = AddTrack( anchor, { 4 * MM, 1590000 } );
    helper->SetSnapLine( false );
    SNAP_RESULT retainedResult = helper->ResolveSnap( anchor, LSET( { F_Cu } ) );
    BOOST_REQUIRE_EQUAL( helper->GetSnapped(), retainedTrack );
    BOOST_REQUIRE_EQUAL( retainedResult.accepted.size(), 1 );

    retainedTrack->SetLayer( B_Cu );
    PCB_TRACK* acceptedTrack = AddTrack( anchor, { 1410000, 4 * MM } );
    magnetic.allLayers = false;
    helper->SetUseGrid( false );
    const SNAP_STABLE_ID rejectedId = retainedResult.accepted.front();
    helper->SetFeasibilityCallback(
            [rejectedId]( const SNAP_SOURCE_CONTEXT& aContext, const std::vector<SNAP_CANDIDATE>& aCandidates )
            {
                SNAP_RESULT result;
                result.position = aContext.sourcePoint;

                if( aCandidates.empty() )
                    return result;

                if( aCandidates.back().id == rejectedId )
                {
                    result.status = SNAP_RESULT_STATUS::INCOMPATIBLE;
                    return result;
                }

                result.position = KiROUND( aCandidates.back().origin );
                result.remainingDof = 0;
                return result;
            } );

    SNAP_RESULT result = helper->ResolveSnap( anchor, LSET( { F_Cu } ) );

    BOOST_REQUIRE_EQUAL( result.accepted.size(), 1 );
    BOOST_CHECK( result.accepted.front() != rejectedId );
    BOOST_CHECK_EQUAL( helper->GetSnapped(), acceptedTrack );
    BOOST_CHECK_EQUAL( helper->MarkerPosition(), anchor );
    BOOST_CHECK_EQUAL( helper->MarkerTypes(), POINT_TYPE::PT_END );
}


BOOST_FIXTURE_TEST_CASE( AlignmentGuideSurvivesPresentationReset, PCB_SNAP_FIXTURE )
{
    PCB_SHAPE* moving = AddSegment( { 2 * MM, 2 * MM }, { 3 * MM, 2 * MM } );
    AddSegment( { 2 * MM, 10 * MM }, { 3 * MM, 10 * MM } );
    settings.m_SnapInference.alignmentDistribution = true;

    SNAP_RESULT result = helper->ResolveSnap( moving->GetStart(), LSET( { F_SilkS } ), GRID_CURRENT, { moving },
                                              moving->GetStart() );
    const auto  guideIt = std::find_if( result.guides.begin(), result.guides.end(),
                                        []( const SNAP_GUIDE& aGuide )
                                        {
                                           return aGuide.style == SNAP_GUIDE_STYLE::SNAP_LINE;
                                       } );
    BOOST_REQUIRE( guideIt != result.guides.end() );
    const SNAP_LINE_MANAGER& manager = helper->SnapManager().GetSnapLineManager();
    BOOST_REQUIRE( manager.GetSnapLineOrigin() );
    BOOST_CHECK( manager.HasCompleteSnapLine() );
    BOOST_CHECK_EQUAL( *manager.GetSnapLineOrigin(), guideIt->start );
}


BOOST_FIXTURE_TEST_CASE( DenseCoincidentTieRetainsPartialBudgetResult, PCB_SNAP_FIXTURE )
{
    const VECTOR2I anchor( 1410000, 1590000 );
    settings.m_SnapInference.objectGeometry = false;
    settings.m_SnapInference.constructionExtensions = false;
    helper->SetSnapLine( false );
    helper->SetUseGrid( false );

    for( int i = 0; i < 8; ++i )
        AddTrack( anchor, { 4 * MM, ( 3 + i ) * MM } );

    helper->SetFeasibilityCallback(
            []( const SNAP_SOURCE_CONTEXT& aContext, const std::vector<SNAP_CANDIDATE>& aCandidates )
            {
                SNAP_RESULT result;
                result.position = aContext.sourcePoint;
                result.remainingDof = 1;

                if( aCandidates.empty() )
                {
                    // Exceed the resolver's 8 ms deadline before accepting the first candidate.
                    std::this_thread::sleep_for( std::chrono::milliseconds( 12 ) );
                    return result;
                }

                result.position = KiROUND( aCandidates.back().origin );
                return result;
            } );

    SNAP_RESULT result = helper->ResolveSnap( anchor, LSET( { F_Cu } ) );

    BOOST_REQUIRE_EQUAL( result.status, SNAP_RESULT_STATUS::BUDGET_EXHAUSTED );
    BOOST_REQUIRE_EQUAL( result.accepted.size(), 1 );
    BOOST_REQUIRE( helper->SnapItem() );
    const auto&                 acceptedAnchor = *helper->SnapItem();
    std::vector<SNAP_TARGET_ID> expectedTargets;

    for( EDA_ITEM* item : acceptedAnchor.items )
    {
        if( item )
            expectedTargets.push_back( item->m_Uuid.AsBytes() );
    }

    SNAP_STABLE_ID pointId =
            MakePointSnapId( SNAP_ID_KIND::INTRINSIC_ANCHOR, acceptedAnchor.pos, acceptedAnchor.pointTypes );
    expectedTargets.push_back( pointId.target );
    BOOST_CHECK( result.accepted.front()
                 == MakeCompositeSnapId( SNAP_ID_KIND::INTRINSIC_ANCHOR, expectedTargets, acceptedAnchor.pointTypes ) );
    BOOST_CHECK_EQUAL( result.position, anchor );
}


BOOST_FIXTURE_TEST_CASE( RectangleInteriorDoesNotCaptureCursor, PCB_SNAP_FIXTURE )
{
    PCB_SHAPE* outline = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
    outline->SetStart( { 0, 0 } );
    outline->SetEnd( { 40 * MM, 40 * MM } );
    outline->SetLayer( Edge_Cuts );
    board.Add( outline );
    view.Add( outline );
    view.SetViewport( BOX2D( { 0, 0 }, { 50 * MM, 50 * MM } ) );

    // Deep inside the outline, far from every edge, corner and the centre.
    const VECTOR2I cursor( 12 * MM, 31 * MM );

    helper->SetUseGrid( false );

    SNAP_RESULT result = helper->ResolveSnap( cursor, LSET( { F_SilkS } ) );

    BOOST_CHECK_EQUAL( result.position, cursor );
}


BOOST_AUTO_TEST_SUITE_END()
