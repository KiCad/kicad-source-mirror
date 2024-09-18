/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <sim/kibis/kibis.h>

namespace
{
std::string GetLibraryPath( const std::string& aBaseName )
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
    fn.SetName( aBaseName );
    fn.SetExt( "ibs" );
    return std::string( fn.GetFullPath().ToUTF8() );
}
} // namespace


BOOST_AUTO_TEST_SUITE( Kibis )


BOOST_AUTO_TEST_CASE( Null )
{
    KIBIS kibis;

    BOOST_TEST( !kibis.m_valid );

    // IBIS_ANY interface
    // If this isn't null, it's uninited and access will crash
    BOOST_REQUIRE( !kibis.m_reporter );

    // Doesn't crash (also doesn't do anything)
    kibis.Report( "Dummy", RPT_SEVERITY_INFO );
}


BOOST_AUTO_TEST_CASE( Load )
{
    WX_STRING_REPORTER reporter;

    std::string path = GetLibraryPath( "ibis_v1_1" );
    KIBIS       top( path, &reporter );

    BOOST_TEST_INFO( "Parsed: " << path );
    BOOST_TEST_INFO( "Reported: " << reporter.GetMessages() );

    BOOST_TEST( top.m_valid );

    KIBIS_MODEL* model = top.GetModel( "Input" );

    BOOST_REQUIRE( model != nullptr );
    BOOST_TEST_INFO( "Model: " << model->m_name );

    BOOST_TEST( model->m_name == "Input" );
    BOOST_TEST( (int) model->m_type == (int) IBIS_MODEL_TYPE::INPUT_STD );
    BOOST_TEST( (int) model->m_polarity == (int) IBIS_MODEL_POLARITY::NON_INVERTING );
    BOOST_TEST( (int) model->m_enable = (int) IBIS_MODEL_ENABLE::ACTIVE_HIGH );

    BOOST_TEST( model->HasGNDClamp() );

    KIBIS_COMPONENT* comp = top.GetComponent( "Virtual" );

    BOOST_REQUIRE( comp != nullptr );

    BOOST_TEST_INFO( "Component: " << comp->m_name );

    BOOST_TEST( comp->m_name == "Virtual" );
    BOOST_TEST( comp->m_pins.size() == 4 );
}

BOOST_AUTO_TEST_SUITE_END()