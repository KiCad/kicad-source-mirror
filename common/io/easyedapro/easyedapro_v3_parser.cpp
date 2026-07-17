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

#include "easyedapro_v3_parser.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <set>

#include <io/easyedapro/easyedapro_parser.h>

#include <core/json_serializers.h>
#include <json_common.h>
#include <ki_exception.h>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>


using EASYEDAPRO::V3_DOC_RAW;
using EASYEDAPRO::V3_ROW;


static std::string ToStdString( const wxString& aStr )
{
    return std::string( aStr.ToUTF8() );
}


namespace EASYEDAPRO
{

wxString V3JsonToString( const nlohmann::json& aValue, const wxString& aDefault )
{
    if( aValue.is_string() )
        return aValue.get<wxString>();

    if( aValue.is_number_integer() )
        return wxString::Format( "%lld", static_cast<long long>( aValue.get<int64_t>() ) );

    if( aValue.is_number_unsigned() )
        return wxString::Format( "%llu", static_cast<unsigned long long>( aValue.get<uint64_t>() ) );

    if( aValue.is_number_float() )
        return wxString::FromDouble( aValue.get<double>() );

    if( aValue.is_boolean() )
        return aValue.get<bool>() ? wxS( "true" ) : wxS( "false" );

    return aDefault;
}


wxString V3GetString( const nlohmann::json& aObj, const char* aKey, const wxString& aDefault )
{
    if( !aObj.is_object() )
        return aDefault;

    auto it = aObj.find( aKey );

    if( it == aObj.end() )
        return aDefault;

    return V3JsonToString( *it, aDefault );
}


wxString V3GetString( const nlohmann::json& aObj, const wxString& aKey, const wxString& aDefault )
{
    return V3GetString( aObj, ToStdString( aKey ).c_str(), aDefault );
}


double V3GetDouble( const nlohmann::json& aObj, const char* aKey, double aDefault )
{
    if( !aObj.is_object() )
        return aDefault;
        
    auto it = aObj.find( aKey );

    if( it == aObj.end() )
        return aDefault;

    if( it->is_number() )
        return it->get<double>();

    if( it->is_string() )
    {
        double value;

        if( it->get<wxString>().ToCDouble( &value ) )
            return value;
    }

    return aDefault;
}


int V3GetInt( const nlohmann::json& aObj, const char* aKey, int aDefault )
{
    if( !aObj.is_object() )
        return aDefault;

    auto it = aObj.find( aKey );

    if( it == aObj.end() )
        return aDefault;

    if( it->is_number_integer() )
        return it->get<int>();

    if( it->is_number_float() )
        return static_cast<int>( std::lround( it->get<double>() ) );

    if( it->is_boolean() )
        return it->get<bool>() ? 1 : 0;

    if( it->is_string() )
    {
        long value = 0;

        if( it->get<wxString>().ToLong( &value ) )
            return static_cast<int>( value );
    }

    return aDefault;
}


bool V3GetBool( const nlohmann::json& aObj, const char* aKey, bool aDefault )
{
    if( !aObj.is_object() )
        return aDefault;
        
    auto it = aObj.find( aKey );

    if( it == aObj.end() )
        return aDefault;

    if( it->is_boolean() )
        return it->get<bool>();

    if( it->is_number() )
        return it->get<double>() != 0.0;

    if( it->is_string() )
    {
        wxString s = it->get<wxString>();

        if( s.CmpNoCase( wxS( "true" ) ) == 0 )
            return true;

        if( s.CmpNoCase( wxS( "false" ) ) == 0 )
            return false;

        long value = 0;

        if( s.ToLong( &value ) )
            return value != 0;
    }

    return aDefault;
}


bool V3IsNullOrMissing( const nlohmann::json& aObj, const char* aKey )
{
    if( !aObj.is_object() )
        return true;

    auto it = aObj.find( aKey );

    return it == aObj.end() || it->is_null();
}


nlohmann::json V3ParseIdArray( const wxString& aId )
{
    if( aId.empty() )
        return nlohmann::json();

    try
    {
        return nlohmann::json::parse( ToStdString( aId ) );
    }
    catch( ... )
    {
        return nlohmann::json();
    }
}

} // namespace EASYEDAPRO


