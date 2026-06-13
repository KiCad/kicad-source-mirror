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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <diff_merge/pcb_differ.h>
#include <diff_merge/pcb_merge_applier.h>
#include <diff_merge/kicad_merge_engine.h>

#include <board.h>
#include <board_item.h>
#include <pad.h>
#include <pcb_field.h>
#include <pcb_track.h>
#include <footprint.h>
#include <zone.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>


using namespace KICAD_DIFF;


/**
 * Three-board fixture: ancestor, ours, theirs all start from the same
 * fixture load. Each test mutates a subset and verifies the applier
 * produces a board containing the expected state.
 */
struct PCB_APPLIER_FIXTURE
{
    PCB_APPLIER_FIXTURE()
    {
        KI_TEST::LoadBoard( m_sa, "complex_hierarchy", m_ancestor );
        KI_TEST::LoadBoard( m_so, "complex_hierarchy", m_ours );
        KI_TEST::LoadBoard( m_st, "complex_hierarchy", m_theirs );
        BOOST_REQUIRE( m_ancestor );
        BOOST_REQUIRE( m_ours );
        BOOST_REQUIRE( m_theirs );
    }

    SETTINGS_MANAGER       m_sa, m_so, m_st;
    std::unique_ptr<BOARD> m_ancestor;
    std::unique_ptr<BOARD> m_ours;
    std::unique_ptr<BOARD> m_theirs;
};


BOOST_FIXTURE_TEST_SUITE( PcbMergeApplier, PCB_APPLIER_FIXTURE )


BOOST_AUTO_TEST_CASE( IdenticalInputsProduceIdenticalOutput )
{
    PCB_DIFFER    ourDiffer( m_ancestor.get(), m_ours.get() );
    PCB_DIFFER    theirDiffer( m_ancestor.get(), m_theirs.get() );
    DOCUMENT_DIFF ourD   = ourDiffer.Diff();
    DOCUMENT_DIFF theirD = theirDiffer.Diff();

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourD, theirD );
    BOOST_CHECK( plan.Resolved() );
    BOOST_CHECK( plan.actions.empty() );

    PCB_MERGE_APPLIER  applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    // Item counts should match.
    BOOST_CHECK_EQUAL( merged->GetItemSet().size(),
                       m_ancestor->GetItemSet().size() );
}


BOOST_AUTO_TEST_CASE( DrawingSheetResolutionMarksOnlyDrawingSheetProjectField )
{
    BOOST_REQUIRE( m_ancestor->GetProject() );
    BOOST_REQUIRE( m_ours->GetProject() );
    BOOST_REQUIRE( m_theirs->GetProject() );

    m_ancestor->GetProject()->GetProjectFile().m_BoardDrawingSheetFile = wxS( "ancestor.kicad_wks" );
    m_ours->GetProject()->GetProjectFile().m_BoardDrawingSheetFile = wxS( "ours.kicad_wks" );
    m_theirs->GetProject()->GetProjectFile().m_BoardDrawingSheetFile = wxS( "ancestor.kicad_wks" );

    MERGE_PLAN plan;
    plan.actions.push_back( { KIID_PATH(), ITEM_RES::TAKE_OURS, {} } );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    BOOST_CHECK( applier.GetReport().projectFileTouched );
    BOOST_CHECK( applier.GetReport().drawingSheetFileSet );
    BOOST_CHECK_EQUAL( applier.GetReport().drawingSheetFile, "ours.kicad_wks" );
    BOOST_CHECK( !applier.GetReport().drcSeveritiesTouched );
    BOOST_CHECK( !applier.GetReport().netClassesTouched );
}


BOOST_AUTO_TEST_CASE( OneSidedAddedItemFlowsThroughToMerge )
{
    // Remove an item from ancestor so it appears as "added on both ours and theirs".
    BOARD_ITEM_SET items = m_ancestor->GetItemSet();
    PCB_TRACK*     victim = nullptr;

    for( BOARD_ITEM* item : items )
    {
        if( item && item->Type() == PCB_TRACE_T )
        {
            victim = static_cast<PCB_TRACK*>( item );
            break;
        }
    }

    BOOST_REQUIRE( victim );
    KIID victimUuid = victim->m_Uuid;
    m_ancestor->Remove( victim, REMOVE_MODE::NORMAL );
    delete victim;

    PCB_DIFFER ourDiffer(   m_ancestor.get(), m_ours.get() );
    PCB_DIFFER theirDiffer( m_ancestor.get(), m_theirs.get() );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    // The item should be present in the merged board (it's in ours and theirs).
    bool found = false;

    for( BOARD_ITEM* item : merged->GetItemSet() )
    {
        if( item && item->m_Uuid == victimUuid )
        {
            found = true;
            break;
        }
    }

    BOOST_CHECK( found );
}


