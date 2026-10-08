/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <boost/test/unit_test.hpp>

#include <compatibility_report.h>
#include <kiplatform/io.h>
#include <qa_utils/temporary_directory.h>

#include <filesystem>
#include <iterator>

#include <wx/ffile.h>


BOOST_AUTO_TEST_SUITE( CompatibilityReport )


BOOST_AUTO_TEST_CASE( TextReportIncludesTargetAndAllActions )
{
    COMPATIBILITY_REPORT report;
    report.Add( DOWNGRADE_BUCKET::BLOCK, wxS( "Pin notation" ), wxS( "Cannot represent this notation." ) );
    report.Add( DOWNGRADE_BUCKET::DROP, wxS( "Net chains" ), wxS( "Chain constraints are omitted." ), 3 );
    report.Add( DOWNGRADE_BUCKET::LOWER, wxS( "Ellipses" ), wxS( "Converted to polygons." ), 2 );
    report.Add( DOWNGRADE_BUCKET::KEEP, wxS( "Supported setting" ), wxS( "Written unchanged." ), 4 );

    const wxString expectedSections = wxS( "\nKeep: 4\n"
                                           "  Supported setting (4)\n"
                                           "    Written unchanged.\n"
                                           "\nApproximate: 2\n"
                                           "  Ellipses (2)\n"
                                           "    Converted to polygons.\n"
                                           "\nDrop: 3\n"
                                           "  Net chains (3)\n"
                                           "    Chain constraints are omitted.\n"
                                           "\nCannot export: 1\n"
                                           "  Pin notation (1)\n"
                                           "    Cannot represent this notation.\n" );

    const wxString target = wxS( "KiCad 9.0" );
    const wxString text = report.ToText( target );

    BOOST_CHECK( text.Contains( target + wxS( "\n\n" ) ) );
    BOOST_CHECK( text.Contains( expectedSections ) );
    BOOST_CHECK( report.IsBlocked() );
    BOOST_CHECK( report.IsLossy() );
}


BOOST_AUTO_TEST_CASE( TextReportIncludesMergedCountsAndDetails )
{
    COMPATIBILITY_REPORT report;
    report.Add( DOWNGRADE_BUCKET::LOWER, wxS( "Ellipses" ), wxEmptyString, 2 );
    report.Add( DOWNGRADE_BUCKET::LOWER, wxS( "Ellipses" ), wxS( "Converted to polygons." ), 3 );

    const wxString target = wxS( "KiCad 10.0" );
    wxString       text = report.ToText( target );

    BOOST_CHECK( text.Contains( target + wxS( "\n\n" ) ) );
    BOOST_CHECK( text.Contains( wxS( "\nApproximate: 5\n  Ellipses (5)\n    Converted to polygons.\n" ) ) );
    BOOST_CHECK( report.IsLossy() );
    BOOST_CHECK( !report.IsBlocked() );
    BOOST_REQUIRE_EQUAL( report.Entries().size(), 1 );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), 5 );
}


BOOST_AUTO_TEST_CASE( EmptyTextReportHasZeroCounts )
{
    COMPATIBILITY_REPORT report;
    wxString             text = report.ToText( wxS( "KiCad 9.0" ) );

    BOOST_CHECK( text.Contains( wxS( "\nKeep: 0\n\nApproximate: 0\n\nDrop: 0\n\nCannot export: 0\n" ) ) );
    BOOST_CHECK( !report.IsLossy() );
    BOOST_CHECK( !report.IsBlocked() );
}


BOOST_AUTO_TEST_CASE( KeptFeaturesAppearWithoutLoss )
{
    COMPATIBILITY_REPORT report;
    report.Add( DOWNGRADE_BUCKET::KEEP, wxS( "Supported setting" ) );

    wxString text = report.ToText( wxS( "KiCad 10.0" ) );

    BOOST_CHECK( text.Contains( wxS( "\nKeep: 1\n  Supported setting (1)\n\nApproximate: 0\n" ) ) );
    BOOST_CHECK( !report.IsLossy() );
    BOOST_CHECK( !report.IsBlocked() );
}


BOOST_AUTO_TEST_CASE( DropOnlyTextReportHasNoApproximationEntries )
{
    COMPATIBILITY_REPORT report;
    report.Add( DOWNGRADE_BUCKET::DROP, wxS( "Barcodes" ), wxS( "Omitted by the selected export policy." ), 2 );

    wxString text = report.ToText( wxS( "KiCad 9.0" ) );

    BOOST_CHECK( text.Contains( wxS( "\nApproximate: 0\n" ) ) );
    BOOST_CHECK( text.Contains( wxS( "\nDrop: 2\n  Barcodes (2)\n    Omitted by the selected export policy.\n" ) ) );
    BOOST_CHECK( report.IsLossy() );
    BOOST_CHECK( !report.IsBlocked() );
}


BOOST_AUTO_TEST_CASE( TextReportRetainsUntranslatedFeatureText )
{
    COMPATIBILITY_REPORT report;
    const wxString       feature = wxString::FromUTF8( "Réglages Ω" );
    const wxString       detail = wxString::FromUTF8( "日本語\nSecond line: 50%" );
    report.Add( DOWNGRADE_BUCKET::DROP, feature, detail );

    const wxString target = wxS( "Target 100%" );
    wxString       text = report.ToText( target );

    BOOST_CHECK( text.Contains( target + wxS( "\n\n" ) ) );
    BOOST_CHECK( text.Contains( feature ) );
    BOOST_CHECK( text.Contains( detail ) );
}


BOOST_AUTO_TEST_CASE( TextReportCanBeSavedAndReplacedAsUtf8 )
{
    KI_TEST::TEMPORARY_DIRECTORY dir( "kicad-compatibility-report-" );
    wxString                     path = wxString::FromUTF8( ( dir.GetPath() / "report.txt" ).string() );
    COMPATIBILITY_REPORT         report;
    report.Add( DOWNGRADE_BUCKET::DROP, wxString::FromUTF8( "Réglages Ω" ), wxString::FromUTF8( "日本語" ) );

    for( const wxString& target : { wxS( "KiCad 9.0" ), wxS( "KiCad 10.0" ) } )
    {
        wxString           text = report.ToText( target );
        wxScopedCharBuffer utf8 = text.ToUTF8();
        wxString           error;

        BOOST_REQUIRE( KIPLATFORM::IO::AtomicWriteFile( path, utf8.data(), utf8.length(), &error ) );
        BOOST_CHECK( error.IsEmpty() );

        wxFFile file( path, wxS( "rb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        wxString saved;
        BOOST_REQUIRE( file.ReadAll( &saved, wxConvUTF8 ) );
        BOOST_CHECK_EQUAL( saved, text );
        BOOST_CHECK_EQUAL( static_cast<size_t>( file.Length() ), utf8.length() );
    }

    BOOST_CHECK_EQUAL( std::distance( std::filesystem::directory_iterator( dir.GetPath() ),
                                      std::filesystem::directory_iterator() ),
                       1 );
}


BOOST_AUTO_TEST_SUITE_END()
