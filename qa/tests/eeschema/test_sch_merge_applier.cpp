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
#include <schematic_utils/schematic_file_util.h>

#include <diff_merge/sch_differ.h>
#include <diff_merge/sch_merge_applier.h>
#include <diff_merge/kicad_merge_engine.h>

#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>


using namespace KICAD_DIFF;


struct SCH_APPLIER_FIXTURE
{
    SCH_APPLIER_FIXTURE()
    {
        KI_TEST::LoadSchematic( m_sa, "issue18606/issue18606", m_ancestor );
        KI_TEST::LoadSchematic( m_so, "issue18606/issue18606", m_ours );
        KI_TEST::LoadSchematic( m_st, "issue18606/issue18606", m_theirs );
        BOOST_REQUIRE( m_ancestor );
        BOOST_REQUIRE( m_ours );
        BOOST_REQUIRE( m_theirs );
    }

    SETTINGS_MANAGER           m_sa, m_so, m_st;
    std::unique_ptr<SCHEMATIC> m_ancestor;
    std::unique_ptr<SCHEMATIC> m_ours;
    std::unique_ptr<SCHEMATIC> m_theirs;
};


BOOST_FIXTURE_TEST_SUITE( SchMergeApplier, SCH_APPLIER_FIXTURE )


BOOST_AUTO_TEST_CASE( EmptyPlanIsNoOp )
{
    SCH_DIFFER    ourDiffer(   m_ancestor.get(), m_ours.get() );
    SCH_DIFFER    theirDiffer( m_ancestor.get(), m_theirs.get() );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );
    BOOST_CHECK( plan.actions.empty() );

    SCH_MERGE_APPLIER applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    BOOST_CHECK( applier.Apply() );

    // Differ from result against ours should still be empty.
    SCH_DIFFER    sanity( m_ancestor.get(), m_ours.get() );
    DOCUMENT_DIFF sanityResult = sanity.Diff();
    BOOST_CHECK( sanityResult.Empty() );
}


BOOST_AUTO_TEST_CASE( DrawingSheetResolutionMarksOnlyDrawingSheetProjectField )
{
    BOOST_REQUIRE( m_ancestor->IsValid() );
    BOOST_REQUIRE( m_ours->IsValid() );
    BOOST_REQUIRE( m_theirs->IsValid() );

    m_ancestor->Settings().m_SchDrawingSheetFileName = wxS( "ancestor.kicad_wks" );
    m_ours->Settings().m_SchDrawingSheetFileName = wxS( "ours.kicad_wks" );
    m_theirs->Settings().m_SchDrawingSheetFileName = wxS( "ancestor.kicad_wks" );

    MERGE_PLAN plan;
    plan.actions.push_back( { KIID_PATH(), ITEM_RES::TAKE_OURS, {} } );

    SCH_MERGE_APPLIER applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    BOOST_CHECK( applier.Apply() );

    BOOST_CHECK_EQUAL( m_ancestor->Settings().m_SchDrawingSheetFileName, "ours.kicad_wks" );
    BOOST_CHECK( applier.GetReport().projectFileTouched );
    BOOST_CHECK( applier.GetReport().drawingSheetFileTouched );
    BOOST_CHECK( !applier.GetReport().ercSeveritiesTouched );
}


BOOST_AUTO_TEST_CASE( TakeOursAppliesOurChange )
{
    // Mutate ours to give the engine a one-sided modification to propagate.
    SCH_SHEET_LIST sheets = m_ours->BuildSheetListSortedByPageNumbers();
    SCH_SYMBOL*    subject = nullptr;
    KIID           subjectUuid;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            subject     = static_cast<SCH_SYMBOL*>( item );
            subjectUuid = subject->m_Uuid;
            break;
        }

        if( subject )
            break;
    }

    BOOST_REQUIRE( subject );
    VECTOR2I origPos = subject->GetPosition();
    subject->SetPosition( origPos + VECTOR2I( 5000, 0 ) );

    SCH_DIFFER    ourDiffer(   m_ancestor.get(), m_ours.get() );
    SCH_DIFFER    theirDiffer( m_ancestor.get(), m_theirs.get() );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );
    BOOST_REQUIRE( !plan.actions.empty() );

    SCH_MERGE_APPLIER applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    BOOST_CHECK( applier.Apply() );

    // After applying, ancestor should contain a symbol with the same UUID and
    // moved position.
    SCH_SYMBOL* found = nullptr;

    for( const SCH_SHEET_PATH& path : m_ancestor->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            if( item->m_Uuid == subjectUuid )
                found = static_cast<SCH_SYMBOL*>( item );
        }
    }

    BOOST_REQUIRE( found );
    BOOST_CHECK_EQUAL( found->GetPosition().x, origPos.x + 5000 );
}


