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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "embedded_files.h"
#include "embedded_files_parser.h"

#include <wx/base64.h>
#include <wx/debug.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/wfstream.h>

#include <map>
#include <memory>
#include <sstream>

#include <zstd.h>

#include <kiid.h>
#include <mmh3_hash.h>
#include <paths.h>



EMBEDDED_FILES::EMBEDDED_FILE* EMBEDDED_FILES::AddFile( const wxFileName& aName, bool aOverwrite )
{
    if( HasFile( aName.GetFullName() ) )
    {
        if( !aOverwrite )
            return m_files[aName.GetFullName()];

        m_files.erase( aName.GetFullName() );
    }

    wxFFileInputStream file( aName.GetFullPath() );
    wxMemoryBuffer     buffer;

    if( !file.IsOk() )
        return nullptr;

    wxFileOffset length = file.GetLength();

    std::unique_ptr<EMBEDDED_FILE> efile = std::make_unique<EMBEDDED_FILE>();
    efile->name = aName.GetFullName();
    efile->decompressedData.resize( length );

    wxString ext = aName.GetExt().Upper();

    // Handle some common file extensions
    if( ext == "STP" || ext == "STPZ" || ext == "STEP" || ext == "WRL" || ext == "WRZ" )
    {
        efile->type = EMBEDDED_FILE::FILE_TYPE::MODEL;
    }
    else if( ext == "WOFF" || ext == "WOFF2" || ext == "TTF" || ext == "OTF" )
    {
        efile->type = EMBEDDED_FILE::FILE_TYPE::FONT;
    }
    else if( ext == "PDF" )
    {
        efile->type = EMBEDDED_FILE::FILE_TYPE::DATASHEET;
    }
    else if( ext == "KICAD_WKS" )
    {
        efile->type = EMBEDDED_FILE::FILE_TYPE::WORKSHEET;
    }

    if( !efile->decompressedData.data() )
        return nullptr;

    char* data = efile->decompressedData.data();
    wxFileOffset total_read = 0;

    while( !file.Eof() && total_read < length )
    {
        file.Read( data, length - total_read );

        size_t read = file.LastRead();
        data += read;
        total_read += read;
    }

    if( CompressAndEncode( *efile ) != RETURN_CODE::OK )
        return nullptr;

    efile->is_valid = true;

    EMBEDDED_FILE* result = efile.release();
    m_files[aName.GetFullName()] = result;

    if( m_fileAddedCallback )
        m_fileAddedCallback( result );

    return m_files[aName.GetFullName()];
}


void EMBEDDED_FILES::AddFile( EMBEDDED_FILE* aFile )
{
    m_files.insert( { aFile->name, aFile } );

    if( m_fileAddedCallback )
        m_fileAddedCallback( aFile );
}


// Remove a file from the collection
void EMBEDDED_FILES::RemoveFile( const wxString& name, bool aErase )
{
    auto it = m_files.find( name );

    if( it != m_files.end() )
    {
        if( aErase )
            delete it->second;

        m_files.erase( it );
    }
}


void EMBEDDED_FILES::ClearEmbeddedFonts()
{
    for( auto it = m_files.begin(); it != m_files.end(); )
    {
        if( it->second->type == EMBEDDED_FILE::FILE_TYPE::FONT )
        {
            delete it->second;
            it = m_files.erase( it );
        }
        else
        {
            ++it;
        }
    }
}


// Write the collection of files to a disk file in the specified format
void EMBEDDED_FILES::WriteEmbeddedFiles( OUTPUTFORMATTER& aOut, bool aWriteData ) const
{
    ssize_t MIME_BASE64_LENGTH = 76;
    aOut.Print( "(embedded_files " );

    for( const auto& [name, entry] : m_files )
    {
        const EMBEDDED_FILE& file = *entry;

        // Skip empty files
        if( file.compressedEncodedData.empty() )
        {
            continue;
        }

        aOut.Print( "(file " );
        aOut.Print( "(name %s)", aOut.Quotew( file.name ).c_str() );

        const char* type = nullptr;

        switch( file.type )
        {
        case EMBEDDED_FILE::FILE_TYPE::DATASHEET: type = "datasheet"; break;
        case EMBEDDED_FILE::FILE_TYPE::FONT:      type = "font";      break;
        case EMBEDDED_FILE::FILE_TYPE::MODEL:     type = "model";     break;
        case EMBEDDED_FILE::FILE_TYPE::WORKSHEET: type = "worksheet"; break;
        default:                                  type = "other";     break;
        }

        aOut.Print( "(type %s)", type );

        if( aWriteData )
        {
            aOut.Print( "(data" );

            size_t first = 0;

            while( first < file.compressedEncodedData.length() )
            {
                ssize_t remaining = file.compressedEncodedData.length() - first;
                int     length = std::min( remaining, MIME_BASE64_LENGTH );

                std::string_view view( file.compressedEncodedData.data() + first, length );

                aOut.Print( "\n%1s%.*s%s\n", first ? "" : "|", length, view.data(),
                            remaining == length ? "|" : "" );
                first += MIME_BASE64_LENGTH;
            }

            aOut.Print( ")" ); // Close data
        }

        aOut.Print( "(checksum %s)", aOut.Quotew( file.data_hash ).c_str() );
        aOut.Print( ")" ); // Close file
    }

    aOut.Print( ")" ); // Close embedded_files
}