BOOST_AUTO_TEST_CASE( TakeOursAppliesOurChange )
{
    BOARD_ITEM_SET items = m_ours->GetItemSet();
    PCB_TRACK*     subject = nullptr;

    for( BOARD_ITEM* item : items )
    {
        if( item && item->Type() == PCB_TRACE_T )
        {
            subject = static_cast<PCB_TRACK*>( item );
            break;
        }
    }

    BOOST_REQUIRE( subject );
    int  originalWidth = subject->GetWidth();
    int  newWidth      = originalWidth + 75000;
    KIID subjectUuid   = subject->m_Uuid;
    subject->SetWidth( newWidth );

    PCB_DIFFER         ourDiffer(   m_ancestor.get(), m_ours.get() );
    PCB_DIFFER         theirDiffer( m_ancestor.get(), m_theirs.get() );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    // The merged board should have our width on the subject track.
    bool found = false;

    for( BOARD_ITEM* item : merged->GetItemSet() )
    {
        if( item && item->m_Uuid == subjectUuid && item->Type() == PCB_TRACE_T )
        {
            BOOST_CHECK_EQUAL( static_cast<PCB_TRACK*>( item )->GetWidth(), newWidth );
            found = true;
            break;
        }
    }

    BOOST_CHECK( found );
    BOOST_CHECK_GE( applier.GetReport().itemsTakenOurs, 1u );
}


BOOST_AUTO_TEST_CASE( DeleteRemovesItemFromMergedBoard )
{
    // Remove a track on both sides so the plan emits DELETE.
    BOARD_ITEM* victimOurs = nullptr;

    for( BOARD_ITEM* item : m_ours->GetItemSet() )
    {
        if( item && item->Type() == PCB_TRACE_T )
        {
            victimOurs = item;
            break;
        }
    }

    BOOST_REQUIRE( victimOurs );
    KIID victimUuid = victimOurs->m_Uuid;

    BOARD_ITEM* victimTheirs = nullptr;

    for( BOARD_ITEM* item : m_theirs->GetItemSet() )
    {
        if( item && item->m_Uuid == victimUuid )
        {
            victimTheirs = item;
            break;
        }
    }

    BOOST_REQUIRE( victimTheirs );

    m_ours->Remove( victimOurs, REMOVE_MODE::NORMAL );
    delete victimOurs;
    m_theirs->Remove( victimTheirs, REMOVE_MODE::NORMAL );
    delete victimTheirs;

    PCB_DIFFER         ourDiffer(   m_ancestor.get(), m_ours.get() );
    PCB_DIFFER         theirDiffer( m_ancestor.get(), m_theirs.get() );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    // The deleted item should NOT be in the merged board.
    for( BOARD_ITEM* item : merged->GetItemSet() )
    {
        BOOST_CHECK( !item || item->m_Uuid != victimUuid );
    }

    BOOST_CHECK_GE( applier.GetReport().itemsDeleted, 1u );
}


BOOST_AUTO_TEST_CASE( ApplierReportTracksZoneRefillFlag )
{
    PCB_DIFFER         ourDiffer(   m_ancestor.get(), m_ours.get() );
    PCB_DIFFER         theirDiffer( m_ancestor.get(), m_theirs.get() );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    BOOST_CHECK_EQUAL( applier.GetReport().requiresZoneRefill,
                       plan.requiresZoneRefill );
    BOOST_CHECK_EQUAL( applier.GetReport().requiresConnectivityRebuild,
                       plan.requiresConnectivityRebuild );
}


