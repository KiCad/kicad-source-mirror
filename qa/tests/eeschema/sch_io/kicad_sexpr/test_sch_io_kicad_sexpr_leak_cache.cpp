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
#include "eeschema_test_utils.h"

#include <lib_symbol.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>

#include <wx/filename.h>


BOOST_AUTO_TEST_SUITE( SchIoKicadSexprLeakCache )


// Threads such as an HTTP library refresher may still read the cache after the exit hook runs
BOOST_AUTO_TEST_CASE( CacheSurvivesLeakCacheAtExit )
{
    wxFileName libPath( KI_TEST::GetEeschemaTestDataDir() );
    libPath.AppendDir( "variants" );
    libPath.SetFullName( "pic_programmer.kicad_sym" );

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    LIB_SYMBOL* before = pi->LoadSymbol( libPath.GetFullPath(), wxS( "74LS125" ) );
    BOOST_REQUIRE( before );

    pi->LeakCacheAtExit();

    BOOST_CHECK_EQUAL( pi->LoadSymbol( libPath.GetFullPath(), wxS( "74LS125" ) ), before );
}


BOOST_AUTO_TEST_SUITE_END()
