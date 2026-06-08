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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <board.h>
#include <footprint.h>
#include <lib_id.h>

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


BOOST_AUTO_TEST_SUITE( FootprintEditorTabs )


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


BOOST_AUTO_TEST_CASE( SettingsOpenTabsRoundTrip )
{
    FOOTPRINT_EDITOR_SETTINGS settings;

    settings.m_OpenTabs.clear();
    settings.m_OpenTabs.push_back( { wxS( "Resistors" ), wxS( "R_0402" ), false } );
    settings.m_OpenTabs.push_back( { wxS( "Caps" ), wxS( "C_0603" ), true } );
    settings.m_ActiveTab = wxS( "Caps:C_0603" );

    settings.Store();

    settings.m_OpenTabs.clear();
    settings.m_ActiveTab = wxEmptyString;

    settings.Load();

    BOOST_REQUIRE_EQUAL( settings.m_OpenTabs.size(), 2u );
    BOOST_CHECK_EQUAL( settings.m_OpenTabs[0].m_lib, wxS( "Resistors" ) );
    BOOST_CHECK_EQUAL( settings.m_OpenTabs[0].m_fpName, wxS( "R_0402" ) );
    BOOST_CHECK( !settings.m_OpenTabs[0].m_pinned );
    BOOST_CHECK_EQUAL( settings.m_OpenTabs[1].m_lib, wxS( "Caps" ) );
    BOOST_CHECK_EQUAL( settings.m_OpenTabs[1].m_fpName, wxS( "C_0603" ) );
    BOOST_CHECK( settings.m_OpenTabs[1].m_pinned );
    BOOST_CHECK_EQUAL( settings.m_ActiveTab, wxS( "Caps:C_0603" ) );
}


BOOST_AUTO_TEST_SUITE_END()
