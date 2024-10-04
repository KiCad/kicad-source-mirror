/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <core/ignore.h>
#include <kiway.h>
#include <sch_io/sch_io.h>

#include "eeschema_test_utils.h"

/**
 * Checks that the SCH_IO manager finds the Eagle plugin
 */
BOOST_AUTO_TEST_CASE( FindPlugin )
{
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EAGLE ) );
    BOOST_CHECK_NE( pi.get(), nullptr );
}


/**
 * Get a schematic file from the test data eagle subdir
 */
static wxFileName getEagleTestSchematic( const wxString& sch_file )
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( "io" );
    fn.AppendDir( "eagle" );
    fn.SetFullName( sch_file );

    return fn;
}


/**
 * Check that a file can be loaded.
 */
BOOST_AUTO_TEST_CASE( Load )
{
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EAGLE ) );

    const auto fn = getEagleTestSchematic( "eagle-import-testfile.sch" );

    BOOST_TEST_MESSAGE( fn.GetFullPath() );

    ignore_unused( pi );
    // This doesn't work with a null KiWay.
    // const SCH_SHEET* sheet = pi->Load( fn.GetFullPath(), nullptr );
    // BOOST_CHECK_NE( nullptr, sheet );
}
