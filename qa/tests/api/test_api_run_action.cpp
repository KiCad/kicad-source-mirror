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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <tool/action_manager.h>
#include <tool/tool_manager.h>


BOOST_AUTO_TEST_SUITE( ApiRunAction )


// The IPC API RunAction command hands a client-supplied name straight to TOOL_MANAGER, so an
// unknown one has to come back as a plain false. Asserting instead put a crash dialog in front
// of users on the assert-enabled builds we ship (Zoho 2466, Sentry KICAD-106E)
BOOST_AUTO_TEST_CASE( UnknownActionNameIsRejectedNotAsserted )
{
    TOOL_MANAGER toolMgr;

    // The name a client actually sent; the registered spelling below shows it never existed
    BOOST_CHECK( !toolMgr.RunAction( "pcbnew.Control.syncSelection" ) );
    BOOST_CHECK( !toolMgr.RunAction( "" ) );

    // Guards the checks above against passing because nothing is registered at all
    BOOST_REQUIRE( toolMgr.GetActionManager()->FindAction(
            "pcbnew.InteractiveSelection.SyncSelection" ) );
}


BOOST_AUTO_TEST_SUITE_END()