namespace
{

// Helper functions for internal use
static inline wxString GetString( const nlohmann::json& aObj, const char* aKey,
                                  const wxString& aDefault = wxEmptyString )
{
    return EASYEDAPRO::V3GetString( aObj, aKey, aDefault );
}

static inline int GetInt( const nlohmann::json& aObj, const char* aKey, int aDefault = 0 )
{
    return EASYEDAPRO::V3GetInt( aObj, aKey, aDefault );
}


static wxString ParseDocType( const nlohmann::json& aHead )
{
    return GetString( aHead, "docType" );
}


static wxString ParseDocUuid( const nlohmann::json& aHead )
{
    return GetString( aHead, "uuid" );
}


static void ParseEpruStream( wxInputStream& aInput, const wxString& aSource,
                             std::vector<V3_DOC_RAW>& aDocs )
{
    wxTextInputStream txt( aInput, wxS( " " ), wxConvUTF8 );

    int                      currentLine = 1;
    std::optional<V3_DOC_RAW> currentDoc;

    auto flushDoc = [&]()
    {
        if( currentDoc )
        {
            aDocs.push_back( std::move( *currentDoc ) );
            currentDoc.reset();
        }
    };

    while( aInput.CanRead() )
    {
        wxString rawLine = txt.ReadLine();

        if( rawLine.empty() )
        {
            currentLine++;
            continue;
        }

        if( rawLine.Last() == '|' )
            rawLine.RemoveLast();

        int sepPos = rawLine.Find( wxS( "||" ) );

        if( sepPos < 0 )
        {
            currentLine++;
            continue;
        }

        wxString outerStr = rawLine.Left( sepPos );
        wxString innerStr = rawLine.Mid( sepPos + 2 );

        try
        {
            nlohmann::json outer = nlohmann::json::parse( ToStdString( outerStr ) );
            nlohmann::json inner;

            if( !innerStr.empty() )
                inner = nlohmann::json::parse( ToStdString( innerStr ) );

            wxString type = GetString( outer, "type" );

            if( type == wxS( "DOCHEAD" ) )
            {
                flushDoc();

                V3_DOC_RAW doc;
                doc.head = inner;
                doc.docType = ParseDocType( inner );
                doc.uuid = ParseDocUuid( inner );

                if( doc.docType.empty() || doc.uuid.empty() )
                {
                    THROW_IO_ERROR( wxString::Format( _( "Invalid DOCHEAD in '%s' at line %d" ),
                                                      aSource, currentLine ) );
                }

                currentDoc = std::move( doc );
                currentLine++;
                continue;
            }

            if( !currentDoc )
            {
                THROW_IO_ERROR( wxString::Format( _( "Expected DOCHEAD in '%s' at line %d" ),
                                                  aSource, currentLine ) );
            }

            V3_ROW row;
            row.outer = std::move( outer );
            row.inner = std::move( inner );
            row.type = type;
            row.id = GetString( row.outer, "id" );
            row.ticket = GetInt( row.outer, "ticket", 0 );

            wxString dedupId = row.id;

            if( dedupId.empty() )
                dedupId = wxString::Format( "__line_%d", currentLine );

            auto it = currentDoc->rowById.find( dedupId );

            if( it == currentDoc->rowById.end() )
            {
                currentDoc->rowById[dedupId] = currentDoc->rows.size();
                currentDoc->rows.emplace_back( std::move( row ) );
            }
            else if( row.ticket >= currentDoc->rows[it->second].ticket )
            {
                currentDoc->rows[it->second] = std::move( row );
            }
        }
        catch( nlohmann::json::exception& e )
        {
            THROW_IO_ERROR( wxString::Format( _( "JSON error in '%s' at line %d: %s" ),
                                              aSource, currentLine, e.what() ) );
        }

        currentLine++;
    }

    flushDoc();
}


struct V3_LIBRARY_CONTENTS
{
    bool               hasIndex = false;
    bool               hasElibu = false;
    std::set<wxString> docTypes;
};


static bool HasValidV3LibraryContents( const V3_LIBRARY_CONTENTS& aContents, const wxString& aRequiredDocType )
{
    if( !aContents.hasIndex || !aContents.hasElibu )
        return false;

    if( aRequiredDocType.empty() )
        return true;

    return aContents.docTypes.contains( aRequiredDocType );
}


static void ScanEpruDocTypes( wxInputStream& aInput, const wxString& aSource, std::set<wxString>& aDocTypes )
{
    wxTextInputStream txt( aInput, wxS( " " ), wxConvUTF8 );
    int               currentLine = 1;

    while( aInput.CanRead() )
    {
        wxString rawLine = txt.ReadLine();

        if( rawLine.empty() )
        {
            currentLine++;
            continue;
        }

        if( rawLine.Last() == '|' )
            rawLine.RemoveLast();

        int sepPos = rawLine.Find( wxS( "||" ) );

        if( sepPos < 0 )
        {
            currentLine++;
            continue;
        }

        wxString outerStr = rawLine.Left( sepPos );
        wxString innerStr = rawLine.Mid( sepPos + 2 );

        try
        {
            nlohmann::json outer = nlohmann::json::parse( ToStdString( outerStr ) );

            if( GetString( outer, "type" ) == wxS( "DOCHEAD" ) && !innerStr.empty() )
            {
                nlohmann::json inner = nlohmann::json::parse( ToStdString( innerStr ) );
                wxString       docType = ParseDocType( inner );

                if( !docType.empty() )
                    aDocTypes.insert( docType );
            }
        }
        catch( nlohmann::json::exception& e )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "JSON error in '%s' at line %d: %s" ), aSource, currentLine, e.what() ) );
        }

        currentLine++;
    }
}


