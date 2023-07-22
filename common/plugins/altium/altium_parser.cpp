/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include "altium_parser.h"
#include "altium_parser_utils.h"

#include <compoundfilereader.h>
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


std::map<wxString, wxString> ALTIUM_COMPOUND_FILE::ListLibFootprints() const
{
    std::map<wxString, wxString> patternMap;

    if( !m_reader )
        return patternMap;

    const CFB::COMPOUND_FILE_ENTRY* root = m_reader->GetRootEntry();

    if( !root )
        return patternMap;

    m_reader->EnumFiles( root, 2,
                         [&]( const CFB::COMPOUND_FILE_ENTRY* entry, const CFB::utf16string& dir,
                              int level ) -> void
                         {
                             std::wstring dirName = UTF16ToWstring( dir.data(), dir.size() );
                             std::wstring fileName = UTF16ToWstring( entry->name, entry->nameLen );

                             if( m_reader->IsStream( entry ) && fileName == L"Parameters" )
                             {
                                 ALTIUM_PARSER                parametersReader( *this, entry );
                                 std::map<wxString, wxString> parameterProperties =
                                         parametersReader.ReadProperties();

                                 wxString key = ALTIUM_PARSER::ReadString(
                                         parameterProperties, wxT( "PATTERN" ), wxT( "" ) );

                                 wxString fpName = ALTIUM_PARSER::ReadUnicodeString(
                                         parameterProperties, wxT( "PATTERN" ), wxT( "" ) );

                                 patternMap.emplace( key, fpName );
                             }
                         } );

    return patternMap;
}


wxString ALTIUM_COMPOUND_FILE::FindLibFootprintDirName( const wxString& aFpUnicodeName ) const
{
    if( !m_reader )
        return wxEmptyString;

    const CFB::COMPOUND_FILE_ENTRY* root = m_reader->GetRootEntry();

    if( !root )
        return wxEmptyString;

    wxString ret;

    m_reader->EnumFiles( root, 2,
                         [&]( const CFB::COMPOUND_FILE_ENTRY* entry, const CFB::utf16string& dir,
                              int level ) -> void
                         {
                             std::wstring dirName = UTF16ToWstring( dir.data(), dir.size() );
                             std::wstring fileName = UTF16ToWstring( entry->name, entry->nameLen );

                             if( m_reader->IsStream( entry ) && fileName == L"Parameters" )
                             {
                                 ALTIUM_PARSER                parametersReader( *this, entry );
                                 std::map<wxString, wxString> parameterProperties =
                                         parametersReader.ReadProperties();

                                 wxString fpName = ALTIUM_PARSER::ReadUnicodeString(
                                         parameterProperties, wxT( "PATTERN" ), wxT( "" ) );

                                 if( fpName == aFpUnicodeName )
                                 {
                                     ret = dirName;
                                 }
                             }
                         } );

    return ret;
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
                              int level ) -> void
                         {
                             if( m_reader->IsStream( entry ) == aIsStream )
                             {
                                 std::string name = UTF16ToUTF8( entry->name );
                                 if( name == aName.c_str() )
                                 {
                                     ret = entry;
                                 }
                             }
                         } );
    return ret;
}


const CFB::COMPOUND_FILE_ENTRY*
ALTIUM_COMPOUND_FILE::FindStream( const std::vector<std::string>& aStreamPath ) const
{
    if( !m_reader )
        return nullptr;

    const CFB::COMPOUND_FILE_ENTRY* currentDirEntry = m_reader->GetRootEntry();

    auto it = aStreamPath.cbegin();
    while( currentDirEntry != nullptr )
    {
        const std::string& name = *it;

        if( ++it == aStreamPath.cend() )
        {
            return FindStreamSingleLevel( currentDirEntry, name, true );
        }
        else
        {
            currentDirEntry = FindStreamSingleLevel( currentDirEntry, name, false );
        }
    }

    return nullptr;
}


