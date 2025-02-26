/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include "altium_binary_parser.h"
#include "altium_parser_utils.h"

#include <compoundfilereader.h>
#include <charconv>
#include <ki_exception.h>
#include <math/util.h>
#include <numeric>
#include <sstream>
#include <utf.h>
#include <wx/log.h>
#include <wx/translation.h>


// Helper for debug logging
std::string FormatPath( const std::vector<std::string>& aVectorPath )
{
    return std::accumulate( aVectorPath.cbegin(), aVectorPath.cend(), std::string(),
                            []( const std::string& ss, const std::string& s )
                            {
                                return ss.empty() ? s : ss + '\\' + s;
                            } );
}


ALTIUM_COMPOUND_FILE::ALTIUM_COMPOUND_FILE()
{
}


ALTIUM_COMPOUND_FILE::ALTIUM_COMPOUND_FILE( const wxString& aFilePath )
{
    // Open file
    FILE* fp = wxFopen( aFilePath, "rb" );

    if( fp == nullptr )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot open file '%s'." ), aFilePath ) );
    }

    fseek( fp, 0, SEEK_END );
    long len = ftell( fp );

    if( len < 0 )
    {
        fclose( fp );
        THROW_IO_ERROR( _( "Error reading file: cannot determine length." ) );
    }

    // Read into buffer (TODO: add support for memory-mapped files to avoid this copy!)
    m_buffer.resize( len );

    fseek( fp, 0, SEEK_SET );

    size_t bytesRead = fread( m_buffer.data(), sizeof( unsigned char ), len, fp );
    fclose( fp );

    if( static_cast<size_t>( len ) != bytesRead )
    {
        THROW_IO_ERROR( _( "Error reading file." ) );
    }

    try
    {
        m_reader = std::make_unique<CFB::CompoundFileReader>( m_buffer.data(), m_buffer.size() );
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
}


ALTIUM_COMPOUND_FILE::ALTIUM_COMPOUND_FILE( const void* aBuffer, size_t aLen )
{
    InitFromBuffer( aBuffer, aLen );
}


