/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_rtree.h>
#include <drc/drc_zone_index.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <settings/settings_manager.h>
#include <zone.h>

#include <geometry/shape_poly_set.h>

#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>


namespace
{

struct PREPARED_CANDIDATES_FIXTURE
{
    void Load( const wxString& aRelPath )
    {
        KI_TEST::LoadBoard( m_settingsManager, aRelPath, m_board );
        BOOST_REQUIRE( m_board );
        m_board->GetDesignSettings().m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );
        BOOST_REQUIRE_GT( m_board->m_DRCMaxClearance, 0 );
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


/// The copper item cache is filled one layer at a time with the board's largest clearance.
void insertCopperItems( DRC_RTREE& aTree, BOARD* aBoard, PCB_LAYER_ID aLayer, int aPadding,
                        BOARD_ITEM* aExceptItem = nullptr, int aExceptPadding = 0 )
{
    auto insert =
            [&]( BOARD_ITEM* aItem )
            {
                if( !aItem->IsOnLayer( aLayer ) )
                    return;

                aTree.Insert( aItem, aLayer, CLEARANCE_CONSTRAINT,
                              aItem == aExceptItem ? aExceptPadding : aPadding );
            };

    for( PCB_TRACK* track : aBoard->Tracks() )
        insert( track );

    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            insert( pad );
    }

    aTree.Build();
}


/// True when every element of aNeedle appears in aHaystack, in order.
bool isSubsequence( const std::vector<std::string>& aNeedle, const std::vector<std::string>& aHaystack )
{
    size_t pos = 0;

    for( const std::string& entry : aNeedle )
    {
        while( pos < aHaystack.size() && aHaystack[pos] != entry )
            ++pos;

        if( pos == aHaystack.size() )
            return false;

        ++pos;
    }

    return true;
}


std::vector<BOARD_ITEM*> copperReferences( BOARD* aBoard, size_t aLimit )
{
    std::vector<BOARD_ITEM*> refs;

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( refs.size() >= aLimit )
            break;

        refs.push_back( track );
    }

    size_t padLimit = refs.size() + aLimit;

    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( refs.size() >= padLimit )
                break;

            refs.push_back( pad );
        }
    }

    return refs;
}


std::vector<std::string> legacyCandidates( DRC_RTREE& aTree, BOARD_ITEM* aRef, PCB_LAYER_ID aLayer, int aClearance,
                                           std::vector<std::string>* aFilterLog = nullptr,
                                           const std::string& aReject = std::string() )
{
    std::vector<std::string> hits;

    aTree.QueryColliding(
            aRef, aLayer, aLayer,
            [&]( BOARD_ITEM* aItem )
            {
                std::string uuid = aItem->m_Uuid.AsString().ToStdString();

                if( aFilterLog )
                    aFilterLog->push_back( uuid );

                return uuid != aReject;
            },
            [&]( BOARD_ITEM* aItem )
            {
                hits.push_back( aItem->m_Uuid.AsString().ToStdString() );
                return true;
            },
            aClearance );

    return hits;
}


std::vector<std::string> preparedCandidates( DRC_RTREE& aTree, BOARD_ITEM* aRef,
                                             const std::shared_ptr<SHAPE>& aRefShape, PCB_LAYER_ID aLayer,
                                             int aClearance, bool aCredit,
                                             std::vector<std::string>* aFilterLog = nullptr,
                                             const std::string& aReject = std::string(),
                                             bool aCheckParentShapes = false )
{
    std::vector<std::string> hits;

    aTree.QueryCollidingPreparedCopper(
            aRef, aRefShape, aLayer,
            [&]( BOARD_ITEM* aItem )
            {
                std::string uuid = aItem->m_Uuid.AsString().ToStdString();

                if( aFilterLog )
                    aFilterLog->push_back( uuid );

                return uuid != aReject;
            },
            [&]( BOARD_ITEM* aItem, const std::shared_ptr<SHAPE>& aParentShape )
            {
                BOOST_REQUIRE( aParentShape );

                if( aCheckParentShapes )
                {
                    // The substituted parent has to be what the provider used to build itself,
                    // which is the plain layer shape, not just something with the same bounds
                    std::shared_ptr<SHAPE> expected = aItem->GetEffectiveShape( aLayer );
                    BOOST_REQUIRE( expected );
                    BOOST_CHECK_EQUAL( aParentShape->Format( false ), expected->Format( false ) );
                }

                hits.push_back( aItem->m_Uuid.AsString().ToStdString() );
                return true;
            },
            aClearance, aCredit );

    return hits;
}

} // namespace


