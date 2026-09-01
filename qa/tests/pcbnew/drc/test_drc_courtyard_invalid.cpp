/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <pcbnew_utils/board_file_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_marker.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <widgets/ui_common.h>



BOOST_AUTO_TEST_SUITE( DrcCourtyardInvalid )


struct COURTYARD_INVALID_INFO
{
    std::string m_refdes;
    int         m_drc_error_code;
};


std::ostream& operator<<( std::ostream& os, const COURTYARD_INVALID_INFO& aInvalid )
{
    os << "COURTYARD_INVALID_INFO[ " << aInvalid.m_refdes;
    os << ", code: " << aInvalid.m_drc_error_code << "]";
    return os;
}


struct COURTYARD_INVALID_CASE
{
    std::string                         m_board_name;  // Fixture under drc_courtyard/invalid/
    std::vector<COURTYARD_INVALID_INFO> m_exp_errors;
};


// clang-format off
static const std::vector<COURTYARD_INVALID_CASE> courtyard_invalid_cases =
{
    {
        // Empty board has no footprints to be invalid
        "empty_board",
        {},
    },
    {
        "single_footprint_no_courtyard",
        {   // one error: the footprint has no courtyard
            {
                "U1",
                DRCE_MISSING_COURTYARD,
            },
        },
    },
    {
        "single_footprint_unclosed_courtyard",
        {   // one error: the footprint has malformed courtyard
            {
                "U1",
                DRCE_MALFORMED_COURTYARD,
            },
        },
    },
    {
        "single_footprint_disjoint_courtyard",
        {   // one error: the footprint has malformed courtyard
            {
                "U1",
                DRCE_MALFORMED_COURTYARD,
            },
        },
    },
    {
        "two_footprints_one_malformed",
        {   // one error: the second footprint has malformed courtyard
            {
                "U2",
                DRCE_MALFORMED_COURTYARD,
            },
        },
    },
};
// clang-format on


/**
 * Check if a #PCB_MARKER is described by a particular #COURTYARD_INVALID_INFO object.
 */
static bool InvalidMatchesExpected( BOARD& aBoard, const PCB_MARKER& aMarker,
                                    const COURTYARD_INVALID_INFO& aInvalid )
{
    auto reporter = std::static_pointer_cast<DRC_ITEM>( aMarker.GetRCItem() );
    const FOOTPRINT* item_a = dynamic_cast<FOOTPRINT*>( aBoard.ResolveItem( reporter->GetMainItemID() ) );

    // This one is more than just a mismatch!
    if( reporter->GetAuxItemID() != niluuid )
    {
        BOOST_WARN_MESSAGE( false, "Expected no auxiliary item for invalid courtyard DRC." );
        return false;
    }

    if( item_a == nullptr )
    {
        BOOST_ERROR( "Could not get board DRC item." );
        return false;
    }

    if( item_a->GetReference() != aInvalid.m_refdes )
        return false;

    if( reporter->GetErrorCode() != aInvalid.m_drc_error_code )
        return false;

    return true;
}


/**
 * Check that the produced markers match the expected. This does NOT
 * check ordering, as that is not part of the contract of the DRC function.
 *
 * @param aMarkers    list of markers produced by the DRC
 * @param aCollisions list of expected collisions
 */
static void CheckInvalidsMatchExpected( BOARD& aBoard,
                                        const std::vector<std::unique_ptr<PCB_MARKER>>& aMarkers,
                                        const std::vector<COURTYARD_INVALID_INFO>& aExpInvalids )
{
    KI_TEST::CheckUnorderedMatches( aExpInvalids, aMarkers,
            [&]( const COURTYARD_INVALID_INFO& aInvalid,
                 const std::unique_ptr<PCB_MARKER>& aMarker )
            {
                return InvalidMatchesExpected( aBoard, *aMarker, aInvalid );
            } );
}


void DoCourtyardInvalidTest( const COURTYARD_INVALID_CASE& aCase )
{
    const std::string path = KI_TEST::GetPcbnewTestDataDir() + "drc_courtyard/invalid/"
                             + aCase.m_board_name + ".kicad_pcb";

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( path );

    BOOST_REQUIRE( board );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    // do the overlap tests - that's a different test, but if not set,
    // the invalid courtyard checks don't run either
    bds.m_DRCSeverities[ DRCE_OVERLAPPING_FOOTPRINTS ] = RPT_SEVERITY_ERROR;

    // we will also check for missing courtyards here
    bds.m_DRCSeverities[ DRCE_MISSING_COURTYARD ] = RPT_SEVERITY_ERROR;

    // list of markers to collect
    std::vector<std::unique_ptr<PCB_MARKER>> markers;

    DRC_ENGINE drcEngine( board.get(), &board->GetDesignSettings() );

    drcEngine.InitEngine( wxFileName() );

    drcEngine.SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if(    aItem->GetErrorCode() == DRCE_OVERLAPPING_FOOTPRINTS
                    || aItem->GetErrorCode() == DRCE_MALFORMED_COURTYARD
                    || aItem->GetErrorCode() == DRCE_MISSING_COURTYARD )
                {
                    markers.push_back( std::make_unique<PCB_MARKER>( aItem, aPos ) );
                }
            } );

    drcEngine.RunTests( EDA_UNITS::MM, true, false );

    CheckInvalidsMatchExpected( *board, markers, aCase.m_exp_errors );
}


BOOST_AUTO_TEST_CASE( InvalidCases )
{
    for( const auto& c : courtyard_invalid_cases )
    {
        BOOST_TEST_CONTEXT( c.m_board_name )
        {
            DoCourtyardInvalidTest( c );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
