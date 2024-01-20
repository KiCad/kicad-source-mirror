/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <wx/app.h>
#include <qa_utils/wx_utils/wx_assert.h>


void wxAssertThrower( const wxString& aFile, int aLine, const wxString& aFunc,
                      const wxString& aCond, const wxString& aMsg )
{
    throw KI_TEST::WX_ASSERT_ERROR( aFile, aLine, aFunc, aCond, aMsg );
}


bool init_unit_test()
{
    boost::unit_test::framework::master_test_suite().p_name.value = "IPC API tests";

    wxApp::SetInstance( new wxAppConsole );

    bool ok = wxInitialize( boost::unit_test::framework::master_test_suite().argc,
                            boost::unit_test::framework::master_test_suite().argv );

    wxSetAssertHandler( &wxAssertThrower );

    return ok;
}


int main( int argc, char* argv[] )
{
    return boost::unit_test::unit_test_main( &init_unit_test, argc, argv );
}
