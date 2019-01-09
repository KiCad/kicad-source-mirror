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

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <sch_io_mgr.h>
#include <kiway.h>

#include <data/fixtures_eagle_plugin.h>

/**
 * Checks that the SCH_IO manager finds the Eagle plugin
 */
BOOST_AUTO_TEST_CASE( FindPlugin )
{
    BOOST_CHECK( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EAGLE ) != NULL );
}

/**
 *
 */
BOOST_AUTO_TEST_CASE( Load )
{
    SCH_PLUGIN* pi = SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EAGLE );

    pi->Load("/home/alejandro/Proyectos/kicad/kicad-alejandro/eeschema/qa/data/eagle_schematics/empty.sch",
             NULL);
}
