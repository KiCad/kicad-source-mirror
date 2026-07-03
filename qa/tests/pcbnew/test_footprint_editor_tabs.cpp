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

#include <memory>

#include <functional>
#include <vector>

#include <board.h>
#include <footprint.h>
#include <lib_id.h>

#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <footprint_editor_tab_context.h>


/// Build an fp-holder BOARD carrying a single footprint with the given identity
static std::unique_ptr<BOARD> makeFpHolder( const wxString& aLib, const wxString& aName )
{
    auto board = std::make_unique<BOARD>();
    board->SetBoardUse( BOARD_USE::FPHOLDER );

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    fp->SetFPID( LIB_ID( aLib, aName ) );
    board->Add( fp );

    return board;
}


/// A BOARD that bumps a counter on destruction, to observe when a displaced tab's board is freed
class INSTRUMENTED_BOARD : public BOARD
{
public:
    explicit INSTRUMENTED_BOARD( int* aDtorCounter ) : m_dtorCounter( aDtorCounter ) {}

    ~INSTRUMENTED_BOARD() override
    {
        if( m_dtorCounter )
            ( *m_dtorCounter )++;
    }

private:
    int* m_dtorCounter;
};


BOOST_AUTO_TEST_SUITE( FootprintEditorTabs )


/// Reusing a preview tab must install the successor board before the displaced board is freed, so the
/// canvas VIEW never clears items from a destroyed board.
BOOST_AUTO_TEST_CASE( ReusedPreviewTabBoardOutlivesInstall )
{
    int dtorCount = 0;

    auto oldBoard = std::make_unique<INSTRUMENTED_BOARD>( &dtorCount );
    oldBoard->SetBoardUse( BOARD_USE::FPHOLDER );

    std::vector<std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>> contexts;
    contexts.push_back( std::make_unique<FOOTPRINT_EDITOR_TAB_CONTEXT>( wxS( "Lib" ), wxS( "A" ),
                                                                        std::move( oldBoard ) ) );

    auto newCtx = std::make_unique<FOOTPRINT_EDITOR_TAB_CONTEXT>(
            wxS( "Lib" ), wxS( "B" ), makeFpHolder( wxS( "Lib" ), wxS( "B" ) ) );
    FOOTPRINT_EDITOR_TAB_CONTEXT* newRaw = newCtx.get();

    int dtorCountDuringInstall = -1;

    FOOTPRINT_EDITOR_TAB_CONTEXT* installed = FOOTPRINT_EDIT_FRAME::placeReusedTabContext(
            contexts, 0, std::move( newCtx ),
            [&]() { dtorCountDuringInstall = dtorCount; } );

    // The displaced board is still alive while the successor is installed, and freed only afterwards.
    BOOST_CHECK_EQUAL( dtorCountDuringInstall, 0 );
    BOOST_CHECK_EQUAL( dtorCount, 1 );

    // The slot holds the successor, stays index-aligned, and is returned raw.
    BOOST_REQUIRE_EQUAL( contexts.size(), 1u );
    BOOST_CHECK_EQUAL( contexts[0].get(), newRaw );
    BOOST_CHECK_EQUAL( installed, newRaw );
    BOOST_CHECK_EQUAL( contexts[0]->GetName(), wxS( "B" ) );
}


BOOST_AUTO_TEST_CASE( ContextIdentityAndDirty )
{
    std::unique_ptr<BOARD> board = makeFpHolder( wxS( "Resistors" ), wxS( "R_0402" ) );
    BOARD*                 raw = board.get();

    FOOTPRINT_EDITOR_TAB_CONTEXT ctx( wxS( "Resistors" ), wxS( "R_0402" ), std::move( board ) );

    BOOST_CHECK_EQUAL( ctx.GetTabKey(), wxS( "Resistors:R_0402" ) );
    BOOST_CHECK_EQUAL( ctx.GetDisplayName(), wxS( "R_0402" ) );

    BOOST_CHECK_EQUAL( ctx.GetBoard(), raw );

    BOOST_CHECK( !ctx.IsModified() );

    ctx.SetModified( true );
    BOOST_CHECK( ctx.IsModified() );

    ctx.SetModified( false );
    BOOST_CHECK( !ctx.IsModified() );
}


