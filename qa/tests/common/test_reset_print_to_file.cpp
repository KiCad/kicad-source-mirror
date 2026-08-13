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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <printing.h>

#include <wx/print.h>
#include <wx/cmndata.h>

#if defined( __WXGTK__ ) && wxUSE_GTKPRINT
#include <wx/gtk/print.h>
#include <gtk/gtk.h>
#endif


BOOST_AUTO_TEST_SUITE( ResetPrintToFile )


// A destination the user typed has to survive so that the print dialog reopens on it.
BOOST_AUTO_TEST_CASE( KeepsWxFilename )
{
    const wxString chosen( wxT( "/home/user/Documents/output.pdf" ) );

    wxPrintData data;
    data.SetFilename( chosen );

    KIPLATFORM::PRINTING::ResetPrintToFilePath( data );

    BOOST_CHECK_EQUAL( data.GetFilename(), chosen );
}


#ifdef __WXGTK__

BOOST_AUTO_TEST_CASE( ClearsWxSpoolFilename )
{
    wxPrintData data;
    data.SetFilename( wxT( "/tmp/gtkprintCG3W42" ) );

    KIPLATFORM::PRINTING::ResetPrintToFilePath( data );

    BOOST_CHECK( data.GetFilename().IsEmpty() );
}


// Only the whole g_mkstemp() template is a spool name, so a file the user happened to name this
// way is still theirs.
BOOST_AUTO_TEST_CASE( KeepsWxSpoolPrefixedFilename )
{
    const wxString chosen( wxT( "/tmp/gtkprint-report.pdf" ) );

    wxPrintData data;
    data.SetFilename( chosen );

    KIPLATFORM::PRINTING::ResetPrintToFilePath( data );

    BOOST_CHECK_EQUAL( data.GetFilename(), chosen );
}

#endif


#if defined( __WXGTK__ ) && wxUSE_GTKPRINT

// GTK carries the print-to-file destination in the native GtkPrintSettings output URI, which the
// wx-level filename does not track.
static void setGtkOutputUri( wxPrintData& aData, const char* aUri )
{
    wxGtkPrintNativeData* nativeData = dynamic_cast<wxGtkPrintNativeData*>( aData.GetNativeData() );
    BOOST_REQUIRE( nativeData );
    BOOST_REQUIRE( nativeData->GetPrintConfig() );

    gtk_print_settings_set( nativeData->GetPrintConfig(), GTK_PRINT_SETTINGS_OUTPUT_URI, aUri );
}


static const gchar* getGtkOutputUri( wxPrintData& aData )
{
    wxGtkPrintNativeData* nativeData = dynamic_cast<wxGtkPrintNativeData*>( aData.GetNativeData() );
    BOOST_REQUIRE( nativeData );
    BOOST_REQUIRE( nativeData->GetPrintConfig() );

    return gtk_print_settings_get( nativeData->GetPrintConfig(), GTK_PRINT_SETTINGS_OUTPUT_URI );
}


BOOST_AUTO_TEST_CASE( KeepsGtkOutputUri )
{
    wxPrintData data;
    setGtkOutputUri( data, "file:///home/user/Documents/output.pdf" );

    KIPLATFORM::PRINTING::ResetPrintToFilePath( data );

    const gchar* uri = getGtkOutputUri( data );

    BOOST_REQUIRE( uri != nullptr );
    BOOST_CHECK_EQUAL( uri, "file:///home/user/Documents/output.pdf" );
}


// The scratch file GTK spools through would otherwise become the suggested output file on the
// next print, which is issue 22985.
BOOST_AUTO_TEST_CASE( ClearsGtkSpoolUri )
{
    wxPrintData data;
    setGtkOutputUri( data, "file:///tmp/gtkprintCG3W42" );

    KIPLATFORM::PRINTING::ResetPrintToFilePath( data );

    BOOST_CHECK( getGtkOutputUri( data ) == nullptr );
}


// The underscored template is the one gtkprintjob.c uses.
BOOST_AUTO_TEST_CASE( ClearsGtkUnderscoredSpoolUri )
{
    wxPrintData data;
    setGtkOutputUri( data, "file:///tmp/gtkprint_CG3W42" );

    KIPLATFORM::PRINTING::ResetPrintToFilePath( data );

    BOOST_CHECK( getGtkOutputUri( data ) == nullptr );
}


BOOST_AUTO_TEST_CASE( KeepsGtkSpoolPrefixedOutputUri )
{
    wxPrintData data;
    setGtkOutputUri( data, "file:///tmp/gtkprint-report.pdf" );

    KIPLATFORM::PRINTING::ResetPrintToFilePath( data );

    const gchar* uri = getGtkOutputUri( data );

    BOOST_REQUIRE( uri != nullptr );
    BOOST_CHECK_EQUAL( uri, "file:///tmp/gtkprint-report.pdf" );
}

#endif


BOOST_AUTO_TEST_SUITE_END()
