/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <board.h>
#include <board_commit.h>
#include <board_text_var_adapter.h>
#include <footprint.h>
#include <netinfo.h>
#include <pcb_barcode.h>
#include <pcb_field.h>
#include <pcb_text.h>
#include <pcbnew_utils/board_test_utils.h>
#include <text_var_dependency.h>
#include <tool/tool_manager.h>


/**
 * End-to-end reactive pipeline: BOARD commits + TEXT_VAR_TRACKER + BOARD_TEXT_VAR_ADAPTER
 * together. These tests exercise the listener-driven flow that the adapter
 * installs in the BOARD constructor.
 */
BOOST_AUTO_TEST_SUITE( BoardTextVarReactive )


BOOST_AUTO_TEST_CASE( AdapterIsInstalledOnConstruction )
{
    BOARD board;
    BOOST_REQUIRE( board.GetTextVarAdapter() );
    BOOST_CHECK_EQUAL( board.GetTextVarAdapter()->Tracker().Index().ItemCount(), 0u );
}


BOOST_AUTO_TEST_CASE( AddingTextItemRegistersDependencies )
{
    BOARD         board;
    TOOL_MANAGER  mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* tool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( tool );
    BOARD_COMMIT commit( tool );

    PCB_TEXT* text = new PCB_TEXT( &board );
    text->SetText( wxT( "${U1:VALUE}" ) );
    commit.Add( text );
    commit.Push( wxT( "add text" ), 0 );

    const TEXT_VAR_DEPENDENCY_INDEX& index = board.GetTextVarAdapter()->Tracker().Index();
    BOOST_CHECK_EQUAL(
            index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "U1:VALUE" ) ) ), 1u );
}


BOOST_AUTO_TEST_CASE( FootprintChangeFiresInvalidation )
{
    BOARD         board;
    TOOL_MANAGER  mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* tool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( tool );

    // Add a footprint U1 with default fields, and a PCB_TEXT that depends on
    // its VALUE.
    {
        BOARD_COMMIT commit( tool );
        FOOTPRINT*   fp = new FOOTPRINT( &board );
        fp->SetReference( wxT( "U1" ) );
        fp->SetValue( wxT( "10k" ) );
        commit.Add( fp );

        PCB_TEXT* text = new PCB_TEXT( &board );
        text->SetText( wxT( "${U1:Value}" ) );
        commit.Add( text );

        commit.Push( wxT( "seed" ), 0 );
    }

    // Register an invalidate callback and then edit the footprint's value.
    std::vector<std::pair<EDA_ITEM*, TEXT_VAR_REF_KEY>> invalidations;
    (void) board.GetTextVarAdapter()->Tracker().AddInvalidateListener(
            [&]( EDA_ITEM* dep, const TEXT_VAR_REF_KEY& key )
            { invalidations.emplace_back( dep, key ); } );

    FOOTPRINT* fp = board.Footprints().front();

    BOARD_COMMIT commit( tool );
    commit.Modify( fp );
    fp->SetValue( wxT( "100k" ) );
    commit.Push( wxT( "edit value" ), 0 );

    // The adapter should have fanned out ${U1:Value} among others. Look for
    // at least one invalidation that targets a PCB_TEXT dependent.
    bool sawTextDependent = std::any_of( invalidations.begin(), invalidations.end(),
                                          []( const auto& pair )
                                          { return pair.first && pair.first->Type() == PCB_TEXT_T; } );

    BOOST_CHECK( sawTextDependent );
}


BOOST_AUTO_TEST_CASE( RemovedItemsAreUnregistered )
{
    BOARD         board;
    TOOL_MANAGER  mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* tool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( tool );

    PCB_TEXT* text = nullptr;

    {
        BOARD_COMMIT commit( tool );
        text = new PCB_TEXT( &board );
        text->SetText( wxT( "${X}" ) );
        commit.Add( text );
        commit.Push( wxT( "add" ), 0 );
    }

    const TEXT_VAR_DEPENDENCY_INDEX& index = board.GetTextVarAdapter()->Tracker().Index();
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "X" ) ) ), 1u );

    {
        BOARD_COMMIT commit( tool );
        commit.Remove( text );
        commit.Push( wxT( "remove" ), 0 );
    }

    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "X" ) ) ), 0u );
}


BOOST_AUTO_TEST_CASE( SpiceOPTokensAreNotRegistered )
{
    BOARD         board;
    TOOL_MANAGER  mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* tool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( tool );
    BOARD_COMMIT commit( tool );

    PCB_TEXT* text = new PCB_TEXT( &board );
    text->SetText( wxT( "${OP:1}" ) );
    commit.Add( text );
    commit.Push( wxT( "add op" ), 0 );

    // IsTrackable() gates OP keys out of registration entirely.
    const TEXT_VAR_DEPENDENCY_INDEX& index = board.GetTextVarAdapter()->Tracker().Index();
    BOOST_CHECK_EQUAL( index.ItemCount(), 0u );
}


