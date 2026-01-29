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

            // Handle pre-encoded PNG data (GTK optimization path).
            // Use wxDF_BITMAP to prevent wx from setting the type to Private
            if( entry.m_useRawPngData && entry.m_data.GetDataLen() > 0 )
            {
                wxCustomDataObject* custom = new wxCustomDataObject( wxDF_BITMAP );
                custom->Alloc( entry.m_data.GetDataLen() );
                custom->SetData( entry.m_data.GetDataLen(), entry.m_data.GetData() );
                data->Add( custom );

                continue;
            }

            // Handle bitmap image data using platform-native clipboard format.
            // wxBitmapDataObject automatically converts to the appropriate format:
            // - Windows: CF_DIB
            // - macOS: kUTTypeTIFF
            if( entry.m_image.has_value() && entry.m_image->IsOk() )
            {
                data->Add( new wxBitmapDataObject( *entry.m_image ) );
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


std::unique_ptr<wxBitmap> GetImageFromClipboard()
{
    std::unique_ptr<wxBitmap> bitmap;
    wxLogNull                 doNotLog; // disable logging of failed clipboard actions

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
                bitmap = std::make_unique<wxBitmap>( data.GetBitmap() );
            }
        }
#ifdef __APPLE__
        // macOS screenshots use PNG format with UTI "public.png"
        else if( wxDataFormat pngFormat( wxS( "public.png" ) ); wxTheClipboard->IsSupported( pngFormat ) )
        {
            wxCustomDataObject pngData( pngFormat );

            if( wxTheClipboard->GetData( pngData ) && pngData.GetSize() > 0 )
            {
                wxMemoryInputStream stream( pngData.GetData(), pngData.GetSize() );
                wxImage             img( stream, wxBITMAP_TYPE_PNG );

                if( img.IsOk() )
                    bitmap = std::make_unique<wxBitmap>( img );
            }
        }
#endif
        else if( wxTheClipboard->IsSupported( wxDF_FILENAME ) )
        {
            wxFileDataObject data;

            if( wxTheClipboard->GetData( data ) && data.GetFilenames().size() == 1 )
            {
                wxImage img( data.GetFilenames()[0] );
                bitmap = std::make_unique<wxBitmap>( img );

                if( !bitmap->IsOk() )
                    bitmap.reset();
            }
        }

        if( !wasAlreadyOpen )
            wxTheClipboard->Close();
    }

    return bitmap;
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


bool AddTransparentImageToClipboardData( wxDataObjectComposite* aData, wxImage aImage )
{
    wxCHECK( wxTheClipboard->IsOpened(), false );
    wxCHECK( aImage.IsOk(), false );

#if defined( __WXGTK__ ) || defined( __WXMSW__ )
    // On GTK, wxDF_BITMAP maps to "image/png" format. wxBitmapDataObject encodes
    // PNG twice internally (once to count size, once to save). We optimize by
    // encoding PNG once ourselves with fast compression settings.
    //
    // On MSW, most apps don't recognize transparency when using CF_DIB, so provide a PNG.

    // Fast PNG settings optimized for schematic graphics:
    // - Compression level Z_BEST_SPEED (fast)
    // - Z_RLE strategy (good for images with runs of identical pixels)
    // - PNG_FILTER_NONE (skip filtering step)

    aImage.SetOption( wxIMAGE_OPTION_PNG_COMPRESSION_LEVEL, 1 );   // Z_BEST_SPEED
    aImage.SetOption( wxIMAGE_OPTION_PNG_COMPRESSION_STRATEGY, 3 ); // Z_RLE
    aImage.SetOption( wxIMAGE_OPTION_PNG_FILTER, 0x08 );            // PNG_FILTER_NONE

    wxMemoryOutputStream   memStream;
    wxBufferedOutputStream bufferedStream( memStream );

    if( aImage.SaveFile( bufferedStream, wxBITMAP_TYPE_PNG ) )
    {
        bufferedStream.Close();

        auto stBuf = memStream.GetOutputStreamBuffer();
#ifdef __WXMSW__
        // Add empty CF_BITMAP so apps recognize the PNG entry
        aData->Add( new wxCustomDataObject( wxDataFormat( wxDataFormatId::wxDF_BITMAP ) ) );

        // Add "PNG" entry
        wxCustomDataObject* pngObj = new wxCustomDataObject( wxDataFormat( "PNG" ) );
        pngObj->SetData( stBuf->GetIntPosition(), stBuf->GetBufferStart() );
        aData->Add( pngObj );
#else // __WXGTK__

        // Handle pre-encoded PNG data (GTK optimization path).
        // Use wxDF_BITMAP to prevent wx from setting the type to Private
        wxCustomDataObject* pngObj = new wxCustomDataObject( wxDF_BITMAP );
        pngObj->SetData( stBuf->GetIntPosition(), stBuf->GetBufferStart() );
        aData->Add( pngObj );
#endif
    }
    else
    {
        wxLogDebug( wxS( "Failed to encode PNG for clipboard" ) );
        return false;
    }
#else // __WXOSX__

    // On macOS (TIFF), wxBitmapDataObject uses native formats
    // that don't require PNG encoding. Pass bitmap directly.
    aData->Add( new wxBitmapDataObject( wxBitmap( aImage ) ) );
#endif

    return true;
}