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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew_utils/board_construction_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_marker.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <widgets/ui_common.h>

#include <pcbnew_utils/board_test_utils.h>


struct COURTYARD_TEST_FIXTURE
{
    const KI_TEST::BOARD_DUMPER m_dumper;
};


BOOST_FIXTURE_TEST_SUITE( DrcCourtyardInvalid, COURTYARD_TEST_FIXTURE )


/*
 * A simple mock footprint with a set of courtyard rectangles and some other information
 */
struct COURTYARD_INVALID_TEST_FP
{
    std::string      m_refdes;   /// Footprint Ref-Des (for identifying DRC errors)
    std::vector<SEG> m_segs;     /// List of segments that will be placed on the courtyard
    VECTOR2I         m_pos;      /// Footprint position
};


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
    std::string                            m_case_name;
    std::vector<COURTYARD_INVALID_TEST_FP> m_mods;
    std::vector<COURTYARD_INVALID_INFO>    m_exp_errors;
};


// clang-format off
static const std::vector<COURTYARD_INVALID_CASE> courtyard_invalid_cases =
{
    {
        // Empty board has no footprints to be invalid
        "empty board",
        {},
        {},
    },
    {
        "single footprint, no courtyard",
        {
            {
                "U1",
                {}, // Empty courtyard layer
                { 0, 0 },
            },
        },
        {   // one error: the footprint has no courtyard
            {
                "U1",
                    DRCE_MISSING_COURTYARD,
            },
        },
    },
    {
        "single footprint, unclosed courtyard",
        {
            {
                "U1",
                { // Unclosed polygon
                    { { 0, 0 }, { 0, pcbIUScale.mmToIU( 10 ) } },
                    { { 0, pcbIUScale.mmToIU( 10 ) }, { pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) } },
                },
                { 0, 0 },
            },
        },
        {   // one error: the footprint has malformed courtyard
            {
                "U1",
                    DRCE_MALFORMED_COURTYARD,
            },
        },
    },
    {
        "single footprint, disjoint courtyard",
        {
            {
                "U1",
                { // Unclosed polygon - two disjoint segments
                    { { 0, 0 }, { 0, pcbIUScale.mmToIU( 10 ) } },
                    { { pcbIUScale.mmToIU( 10 ), 0 }, { pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) } },
                },
                { 0, 0 },
            },
        },
        {   // one error: the footprint has malformed courtyard
            {
                "U1",
                DRCE_MALFORMED_COURTYARD,
            },
        },
    },
    {
        "two footprints, one OK, one malformed",
        {
            {
                "U1",
                { // Closed polygon - triangle
                    {
                        { 0, 0 },
                        { 0, pcbIUScale.mmToIU( 10 ) },
                    },
                    {
                        { 0, pcbIUScale.mmToIU( 10 ) },
                        { pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) }
                    },
                    {
                        { pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) },
                        { 0, 0 }
                    },
                },
                { 0, 0 },
            },
            {
                "U2",
                { // Un-Closed polygon - one seg
                    {
                        { 0, 0 },
                        { 0, pcbIUScale.mmToIU( 10 ) },
                    },
                },
                { 0, 0 },
            },
        },
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
 * Construct a #FOOTPRINT to use in a courtyard test from a #COURTYARD_TEST_FP
 * definition.
 */
std::unique_ptr<FOOTPRINT> MakeInvalidCourtyardTestFP( BOARD& aBoard,
                                                       const COURTYARD_INVALID_TEST_FP& aFPDef )
{
    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( &aBoard );

    for( const SEG& seg : aFPDef.m_segs )
    {
        const PCB_LAYER_ID layer = F_CrtYd; // aRect.m_front ? F_CrtYd : B_CrtYd;
        const int          width = pcbIUScale.mmToIU( 0.1 );

        KI_TEST::DrawSegment( *footprint, seg, width, layer );
    }

    footprint->SetReference( aFPDef.m_refdes );

    // As of 2019-01-17, this has to go after adding the courtyards,
    // or all the poly sets are empty when DRC'd
    footprint->SetPosition( aFPDef.m_pos );

    return footprint;
}


std::unique_ptr<BOARD> MakeBoard( const std::vector<COURTYARD_INVALID_TEST_FP>& aTestFPDefs )
{
    auto board = std::make_unique<BOARD>();

    for( const COURTYARD_INVALID_TEST_FP& fpDef : aTestFPDefs )
    {
        std::unique_ptr<FOOTPRINT> footprint = MakeInvalidCourtyardTestFP( *board, fpDef );

        board->Add( footprint.release() );
    }

    return board;
}


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


void DoCourtyardInvalidTest( const COURTYARD_INVALID_CASE& aCase,
                             const KI_TEST::BOARD_DUMPER& aDumper )
{
    auto board = MakeBoard( aCase.m_mods );

    // Dump if env var set
    aDumper.DumpBoardToFile( *board, aCase.m_case_name );

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
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            DoCourtyardInvalidTest( c, m_dumper );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
