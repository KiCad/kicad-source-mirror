/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
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

#include <tools/backannotate.h>
#include <lib_symbol.h>
#include <pin_map.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <sch_sheet_path.h>

#include <map>
#include <memory>
#include <vector>


static BACKANNOTATE_UNIT_SWAP_CANDIDATE
make_candidate( const char* aRef, int aUnit, const std::vector<const char*>& aPins,
                const std::initializer_list<std::pair<const char*, const char*>>& aNets )
{
    BACKANNOTATE_UNIT_SWAP_CANDIDATE candidate;
    candidate.m_ref = wxString::FromUTF8( aRef );
    candidate.m_currentUnit = aUnit;

    for( const char* pin : aPins )
        candidate.m_unitPinNumbers.push_back( wxString::FromUTF8( pin ) );

    for( const auto& [pin, net] : aNets )
        candidate.m_schNetsByPin[wxString::FromUTF8( pin )] = wxString::FromUTF8( net );

    return candidate;
}


static std::map<wxString, wxString>
make_pin_map( const std::initializer_list<std::pair<const char*, const char*>>& aNets )
{
    std::map<wxString, wxString> pinMap;

    for( const auto& [pin, net] : aNets )
        pinMap[wxString::FromUTF8( pin )] = wxString::FromUTF8( net );

    return pinMap;
}


// aMappingOk = every candidate should be matched to exactly one destination unit pattern
// aIdentity = each candidate should map to itself
// aExpectedSwappedCandidateIndices = list of units that are going to be swapped

static void check_plan( const BACKANNOTATE_UNIT_SWAP_PLAN& aPlan, bool aMappingOk, bool aIdentity,
                        std::initializer_list<size_t> aExpectedSwappedCandidateIndices )
{
    BOOST_CHECK_EQUAL( aPlan.m_mappingOk, aMappingOk );
    BOOST_CHECK_EQUAL( aPlan.m_identity, aIdentity );
    BOOST_CHECK_EQUAL( aPlan.m_swappedCandidateIndices.size(), aExpectedSwappedCandidateIndices.size() );

    for( size_t candidateIdx : aExpectedSwappedCandidateIndices )
        BOOST_CHECK_EQUAL( aPlan.m_swappedCandidateIndices.count( candidateIdx ), 1U );
}


// Check that the final unit assignments for the candidates match the expected units after applying the swap plan.
// aExpectedUnits = rearranged list of which units positions the swapping should apply, e.g. if you
// swapped 1 and 2 in the candidates list, and the original units were { 1, 2 }, then the expected units would be { 2, 1 }.
static void check_final_units( const std::vector<BACKANNOTATE_UNIT_SWAP_CANDIDATE>& aCandidates,
                               const BACKANNOTATE_UNIT_SWAP_PLAN&                    aPlan,
                               std::initializer_list<int>                            aExpectedUnits )
{
    std::vector<int> finalUnits;

    for( const BACKANNOTATE_UNIT_SWAP_CANDIDATE& candidate : aCandidates )
        finalUnits.push_back( candidate.m_currentUnit );

    for( const BACKANNOTATE_UNIT_SWAP_STEP& step : aPlan.m_steps )
        std::swap( finalUnits[step.m_firstIndex], finalUnits[step.m_secondIndex] );

    BOOST_REQUIRE_EQUAL( finalUnits.size(), aExpectedUnits.size() );

    size_t idx = 0;

    for( int expectedUnit : aExpectedUnits )
        BOOST_CHECK_EQUAL( finalUnits[idx++], expectedUnit );
}