BOOST_AUTO_TEST_CASE( ContextEmptyBoardIsNeverDirty )
{
    auto board = std::make_unique<BOARD>();
    board->SetBoardUse( BOARD_USE::FPHOLDER );

    FOOTPRINT_EDITOR_TAB_CONTEXT ctx( wxS( "Lib" ), wxS( "Empty" ), std::move( board ) );

    ctx.SetModified( true );
    BOOST_CHECK( !ctx.IsModified() );
}


/// An instance (board) tab owns its fp-holder board, reports IsTransient(), keys off the source
/// board footprint UUID in a namespace disjoint from library keys, and carries the board-uuid remap
BOOST_AUTO_TEST_CASE( InstanceTabContextIsTransientAndKeyedByUuid )
{
    KIID                   sourceUuid;
    std::unique_ptr<BOARD> board = makeFpHolder( wxS( "Resistors" ), wxS( "R_0402" ) );
    BOARD*                 raw = board.get();

    FOOTPRINT_EDITOR_TAB_CONTEXT ctx( sourceUuid, wxS( "R5" ), std::move( board ) );

    BOOST_CHECK( ctx.IsTransient() );
    BOOST_CHECK( ctx.IsFromBoard() );
    BOOST_CHECK_EQUAL( ctx.GetReference(), wxS( "R5" ) );
    BOOST_CHECK_EQUAL( ctx.GetSourceUuid().AsString(), sourceUuid.AsString() );
    BOOST_CHECK_EQUAL( ctx.GetBoard(), raw );
    BOOST_CHECK_EQUAL( ctx.GetDisplayName(), wxS( "R5" ) );

    BOOST_CHECK_EQUAL( ctx.GetTabKey(),
                       FOOTPRINT_EDITOR_TAB_CONTEXT::MakeInstanceTabKey( sourceUuid ) );
    BOOST_CHECK( ctx.GetTabKey() != wxS( "Resistors:R_0402" ) );
    BOOST_CHECK( ctx.GetTabKey().StartsWith( wxString( wxT( "\x01" ) ) ) );

    // The remap is mutable and survives on the context for save-back.
    KIID editorId, boardId;
    ctx.BoardFootprintUuids()[editorId] = boardId;
    BOOST_REQUIRE_EQUAL( ctx.BoardFootprintUuids().count( editorId ), 1u );
    BOOST_CHECK_EQUAL( ctx.BoardFootprintUuids()[editorId].AsString(), boardId.AsString() );
}


BOOST_AUTO_TEST_CASE( SettingsOpenTabsRoundTrip )
{
    FOOTPRINT_EDITOR_SETTINGS settings;

    settings.m_OpenTabs.clear();
    settings.m_OpenTabs.push_back( { wxS( "Resistors" ), wxS( "R_0402" ) } );
    settings.m_OpenTabs.push_back( { wxS( "Caps" ), wxS( "C_0603" ) } );
    settings.m_ActiveTab = wxS( "Caps:C_0603" );

    settings.Store();

    settings.m_OpenTabs.clear();
    settings.m_ActiveTab = wxEmptyString;

    settings.Load();

    BOOST_REQUIRE_EQUAL( settings.m_OpenTabs.size(), 2u );
    BOOST_CHECK_EQUAL( settings.m_OpenTabs[0].m_lib, wxS( "Resistors" ) );
    BOOST_CHECK_EQUAL( settings.m_OpenTabs[0].m_fpName, wxS( "R_0402" ) );
    BOOST_CHECK_EQUAL( settings.m_OpenTabs[1].m_lib, wxS( "Caps" ) );
    BOOST_CHECK_EQUAL( settings.m_OpenTabs[1].m_fpName, wxS( "C_0603" ) );
    BOOST_CHECK_EQUAL( settings.m_ActiveTab, wxS( "Caps:C_0603" ) );
}


BOOST_AUTO_TEST_SUITE_END()