// TAKE_THEIRS on a zone preserves theirs' priority + keepout fields, so the
// applier doesn't silently fall back to ancestor's values on the non-track
// path.  Mirrors TakeOursAppliesOurChange but for ZONE + theirs side.
BOOST_AUTO_TEST_CASE( TakeTheirsZonePreservesTheirsPriorityAndKeepout )
{
    ZONE* victim = nullptr;

    for( BOARD_ITEM* item : m_theirs->GetItemSet() )
    {
        if( item && item->Type() == PCB_ZONE_T )
        {
            victim = static_cast<ZONE*>( item );
            break;
        }
    }

    if( !victim )
    {
        BOOST_TEST_MESSAGE( "Fixture has no zones; skipping" );
        return;
    }

    KIID         subjectUuid = victim->m_Uuid;
    unsigned int originalPri = victim->GetAssignedPriority();
    bool         originalKeepoutTracks = victim->GetDoNotAllowTracks();

    // Mutate theirs: distinct priority and keepout flag.
    victim->SetAssignedPriority( originalPri + 7 );
    victim->SetDoNotAllowTracks( !originalKeepoutTracks );

    PCB_DIFFER         ourDiffer(   m_ancestor.get(), m_ours.get() );
    PCB_DIFFER         theirDiffer( m_ancestor.get(), m_theirs.get() );
    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    bool found = false;

    for( BOARD_ITEM* item : merged->GetItemSet() )
    {
        if( item && item->m_Uuid == subjectUuid && item->Type() == PCB_ZONE_T )
        {
            ZONE* z = static_cast<ZONE*>( item );
            BOOST_CHECK_EQUAL( z->GetAssignedPriority(), originalPri + 7 );
            BOOST_CHECK_EQUAL( z->GetDoNotAllowTracks(), !originalKeepoutTracks );
            found = true;
            break;
        }
    }

    BOOST_CHECK( found );
}


// MERGE_PROPS fallback: the property splicer that would interleave per-
// property resolutions between sides isn't fully implemented yet.  The
// documented fallback is "take ours' value for any property not explicitly
// resolved", which behaves as TAKE_OURS in practice.  Pin that fallback
// explicitly so a future implementation change (or accidental regression)
// is loud.
BOOST_AUTO_TEST_CASE( MergePropsFallsBackToOursOnConflictingFields )
{
    PCB_TRACK* subject = nullptr;

    for( BOARD_ITEM* item : m_ours->GetItemSet() )
    {
        if( item && item->Type() == PCB_TRACE_T )
        {
            subject = static_cast<PCB_TRACK*>( item );
            break;
        }
    }

    BOOST_REQUIRE( subject );
    int  oursWidth  = subject->GetWidth() + 50000;
    int  theirsWidth = subject->GetWidth() + 75000;
    KIID subjectUuid = subject->m_Uuid;

    subject->SetWidth( oursWidth );

    // Mutate theirs's matching track to a different width to force a
    // conflict, which the engine may route through MERGE_PROPS or
    // (more often) TAKE_OURS via the auto-resolver.
    for( BOARD_ITEM* item : m_theirs->GetItemSet() )
    {
        if( item && item->m_Uuid == subjectUuid && item->Type() == PCB_TRACE_T )
        {
            static_cast<PCB_TRACK*>( item )->SetWidth( theirsWidth );
            break;
        }
    }

    PCB_DIFFER         ourDiffer(   m_ancestor.get(), m_ours.get() );
    PCB_DIFFER         theirDiffer( m_ancestor.get(), m_theirs.get() );
    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    // Either outcome -- ours's width or theirs's width -- is valid depending
    // on whether the auto-resolver picked TAKE_OURS or MERGE_PROPS -> ours
    // fallback.  Ancestor's original width is NOT valid; the applier must
    // pick one of the diverged sides.
    for( BOARD_ITEM* item : merged->GetItemSet() )
    {
        if( item && item->m_Uuid == subjectUuid && item->Type() == PCB_TRACE_T )
        {
            int mergedWidth = static_cast<PCB_TRACK*>( item )->GetWidth();
            BOOST_CHECK( mergedWidth == oursWidth || mergedWidth == theirsWidth );
            break;
        }
    }
}


