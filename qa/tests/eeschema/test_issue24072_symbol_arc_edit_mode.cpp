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

/**
 * @file test_issue24072_symbol_arc_edit_mode.cpp
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24072
 *
 * The Symbol Editor uses SYMBOL_EDITOR_SETTINGS, not EESCHEMA_SETTINGS.  Cycling
 * the arc editing mode previously dereferenced a null EESCHEMA_SETTINGS pointer
 * via SCH_BASE_FRAME::eeconfig() and segfaulted.  This test verifies that the
 * Symbol Editor settings carry an arc edit mode that can be loaded, stored, and
 * round-tripped through the JSON parameter store.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <settings/app_settings.h>
#include <symbol_editor/symbol_editor_settings.h>


BOOST_AUTO_TEST_SUITE( Issue24072SymbolArcEditMode )


BOOST_AUTO_TEST_CASE( DefaultValueIsKeepCenter )
{
    SYMBOL_EDITOR_SETTINGS cfg;

    BOOST_CHECK( cfg.m_ArcEditMode == ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS );
}


BOOST_AUTO_TEST_CASE( ArcEditModeRoundTripsThroughJson )
{
    SYMBOL_EDITOR_SETTINGS cfg;

    cfg.m_ArcEditMode = ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION;
    BOOST_CHECK( cfg.Store() );

    std::optional<int> stored = cfg.Get<int>( "editing.arc_edit_mode" );
    BOOST_REQUIRE( stored.has_value() );
    BOOST_CHECK_EQUAL( *stored,
                       static_cast<int>( ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION ) );

    cfg.Set<int>( "editing.arc_edit_mode",
                  static_cast<int>( ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE ) );
    cfg.Load();

    BOOST_CHECK( cfg.m_ArcEditMode == ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE );
}


BOOST_AUTO_TEST_SUITE_END()