ALTIUM_PARSER::ALTIUM_PARSER( const ALTIUM_COMPOUND_FILE&     aFile,
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


ALTIUM_PARSER::ALTIUM_PARSER( std::unique_ptr<char[]>& aContent, size_t aSize )
{
    m_subrecord_end = nullptr;
    m_size = aSize;
    m_error = false;
    m_content = std::move( aContent );
    m_pos = m_content.get();
}


std::map<wxString, wxString> ALTIUM_PARSER::ReadProperties()
{
    std::map<wxString, wxString> kv;

    uint32_t length = Read<uint32_t>();

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

    if( !hasNullByte )
    {
        wxLogError( _( "Missing null byte at end of property list. Imported data might be "
                       "malformed or missing." ) );
    }

    // we use std::string because std::string can handle NULL-bytes
    // wxString would end the string at the first NULL-byte
    std::string str = std::string( m_pos, length - ( hasNullByte ? 1 : 0 ) );
    m_pos += length;

    std::size_t token_end = 0;

    while( token_end < str.size() && token_end != std::string::npos )
    {
        std::size_t token_start = str.find( '|', token_end );
        std::size_t token_equal = str.find( '=', token_start );
        token_end = str.find( '|', token_start + 1 );

        if( token_equal >= token_end )
        {
            continue; // this looks like an error: skip the entry. Also matches on std::string::npos
        }

        if( token_end == std::string::npos )
        {
            token_end = str.size() + 1; // this is the correct offset
        }

        std::string keyS   = str.substr( token_start + 1, token_equal - token_start - 1 );
        std::string valueS = str.substr( token_equal + 1, token_end - token_equal - 1 );

        // convert the strings to wxStrings, since we use them everywhere
        // value can have non-ASCII characters, so we convert them from LATIN1/ISO8859-1
        wxString key( keyS.c_str(), wxConvISO8859_1 );
        // Altium stores keys either in Upper, or in CamelCase. Lets unify it.
        wxString canonicalKey = key.Trim( false ).Trim( true ).MakeUpper();
        // If the key starts with '%UTF8%' we have to parse the value using UTF8
        wxString value;

        if( canonicalKey.StartsWith( "%UTF8%" ) )
            value = wxString( valueS.c_str(), wxConvUTF8 );
        else
            value = wxString( valueS.c_str(), wxConvISO8859_1 );

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
            value = AltiumPropertyToKiCadString( value.Trim() );
        }

        kv.insert( { canonicalKey, value.Trim() } );
    }

    return kv;
}


int32_t ALTIUM_PARSER::ConvertToKicadUnit( const double aValue )
{
    constexpr double int_limit = ( std::numeric_limits<int>::max() - 10 ) / 2.54;

    int32_t iu = KiROUND( Clamp<double>( -int_limit, aValue, int_limit ) * 2.54 );

    // Altium's internal precision is 0.1uinch.  KiCad's is 1nm.  Round to nearest 10nm to clean
    // up most rounding errors.  This allows lossless conversion of increments of 0.05mils and
    // 0.01um.
    return KiROUND( (double) iu / 10.0 ) * 10;
}


int ALTIUM_PARSER::ReadInt( const std::map<wxString, wxString>& aProps, const wxString& aKey,
                            int aDefault )
{
    const std::map<wxString, wxString>::const_iterator& value = aProps.find( aKey );
    return value == aProps.end() ? aDefault : wxAtoi( value->second );
}


double ALTIUM_PARSER::ReadDouble( const std::map<wxString, wxString>& aProps, const wxString& aKey,
                                  double aDefault )
{
    const std::map<wxString, wxString>::const_iterator& value = aProps.find( aKey );

    if( value == aProps.end() )
        return aDefault;

    // Locale independent str -> double conversation
    std::istringstream istr( (const char*) value->second.mb_str() );
    istr.imbue( std::locale::classic() );

    double doubleValue;
    istr >> doubleValue;
    return doubleValue;
}


bool ALTIUM_PARSER::ReadBool( const std::map<wxString, wxString>& aProps, const wxString& aKey,
                              bool aDefault )
{
    const std::map<wxString, wxString>::const_iterator& value = aProps.find( aKey );

    if( value == aProps.end() )
        return aDefault;
    else
        return value->second == "T" || value->second == "TRUE";
}


int32_t ALTIUM_PARSER::ReadKicadUnit( const std::map<wxString, wxString>& aProps,
                                      const wxString& aKey, const wxString& aDefault )
{
    const wxString& value = ReadString( aProps, aKey, aDefault );

    wxString prefix;

    if( !value.EndsWith( "mil", &prefix ) )
    {
        wxLogError( _( "Unit '%s' does not end with 'mil'." ), value );
        return 0;
    }

    double mils;

    if( !prefix.ToCDouble( &mils ) )
    {
        wxLogError( _( "Cannot convert '%s' to double." ), prefix );
        return 0;
    }

    return ConvertToKicadUnit( mils * 10000 );
}


wxString ALTIUM_PARSER::ReadString( const std::map<wxString, wxString>& aProps,
                                    const wxString& aKey, const wxString& aDefault )
{
    const auto& utf8Value = aProps.find( wxString( "%UTF8%" ) + aKey );

    if( utf8Value != aProps.end() )
        return utf8Value->second;

    const auto& value = aProps.find( aKey );

    if( value != aProps.end() )
        return value->second;

    return aDefault;
}


wxString ALTIUM_PARSER::ReadUnicodeString( const std::map<wxString, wxString>& aProps,
                                           const wxString& aKey, const wxString& aDefault )
{
    const auto& unicodeFlag = aProps.find( wxS( "UNICODE" ) );

    if( unicodeFlag != aProps.end() && unicodeFlag->second.Contains( wxS( "EXISTS" ) ) )
    {
        const auto& unicodeValue = aProps.find( wxString( "UNICODE__" ) + aKey );

        if( unicodeValue != aProps.end() )
        {
            wxArrayString arr = wxSplit( unicodeValue->second, ',', '\0' );
            wxString      out;

            for( wxString part : arr )
                out += wxString( wchar_t( wxAtoi( part ) ) );

            return out;
        }
    }

    return ReadString( aProps, aKey, aDefault );
}