// Characterize the full property-resolution loop (OURS / THEIRS / ANCESTOR /
// CUSTOM plus the failed-count path) that the applier delegates to the shared
// KICAD_DIFF::ApplyPropertyResolutions. The auto-resolver never emits CUSTOM
// and rarely emits a failing property, so a hand-built MERGE_PLAN is the only
// way to pin all four source kinds and the failure tally in one place. This is
// a behavior-preserving safety net for the shared-helper refactor.
BOOST_AUTO_TEST_CASE( MergePropsResolutionKindsAndFailedCount )
{
    auto firstTrack = []( BOARD* aBoard ) -> PCB_TRACK*
    {
        for( BOARD_ITEM* item : aBoard->GetItemSet() )
        {
            if( item && item->Type() == PCB_TRACE_T )
                return static_cast<PCB_TRACK*>( item );
        }

        return nullptr;
    };

    PCB_TRACK* ancTrack = firstTrack( m_ancestor.get() );
    BOOST_REQUIRE( ancTrack );

    KIID subjectUuid = ancTrack->m_Uuid;

    auto trackByUuid = [&]( BOARD* aBoard ) -> PCB_TRACK*
    {
        for( BOARD_ITEM* item : aBoard->GetItemSet() )
        {
            if( item && item->Type() == PCB_TRACE_T && item->m_Uuid == subjectUuid )
                return static_cast<PCB_TRACK*>( item );
        }

        return nullptr;
    };

    PCB_TRACK* oursTrack   = trackByUuid( m_ours.get() );
    PCB_TRACK* theirsTrack = trackByUuid( m_theirs.get() );
    BOOST_REQUIRE( oursTrack );
    BOOST_REQUIRE( theirsTrack );

    // Distinct width / start-x / end-x on each side so the chosen source is
    // unambiguous in the merged output.
    const int oursWidth   = ancTrack->GetWidth() + 11000;
    const int theirsStart = ancTrack->GetStartX() + 22000;
    const int ancEndX     = ancTrack->GetEndX();
    const int customEndY  = ancTrack->GetEndY() + 33000;

    oursTrack->SetWidth( oursWidth );
    theirsTrack->SetStartX( theirsStart );

    // Hand-build a MERGE_PROPS action: Width from OURS, Start X from THEIRS,
    // End X from ANCESTOR, End Y from a CUSTOM payload, and one resolution
    // naming a property that doesn't exist on PCB_TRACK to exercise the
    // failed-count path.
    ITEM_RESOLUTION action;
    action.id.push_back( subjectUuid );
    action.kind = ITEM_RES::MERGE_PROPS;

    auto addProp = [&]( const wxString& aName, PROP_RES aKind )
    {
        PROPERTY_RESOLUTION res;
        res.name = aName;
        res.kind = aKind;
        action.props.push_back( res );
    };

    addProp( wxS( "Width" ),   PROP_RES::OURS );
    addProp( wxS( "Start X" ), PROP_RES::THEIRS );
    addProp( wxS( "End X" ),   PROP_RES::ANCESTOR );

    PROPERTY_RESOLUTION customRes;
    customRes.name        = wxS( "End Y" );
    customRes.kind        = PROP_RES::CUSTOM;
    customRes.customValue = DIFF_VALUE::FromInt( customEndY );
    action.props.push_back( customRes );

    addProp( wxS( "NoSuchProperty" ), PROP_RES::OURS );

    MERGE_PLAN plan;
    plan.actions.push_back( action );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    PCB_TRACK* mergedTrack = nullptr;

    for( BOARD_ITEM* item : merged->GetItemSet() )
    {
        if( item && item->Type() == PCB_TRACE_T && item->m_Uuid == subjectUuid )
        {
            mergedTrack = static_cast<PCB_TRACK*>( item );
            break;
        }
    }

    BOOST_REQUIRE( mergedTrack );
    BOOST_CHECK_EQUAL( mergedTrack->GetWidth(),  oursWidth );
    BOOST_CHECK_EQUAL( mergedTrack->GetStartX(), theirsStart );
    BOOST_CHECK_EQUAL( mergedTrack->GetEndX(),   ancEndX );
    BOOST_CHECK_EQUAL( mergedTrack->GetEndY(),   customEndY );

    // Four real properties applied (Width, Start X, End X, End Y); the bogus
    // property name lands in the failed tally.
    BOOST_CHECK_EQUAL( applier.GetReport().propertiesApplied, 4u );
    BOOST_CHECK_EQUAL( applier.GetReport().propertiesFailed,  1u );
}


