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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "clipboard.h"

#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/image.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/string.h>
#include <wx/sstream.h>

#include <sstream>

#include <io/csv.h>


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


bool SaveClipboard( const std::string& aTextUTF8, const std::vector<CLIPBOARD_MIME_DATA>& aMimeData )
{
    if( aMimeData.empty() )
        return SaveClipboard( aTextUTF8 );

    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( wxTheClipboard->Open() )
    {
        wxDataObjectComposite* data = new wxDataObjectComposite();
        data->Add( new wxTextDataObject( wxString( aTextUTF8.c_str(), wxConvUTF8 ) ), true );

        for( const CLIPBOARD_MIME_DATA& entry : aMimeData )
        {
            // Skip entries with no data (check both buffer and image)
            if( entry.m_data.GetDataLen() == 0 && !entry.m_image.has_value() )
                continue;

            // For PNG data, add as wxBitmapDataObject for compatibility with image
            // applications like GIMP that expect standard bitmap clipboard format.
            // We use wxBitmapDataObject instead of custom format because it's more
            // widely supported and avoids clipboard format query issues.
            if( entry.m_mimeType == wxS( "image/png" ) )
            {
                wxImage image;

                // Use pre-decoded image if available (avoids expensive PNG decode)
                if( entry.m_image.has_value() && entry.m_image->IsOk() )
                {
                    image = *entry.m_image;
                }
                else if( entry.m_data.GetDataLen() > 0 )
                {
                    wxMemoryInputStream stream( entry.m_data.GetData(), entry.m_data.GetDataLen() );
                    image.LoadFile( stream, wxBITMAP_TYPE_PNG );
                }

                if( image.IsOk() )
                {
                    data->Add( new wxBitmapDataObject( wxBitmap( image ) ) );
                }

                // Don't also add as custom format - wxBitmapDataObject is sufficient
                // for image apps, and adding both can cause clipboard query issues
                continue;
            }

            // Add custom format data - note that on GTK, custom MIME types may not work
            // with all applications (they often become wxDF_PRIVATE internally), but they
            // work for KiCad-to-KiCad transfers.
            // We allocate and set data in a way that ensures the object is fully initialized.
            wxDataFormat format( entry.m_mimeType );
            wxCustomDataObject* custom = new wxCustomDataObject( format );

            // Allocate buffer first, then copy data - this ensures proper initialization
            custom->Alloc( entry.m_data.GetDataLen() );
            custom->SetData( entry.m_data.GetDataLen(), entry.m_data.GetData() );
            data->Add( custom );
        }

        wxTheClipboard->SetData( data );
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

    // Check if clipboard is already open. This can happen if another event handler (such as
    // wxTextEntry::CanPaste() called during UPDATE_UI processing) opened the clipboard and
    // didn't close it. We handle this by reusing the existing session.
    bool wasAlreadyOpen = wxTheClipboard->IsOpened();
    bool isOpen = wasAlreadyOpen || wxTheClipboard->Open();

    if( isOpen )
    {
        if( wxTheClipboard->IsSupported( wxDataFormat( wxS( "application/kicad" ) ) ) )
        {
            wxCustomDataObject data( wxDataFormat( wxS( "application/kicad" ) ) );

            if( wxTheClipboard->GetData( data ) )
            {
                result.assign( static_cast<const char*>( data.GetData() ), data.GetSize() );

                if( !wasAlreadyOpen )
                    wxTheClipboard->Close();

                return result;
            }
        }

        if( wxTheClipboard->IsSupported( wxDF_TEXT )
            || wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
        {
            wxTextDataObject data;
            wxTheClipboard->GetData( data );

            // The clipboard is expected containing a Unicode string, so return it
            // as UTF8 string
            result = data.GetText().utf8_str();
        }

        if( !wasAlreadyOpen )
            wxTheClipboard->Close();
    }

    return result;
}


std::unique_ptr<wxImage> GetImageFromClipboard()
{
    std::unique_ptr<wxImage> image;
    wxLogNull                doNotLog; // disable logging of failed clipboard actions

    // Check if clipboard is already open (same handling as GetClipboardUTF8)
    bool wasAlreadyOpen = wxTheClipboard->IsOpened();
    bool isOpen = wasAlreadyOpen || wxTheClipboard->Open();

    if( isOpen )
    {
        if( wxTheClipboard->IsSupported( wxDF_BITMAP ) )
        {
            wxBitmapDataObject data;

            if( wxTheClipboard->GetData( data ) )
            {
                image = std::make_unique<wxImage>( data.GetBitmap().ConvertToImage() );
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

        if( !wasAlreadyOpen )
            wxTheClipboard->Close();
    }

    return image;
}


bool SaveTabularDataToClipboard( const std::vector<std::vector<wxString>>& aData )
{
    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( wxTheClipboard->Open() )
    {
        wxDataObjectComposite* data = new wxDataObjectComposite();

        // Set plain text CSV
        {
            wxStringOutputStream os;
            CSV_WRITER           writer( os );
            writer.WriteLines( aData );

            data->Add( new wxTextDataObject( os.GetString() ), true );
        }

        // At this point, it would be great if we could add some format that spreadsheet
        // programs can understand without asking the user for options: perhaps SYLK or DIF.
        // But it doesn't seem like WX allows to put arbitrary MIME types on the clipboard,
        // even with wxCustomDataObject( wxDataFormat( "mime/type" ) ), which just ends up as
        // wxDF_PRIVATE, and wxDF_SYLK/DIF aren't mapped on GTK.

        wxTheClipboard->SetData( data );
        wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
        wxTheClipboard->Close();

        return true;
    }

    return false;
}


bool GetTabularDataFromClipboard( std::vector<std::vector<wxString>>& aData )
{
    // Again, it would be ideal if we could detect a spreadsheet mimetype here,
    // but WX doesn't seem to do that on Linux, at least.

    bool ok = false;

    // Check if clipboard is already open (same handling as GetClipboardUTF8)
    bool wasAlreadyOpen = wxTheClipboard->IsOpened();
    bool isOpen = wasAlreadyOpen || wxTheClipboard->Open();

    if( isOpen )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
        {
            wxTextDataObject data;

            if( wxTheClipboard->GetData( data ) )
            {
                ok = AutoDecodeCSV( data.GetText(), aData );
            }
        }

        // We could also handle .csv wxDF_FILENAMEs here

        if( !wasAlreadyOpen )
            wxTheClipboard->Close();
    }

    return ok;
}
