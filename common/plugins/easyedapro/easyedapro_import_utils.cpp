/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "easyedapro_import_utils.h"

#include <string_utils.h>
#include <nlohmann/json.hpp>

#include <wx/log.h>
#include <wx/stream.h>
#include <wx/zipstrm.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include <wx/txtstrm.h>


wxString EASYEDAPRO::ShortenLibName( wxString aProjectName )
{
    wxString shortenedName = aProjectName;
    shortenedName.Replace( wxS( "ProProject_" ), wxS( "" ) );
    shortenedName.Replace( wxS( "ProDocument_" ), wxS( "" ) );
    shortenedName = shortenedName.substr( 0, 10 );

    return LIB_ID::FixIllegalChars( shortenedName + wxS( "-easyedapro" ), true );
}


LIB_ID EASYEDAPRO::ToKiCadLibID( const wxString& aLibName, const wxString& aLibReference )
{
    wxString libName = LIB_ID::FixIllegalChars( aLibName, true );
    wxString libReference = EscapeString( aLibReference, CTX_LIBID );

    wxString key = !aLibName.empty() ? ( aLibName + ':' + libReference ) : libReference;

    LIB_ID libId;
    libId.Parse( key, true );

    return libId;
}


nlohmann::json EASYEDAPRO::ReadProjectFile( const wxString& aZipFileName )
{
    std::shared_ptr<wxZipEntry> entry;
    wxFFileInputStream          in( aZipFileName );
    wxZipInputStream            zip( in );

    while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
    {
        wxString name = entry->GetName();

        if( name == wxS( "project.json" ) )
        {
            wxMemoryOutputStream memos;
            memos << zip;
            wxStreamBuffer* buf = memos.GetOutputStreamBuffer();
            wxString        str( (char*) buf->GetBufferStart(), buf->GetBufferSize() );

            return nlohmann::json::parse( str );
        }
    }

    THROW_IO_ERROR( wxString::Format( _( "'%s' does not appear to be a valid EasyEDA (JLCEDA) Pro "
                                         "project file. Cannot find project.json." ),
                                      aZipFileName ) );
}


void EASYEDAPRO::IterateZipFiles(
        const wxString&                                                         aFileName,
        std::function<bool( const wxString&, const wxString&, wxInputStream& )> aCallback )
{
    std::shared_ptr<wxZipEntry> entry;
    wxFFileInputStream          in( aFileName );
    wxZipInputStream            zip( in );

    if( !zip.IsOk() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot read ZIP archive '%s'" ), aFileName ) );
    }

    while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
    {
        wxString name = entry->GetName();
        wxString baseName = name.AfterLast( '\\' ).AfterLast( '/' ).BeforeFirst( '.' );

        try
        {
            if( aCallback( name, baseName, zip ) )
                break;
        }
        catch( nlohmann::json::exception& e )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "JSON error reading '%s': %s" ), name, e.what() ) );
        }
        catch( std::exception& e )
        {
            THROW_IO_ERROR( wxString::Format( _( "Error reading '%s': %s" ), name, e.what() ) );
        }
    }
}


std::vector<nlohmann::json> EASYEDAPRO::ParseJsonLines( wxInputStream&  aInput,
                                                        const wxString& aSource )
{
    wxTextInputStream txt( aInput, wxS( " " ), wxConvUTF8 );

    int currentLine = 1;

    std::vector<nlohmann::json> lines;
    while( aInput.CanRead() )
    {
        try
        {
            nlohmann::json js = nlohmann::json::parse( txt.ReadLine() );
            lines.emplace_back( js );
        }
        catch( nlohmann::json::exception& e )
        {
            wxLogWarning( wxString::Format( _( "Cannot parse JSON line %d in '%s': %s" ),
                                            currentLine, aSource, e.what() ) );
        }

        currentLine++;
    }

    return lines;
}