// Compress and Base64 encode data
EMBEDDED_FILES::RETURN_CODE EMBEDDED_FILES::CompressAndEncode( EMBEDDED_FILE& aFile )
{
    std::vector<char> compressedData;
    size_t estCompressedSize = ZSTD_compressBound( aFile.decompressedData.size() );
    compressedData.resize( estCompressedSize );
    size_t compressedSize = ZSTD_compress( compressedData.data(), estCompressedSize,
                                           aFile.decompressedData.data(),
                                           aFile.decompressedData.size(), 15 );

    if( ZSTD_isError( compressedSize ) )
    {
        compressedData.clear();
        return RETURN_CODE::OUT_OF_MEMORY;
    }

    const size_t dstLen = wxBase64EncodedSize( compressedSize );
    aFile.compressedEncodedData.resize( dstLen );
    size_t retval = wxBase64Encode( aFile.compressedEncodedData.data(), dstLen,
                                          compressedData.data(), compressedSize );

    if( retval != dstLen )
    {
        aFile.compressedEncodedData.clear();
        return RETURN_CODE::OUT_OF_MEMORY;
    }

    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.add( aFile.decompressedData );
    aFile.data_hash = hash.digest().ToString();

    return RETURN_CODE::OK;
}


// Decompress and Base64 decode data
EMBEDDED_FILES::RETURN_CODE EMBEDDED_FILES::DecompressAndDecode( EMBEDDED_FILE& aFile )
{
    std::vector<char> compressedData;
    size_t            compressedSize = wxBase64DecodedSize( aFile.compressedEncodedData.size() );

    if( compressedSize == 0 )
    {
        wxLogTrace( wxT( "KICAD_EMBED" ),
                    wxT( "%s:%s:%d\n * Base64DecodedSize failed for file '%s' with size %zu" ),
                    __FILE__, __FUNCTION__, __LINE__, aFile.name,
                    aFile.compressedEncodedData.size() );
        return RETURN_CODE::OUT_OF_MEMORY;
    }

    compressedData.resize( compressedSize );
    void* compressed = compressedData.data();

    // The return value from wxBase64Decode is the actual size of the decoded data avoiding
    // the modulo 4 padding of the base64 encoding
    compressedSize = wxBase64Decode( compressed, compressedSize, aFile.compressedEncodedData );

    unsigned long long estDecompressedSize = ZSTD_getFrameContentSize( compressed, compressedSize );

    if( estDecompressedSize > 1e9 ) // Limit to 1GB
        return RETURN_CODE::OUT_OF_MEMORY;

    if( estDecompressedSize == ZSTD_CONTENTSIZE_ERROR
        || estDecompressedSize == ZSTD_CONTENTSIZE_UNKNOWN )
    {
        return RETURN_CODE::OUT_OF_MEMORY;
    }

    aFile.decompressedData.resize( estDecompressedSize );
    void* decompressed = aFile.decompressedData.data();

    size_t decompressedSize = ZSTD_decompress( decompressed, estDecompressedSize,
                                               compressed, compressedSize );

    if( ZSTD_isError( decompressedSize ) )
    {
        wxLogTrace( wxT( "KICAD_EMBED" ),
                    wxT( "%s:%s:%d\n * ZSTD_decompress failed with error '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, ZSTD_getErrorName( decompressedSize ) );
        aFile.decompressedData.clear();
        return RETURN_CODE::OUT_OF_MEMORY;
    }

    aFile.decompressedData.resize( decompressedSize );
    std::string test_hash;
    std::string new_hash;

    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.add( aFile.decompressedData );
    new_hash = hash.digest().ToString();

    if( aFile.data_hash.length() == 64 )
        picosha2::hash256_hex_string( aFile.decompressedData, test_hash );
    else
        test_hash = new_hash;

    if( test_hash != aFile.data_hash )
    {
        wxLogTrace( wxT( "KICAD_EMBED" ),
                    wxT( "%s:%s:%d\n * Checksum error in embedded file '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aFile.name );
        aFile.decompressedData.clear();
        return RETURN_CODE::CHECKSUM_ERROR;
    }

    aFile.data_hash = new_hash;

    return RETURN_CODE::OK;
}


EMBEDDED_FILES::RETURN_CODE EMBEDDED_FILES::ComputeFileHash( const wxFileName& aFileName,
                                                             std::string& aHash )
{
    wxFFileInputStream file( aFileName.GetFullPath() );

    if( !file.IsOk() )
        return RETURN_CODE::FILE_NOT_FOUND;

    wxFileOffset length = file.GetLength();
    std::vector<char> data( length );

    if( !data.data() )
        return RETURN_CODE::OUT_OF_MEMORY;

    char* dataPtr = data.data();
    wxFileOffset totalRead = 0;

    while( !file.Eof() && totalRead < length )
    {
        file.Read( dataPtr, length - totalRead );
        size_t bytesRead = file.LastRead();
        dataPtr += bytesRead;
        totalRead += bytesRead;
    }

    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.add( data );
    aHash = hash.digest().ToString();

    return RETURN_CODE::OK;
}


// Parsing method
void EMBEDDED_FILES_PARSER::ParseEmbedded( EMBEDDED_FILES* aFiles )
{
    // embedded files are version 20240706 and uses also Bars as separator
    SetKnowsBar( true );

    if( !aFiles )
        THROW_PARSE_ERROR( "No embedded files object provided", CurSource(), CurLine(),
                           CurLineNumber(), CurOffset() );

    using namespace EMBEDDED_FILES_T;

    std::unique_ptr<EMBEDDED_FILES::EMBEDDED_FILE> file( nullptr );

    for( T token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_file )
            Expecting( "file" );

        if( file )
        {
            if( !file->compressedEncodedData.empty() )
            {
                EMBEDDED_FILES::DecompressAndDecode( *file );

                if( !file->Validate() )
                    THROW_PARSE_ERROR( "Checksum error in embedded file " + file->name, CurSource(),
                                        CurLine(), CurLineNumber(), CurOffset() );
            }

            aFiles->AddFile( file.release() );
        }

        file = std::unique_ptr<EMBEDDED_FILES::EMBEDDED_FILE>( nullptr );

        for( token = NextTok(); token != T_RIGHT; token = NextTok() )
        {
            if( token != T_LEFT )
                Expecting( T_LEFT );

            token = NextTok();

            switch( token )
            {
            case T_checksum:
                if( !file )
                    Expecting( T_name );

                NeedSYMBOLorNUMBER();

                if( !IsSymbol( token ) )
                    Expecting( "checksum data" );

                file->data_hash = CurStr();
                NeedRIGHT();
                break;

            case T_data:
                if( !file )
                    Expecting( T_name);

                try
                {
                    NeedBAR();
                }
                catch( const PARSE_ERROR& e )
                {
                    // No data in the file -- due to bug in writer for 9.0.0
                    if( curTok == T_RIGHT )
                        break;
                    else
                        throw e;
                }
                catch( ... )
                {
                    throw;
                }

                token = NextTok();

                file->compressedEncodedData.reserve( 1 << 17 );

                while( token != T_BAR )
                {
                    if( !IsSymbol( token ) )
                        Expecting( "base64 file data" );

                    file->compressedEncodedData += CurStr();
                    token = NextTok();
                }

                file->compressedEncodedData.shrink_to_fit();

                NeedRIGHT();
                break;

            case T_name:
                if( file )
                {
                    wxLogTrace( wxT( "KICAD_EMBED" ),
                                wxT( "Duplicate 'name' tag in embedded file %s" ), file->name );
                }

                NeedSYMBOLorNUMBER();

                file = std::make_unique<EMBEDDED_FILES::EMBEDDED_FILE>();
                file->name = CurStr();
                NeedRIGHT();

                break;

            case T_type:
                if( !file )
                    Expecting( T_name );

                token = NextTok();

                switch( token )
                {
                case T_datasheet: file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::DATASHEET; break;
                case T_font:      file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::FONT;      break;
                case T_model:     file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::MODEL;     break;
                case T_worksheet: file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::WORKSHEET; break;
                case T_other:     file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::OTHER;     break;
                default:          Expecting( "datasheet, font, model, worksheet or other" );        break;
                }

                NeedRIGHT();
                break;

            default:
                Expecting( "checksum, data or name" );
            }
        }
    }

    // Add the last file in the collection
    if( file )
    {
        if( !file->compressedEncodedData.empty() )
        {
            if( EMBEDDED_FILES::DecompressAndDecode( *file ) == EMBEDDED_FILES::RETURN_CODE::CHECKSUM_ERROR )
                THROW_PARSE_ERROR( "Checksum error in embedded file " + file->name, CurSource(),
                                    CurLine(), CurLineNumber(), CurOffset() );
        }

        aFiles->AddFile( file.release() );
    }
}


wxFileName EMBEDDED_FILES::GetTemporaryFileName( const wxString& aName ) const
{
    wxFileName cacheFile;

    auto it = m_files.find( aName );

    if( it == m_files.end() )
        return cacheFile;

    cacheFile.AssignDir( PATHS::GetUserCachePath() );
    cacheFile.AppendDir( wxT( "embed" ) );

    if( !PATHS::EnsurePathExists( cacheFile.GetFullPath() ) )
    {
        wxLogTrace( wxT( "KICAD_EMBED" ),
                    wxT( "%s:%s:%d\n * failed to create embed cache directory '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, cacheFile.GetPath() );

        cacheFile.SetPath( wxFileName::GetTempDir() );
    }

    wxFileName inputName( aName );

    // Store the cache file name using the data hash to allow for shared data between
    // multiple projects using the same files as well as deconflicting files with the same name
    cacheFile.SetName( "kicad_embedded_" + it->second->data_hash );
    cacheFile.SetExt( inputName.GetExt() );

    if( cacheFile.FileExists() && cacheFile.IsFileReadable() )
        return cacheFile;

    wxFFileOutputStream out( cacheFile.GetFullPath() );

    if( !out.IsOk() )
    {
        cacheFile.Clear();
        return cacheFile;
    }

    out.Write( it->second->decompressedData.data(), it->second->decompressedData.size() );

    return cacheFile;
}


const std::vector<wxString>* EMBEDDED_FILES::GetFontFiles() const
{
    return &m_fontFiles;
}


const std::vector<wxString>* EMBEDDED_FILES::UpdateFontFiles()
{
    m_fontFiles.clear();

    for( const auto& [name, entry] : m_files )
    {
        if( entry->type == EMBEDDED_FILE::FILE_TYPE::FONT )
            m_fontFiles.push_back( GetTemporaryFileName( name ).GetFullPath() );
    }

    return &m_fontFiles;
}


// Move constructor
EMBEDDED_FILES::EMBEDDED_FILES( EMBEDDED_FILES&& other ) noexcept :
        m_files( std::move( other.m_files ) ),
        m_fontFiles( std::move( other.m_fontFiles ) ),
        m_fileAddedCallback( std::move( other.m_fileAddedCallback ) ),
        m_embedFonts( other.m_embedFonts )
{
    other.m_embedFonts = false;
}


// Move assignment operator
EMBEDDED_FILES& EMBEDDED_FILES::operator=(EMBEDDED_FILES&& other) noexcept
{
    if (this != &other)
    {
        ClearEmbeddedFiles();
        m_files = std::move( other.m_files );
        m_fontFiles = std::move( other.m_fontFiles );
        m_fileAddedCallback = std::move( other.m_fileAddedCallback );
        m_embedFonts = other.m_embedFonts;
        other.m_embedFonts = false;
    }

    return *this;
}


// Copy constructor
EMBEDDED_FILES::EMBEDDED_FILES( const EMBEDDED_FILES& other ) :
        m_embedFonts( other.m_embedFonts )
{
    for( const auto& [name, file] : other.m_files )
        m_files[name] = new EMBEDDED_FILE( *file );

    m_fontFiles = other.m_fontFiles;
    m_fileAddedCallback = other.m_fileAddedCallback;
}


EMBEDDED_FILES::EMBEDDED_FILES( const EMBEDDED_FILES& other, bool aDeepCopy ) :
        m_embedFonts( other.m_embedFonts )
{
    if( aDeepCopy )
    {
        for( const auto& [name, file] : other.m_files )
            m_files[name] = new EMBEDDED_FILE( *file );

        m_fontFiles = other.m_fontFiles;
    }

    m_fileAddedCallback = other.m_fileAddedCallback;
}


// Copy assignment operator
EMBEDDED_FILES& EMBEDDED_FILES::operator=( const EMBEDDED_FILES& other )
{
    if( this != &other )
    {
        ClearEmbeddedFiles();

        for( const auto& [name, file] : other.m_files )
            m_files[name] = new EMBEDDED_FILE( *file );

        m_fontFiles = other.m_fontFiles;
        m_fileAddedCallback = other.m_fileAddedCallback;
        m_embedFonts = other.m_embedFonts;
    }

    return *this;
}