BOOST_FIXTURE_TEST_CASE( DRCPreparedCopperQueryMatchesLegacy, PREPARED_CANDIDATES_FIXTURE )
{
    const wxString boards[] = { wxT( "issue12609" ), wxT( "issue6443" ), wxT( "issue22102" ) };

    size_t totalHits = 0;
    size_t widestQuery = 0;
    size_t negativeClearanceQueries = 0;

    for( const wxString& boardName : boards )
    {
        Load( boardName );

        const int maxClearance = m_board->m_DRCMaxClearance;
        const int clearances[] = { -maxClearance, 0, maxClearance / 2, maxClearance, maxClearance + 1 };

        BOOST_REQUIRE( m_board->m_CopperItemRTreeCache );

        for( BOARD_ITEM* ref : copperReferences( m_board.get(), 40 ) )
        {
            for( PCB_LAYER_ID layer : LSET( ref->GetLayerSet() & LSET::AllCuMask( m_board->GetCopperLayerCount() ) ) )
            {
                std::shared_ptr<SHAPE> refShape = ref->GetEffectiveShape( layer );
                BOOST_REQUIRE( refShape );

                for( int clearance : clearances )
                {
                    std::vector<std::string> legacy =
                            legacyCandidates( *m_board->m_CopperItemRTreeCache, ref, layer, clearance );
                    std::vector<std::string> prepared =
                            preparedCandidates( *m_board->m_CopperItemRTreeCache, ref, refShape, layer, clearance,
                                                true, nullptr, std::string(), true );

                    BOOST_CHECK_EQUAL_COLLECTIONS( legacy.begin(), legacy.end(), prepared.begin(), prepared.end() );

                    totalHits += legacy.size();
                    widestQuery = std::max( widestQuery, legacy.size() );

                    if( clearance < 0 )
                        ++negativeClearanceQueries;
                }
            }
        }
    }

    // Several references have to actually collide, and at least one with more than one candidate,
    // or the collection comparisons above are vacuous
    BOOST_CHECK_GT( totalHits, 0u );
    BOOST_CHECK_GT( widestQuery, 1u );
    BOOST_CHECK_GT( negativeClearanceQueries, 0u );
}


BOOST_FIXTURE_TEST_CASE( DRCPreparedCopperFilterInvocationParity, PREPARED_CANDIDATES_FIXTURE )
{
    Load( wxT( "issue6443" ) );

    const int maxClearance = m_board->m_DRCMaxClearance;
    DRC_RTREE& tree = *m_board->m_CopperItemRTreeCache;

    size_t plainInvocations = 0;
    size_t creditInvocations = 0;
    size_t rejections = 0;

    for( BOARD_ITEM* ref : copperReferences( m_board.get(), 40 ) )
    {
        for( PCB_LAYER_ID layer : LSET( ref->GetLayerSet() & LSET::AllCuMask( m_board->GetCopperLayerCount() ) ) )
        {
            std::shared_ptr<SHAPE> refShape = ref->GetEffectiveShape( layer );

            // Reject whichever candidate the legacy query sees first, so the memoized filter
            // result is exercised on a parent that really is in the tree
            std::vector<std::string> probe;
            legacyCandidates( tree, ref, layer, maxClearance, &probe );

            if( probe.empty() )
                continue;

            const std::string reject = probe.front();
            ++rejections;

            std::vector<std::string> legacyLog;
            std::vector<std::string> plainLog;
            std::vector<std::string> creditLog;

            std::vector<std::string> legacy =
                    legacyCandidates( tree, ref, layer, maxClearance, &legacyLog, reject );
            std::vector<std::string> plain = preparedCandidates( tree, ref, refShape, layer, maxClearance, false,
                                                                 &plainLog, reject );
            std::vector<std::string> credit = preparedCandidates( tree, ref, refShape, layer, maxClearance, true,
                                                                  &creditLog, reject );

            // Without credit the broad phase is the legacy one, so the filter has to be asked the
            // same questions in the same order
            BOOST_CHECK_EQUAL_COLLECTIONS( legacyLog.begin(), legacyLog.end(), plainLog.begin(), plainLog.end() );
            BOOST_CHECK_EQUAL_COLLECTIONS( legacy.begin(), legacy.end(), plain.begin(), plain.end() );

            // Credit only shrinks the query box, so it asks a subsequence of the same questions
            // and still finds every collision
            BOOST_CHECK( isSubsequence( creditLog, plainLog ) );
            BOOST_CHECK_EQUAL_COLLECTIONS( legacy.begin(), legacy.end(), credit.begin(), credit.end() );

            // Memoization: a parent is never handed to the filter twice
            std::vector<std::string> sortedCredit = creditLog;
            std::sort( sortedCredit.begin(), sortedCredit.end() );
            BOOST_CHECK( std::adjacent_find( sortedCredit.begin(), sortedCredit.end() ) == sortedCredit.end() );

            plainInvocations += plainLog.size();
            creditInvocations += creditLog.size();
        }
    }

    BOOST_CHECK_GT( rejections, 0u );

    // Pruning pure filter invocations is the whole point of the credit, so an equal-call-count
    // requirement would be wrong; what has to hold is that it never asks more
    BOOST_CHECK_LT( creditInvocations, plainInvocations );
}


