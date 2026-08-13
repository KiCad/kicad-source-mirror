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

#ifndef KIPLATFORM_PRINTING_H_
#define KIPLATFORM_PRINTING_H_

#include <string>
#include <wx/translation.h>

class wxPrintData;

#define PRINTING_TRACE "KICAD_PRINT"
namespace KIPLATFORM
{
namespace PRINTING
{
    enum class PRINT_RESULT
    {
        OK = 0,
        CANCELLED,
        FILE_NOT_FOUND,
        FAILED_TO_LOAD,
        FAILED_TO_PRINT,
        UNSUPPORTED,
        UNKNOWN_ERROR
    };

    inline const wxString PrintResultToString( PRINT_RESULT aResult )
    {
        switch( aResult )
        {
        case PRINT_RESULT::OK:             return _( "Success" );
        case PRINT_RESULT::CANCELLED:      return _( "Cancelled" );
        case PRINT_RESULT::FILE_NOT_FOUND: return _( "File not found" );
        case PRINT_RESULT::FAILED_TO_LOAD: return _( "Failed to load PDF" );
        case PRINT_RESULT::FAILED_TO_PRINT:return _( "Failed to print" );
        case PRINT_RESULT::UNSUPPORTED:    return _( "Unsupported" );
        default:                           return _( "Unknown error" );
        }
    }

    PRINT_RESULT PrintPDF( const std::string& aFile );

    /**
     * Discard a "print to file" destination in @a aData that the platform print backend
     * generated for its own spooling, leaving a destination the user chose untouched.
     *
     * Print settings are carried from one print to the next so that the dialog reopens the way
     * the user left it. Under the GTK portal backend (Flatpak, snap, GTK_USE_PORTAL) that also
     * carries a gtkprintXXXXXX scratch path, because the portal overwrites the output URI with
     * its own temporary file before handing the settings back, and the scratch path then shows
     * up as the suggested output file. It lives in the native GtkPrintSettings output URI,
     * which the wx-level filename does not track, so it has to be inspected there.
     */
    void ResetPrintToFilePath( wxPrintData& aData );
}
} // namespace KIPLATFORM

#endif // KIPLATFORM_PRINTING_H_

