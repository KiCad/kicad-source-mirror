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

/**
 * @file
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/25900
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <symbol_library_manager.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_screen.h>
#include <lib_symbol.h>
#include <wx/filename.h>

BOOST_AUTO_TEST_SUITE( DeleteBaseSymbol )


// Deleting 74LS02 with its derived symbols removed the unrelated 74469 and left 74LS02 behind
BOOST_AUTO_TEST_CASE( DeleteBaseRemovesBaseAndDerived )
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir(), wxS( "issue25900_74xx.kicad_sym" ) );

    IO_RELEASER<SCH_IO>      plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    std::vector<LIB_SYMBOL*> originals;
    plugin->EnumerateSymbolLib( originals, fn.GetFullPath() );

    // Fill the buffer the way SYMBOL_LIBRARY_MANAGER does
    LIB_BUFFER buf( wxS( "74xx" ) );

    for( LIB_SYMBOL* symbol : originals )
    {
        auto copy = std::make_unique<LIB_SYMBOL>( *symbol );

        if( symbol->IsDerived() )
        {
            LIB_SYMBOL* parent = buf.GetSymbol( symbol->GetParent().lock()->GetName() );

            if( !parent )
            {
                auto newParent = std::make_unique<LIB_SYMBOL>( *symbol->GetParent().lock() );
                parent = newParent.get();
                buf.CreateBuffer( std::move( newParent ), std::make_unique<SCH_SCREEN>() );
            }

            copy->SetParent( parent );
        }
        else if( buf.GetSymbol( symbol->GetName() ) )
        {
            continue;
        }

        buf.CreateBuffer( std::move( copy ), std::make_unique<SCH_SCREEN>() );
    }

    BOOST_REQUIRE_EQUAL( buf.GetBuffers().size(), 8 );

    std::shared_ptr<SYMBOL_BUFFER> base = buf.GetBuffer( wxS( "74LS02" ) );
    BOOST_REQUIRE( base );

    BOOST_CHECK( buf.DeleteBuffer( *base ) );

    BOOST_REQUIRE_EQUAL( buf.GetBuffers().size(), 3 );
    BOOST_CHECK( buf.GetSymbol( wxS( "74469" ) ) );
    BOOST_CHECK( buf.GetSymbol( wxS( "74LS00" ) ) );
    BOOST_CHECK( buf.GetSymbol( wxS( "7400" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