static nlohmann::json ReadJsonEntry( wxInputStream& aInput, const wxString& aEntryName )
{
    wxMemoryOutputStream mem;
    mem << aInput;

    wxStreamBuffer* buf = mem.GetOutputStreamBuffer();
    std::string     jsonStr( static_cast<const char*>( buf->GetBufferStart() ), buf->GetBufferSize() );

    try
    {
        return nlohmann::json::parse( jsonStr );
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "JSON error reading '%s': %s" ), aEntryName, e.what() ) );
    }
}


static void MergeLibraryIndex( nlohmann::json& aTarget, nlohmann::json&& aIndex )
{
    if( !aIndex.is_object() )
        return;

    for( auto& [key, value] : aIndex.items() )
    {
        if( aTarget.contains( key ) && aTarget[key].is_object() && value.is_object() )
        {
            for( auto& [itemKey, itemValue] : value.items() )
                aTarget[key][itemKey] = std::move( itemValue );
        }
        else if( !aTarget.contains( key ) || !value.is_object() || !value.empty() )
        {
            aTarget[key] = std::move( value );
        }
    }
}


static V3_LIBRARY_CONTENTS ScanV3LibraryArchive( const wxString& aFileName, const wxString& aRequiredDocType )
{
    V3_LIBRARY_CONTENTS contents;

    std::shared_ptr<wxZipEntry> entry;
    wxFFileInputStream          in( aFileName );
    wxZipInputStream            zip( in );

    if( !zip.IsOk() )
        THROW_IO_ERROR( wxString::Format( _( "Cannot read ZIP archive '%s'" ), aFileName ) );

    while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
    {
        wxString entryName = entry->GetName();
        wxString baseName = entryName.AfterLast( '\\' ).AfterLast( '/' ).Lower();

        if( baseName == wxS( "symbol2.json" ) || baseName == wxS( "footprint2.json" )
            || baseName == wxS( "device2.json" ) )
        {
            contents.hasIndex = true;
        }
        else if( baseName.EndsWith( wxS( ".elibu" ) ) )
        {
            contents.hasElibu = true;

            if( !aRequiredDocType.empty() )
                ScanEpruDocTypes( zip, entryName, contents.docTypes );
        }

        if( HasValidV3LibraryContents( contents, aRequiredDocType ) )
            break;
    }

    return contents;
}


static std::vector<V3_DOC_RAW> ParseV3DocsFromArchive( const wxString& aFileName, nlohmann::json& aProject2 )
{
    std::vector<V3_DOC_RAW> docs;

    bool hasProject2 = false;
    bool hasEpru = false;

    std::shared_ptr<wxZipEntry> entry;
    wxFFileInputStream          in( aFileName );
    wxZipInputStream            zip( in );

    if( !zip.IsOk() )
        THROW_IO_ERROR( wxString::Format( _( "Cannot read ZIP archive '%s'" ), aFileName ) );

    while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
    {
        wxString entryName = entry->GetName();
        wxString baseName = entryName.AfterLast( '\\' ).AfterLast( '/' );

        if( baseName.CmpNoCase( wxS( "project2.json" ) ) == 0 )
        {
            aProject2 = ReadJsonEntry( zip, entryName );
                hasProject2 = true;
            }
        else if( baseName.Lower().EndsWith( wxS( ".epru" ) ) )
        {
            hasEpru = true;
            ParseEpruStream( zip, entryName, docs );
        }
    }

    if( !hasProject2 || !hasEpru )
    {
        THROW_IO_ERROR( wxString::Format( _( "'%s' does not appear to be a valid EasyEDA (JLCEDA) Pro v3 project. "
                   "Cannot find project2.json and .epru documents." ),
                aFileName ) );
    }

    return docs;
}