BOOST_FIXTURE_TEST_CASE( DRCPreparedCopperCreditUsesMinimumPadding, PREPARED_CANDIDATES_FIXTURE )
{
    // The falsifying case needs two real copper items whose bounding boxes are disjoint but which
    // still collide within the board's worst clearance.  Not every fixture has one, so scan.
    const wxString candidates[] = { wxT( "issue12609" ), wxT( "issue6443" ), wxT( "issue22102" ),
                                    wxT( "issue5978" ), wxT( "issue14008" ) };

    const PCB_LAYER_ID layer = F_Cu;

    BOARD_ITEM* reference = nullptr;
    BOARD_ITEM* farCandidate = nullptr;
    int         maxClearance = 0;
    DRC_RTREE   uniform;

    for( const wxString& boardName : candidates )
    {
        Load( boardName );
        maxClearance = m_board->m_DRCMaxClearance;

        uniform.clear();
        insertCopperItems( uniform, m_board.get(), layer, maxClearance );

        std::vector<BOARD_ITEM*> items = copperReferences( m_board.get(), 100000 );

        for( BOARD_ITEM* ref : items )
        {
            if( !ref->IsOnLayer( layer ) )
                continue;

            std::vector<std::string> reachable = legacyCandidates( uniform, ref, layer, maxClearance );

            for( BOARD_ITEM* candidate : items )
            {
                if( candidate == ref || !candidate->IsOnLayer( layer ) )
                    continue;

                if( ref->GetBoundingBox().Intersects( candidate->GetBoundingBox() ) )
                    continue;

                if( std::find( reachable.begin(), reachable.end(),
                               candidate->m_Uuid.AsString().ToStdString() ) == reachable.end() )
                {
                    continue;
                }

                reference = ref;
                farCandidate = candidate;
                break;
            }

            if( reference )
                break;
        }

        BOOST_TEST_MESSAGE( wxString::Format( "MINPAD %s maxClearance=%d qualifyingPair=%s", boardName,
                                              maxClearance, reference ? "yes" : "no" ) );

        if( reference )
            break;
    }

    BOOST_REQUIRE( reference );
    BOOST_REQUIRE( farCandidate );

    // One item inserted with no padding drops the layer minimum to zero, which has to cost the
    // query its whole credit
    DRC_RTREE mixed;
    insertCopperItems( mixed, m_board.get(), layer, maxClearance, farCandidate, 0 );

    std::shared_ptr<SHAPE> refShape = reference->GetEffectiveShape( layer );
    std::vector<std::string> expected = legacyCandidates( mixed, reference, layer, maxClearance );
    std::vector<std::string> credit =
            preparedCandidates( mixed, reference, refShape, layer, maxClearance, true );

    BOOST_CHECK_EQUAL_COLLECTIONS( expected.begin(), expected.end(), credit.begin(), credit.end() );
    BOOST_CHECK( std::find( credit.begin(), credit.end(), farCandidate->m_Uuid.AsString().ToStdString() )
                 != credit.end() );

    // clear() drops the recorded padding with the entries; rebuilding on another layer must not
    // inherit either
    mixed.clear();
    BOOST_CHECK_EQUAL( preparedCandidates( mixed, reference, refShape, layer, maxClearance, true ).size(), 0u );

    insertCopperItems( mixed, m_board.get(), B_Cu, maxClearance / 2 );

    for( BOARD_ITEM* ref : copperReferences( m_board.get(), 40 ) )
    {
        if( !ref->IsOnLayer( B_Cu ) )
            continue;

        std::shared_ptr<SHAPE> backShape = ref->GetEffectiveShape( B_Cu );
        std::vector<std::string> legacy = legacyCandidates( mixed, ref, B_Cu, maxClearance );
        std::vector<std::string> rebuilt =
                preparedCandidates( mixed, ref, backShape, B_Cu, maxClearance, true );

        BOOST_CHECK_EQUAL_COLLECTIONS( legacy.begin(), legacy.end(), rebuilt.begin(), rebuilt.end() );
    }
}