BOOST_AUTO_TEST_CASE( VariantSwitchFiresCrossRefInvalidation )
{
    BOARD         board;
    TOOL_MANAGER  mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* tool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( tool );

    {
        BOARD_COMMIT commit( tool );
        FOOTPRINT*   fp = new FOOTPRINT( &board );
        fp->SetReference( wxT( "U1" ) );
        fp->SetValue( wxT( "10k" ) );
        commit.Add( fp );

        PCB_TEXT* text = new PCB_TEXT( &board );
        text->SetText( wxT( "${U1:Value}" ) );
        commit.Add( text );

        commit.Push( wxT( "seed" ), 0 );
    }

    board.AddVariant( wxT( "HighPrecision" ) );

    std::vector<EDA_ITEM*> invalidations;
    (void) board.GetTextVarAdapter()->Tracker().AddInvalidateListener(
            [&]( EDA_ITEM* dep, const TEXT_VAR_REF_KEY& ) { invalidations.push_back( dep ); } );

    board.SetCurrentVariant( wxT( "HighPrecision" ) );

    // The PCB_TEXT that depends on ${U1:Value} must have been invalidated —
    // variant overrides change how that cross-ref resolves.
    bool sawPcbText = std::any_of( invalidations.begin(), invalidations.end(),
                                    []( EDA_ITEM* item )
                                    { return item && item->Type() == PCB_TEXT_T; } );
    BOOST_CHECK( sawPcbText );

    // Redundant switch to the same variant must be a no-op.
    invalidations.clear();
    board.SetCurrentVariant( wxT( "HighPrecision" ) );
    BOOST_CHECK( invalidations.empty() );
}


/**
 * Regression for issue 24100. Pasting items into a real BOARD calls RemoveAll on
 * the temporary clipboard board, which deletes its NETINFO_ITEMs via
 * NETINFO_LIST::clear() before FinalizeBulkRemove fires the listener
 * notifications. BOARD_TEXT_VAR_ADAPTER::unregisterItem then dereferences the
 * dangling pointers via dynamic_cast, segfaulting.
 */
BOOST_AUTO_TEST_CASE( RemoveAllNetInfoDoesNotUseDeletedPointers )
{
    BOARD board;

    // Seed the board with an extra net so RemoveAll has real items to remove.
    NETINFO_ITEM* net = new NETINFO_ITEM( &board, wxT( "net-1" ), 1 );
    board.Add( net );

    // Default RemoveAll() includes PCB_NETINFO_T. Listeners (including the
    // BOARD_TEXT_VAR_ADAPTER installed in the BOARD constructor) must not see
    // dangling pointers.
    BOOST_CHECK_NO_THROW( board.RemoveAll() );
}


BOOST_AUTO_TEST_CASE( BarcodeIsRegisteredOnAdd )
{
    BOARD        board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* tool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( tool );
    BOARD_COMMIT commit( tool );

    PCB_BARCODE* barcode = new PCB_BARCODE( &board );
    barcode->SetText( wxT( "${SN}" ) );
    commit.Add( barcode );
    commit.Push( wxT( "add barcode" ), 0 );

    const TEXT_VAR_DEPENDENCY_INDEX& index = board.GetTextVarAdapter()->Tracker().Index();
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "SN" ) ) ), 1u );
}


BOOST_AUTO_TEST_CASE( BarcodeInvalidationFires )
{
    BOARD        board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* tool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( tool );

    {
        BOARD_COMMIT commit( tool );
        FOOTPRINT*   fp = new FOOTPRINT( &board );
        fp->SetReference( wxT( "U1" ) );
        fp->SetValue( wxT( "10k" ) );
        commit.Add( fp );

        PCB_BARCODE* barcode = new PCB_BARCODE( &board );
        barcode->SetText( wxT( "${U1:Value}" ) );
        commit.Add( barcode );

        commit.Push( wxT( "seed" ), 0 );
    }

    std::vector<EDA_ITEM*> invalidations;
    (void) board.GetTextVarAdapter()->Tracker().AddInvalidateListener(
            [&]( EDA_ITEM* dep, const TEXT_VAR_REF_KEY& )
            {
                invalidations.push_back( dep );
            } );

    FOOTPRINT*   fp = board.Footprints().front();
    BOARD_COMMIT commit( tool );
    commit.Modify( fp );
    fp->SetValue( wxT( "100k" ) );
    commit.Push( wxT( "edit value" ), 0 );

    bool sawBarcode = std::any_of( invalidations.begin(), invalidations.end(),
                                   []( EDA_ITEM* item )
                                   {
                                       return item && item->Type() == PCB_BARCODE_T;
                                   } );
    BOOST_CHECK( sawBarcode );
}


BOOST_AUTO_TEST_SUITE_END()