struct MAPPED_UNIT_SWAP_FIXTURE
{
    MAPPED_UNIT_SWAP_FIXTURE()
    {
        LIB_SYMBOL lib( wxS( "MappedUnits" ), nullptr );
        lib.SetUnitCount( 2, false );

        for( int number = 1; number <= 4; ++number )
        {
            SCH_PIN* pin = new SCH_PIN( &lib );
            pin->SetNumber( wxString::Format( wxS( "%d" ), number ) );
            pin->SetUnit( number <= 2 ? 1 : 2 );
            pin->SetPosition( VECTOR2I( 0, number * 100 ) );
            pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
            lib.AddDrawItem( pin );
        }

        PIN_MAP map( wxS( "ReversedPads" ) );
        map.SetEntry( wxS( "1" ), wxS( "3" ) );
        map.SetEntry( wxS( "2" ), wxS( "4" ) );
        map.SetEntry( wxS( "3" ), wxS( "1" ) );
        map.SetEntry( wxS( "4" ), wxS( "2" ) );
        lib.PinMaps().AddOrReplace( map );

        LIB_ID footprint;
        footprint.Parse( wxS( "Test:MappedUnits" ) );
        lib.SetAssociatedFootprints( { { footprint, wxS( "ReversedPads" ) } } );

        m_first = std::make_unique<SCH_SYMBOL>( lib, lib.GetLibId(), nullptr, 1 );
        m_second = std::make_unique<SCH_SYMBOL>( lib, lib.GetLibId(), nullptr, 2 );
        m_first->SetFootprintFieldText( footprint.Format() );
        m_second->SetFootprintFieldText( footprint.Format() );

        m_candidates = { make_candidate( "U1A", 1, { "1", "2" }, { { "1", "A_IN" }, { "2", "A_OUT" } } ),
                         make_candidate( "U1B", 2, { "3", "4" }, { { "3", "B_IN" }, { "4", "B_OUT" } } ) };
    }

    BACKANNOTATE_UNIT_SWAP_PLAN Plan( const std::map<wxString, wxString>& aPcbNetsByPad )
    {
        std::map<wxString, wxString> pcbNetsByLogicalPin;
        BOOST_REQUIRE(
                CollectBackannotatePcbNets( *m_first, m_sheet, wxEmptyString, aPcbNetsByPad, pcbNetsByLogicalPin ) );
        BOOST_REQUIRE(
                CollectBackannotatePcbNets( *m_second, m_sheet, wxEmptyString, aPcbNetsByPad, pcbNetsByLogicalPin ) );
        BOOST_REQUIRE_EQUAL( pcbNetsByLogicalPin.size(), 4U );
        return PlanBackannotateUnitSwaps( m_candidates, pcbNetsByLogicalPin );
    }

    SCH_SHEET_PATH                                m_sheet;
    std::unique_ptr<SCH_SYMBOL>                   m_first;
    std::unique_ptr<SCH_SYMBOL>                   m_second;
    std::vector<BACKANNOTATE_UNIT_SWAP_CANDIDATE> m_candidates;
};


BOOST_AUTO_TEST_SUITE( BackannotateUnitSwapPlanner )


BOOST_AUTO_TEST_CASE( BackannotateUnitSwapPlanner_DetectsIdentityMapping )
{
    std::vector<BACKANNOTATE_UNIT_SWAP_CANDIDATE> candidates =
    {
        make_candidate( "U1A", 1, { "1", "2", "3" },
                        { { "1", "Aout" }, { "2", "Net-(U1A--)" }, { "3", "A+" } } ),
        make_candidate( "U1B", 2, { "5", "6", "7" },
                        { { "5", "B+" }, { "6", "B-" }, { "7", "Bout" } } )
    };

    BACKANNOTATE_UNIT_SWAP_PLAN plan =
            PlanBackannotateUnitSwaps( candidates,
                                       make_pin_map( { { "1", "Aout" }, { "2", "Net-(U1A--)" }, { "3", "A+" },
                                                       { "5", "B+" }, { "6", "B-" }, { "7", "Bout" } } ) );

    check_plan( plan, true, true, {} );
    check_final_units( candidates, plan, { 1, 2 } );
}


BOOST_AUTO_TEST_CASE( BackannotateUnitSwapPlanner_DetectsSimpleUnitSwap )
{
    std::vector<BACKANNOTATE_UNIT_SWAP_CANDIDATE> candidates =
    {
        make_candidate( "U1A", 1, { "1", "2", "3" },
                        { { "1", "A1" }, { "2", "A2" }, { "3", "A3" } } ),
        make_candidate( "U1B", 2, { "4", "5", "6" },
                        { { "4", "B1" }, { "5", "B2" }, { "6", "B3" } } )
    };

    BACKANNOTATE_UNIT_SWAP_PLAN plan =
            PlanBackannotateUnitSwaps( candidates,
                                       make_pin_map( { { "1", "B1" }, { "2", "B2" }, { "3", "B3" },
                                                       { "4", "A1" }, { "5", "A2" }, { "6", "A3" } } ) );

    check_plan( plan, true, false, { 0, 1 } );
    check_final_units( candidates, plan, { 2, 1 } );
}


