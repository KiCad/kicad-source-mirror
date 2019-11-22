/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file
 * Test suite for IO_MGR functions
 */

#include <unit_test_utils/unit_test_utils.h>

#include <class_module.h>
#include <kicad_plugin.h>


BOOST_AUTO_TEST_SUITE( IoMgr )


BOOST_AUTO_TEST_CASE( SimpleLookup )
{
    PLUGIN* plugin = IO_MGR::PluginFind( IO_MGR::KICAD_SEXP );

    BOOST_CHECK_NE( nullptr, plugin );

    // hand it back to the manager
    IO_MGR::PluginRelease( plugin );
}

BOOST_AUTO_TEST_CASE( TestReleaser )
{
    PLUGIN::RELEASER owned_plugin( IO_MGR::PluginFind( IO_MGR::KICAD_SEXP ) );

    // check we can get to it
    BOOST_CHECK_NE( nullptr, (PLUGIN*) owned_plugin );

    // no release required
}

BOOST_AUTO_TEST_CASE( TestReleaserRelease )
{
    PLUGIN::RELEASER owned_plugin( IO_MGR::PluginFind( IO_MGR::KICAD_SEXP ) );

    // check we can get to it
    BOOST_CHECK_NE( nullptr, (PLUGIN*) owned_plugin );

    // no release required, but if we do, it shouldn't explode
    owned_plugin.release();
}

BOOST_AUTO_TEST_CASE( TestReleaserMove )
{
    PLUGIN*          orig_plugin = IO_MGR::PluginFind( IO_MGR::KICAD_SEXP );
    PLUGIN::RELEASER owned_plugin( orig_plugin );

    // check we could get to it
    BOOST_CHECK_EQUAL( orig_plugin, (PLUGIN*) owned_plugin );

    PLUGIN::RELEASER r2;

    // null at start
    BOOST_CHECK_EQUAL( nullptr, (PLUGIN*) r2 );

    r2 = std::move( owned_plugin );

    // did we transfer it ok?
    BOOST_CHECK_EQUAL( orig_plugin, (PLUGIN*) r2 );
    BOOST_CHECK_EQUAL( nullptr, (PLUGIN*) owned_plugin );

    // no release required, for either one
}

BOOST_AUTO_TEST_SUITE_END()