BOOST_FIXTURE_TEST_CASE( DRCCreditCallSitesSatisfyContainment, PREPARED_CANDIDATES_FIXTURE )
{
    // Tracks and pads are the only two call sites that opt into the padding credit, and the credit
    // is only sound where the item's bounding box contains the shape the query collides against.
    // Nothing in the type system enforces that, so assert it directly: PAD::GetBoundingBox()
    // contains the hole only because BuildEffectiveShapes() merges it in, and a refactor to a
    // copper-only box would break DRC with no other test noticing.
    const wxString boards[] = { wxT( "issue12609" ), wxT( "issue6443" ), wxT( "issue22102" ),
                                wxT( "issue14008" ), wxT( "reverse_via" ) };

    size_t examined = 0;

    for( const wxString& boardName : boards )
    {
        Load( boardName );

        LSET copper = LSET::AllCuMask( m_board->GetCopperLayerCount() );

        auto check =
                [&]( BOARD_ITEM* aItem )
                {
                    for( PCB_LAYER_ID layer : LSET( aItem->GetLayerSet() & copper ) )
                    {
                        std::shared_ptr<SHAPE> shape = aItem->GetEffectiveShape( layer );

                        if( !shape )
                            continue;

                        ++examined;
                        BOOST_CHECK_MESSAGE( aItem->GetBoundingBox().Contains( shape->BBox() ),
                                             boardName + wxT( " " ) + aItem->m_Uuid.AsString()
                                                     + wxT( " does not contain its own shape" ) );
                    }
                };

        for( PCB_TRACK* track : m_board->Tracks() )
            check( track );

        for( FOOTPRINT* footprint : m_board->Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
                check( pad );
        }
    }

    // A clean verdict over zero objects is not a clean verdict
    BOOST_CHECK_GT( examined, 0u );
}


BOOST_FIXTURE_TEST_CASE( DRCGraphicBoundsDoNotContainTheirEffectiveShape, PREPARED_CANDIDATES_FIXTURE )
{
    // Padding credit is only sound where the item's bounding box contains the shape the query
    // will collide against.  Copper graphics do not satisfy that, which is why
    // testGraphicClearances() opts out.  Red if the two bounding boxes are ever reconciled, at
    // which point the credit could be turned back on there.
    Load( wxT( "issue22102" ) );

    size_t copperGraphics = 0;
    size_t uncontained = 0;

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        PCB_LAYER_ID layer = item->GetLayer();

        if( !IsCopperLayer( layer ) )
            continue;

        std::shared_ptr<SHAPE> shape = item->GetEffectiveShape( layer );

        if( !shape )
            continue;

        ++copperGraphics;

        if( !item->GetBoundingBox().Contains( shape->BBox() ) )
            ++uncontained;
    }

    BOOST_CHECK_GT( copperGraphics, 0u );
    BOOST_CHECK_GT( uncontained, 0u );
}


BOOST_FIXTURE_TEST_CASE( DRCZoneIndexMatchesOrderedLinearScan, PREPARED_CANDIDATES_FIXTURE )
{
    const wxString boards[] = { wxT( "fill_bad" ), wxT( "intersectingzones" ), wxT( "issue5750" ) };

    size_t exercised = 0;
    size_t multiCandidateQueries = 0;

    for( const wxString& boardName : boards )
    {
        Load( boardName );

        BOOST_REQUIRE( !m_board->m_DRCCopperZonesByLayer.empty() );

        DRC_ZONE_INDEX index;
        index.Build( m_board->m_DRCCopperZonesByLayer );

        for( const auto& [layer, zones] : m_board->m_DRCCopperZonesByLayer )
        {
            for( ZONE* zone : zones )
            {
                BOX2I query = zone->GetBoundingBox();
                query.Inflate( m_board->m_DRCMaxClearance );
                query.Normalize();

                std::vector<ZONE*> expected;
                std::vector<ZONE*> actual;

                for( ZONE* candidate : zones )
                {
                    if( query.Intersects( candidate->GetBoundingBox() ) )
                        expected.push_back( candidate );
                }

                index.Query( layer, query, actual );
                BOOST_CHECK_EQUAL_COLLECTIONS( expected.begin(), expected.end(), actual.begin(), actual.end() );

                ++exercised;

                if( actual.size() > 1 )
                    ++multiCandidateQueries;
            }
        }

        // A layer the board has no copper zones on must simply come back empty
        std::vector<ZONE*> absent;
        index.Query( UNDEFINED_LAYER, BOX2I(), absent );
        BOOST_CHECK( absent.empty() );

        index.Clear();
        index.Query( F_Cu, BOX2I(), absent );
        BOOST_CHECK( absent.empty() );
    }

    BOOST_CHECK_GT( exercised, 0u );
    BOOST_CHECK_GT( multiCandidateQueries, 0u );
}