BOOST_AUTO_TEST_CASE( BackannotateUnitSwapPlanner_DetectsBAS16TWGateRotation )
{
    std::vector<BACKANNOTATE_UNIT_SWAP_CANDIDATE> candidates = {
        make_candidate( "D1", 1, { "6", "1" }, { { "6", "K1" }, { "1", "A1" } } ),
        make_candidate( "D1", 2, { "5", "2" }, { { "5", "K2" }, { "2", "A2" } } ),
        make_candidate( "D1", 3, { "4", "3" }, { { "4", "K3" }, { "3", "A3" } } )
    };

    BACKANNOTATE_UNIT_SWAP_PLAN plan = PlanBackannotateUnitSwaps(
            candidates,
            make_pin_map(
                    { { "6", "K2" }, { "1", "A2" }, { "5", "K3" }, { "2", "A3" }, { "4", "K1" }, { "3", "A1" } } ) );

    check_plan( plan, true, false, { 0, 1, 2 } );
    check_final_units( candidates, plan, { 3, 1, 2 } );
}


BOOST_AUTO_TEST_CASE( BackannotateUnitSwapPlanner_DetectsRN1GateRotationWithSharedCommonPin )
{
    std::vector<BACKANNOTATE_UNIT_SWAP_CANDIDATE> candidates = {
        make_candidate( "RN1", 1, { "1", "2" }, { { "1", "R1" }, { "2", "R1.2" } } ),
        make_candidate( "RN1", 2, { "1", "3" }, { { "1", "R1" }, { "3", "R2.2" } } ),
        make_candidate( "RN1", 3, { "1", "4" }, { { "1", "R1" }, { "4", "R3.2" } } )
    };

    BACKANNOTATE_UNIT_SWAP_PLAN plan = PlanBackannotateUnitSwaps(
            candidates,
            make_pin_map(
                    { { "1", "R1" }, { "2", "R2.2" }, { "3", "R3.2" }, { "4", "R1.2" } } ) );

    check_plan( plan, true, false, { 0, 1, 2 } );
    check_final_units( candidates, plan, { 3, 1, 2 } );
}


BOOST_AUTO_TEST_CASE( BackannotateUnitSwapPlanner_DoesNotTreatHybridBAS16TWGateAndPinSwapAsPureUnitRotation )
{
    std::vector<BACKANNOTATE_UNIT_SWAP_CANDIDATE> candidates = {
        make_candidate( "D1", 1, { "6", "1" }, { { "6", "K1" }, { "1", "A1" } } ),
        make_candidate( "D1", 2, { "5", "2" }, { { "5", "K2" }, { "2", "A2" } } ),
        make_candidate( "D1", 3, { "4", "3" }, { { "4", "K3" }, { "3", "A3" } } )
    };

    // Simulate these PCB operations in order:
    // 1. Swap diode gates B and C, so the B/C unit positions exchange their net pairs.
    // 2. After that gate swap, swap the two pads on the gate now sitting in slot B.
    //
    // The resulting footprint pad-to-net assignment is:
    //   1 -> A1
    //   2 -> K3
    //   3 -> A2
    //   4 -> K2
    //   5 -> A3
    //   6 -> K1
    //
    // This is not a pure unit rotation anymore.  Unit A still matches, unit C now matches
    // the old B gate, but unit B has its A/K pins inverted.  The unit-swap planner should
    // reject this as a complete unit mapping instead of treating it as a valid gate-only swap.
    BACKANNOTATE_UNIT_SWAP_PLAN plan = PlanBackannotateUnitSwaps(
            candidates,
            make_pin_map(
                    { { "1", "A1" }, { "2", "K3" }, { "3", "A2" }, { "4", "K2" }, { "5", "A3" }, { "6", "K1" } } ) );

    BOOST_CHECK( !plan.m_mappingOk );
    BOOST_CHECK( plan.m_steps.empty() );
    BOOST_CHECK( plan.m_swappedCandidateIndices.empty() );
}

BOOST_FIXTURE_TEST_CASE( MappedPadsAlreadyMatch, MAPPED_UNIT_SWAP_FIXTURE )
{
    // The footprint pad order differs from the logical pin order, but the nets already agree.
    // Comparing raw pad numbers would incorrectly infer an A/B unit swap here.
    const auto plan = Plan( make_pin_map( { { "1", "B_IN" }, { "2", "B_OUT" }, { "3", "A_IN" }, { "4", "A_OUT" } } ) );

    check_plan( plan, true, true, {} );
    BOOST_CHECK( plan.m_steps.empty() );
    check_final_units( m_candidates, plan, { 1, 2 } );
}


