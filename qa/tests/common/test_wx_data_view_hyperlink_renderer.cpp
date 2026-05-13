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

#include <vector>
#include <wx/string.h>

#include <widgets/wx_data_view_hyperlink_renderer.h>


using RUN = HYPERLINK_DV_RENDERER::RUN;


BOOST_AUTO_TEST_SUITE( HyperlinkDvRenderer )


BOOST_AUTO_TEST_CASE( PlainText )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "plain text" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "plain text" ) );
    BOOST_CHECK( runs[0].href.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( SingleLink )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[label](https://example.com)" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "label" ) );
    BOOST_CHECK_EQUAL( runs[0].href, wxString( "https://example.com" ) );
}


BOOST_AUTO_TEST_CASE( TextLinkText )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "before [label](https://example.com) after" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 3u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "before " ) );
    BOOST_CHECK( runs[0].href.IsEmpty() );
    BOOST_CHECK_EQUAL( runs[1].text, wxString( "label" ) );
    BOOST_CHECK_EQUAL( runs[1].href, wxString( "https://example.com" ) );
    BOOST_CHECK_EQUAL( runs[2].text, wxString( " after" ) );
    BOOST_CHECK( runs[2].href.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( AdjacentLinks )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[a](https://a.test)[b](https://b.test)" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 2u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "a" ) );
    BOOST_CHECK_EQUAL( runs[0].href, wxString( "https://a.test" ) );
    BOOST_CHECK_EQUAL( runs[1].text, wxString( "b" ) );
    BOOST_CHECK_EQUAL( runs[1].href, wxString( "https://b.test" ) );
}


BOOST_AUTO_TEST_CASE( UnclosedBracket )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "unclosed [bracket" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "unclosed [bracket" ) );
    BOOST_CHECK( runs[0].href.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( BracketsNoParens )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[label] no parens" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "[label] no parens" ) );
    BOOST_CHECK( runs[0].href.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( EmptyLabel )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[](https://example.com)" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "" ) );
    BOOST_CHECK_EQUAL( runs[0].href, wxString( "https://example.com" ) );
}


BOOST_AUTO_TEST_CASE( EmptyHrefRejected )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[label]()" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "label" ) );
    BOOST_CHECK( runs[0].href.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( BalancedParensInUrl )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[doc](file:///foo (1).pdf)" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "doc" ) );
    BOOST_CHECK_EQUAL( runs[0].href, wxString( "file:///foo (1).pdf" ) );
}


BOOST_AUTO_TEST_CASE( ArrayIndexInPlainText )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "array[0] index" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "array[0] index" ) );
    BOOST_CHECK( runs[0].href.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( Empty )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxEmptyString, runs );

    BOOST_CHECK( runs.empty() );
}


BOOST_AUTO_TEST_CASE( LinkPlainLink )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[a](https://a.test) middle [b](https://b.test)" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 3u );
    BOOST_CHECK_EQUAL( runs[0].href, wxString( "https://a.test" ) );
    BOOST_CHECK_EQUAL( runs[1].text, wxString( " middle " ) );
    BOOST_CHECK( runs[1].href.IsEmpty() );
    BOOST_CHECK_EQUAL( runs[2].href, wxString( "https://b.test" ) );
}


BOOST_AUTO_TEST_CASE( NestedBracketInLabel )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[foo [bar]](https://example.com)" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "[foo [bar]](https://example.com)" ) );
    BOOST_CHECK( runs[0].href.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( UnsafeUncPathRejected )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[click](\\\\server\\share\\foo.exe)" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "click" ) );
    BOOST_CHECK( runs[0].href.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( UnsafeJavascriptRejected )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[click](javascript:alert(1))" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "click" ) );
    BOOST_CHECK( runs[0].href.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( UnsafeFileExeRejected )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[click](file:///C:/Windows/System32/cmd.exe)" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "click" ) );
    BOOST_CHECK( runs[0].href.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( SafeFilePdfAccepted )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[spec](file:///docs/spec.pdf)" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "spec" ) );
    BOOST_CHECK_EQUAL( runs[0].href, wxString( "file:///docs/spec.pdf" ) );
}


BOOST_AUTO_TEST_CASE( SafeUncPathAccepted )
{
    std::vector<RUN> runs;
    HYPERLINK_DV_RENDERER::ParseRuns( wxString( "[spec](\\\\server\\share\\foo.pdf)" ), runs );

    BOOST_REQUIRE_EQUAL( runs.size(), 1u );
    BOOST_CHECK_EQUAL( runs[0].text, wxString( "spec" ) );
    BOOST_CHECK_EQUAL( runs[0].href, wxString( "\\\\server\\share\\foo.pdf" ) );
}


BOOST_AUTO_TEST_CASE( IsSafeUrlDirect )
{
    BOOST_CHECK( HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "http://example.com" ) ) );
    BOOST_CHECK( HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "HTTPS://EXAMPLE.COM" ) ) );
    BOOST_CHECK( HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "file:///home/user/doc.pdf" ) ) );

    BOOST_CHECK( !HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "" ) ) );
    BOOST_CHECK( !HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "ftp://example.com" ) ) );
    BOOST_CHECK( !HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "javascript:alert(1)" ) ) );
    BOOST_CHECK( !HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "data:text/html,<script>" ) ) );
    BOOST_CHECK( !HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "\\\\server\\share\\foo.exe" ) ) );
    BOOST_CHECK( !HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "mailto:user@example.com" ) ) );
    BOOST_CHECK( !HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "file:///foo.exe" ) ) );
    BOOST_CHECK( !HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "file:///FOO.EXE" ) ) );
    BOOST_CHECK( !HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "file:///foo.bat" ) ) );

    BOOST_CHECK( HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "\\\\server\\share\\foo.pdf" ) ) );
    BOOST_CHECK( HYPERLINK_DV_RENDERER::IsSafeUrl( wxString( "\\\\server\\share\\Spec.PDF" ) ) );
}


BOOST_AUTO_TEST_CASE( StripMarkupPlainText )
{
    BOOST_CHECK_EQUAL( HYPERLINK_DV_RENDERER::StripMarkup( wxString( "plain text" ) ), wxString( "plain text" ) );
}


BOOST_AUTO_TEST_CASE( StripMarkupSingleLink )
{
    BOOST_CHECK_EQUAL( HYPERLINK_DV_RENDERER::StripMarkup( wxString( "See [the spec](https://foo.com/spec) here" ) ),
                       wxString( "See the spec here" ) );
}


BOOST_AUTO_TEST_CASE( StripMarkupDropsUnsafeUrl )
{
    BOOST_CHECK_EQUAL( HYPERLINK_DV_RENDERER::StripMarkup( wxString( "Run [this](\\\\evil\\foo.exe) now" ) ),
                       wxString( "Run this now" ) );
}


BOOST_AUTO_TEST_CASE( StripMarkupEmpty )
{
    BOOST_CHECK_EQUAL( HYPERLINK_DV_RENDERER::StripMarkup( wxEmptyString ), wxEmptyString );
}


BOOST_AUTO_TEST_SUITE_END()
