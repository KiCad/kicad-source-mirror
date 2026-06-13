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
 */

#include <boost/test/unit_test.hpp>
#include <kiplatform/app.h>
#include <mock_pgm_base.h>
#include <qa_utils/wx_utils/wx_assert.h>

#include <wx/init.h>


bool init_unit_test()
{
    SetPgm( new MOCK_PGM_BASE() );
    KIPLATFORM::APP::Init();
    boost::unit_test::framework::master_test_suite().p_name.value = "Diff/merge module tests";
    bool ok = wxInitialize();

    if( ok )
    {
        // Convert wxASSERT / wxFAIL_MSG into thrown WX_ASSERT_ERROR so QA can
        // distinguish "the contract held" from "the code logged a warning and
        // kept going". Required for explicit-contract tests like the
        // ResolvePropertyConflict both-null precondition guard.
        wxSetAssertHandler( &KI_TEST::wxAssertThrower );
    }

    return ok;
}


int main( int argc, char* argv[] )
{
    int ret = boost::unit_test::unit_test_main( &init_unit_test, argc, argv );

    wxUninitialize();
    return ret;
}
