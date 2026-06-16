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

/**
 * @file test_text_variable_resolution.cpp
 * Unit tests for text variable resolution in PCB, specifically testing that
 * project text variables are resolved dynamically rather than from cached values.
 *
 * This addresses GitLab issue #14360 where text variables were not updated
 * when the project file changed externally.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <board.h>
#include <pcb_text.h>
#include <project.h>
#include <settings/settings_manager.h>


struct TEXT_VAR_RESOLUTION_FIXTURE
{
    TEXT_VAR_RESOLUTION_FIXTURE()
    {
        m_settingsMgr.LoadProject( "" );
        m_board.SetProject( &m_settingsMgr.Prj() );
    }

    std::map<wxString, wxString>& ProjectVars() { return m_settingsMgr.Prj().GetTextVars(); }

    SETTINGS_MANAGER m_settingsMgr;
    BOARD            m_board;
};


BOOST_FIXTURE_TEST_SUITE( TextVariableResolution, TEXT_VAR_RESOLUTION_FIXTURE )


/**
 * Project text variables must win over the board's cached properties so an external edit
 * (schematic, project settings) is reflected without reloading the board. This is the core
 * regression guard for issue #14360 and fails under the old properties-first ordering.
 */
BOOST_AUTO_TEST_CASE( ProjectTextVarsTakePrecedence )
{
    ProjectVars()["MYVAR"] = "project_value";

    // A stale snapshot in the board properties simulates the old load behaviour.
    std::map<wxString, wxString> boardProps;
    boardProps["MYVAR"] = "cached_board_value";
    m_board.SetProperties( boardProps );

    wxString token = "MYVAR";

    BOOST_CHECK( m_board.ResolveTextVar( &token, 0 ) );
    BOOST_CHECK_EQUAL( token, "project_value" );
}


/**
 * Updating a project text variable is reflected immediately, without a SynchronizeProperties call.
 */
BOOST_AUTO_TEST_CASE( ProjectTextVarsUpdateDynamically )
{
    ProjectVars()["MY_REVISION"] = "1.0";

    wxString token = "MY_REVISION";

    BOOST_CHECK( m_board.ResolveTextVar( &token, 0 ) );
    BOOST_CHECK_EQUAL( token, "1.0" );

    ProjectVars()["MY_REVISION"] = "2.0";
    token = "MY_REVISION";

    BOOST_CHECK( m_board.ResolveTextVar( &token, 0 ) );
    BOOST_CHECK_EQUAL( token, "2.0" );
}


/**
 * Title block fields win over both the project text variables and the board's cached properties,
 * matching the resolution order used by the schematic. The colliding project var and board
 * property exercise the title-block-over-properties ordering that this fix changed; the test fails
 * under the old properties-first ordering.
 */
BOOST_AUTO_TEST_CASE( TitleBlockVarsTakePrecedence )
{
    m_board.GetTitleBlock().SetTitle( "board_title" );
    ProjectVars()["TITLE"] = "project_title";

    std::map<wxString, wxString> boardProps;
    boardProps["TITLE"] = "cached_board_value";
    m_board.SetProperties( boardProps );

    wxString token = "TITLE";

    BOOST_CHECK( m_board.ResolveTextVar( &token, 0 ) );
    BOOST_CHECK_EQUAL( token, "board_title" );
}


/**
 * Board properties are used as a fallback when the variable is not defined in the project, keeping
 * legacy boards (and boards loaded without a project) working.
 */
BOOST_AUTO_TEST_CASE( BoardPropertiesFallback )
{
    std::map<wxString, wxString> boardProps;
    boardProps["LEGACY_VAR"] = "legacy_value";
    m_board.SetProperties( boardProps );

    wxString token = "LEGACY_VAR";

    BOOST_CHECK( m_board.ResolveTextVar( &token, 0 ) );
    BOOST_CHECK_EQUAL( token, "legacy_value" );
}


/**
 * PCB_TEXT::GetShownText resolves variables through the board with project precedence and reflects
 * a project variable update on the next call. The colliding board property guards the ordering, so
 * the test fails under the old properties-first ordering.
 */
BOOST_AUTO_TEST_CASE( PcbTextResolvesProjectVars )
{
    ProjectVars()["VERSION"] = "3.5";

    std::map<wxString, wxString> boardProps;
    boardProps["VERSION"] = "cached_board_value";
    m_board.SetProperties( boardProps );

    PCB_TEXT text( &m_board );
    text.SetText( "Version: ${VERSION}" );

    BOOST_CHECK_EQUAL( text.GetShownText( true ), "Version: 3.5" );

    ProjectVars()["VERSION"] = "4.0";

    BOOST_CHECK_EQUAL( text.GetShownText( true ), "Version: 4.0" );
}


BOOST_AUTO_TEST_SUITE_END()
