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

#include <boost/test/unit_test.hpp>

#include <printing.h>

#include <wx/print.h>
#include <wx/cmndata.h>

#if defined( __WXGTK__ ) && wxUSE_GTKPRINT
#include <wx/gtk/print.h>
#include <gtk/gtk.h>
#endif


BOOST_AUTO_TEST_SUITE( ResetPrintToFile )


// The wx-level filename is always cleared, on every platform.
BOOST_AUTO_TEST_CASE( ClearsWxFilename )
{
    wxPrintData data;
    data.SetFilename( wxT( "/tmp/gtkprintXXXXXX" ) );

    KIPLATFORM::PRINTING::ResetPrintToFilePath( data );

    BOOST_CHECK( data.GetFilename().IsEmpty() );
}


#if defined( __WXGTK__ ) && wxUSE_GTKPRINT

// The reported bug is that GTK stores the print-to-file destination in the native
// GtkPrintSettings output URI, which the wx-level filename does not track. Seed the URI the
// way a completed print-to-file operation would and confirm the reset clears it.
BOOST_AUTO_TEST_CASE( ClearsGtkOutputUri )
{
    wxPrintData data;

    wxGtkPrintNativeData* nativeData = dynamic_cast<wxGtkPrintNativeData*>( data.GetNativeData() );
    BOOST_REQUIRE( nativeData );
    BOOST_REQUIRE( nativeData->GetPrintConfig() );

    gtk_print_settings_set( nativeData->GetPrintConfig(), GTK_PRINT_SETTINGS_OUTPUT_URI,
                            "file:///tmp/gtkprintXXXXXX" );

    BOOST_REQUIRE( gtk_print_settings_get( nativeData->GetPrintConfig(),
                                           GTK_PRINT_SETTINGS_OUTPUT_URI ) != nullptr );

    KIPLATFORM::PRINTING::ResetPrintToFilePath( data );

    BOOST_CHECK( gtk_print_settings_get( nativeData->GetPrintConfig(),
                                         GTK_PRINT_SETTINGS_OUTPUT_URI ) == nullptr );
}

#endif


BOOST_AUTO_TEST_SUITE_END()