void ALTIUM_COMPOUND_FILE::InitFromBuffer( const void* aBuffer, size_t aLen )
{
    m_buffer.resize( aLen );
    memcpy( m_buffer.data(), aBuffer, aLen );

    try
    {
        m_reader = std::make_unique<CFB::CompoundFileReader>( m_buffer.data(), m_buffer.size() );
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
}


bool ALTIUM_COMPOUND_FILE::DecodeIntLibStream( const CFB::COMPOUND_FILE_ENTRY& cfe,
                                               ALTIUM_COMPOUND_FILE* aOutput )
{
    wxCHECK( aOutput, false );
    wxCHECK( cfe.size >= 1, false );

    size_t         streamSize = cfe.size;
    wxMemoryBuffer buffer( streamSize );
    buffer.SetDataLen( streamSize );

    // read file into buffer
    GetCompoundFileReader().ReadFile( &cfe, 0, reinterpret_cast<char*>( buffer.GetData() ),
                                      streamSize );

    // 0x02: compressed stream, 0x00: uncompressed
    if( buffer[0] == 0x02 )
    {
        wxMemoryInputStream memoryInputStream( buffer.GetData(), streamSize );
        memoryInputStream.SeekI( 1, wxFromStart );

        wxZlibInputStream    zlibInputStream( memoryInputStream );
        wxMemoryOutputStream decodedPcbLibStream;
        decodedPcbLibStream << zlibInputStream;

        wxStreamBuffer* outStream = decodedPcbLibStream.GetOutputStreamBuffer();
        aOutput->InitFromBuffer( outStream->GetBufferStart(), outStream->GetIntPosition() );
        return true;
    }
    else if( buffer[0] == 0x00 )
    {
        aOutput->InitFromBuffer( static_cast<uint8_t*>( buffer.GetData() ) + 1, streamSize - 1 );
        return true;
    }
    else
    {
        wxFAIL_MSG( wxString::Format( "Altium IntLib unknown header: %02x %02x %02x %02x %02x",
                                      buffer[0], buffer[1], buffer[2], buffer[3], buffer[4] ) );
    }

    return false;
}


const CFB::COMPOUND_FILE_ENTRY*
ALTIUM_COMPOUND_FILE::FindStreamSingleLevel( const CFB::COMPOUND_FILE_ENTRY* aEntry,
                                             const std::string aName, const bool aIsStream ) const
{
    if( !m_reader || !aEntry )
        return nullptr;

    const CFB::COMPOUND_FILE_ENTRY* ret = nullptr;

    m_reader->EnumFiles( aEntry, 1,
                         [&]( const CFB::COMPOUND_FILE_ENTRY* entry, const CFB::utf16string& dir,
                              int level ) -> int
                         {
                            if( ret != nullptr )
                                return 1;

                             if( m_reader->IsStream( entry ) == aIsStream )
                             {
                                 std::string name = UTF16ToUTF8( entry->name );
                                 if( name == aName.c_str() )
                                 {
                                     ret = entry;
                                     return 1;
                                 }
                             }

                             return 0;
                         } );
    return ret;
}


std::map<wxString, ALTIUM_SYMBOL_DATA>
ALTIUM_COMPOUND_FILE::GetLibSymbols( const CFB::COMPOUND_FILE_ENTRY* aStart ) const
{
    const CFB::COMPOUND_FILE_ENTRY* root = aStart ? aStart : m_reader->GetRootEntry();

    if( !root )
        return {};

    std::map<wxString, ALTIUM_SYMBOL_DATA> folders;

    m_reader->EnumFiles( root, 1, [&]( const CFB::COMPOUND_FILE_ENTRY* tentry,
                                       const CFB::utf16string&, int ) -> int
    {
        wxString dirName = UTF16ToWstring( tentry->name, tentry->nameLen );

        if( m_reader->IsStream( tentry ) )
            return 0;

        m_reader->EnumFiles( tentry, 1,
                    [&]( const CFB::COMPOUND_FILE_ENTRY* entry,
                         const CFB::utf16string&, int ) -> int
                    {
                        std::wstring fileName = UTF16ToWstring( entry->name, entry->nameLen );

                        if( m_reader->IsStream( entry ) && fileName == L"Data" )
                            folders[dirName].m_symbol = entry;

                        if( m_reader->IsStream( entry ) && fileName == L"PinFrac" )
                            folders[dirName].m_pinsFrac = entry;

                        if( m_reader->IsStream( entry ) && fileName == L"PinWideText" )
                            folders[dirName].m_pinsWideText = entry;

                        if( m_reader->IsStream( entry ) && fileName == L"PinTextData" )
                            folders[dirName].m_pinsTextData = entry;

                        return 0;
                    } );

        return 0;
    } );

    return folders;
}


std::map<wxString, const CFB::COMPOUND_FILE_ENTRY*>
ALTIUM_COMPOUND_FILE::EnumDir( const std::wstring& aDir ) const
{
    const CFB::COMPOUND_FILE_ENTRY* root = m_reader->GetRootEntry();

    if( !root )
        return {};

    std::map<wxString, const CFB::COMPOUND_FILE_ENTRY*> files;

    m_reader->EnumFiles(
            root, 1,
            [&]( const CFB::COMPOUND_FILE_ENTRY* tentry, const CFB::utf16string& dir,
                 int level ) -> int
            {
                if( m_reader->IsStream( tentry ) )
                    return 0;

                std::wstring dirName = UTF16ToWstring( tentry->name, tentry->nameLen );

                if( dirName != aDir )
                    return 0;

                m_reader->EnumFiles(
                        tentry, 1,
                        [&]( const CFB::COMPOUND_FILE_ENTRY* entry, const CFB::utf16string&,
                             int ) -> int
                        {
                            if( m_reader->IsStream( entry ) )
                            {
                                std::wstring fileName =
                                        UTF16ToWstring( entry->name, entry->nameLen );

                                files[fileName] = entry;
                            }

                            return 0;
                        } );
                return 0;
            } );

    return files;
}


const CFB::COMPOUND_FILE_ENTRY*
ALTIUM_COMPOUND_FILE::FindStream( const CFB::COMPOUND_FILE_ENTRY* aStart,
                                  const std::vector<std::string>& aStreamPath ) const
{
    if( !m_reader )
        return nullptr;

    if( !aStart )
        aStart = m_reader->GetRootEntry();

    auto it = aStreamPath.cbegin();

    while( aStart != nullptr )
    {
        const std::string& name = *it;

        if( ++it == aStreamPath.cend() )
        {
            const CFB::COMPOUND_FILE_ENTRY* ret = FindStreamSingleLevel( aStart, name, true );
            return ret;
        }
        else
        {
            const CFB::COMPOUND_FILE_ENTRY* ret = FindStreamSingleLevel( aStart, name, false );
            aStart = ret;
        }
    }

    return nullptr;
}


const CFB::COMPOUND_FILE_ENTRY*
ALTIUM_COMPOUND_FILE::FindStream( const std::vector<std::string>& aStreamPath ) const
{
    return FindStream( nullptr, aStreamPath );
}


ALTIUM_BINARY_PARSER::ALTIUM_BINARY_PARSER( const ALTIUM_COMPOUND_FILE&     aFile,
                              const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    m_subrecord_end = nullptr;
    m_size          = static_cast<size_t>( aEntry->size );
    m_error         = false;
    m_content.reset( new char[m_size] );
    m_pos = m_content.get();

    // read file into buffer
    aFile.GetCompoundFileReader().ReadFile( aEntry, 0, m_content.get(), m_size );
}


ALTIUM_BINARY_PARSER::ALTIUM_BINARY_PARSER( std::unique_ptr<char[]>& aContent, size_t aSize )
{
    m_subrecord_end = nullptr;
    m_size = aSize;
    m_error = false;
    m_content = std::move( aContent );
    m_pos = m_content.get();
}


std::map<wxString, wxString> ALTIUM_BINARY_PARSER::ReadProperties(
        std::function<std::map<wxString, wxString>( const std::string& )> handleBinaryData )
{
    // TSAN reports calling this wx macro is not thread-safe
    static wxCSConv convISO8859_1 = wxConvISO8859_1;

    std::map<wxString, wxString> kv;

    uint32_t length = Read<uint32_t>();
    bool isBinary = ( length & 0xff000000 ) != 0;

    length &= 0x00ffffff;

    if( length > GetRemainingBytes() )
    {
        m_error = true;
        return kv;
    }

    if( length == 0 )
    {
        return kv;
    }

    // There is one case by kliment where Board6 ends with "|NEARDISTANCE=1000mi".
    // Both the 'l' and the null-byte are missing, which looks like Altium swallowed two bytes.
    bool hasNullByte = m_pos[length - 1] == '\0';

    if( !hasNullByte && !isBinary )
    {
        wxLogTrace( "ALTIUM", wxT( "Missing null byte at end of property list. Imported data "
                                   "might be  malformed or missing." ) );
    }

    // we use std::string because std::string can handle NULL-bytes
    // wxString would end the string at the first NULL-byte
    std::string str = std::string( m_pos, length - ( hasNullByte ? 1 : 0 ) );
    m_pos += length;

    if( isBinary )
    {
        return handleBinaryData( str );
    }

    std::size_t token_end = 0;

    while( token_end < str.size() && token_end != std::string::npos )
    {
        std::size_t token_start = str.find( '|', token_end );
        std::size_t token_equal = str.find( '=', token_end );
        std::size_t key_start;

        if( token_start <= token_equal )
        {
            key_start = token_start + 1;
        }
        else
        {
            // Leading "|" before "RECORD=28" may be missing in older schematic versions.
            key_start = token_end;
        }

        token_end = str.find( '|', key_start );

        if( token_equal >= token_end )
        {
            continue; // this looks like an error: skip the entry. Also matches on std::string::npos
        }

        if( token_end == std::string::npos )
        {
            token_end = str.size() + 1; // this is the correct offset
        }

        std::string keyS = str.substr( key_start, token_equal - key_start );
        std::string valueS = str.substr( token_equal + 1, token_end - token_equal - 1 );

        // convert the strings to wxStrings, since we use them everywhere
        // value can have non-ASCII characters, so we convert them from LATIN1/ISO8859-1
        wxString key( keyS.c_str(), convISO8859_1 );

        // Altium stores keys either in Upper, or in CamelCase. Lets unify it.
        wxString canonicalKey = key.Trim( false ).Trim( true ).MakeUpper();

        // If the key starts with '%UTF8%' we have to parse the value using UTF8
        wxString value;

        if( canonicalKey.StartsWith( "%UTF8%" ) )
            value = wxString( valueS.c_str(), wxConvUTF8 );
        else
            value = wxString( valueS.c_str(), convISO8859_1 );

        if( canonicalKey != wxS( "PATTERN" ) && canonicalKey != wxS( "SOURCEFOOTPRINTLIBRARY" ) )
        {
            // Breathless hack because I haven't a clue what the story is here (but this character
            // appears in a lot of radial dimensions and is rendered by Altium as a space).
            value.Replace( wxT( "ÿ" ), wxT( " " ) );
        }

        if( canonicalKey == wxT( "DESIGNATOR" )
                || canonicalKey == wxT( "NAME" )
                || canonicalKey == wxT( "TEXT" ) )
        {
            if( kv[ wxT( "RECORD" ) ] != wxT( "4" ) )
                value = AltiumPropertyToKiCadString( value.Trim() );
        }

        kv.insert( { canonicalKey, value.Trim() } );
    }

    return kv;
}
