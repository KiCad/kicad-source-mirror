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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <libeval_compiler/libeval_compiler.h>


BOOST_AUTO_TEST_SUITE( LibevalStringLiteral )


BOOST_AUTO_TEST_CASE( EscapedTextCompilesBackUnchanged )
{
    for( const wxString& text : { wxString( wxS( "it's" ) ), wxString( wxS( "a\\'b" ) ), wxString( wxS( "a\\b" ) ) } )
    {
        BOOST_TEST_CONTEXT( text )
        {
            std::optional<wxString> literal = LIBEVAL::EscapeStringLiteral( text );
            BOOST_REQUIRE( literal );

            LIBEVAL::COMPILER compiler;
            LIBEVAL::UCODE    code;
            LIBEVAL::CONTEXT  context;

            BOOST_REQUIRE( compiler.Compile( wxS( "'" ) + *literal + wxS( "'" ), &code, &context ) );
            BOOST_CHECK_EQUAL( code.Run( &context )->AsString(), text );
        }
    }

    BOOST_CHECK( !LIBEVAL::EscapeStringLiteral( wxS( "BPECL2\\" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