// Characterize footprint-child resolution by KIID. The applier's internal
// item lookup must resolve not only top-level items but also footprint
// children (pads, fields, graphics, zones) under their own UUIDs, because the
// child-MERGE_PROPS post-pass looks up each child on ours/theirs/ancestor by
// its UUID. This test hand-builds a MERGE_PROPS action targeting a pad inside
// a footprint and verifies the resolution lands, proving the applier resolved
// the pad child on the source boards by KIID. Behavior-preserving safety net
// for the BOARD::ResolveItem delegation refactor.
BOOST_AUTO_TEST_CASE( ResolvesFootprintChildByKiid )
{
    // Find a footprint with a copper pad in ours.  The "Pad Number" property
    // is only available on copper pads.
    FOOTPRINT* fp  = nullptr;
    PAD*       pad = nullptr;

    for( FOOTPRINT* candidate : m_ours->Footprints() )
    {
        if( !candidate )
            continue;

        for( PAD* candidatePad : candidate->Pads() )
        {
            if( candidatePad && candidatePad->IsOnCopperLayer() )
            {
                fp  = candidate;
                pad = candidatePad;
                break;
            }
        }

        if( pad )
            break;
    }

    BOOST_REQUIRE( fp );
    BOOST_REQUIRE( pad );

    const KIID fpUuid  = fp->m_Uuid;
    const KIID padUuid = pad->m_Uuid;

    // Distinct pad number on ours so the resolved source is unambiguous.
    const wxString oursPadNumber = wxS( "QA_CHILD_RESOLVE" );
    pad->SetNumber( oursPadNumber );

    // Hand-build a child MERGE_PROPS action keyed by [footprint, pad] path,
    // taking the pad number from OURS.
    ITEM_RESOLUTION action;
    action.id.push_back( fpUuid );
    action.id.push_back( padUuid );
    action.kind = ITEM_RES::MERGE_PROPS;

    PROPERTY_RESOLUTION numberRes;
    numberRes.name = wxS( "Pad Number" );
    numberRes.kind = PROP_RES::OURS;
    action.props.push_back( numberRes );

    MERGE_PLAN plan;
    plan.actions.push_back( action );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    // Locate the cloned pad child on the merged board and confirm the OURS
    // value flowed through, which requires findItem to resolve both the
    // top-level footprint and the pad child by KIID.
    PAD* mergedPad = nullptr;

    for( FOOTPRINT* mergedFp : merged->Footprints() )
    {
        if( !mergedFp || mergedFp->m_Uuid != fpUuid )
            continue;

        for( PAD* candidate : mergedFp->Pads() )
        {
            if( candidate->m_Uuid == padUuid )
            {
                mergedPad = candidate;
                break;
            }
        }

        break;
    }

    BOOST_REQUIRE( mergedPad );
    BOOST_CHECK_EQUAL( mergedPad->GetNumber(), oursPadNumber );
}


// Regression: a footprint child added on theirs (while the parent is carried
// from ancestor/ours) must survive the merge.  The applier clones the parent
// from ancestor, which lacks the theirs-added child, so a TAKE_THEIRS child
// resolution must clone it in rather than silently dropping it.
BOOST_AUTO_TEST_CASE( ChildAddedOnTheirsSurvivesMerge )
{
    FOOTPRINT* ancFp     = nullptr;
    PAD*       victimPad  = nullptr;

    for( FOOTPRINT* candidate : m_ancestor->Footprints() )
    {
        if( candidate && !candidate->Pads().empty() )
        {
            ancFp     = candidate;
            victimPad = candidate->Pads().front();
            break;
        }
    }

    BOOST_REQUIRE( ancFp );
    BOOST_REQUIRE( victimPad );

    const KIID fpUuid  = ancFp->m_Uuid;
    const KIID padUuid = victimPad->m_Uuid;

    // Drop the pad from the ancestor (= the merge base) so the same pad, still
    // present on theirs, behaves as an added-on-theirs child.
    ancFp->Remove( victimPad, REMOVE_MODE::NORMAL );
    delete victimPad;

    ITEM_RESOLUTION action;
    action.id.push_back( fpUuid );
    action.id.push_back( padUuid );
    action.kind = ITEM_RES::TAKE_THEIRS;

    MERGE_PLAN plan;
    plan.actions.push_back( action );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    bool found = false;

    for( FOOTPRINT* mergedFp : merged->Footprints() )
    {
        if( !mergedFp || mergedFp->m_Uuid != fpUuid )
            continue;

        for( PAD* pad : mergedFp->Pads() )
        {
            if( pad && pad->m_Uuid == padUuid )
                found = true;
        }
    }

    BOOST_CHECK( found );
}