BOOST_FIXTURE_TEST_CASE( DRCZonePairEnumerationMatchesNestedLoop, PREPARED_CANDIDATES_FIXTURE )
{
    const wxString boards[] = { wxT( "fill_bad" ), wxT( "intersectingzones" ), wxT( "issue5750" ) };

    size_t totalPairs = 0;
    size_t emptyLayers = 0;
    size_t defaultBoxes = 0;
    size_t noFillEntries = 0;

    for( const wxString& boardName : boards )
    {
        Load( boardName );

        // Same pre-sort the provider does
        std::map<PCB_LAYER_ID, std::vector<size_t>> zoneIdxByLayer;

        for( size_t ii = 0; ii < m_board->m_DRCCopperZones.size(); ++ii )
        {
            ZONE* zone = m_board->m_DRCCopperZones[ii];

            if( zone->IsTeardropArea() )
                continue;

            for( PCB_LAYER_ID layer : zone->GetLayerSet() )
            {
                if( IsCopperLayer( layer ) )
                    zoneIdxByLayer[layer].push_back( ii );
            }
        }

        for( const auto& [layer, indices] : zoneIdxByLayer )
        {
            auto fillBox =
                    [&]( size_t aIndex ) -> std::optional<BOX2I>
                    {
                        if( SHAPE_POLY_SET* poly = m_board->m_DRCCopperZones[aIndex]->GetFill( layer ) )
                            return poly->BBoxFromCaches();

                        return std::nullopt;
                    };

            std::vector<std::string> expected;

            for( size_t ia = 0; ia < indices.size(); ++ia )
            {
                SHAPE_POLY_SET* polyA = m_board->m_DRCCopperZones[indices[ia]]->GetFill( layer );

                if( !polyA )
                {
                    ++noFillEntries;
                    continue;
                }

                if( polyA->BBoxFromCaches() == BOX2I() )
                    ++defaultBoxes;

                for( size_t ib = ia + 1; ib < indices.size(); ++ib )
                {
                    SHAPE_POLY_SET* polyB = m_board->m_DRCCopperZones[indices[ib]]->GetFill( layer );

                    if( !polyB || !polyA->BBoxFromCaches().Intersects( polyB->BBoxFromCaches() ) )
                        continue;

                    expected.push_back( std::to_string( indices[ia] ) + "-" + std::to_string( indices[ib] ) );
                }
            }

            std::vector<std::pair<size_t, size_t>> pairs = CollectOverlappingPairs( indices, fillBox );
            std::vector<std::string>               actual;

            for( const auto& [first, second] : pairs )
                actual.push_back( std::to_string( first ) + "-" + std::to_string( second ) );

            BOOST_CHECK_EQUAL_COLLECTIONS( expected.begin(), expected.end(), actual.begin(), actual.end() );

            totalPairs += actual.size();

            if( pairs.empty() )
                ++emptyLayers;

            std::set<size_t> participating;

            for( const auto& [first, second] : pairs )
            {
                participating.insert( first );
                participating.insert( second );
            }

            // Independent of the expected-pair comparison above, a zone the provider would build
            // a segment tree for has to be one of this layer's filled zones
            for( size_t idx : participating )
            {
                bool known = std::find( indices.begin(), indices.end(), idx ) != indices.end();

                BOOST_CHECK( known );

                // Guarded so an out-of-range index reports a clean failure instead of faulting
                if( known )
                    BOOST_CHECK( m_board->m_DRCCopperZones[idx]->GetFill( layer ) != nullptr );
            }
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format( "ZONEPAIRS totalPairs=%zu emptyLayers=%zu defaultBoxes=%zu "
                                          "noFillEntries=%zu",
                                          totalPairs, emptyLayers, defaultBoxes, noFillEntries ) );

    BOOST_CHECK_GT( totalPairs, 0u );
    BOOST_CHECK_GT( emptyLayers, 0u );

    // No fixture here produces a zone/layer with no fill or a default-origin BBoxFromCaches(),
    // so those two branches stay unasserted
    BOOST_TEST_MESSAGE( wxString::Format( "degenerate paths unreached by real fixtures: "
                                          "defaultBoxes=%zu noFillEntries=%zu",
                                          defaultBoxes, noFillEntries ) );
}