BOOST_FIXTURE_TEST_CASE( MappedPadsDetectRealSwap, MAPPED_UNIT_SWAP_FIXTURE )
{
    // Swap the physical gates on the PCB. Raw pad lookup would incorrectly call this unchanged.
    const auto plan = Plan( make_pin_map( { { "1", "A_IN" }, { "2", "A_OUT" }, { "3", "B_IN" }, { "4", "B_OUT" } } ) );

    check_plan( plan, true, false, { 0, 1 } );
    check_final_units( m_candidates, plan, { 2, 1 } );
}


BOOST_FIXTURE_TEST_CASE( IdentityOverrideKeepsOriginalPinNumbers, MAPPED_UNIT_SWAP_FIXTURE )
{
    PIN_MAP_INSTANCE_OVERRIDE pinOverride;
    pinOverride.m_Mode = PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY;
    m_first->SetPinMapOverride( pinOverride );
    m_second->SetPinMapOverride( pinOverride );

    const auto plan = Plan( make_pin_map( { { "1", "A_IN" }, { "2", "A_OUT" }, { "3", "B_IN" }, { "4", "B_OUT" } } ) );

    check_plan( plan, true, true, {} );
    check_final_units( m_candidates, plan, { 1, 2 } );
}


BOOST_FIXTURE_TEST_CASE( StackedPadsMustAgree, MAPPED_UNIT_SWAP_FIXTURE )
{
    PIN_MAP_INSTANCE_OVERRIDE pinOverride;
    pinOverride.m_Edits.push_back( { wxS( "1" ), wxS( "[3,5]" ) } );
    m_first->SetPinMapOverride( pinOverride );

    auto                         pcbNetsByPad = make_pin_map( { { "3", "A_IN" }, { "4", "A_OUT" }, { "5", "A_IN" } } );
    std::map<wxString, wxString> logicalNets;

    BOOST_REQUIRE( CollectBackannotatePcbNets( *m_first, m_sheet, wxEmptyString, pcbNetsByPad, logicalNets ) );
    BOOST_CHECK_EQUAL( logicalNets.at( wxS( "1" ) ), wxS( "A_IN" ) );

    // One logical pin cannot describe two different PCB nets.
    pcbNetsByPad[wxS( "5" )] = wxS( "OTHER_NET" );
    logicalNets.clear();
    BOOST_CHECK( !CollectBackannotatePcbNets( *m_first, m_sheet, wxEmptyString, pcbNetsByPad, logicalNets ) );

    pcbNetsByPad.erase( wxS( "5" ) );
    logicalNets.clear();
    BOOST_CHECK( !CollectBackannotatePcbNets( *m_first, m_sheet, wxEmptyString, pcbNetsByPad, logicalNets ) );
}


BOOST_AUTO_TEST_CASE( StackedLogicalPinsUseExpandedNumbers )
{
    LIB_SYMBOL lib( wxS( "StackedUnit" ), nullptr );
    SCH_PIN*   pin = new SCH_PIN( &lib );
    pin->SetNumber( wxS( "[1,2]" ) );
    pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    lib.AddDrawItem( pin );

    SCH_SYMBOL                symbol( lib, lib.GetLibId(), nullptr, 1 );
    PIN_MAP_INSTANCE_OVERRIDE pinOverride;
    pinOverride.m_Edits.push_back( { wxS( "[1,2]" ), wxS( "[8,9]" ) } );
    symbol.SetPinMapOverride( pinOverride );
    SCH_SHEET_PATH               sheet;
    std::map<wxString, wxString> logicalNets;

    BOOST_REQUIRE( CollectBackannotatePcbNets(
            symbol, sheet, wxEmptyString, make_pin_map( { { "8", "SIGNAL" }, { "9", "SIGNAL" } } ), logicalNets ) );
    BOOST_REQUIRE_EQUAL( logicalNets.size(), 2U );
    BOOST_CHECK_EQUAL( logicalNets.at( wxS( "1" ) ), wxS( "SIGNAL" ) );
    BOOST_CHECK_EQUAL( logicalNets.at( wxS( "2" ) ), wxS( "SIGNAL" ) );
}


BOOST_AUTO_TEST_SUITE_END()
