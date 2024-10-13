/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "clipboard.h"

#include <wx/clipbrd.h>
#include <wx/image.h>
#include <wx/log.h>


bool SaveClipboard( const std::string& aTextUTF8 )
{
    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( wxTheClipboard->Open() )
    {
        // Store the UTF8 string as Unicode string in clipboard:
        wxTheClipboard->SetData(
                new wxTextDataObject( wxString( aTextUTF8.c_str(), wxConvUTF8 ) ) );

        wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
        wxTheClipboard->Close();

        return true;
    }

    return false;
}


std::string GetClipboardUTF8()
{
    std::string result;

    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( wxTheClipboard->Open() )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT )
            || wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
        {
            wxTextDataObject data;
            wxTheClipboard->GetData( data );

            // The clipboard is expected containing a Unicode string, so return it
            // as UTF8 string
            result = data.GetText().utf8_str();
        }

        wxTheClipboard->Close();
    }

    return result;
}


std::unique_ptr<wxImage> GetImageFromClipboard()
{
    std::unique_ptr<wxImage> image;
    wxLogNull                doNotLog; // disable logging of failed clipboard actions

    // First try for an image
    if( wxTheClipboard->Open() )
    {
        if( wxTheClipboard->IsSupported( wxDF_BITMAP ) )
        {
            wxImageDataObject data;
            if( wxTheClipboard->GetData( data ) )
            {
                image = std::make_unique<wxImage>( data.GetImage() );
            }
        }
        else if( wxTheClipboard->IsSupported( wxDF_FILENAME ) )
        {
            wxFileDataObject data;
            if( wxTheClipboard->GetData( data ) && data.GetFilenames().size() == 1 )
            {
                image = std::make_unique<wxImage>( data.GetFilenames()[0] );

                if( !image->IsOk() )
                    image.reset();
            }
        }

        wxTheClipboard->Close();
    }

    return image;
}