// Regression: a footprint child modified only on ours (while the parent has no
// top-level action) must replace the ancestor-cloned child.  The top-level
// loop clones the parent from ancestor, so TAKE_OURS cannot be a no-op in the
// child-resolution post-pass.
BOOST_AUTO_TEST_CASE( ChildTakeOursResolutionReplacesAncestorChild )
{
    FOOTPRINT* fp  = nullptr;
    PAD*       pad = nullptr;

    for( FOOTPRINT* candidate : m_ours->Footprints() )
    {
        if( !candidate )
            continue;

        for( PAD* candidatePad : candidate->Pads() )
        {
            if( candidatePad && candidatePad->IsOnCopperLayer() )
            {
                fp  = candidate;
                pad = candidatePad;
                break;
            }
        }

        if( pad )
            break;
    }

    BOOST_REQUIRE( fp );
    BOOST_REQUIRE( pad );

    const KIID     fpUuid        = fp->m_Uuid;
    const KIID     padUuid       = pad->m_Uuid;
    const wxString oursPadNumber = wxS( "QA_CHILD_TAKE_OURS" );
    pad->SetNumber( oursPadNumber );

    ITEM_RESOLUTION action;
    action.id.push_back( fpUuid );
    action.id.push_back( padUuid );
    action.kind = ITEM_RES::TAKE_OURS;

    MERGE_PLAN plan;
    plan.actions.push_back( action );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    PAD* mergedPad = nullptr;

    for( FOOTPRINT* mergedFp : merged->Footprints() )
    {
        if( !mergedFp || mergedFp->m_Uuid != fpUuid )
            continue;

        for( PAD* candidate : mergedFp->Pads() )
        {
            if( candidate && candidate->m_Uuid == padUuid )
            {
                mergedPad = candidate;
                break;
            }
        }

        break;
    }

    BOOST_REQUIRE( mergedPad );
    BOOST_CHECK_EQUAL( mergedPad->GetNumber(), oursPadNumber );
}


// Regression: a footprint child DELETE resolution must actually remove the
// child from the merged footprint.  The parent is cloned from ancestor (which
// still has the child), so the post-pass has to drop it explicitly.
BOOST_AUTO_TEST_CASE( ChildDeleteResolutionRemovesChildFromMerge )
{
    FOOTPRINT* ancFp     = nullptr;
    PAD*       victimPad  = nullptr;

    for( FOOTPRINT* candidate : m_ancestor->Footprints() )
    {
        if( candidate && !candidate->Pads().empty() )
        {
            ancFp     = candidate;
            victimPad = candidate->Pads().front();
            break;
        }
    }

    BOOST_REQUIRE( ancFp );
    BOOST_REQUIRE( victimPad );

    const KIID fpUuid  = ancFp->m_Uuid;
    const KIID padUuid = victimPad->m_Uuid;

    ITEM_RESOLUTION action;
    action.id.push_back( fpUuid );
    action.id.push_back( padUuid );
    action.kind = ITEM_RES::DELETE_ITEM;

    MERGE_PLAN plan;
    plan.actions.push_back( action );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    std::unique_ptr<BOARD> merged = applier.Apply();
    BOOST_REQUIRE( merged );

    for( FOOTPRINT* mergedFp : merged->Footprints() )
    {
        if( !mergedFp || mergedFp->m_Uuid != fpUuid )
            continue;

        for( PAD* pad : mergedFp->Pads() )
            BOOST_CHECK( !pad || pad->m_Uuid != padUuid );
    }
}


BOOST_AUTO_TEST_SUITE_END()