BOOST_AUTO_TEST_CASE( DeleteRemovesItem )
{
    // Find and delete the same symbol from both ours and theirs so the plan
    // emits DELETE.
    KIID victimUuid;

    auto findFirstSymbol = []( SCHEMATIC* aSch ) -> SCH_SYMBOL*
    {
        for( const SCH_SHEET_PATH& path : aSch->BuildSheetListSortedByPageNumbers() )
        {
            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
                return static_cast<SCH_SYMBOL*>( item );
        }

        return nullptr;
    };

    SCH_SYMBOL* victimOurs = findFirstSymbol( m_ours.get() );
    BOOST_REQUIRE( victimOurs );
    victimUuid = victimOurs->m_Uuid;

    auto removeByUuid = []( SCHEMATIC* aSch, const KIID& aUuid )
    {
        for( const SCH_SHEET_PATH& path : aSch->BuildSheetListSortedByPageNumbers() )
        {
            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                if( item->m_Uuid == aUuid )
                {
                    path.LastScreen()->DeleteItem( item );
                    return;
                }
            }
        }
    };

    removeByUuid( m_ours.get(),   victimUuid );
    removeByUuid( m_theirs.get(), victimUuid );

    SCH_DIFFER    ourDiffer(   m_ancestor.get(), m_ours.get() );
    SCH_DIFFER    theirDiffer( m_ancestor.get(), m_theirs.get() );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );

    SCH_MERGE_APPLIER applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    BOOST_CHECK( applier.Apply() );

    // Ancestor should no longer have a symbol with that UUID.
    for( const SCH_SHEET_PATH& path : m_ancestor->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            BOOST_CHECK( item->m_Uuid != victimUuid );
        }
    }

    BOOST_CHECK_GE( applier.GetReport().itemsDeleted, 1u );
}


// MERGE_PROPS splicer on an orthogonal-edits conflict: ours mutates X,
// theirs mutates Y of the same symbol's position.  The applier's per-
// property splicer is expected to combine the two deltas (X from ours,
// Y from theirs) rather than picking a single side wholesale.  Documents
// the actual MERGE_PROPS behaviour as observed in practice.
BOOST_AUTO_TEST_CASE( OrthogonalSymbolPositionEditsSpliceViaMergeProps )
{
    SCH_SHEET_LIST sheets = m_ours->BuildSheetListSortedByPageNumbers();
    SCH_SYMBOL*    oursSubject = nullptr;
    KIID           subjectUuid;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            oursSubject = static_cast<SCH_SYMBOL*>( item );
            subjectUuid = oursSubject->m_Uuid;
            break;
        }

        if( oursSubject )
            break;
    }

    BOOST_REQUIRE( oursSubject );
    VECTOR2I origPos = oursSubject->GetPosition();
    oursSubject->SetPosition( origPos + VECTOR2I( 1000, 0 ) );

    // Mutate the matching symbol on theirs to a different position to force
    // a conflict.
    for( const SCH_SHEET_PATH& path : m_theirs->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            if( item->m_Uuid == subjectUuid )
            {
                static_cast<SCH_SYMBOL*>( item )->SetPosition( origPos + VECTOR2I( 0, 1000 ) );
                break;
            }
        }
    }

    SCH_DIFFER         ourDiffer(   m_ancestor.get(), m_ours.get() );
    SCH_DIFFER         theirDiffer( m_ancestor.get(), m_theirs.get() );
    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );

    SCH_MERGE_APPLIER applier( m_ancestor.get(), m_ours.get(), m_theirs.get(), plan );
    BOOST_CHECK( applier.Apply() );

    // The merged ancestor's symbol must take one of the diverged sides;
    // ancestor's original position is a regression signal.
    SCH_SYMBOL* found = nullptr;

    for( const SCH_SHEET_PATH& path : m_ancestor->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            if( item->m_Uuid == subjectUuid )
                found = static_cast<SCH_SYMBOL*>( item );
        }
    }

    BOOST_REQUIRE( found );

    // Position decomposes into scalar X / Y properties.  Ours touched X
    // (delta +1000, 0), theirs touched Y (delta 0, +1000).  The MERGE_PROPS
    // splicer applies each delta to its respective field, producing the
    // hybrid (+1000, +1000).
    BOOST_CHECK_EQUAL( found->GetPosition().x, origPos.x + 1000 );
    BOOST_CHECK_EQUAL( found->GetPosition().y, origPos.y + 1000 );
}


BOOST_AUTO_TEST_SUITE_END()