static std::vector<V3_DOC_RAW> ParseV3DocsFromLibraryArchive( const wxString& aFileName, nlohmann::json& aLibraryIndex )
{
    std::vector<V3_DOC_RAW> docs;
    V3_LIBRARY_CONTENTS     contents;

    std::shared_ptr<wxZipEntry> entry;
    wxFFileInputStream          in( aFileName );
    wxZipInputStream            zip( in );

    if( !zip.IsOk() )
        THROW_IO_ERROR( wxString::Format( _( "Cannot read ZIP archive '%s'" ), aFileName ) );

    aLibraryIndex = nlohmann::json::object();

    while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
    {
        wxString entryName = entry->GetName();
        wxString baseName = entryName.AfterLast( '\\' ).AfterLast( '/' );
        wxString lowerBaseName = baseName.Lower();

        if( lowerBaseName == wxS( "symbol2.json" ) || lowerBaseName == wxS( "footprint2.json" )
            || lowerBaseName == wxS( "device2.json" ) )
        {
            nlohmann::json index = ReadJsonEntry( zip, entryName );

            MergeLibraryIndex( aLibraryIndex, std::move( index ) );

            contents.hasIndex = true;
        }
        else if( lowerBaseName.EndsWith( wxS( ".elibu" ) ) )
        {
            contents.hasElibu = true;
            ParseEpruStream( zip, entryName, docs );

            for( const V3_DOC_RAW& doc : docs )
                contents.docTypes.insert( doc.docType );
        }
    }

    if( !HasValidV3LibraryContents( contents, wxEmptyString ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "'%s' does not appear to be a valid EasyEDA (JLCEDA) Pro v3 library. "
                                             "Cannot find symbol2.json, footprint2.json or device2.json and .elibu "
                                             "documents." ),
                                          aFileName ) );
    }

    return docs;
}


} // namespace


namespace EASYEDAPRO
{

V3_DOC_PARSER::V3_DOC_PARSER( const wxString& aFileName ) :
        m_fileName( aFileName )
{
}


bool V3_DOC_PARSER::IsV3Archive( const wxString& aFileName )
{
    wxFileName fn( aFileName );
    wxString   ext = fn.GetExt().Lower();

    if( ext != wxS( "epro2" ) && ext != wxS( "zip" ) )
        return false;

    try
    {
        std::shared_ptr<wxZipEntry> entry;
        wxFFileInputStream          in( aFileName );
        wxZipInputStream            zip( in );

        if( !zip.IsOk() )
            return false;

        bool hasProject2 = false;
        bool hasEpru = false;

        while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
        {
            wxString entryName = entry->GetName();
            wxString baseName = entryName.AfterLast( '\\' ).AfterLast( '/' );

            if( baseName.CmpNoCase( wxS( "project2.json" ) ) == 0 )
                hasProject2 = true;
            else if( baseName.Lower().EndsWith( wxS( ".epru" ) ) )
                hasEpru = true;

            if( hasProject2 && hasEpru )
                return true;
        }

        return false;
    }
    catch( ... )
    {
        return false;
    }
}


bool V3_DOC_PARSER::IsV3Library( const wxString& aFileName, const wxString& aRequiredDocType )
{
    wxFileName fn( aFileName );

    if( fn.GetExt().Lower() != wxS( "elibz2" ) )
        return false;

    try
    {
        V3_LIBRARY_CONTENTS contents = ScanV3LibraryArchive( aFileName, aRequiredDocType );
        return HasValidV3LibraryContents( contents, aRequiredDocType );
    }
    catch( ... )
    {
        return false;
    }
}


void V3_DOC_PARSER::Load()
{
    m_rawDocs.clear();
    m_project2 = nlohmann::json();
    m_libraryIndex = nlohmann::json();
    m_skippedCount = 0;

    std::vector<V3_DOC_RAW> rawDocs = ParseV3DocsFromArchive( m_fileName, m_project2 );

    for( V3_DOC_RAW& raw : rawDocs )
    {
        m_rawDocs[raw.docType][raw.uuid] = std::move( raw );
    }
}


void V3_DOC_PARSER::LoadLibrary()
{
    m_rawDocs.clear();
    m_project2 = nlohmann::json();
    m_libraryIndex = nlohmann::json();
    m_skippedCount = 0;

    std::vector<V3_DOC_RAW> rawDocs = ParseV3DocsFromLibraryArchive( m_fileName, m_libraryIndex );

    for( V3_DOC_RAW& raw : rawDocs )
        m_rawDocs[raw.docType][raw.uuid] = std::move( raw );
}


const std::map<wxString, V3_DOC_RAW>& V3_DOC_PARSER::GetRawDocs( const wxString& aDocType ) const
{
    static const std::map<wxString, V3_DOC_RAW> empty;

    auto it = m_rawDocs.find( aDocType );

    if( it == m_rawDocs.end() )
        return empty;

    return it->second;
}


const V3_DOC_RAW* V3_DOC_PARSER::FindRawDoc( const wxString& aDocType, const wxString& aUuid ) const
{
    auto typeIt = m_rawDocs.find( aDocType );

    if( typeIt == m_rawDocs.end() )
        return nullptr;

    auto docIt = typeIt->second.find( aUuid );

    if( docIt == typeIt->second.end() )
        return nullptr;

    return &docIt->second;
}

} // namespace EASYEDAPRO
