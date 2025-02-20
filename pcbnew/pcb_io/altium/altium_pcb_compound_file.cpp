/*
 * This program is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <altium_pcb_compound_file.h>
#include <utf.h>

#include <wx/string.h>

#include <compoundfilereader.h>
#include <map>

ALTIUM_PCB_COMPOUND_FILE::ALTIUM_PCB_COMPOUND_FILE( const wxString& aFilePath )
    : ALTIUM_COMPOUND_FILE( aFilePath )
{
}

ALTIUM_PCB_COMPOUND_FILE::ALTIUM_PCB_COMPOUND_FILE( const void* aBuffer, size_t aLen )
    : ALTIUM_COMPOUND_FILE( aBuffer, aLen )
{
}


ALTIUM_PCB_COMPOUND_FILE::~ALTIUM_PCB_COMPOUND_FILE()
{
}

CASE_INSENSITIVE_MAP<wxString> ALTIUM_PCB_COMPOUND_FILE::ListLibFootprints()
{
    if( m_libFootprintDirNameCache.empty() )
        cacheLibFootprintNames();

    return m_libFootprintDirNameCache;
}


std::tuple<wxString, const CFB::COMPOUND_FILE_ENTRY*>
ALTIUM_PCB_COMPOUND_FILE::FindLibFootprintDirName( const wxString& aFpUnicodeName )
{
    if( m_libFootprintNameCache.empty() )
        cacheLibFootprintNames();

    auto it = m_libFootprintNameCache.find( aFpUnicodeName );

    if( it == m_libFootprintNameCache.end() )
        return { wxEmptyString, nullptr };

    return { it->first, it->second };
}


const std::pair<AMODEL, std::vector<char>>* ALTIUM_PCB_COMPOUND_FILE::GetLibModel( const wxString& aModelName ) const
{
    auto it = m_libModelsCache.find( aModelName );

    if( it == m_libModelsCache.end() )
        return nullptr;

    return &it->second;
}


void ALTIUM_PCB_COMPOUND_FILE::cacheLibFootprintNames()
{
    m_libFootprintDirNameCache.clear();
    m_libFootprintNameCache.clear();

    if( !m_reader )
        return;

    const CFB::COMPOUND_FILE_ENTRY* root = m_reader->GetRootEntry();

    if( !root )
        return;

    m_reader->EnumFiles( root, 1,
        [this]( const CFB::COMPOUND_FILE_ENTRY* tentry, const CFB::utf16string& dir, int level ) -> int
        {
            if( m_reader->IsStream( tentry ) )
                return 0;

            m_reader->EnumFiles( tentry, 1,
                        [&]( const CFB::COMPOUND_FILE_ENTRY* entry, const CFB::utf16string&, int ) -> int
                        {
                            std::wstring fileName = UTF16ToWstring( entry->name, entry->nameLen );

                            if( m_reader->IsStream( entry ) && fileName == L"Parameters" )
                            {
                                ALTIUM_BINARY_PARSER         parametersReader( *this, entry );
                                std::map<wxString, wxString> parameterProperties =
                                        parametersReader.ReadProperties();

                                wxString key = ALTIUM_PROPS_UTILS::ReadString(
                                        parameterProperties, wxT( "PATTERN" ), wxT( "" ) );
                                wxString fpName = ALTIUM_PROPS_UTILS::ReadUnicodeString(
                                        parameterProperties, wxT( "PATTERN" ), wxT( "" ) );

                                m_libFootprintDirNameCache[key] = fpName;
                                m_libFootprintNameCache[fpName] = tentry;
                            }

                            return 0;
                        } );
            return 0;
            } );
}


bool ALTIUM_PCB_COMPOUND_FILE::CacheLibModels()
{
    const CFB::COMPOUND_FILE_ENTRY* models_root = nullptr;
    const CFB::COMPOUND_FILE_ENTRY* models_data = nullptr;
    bool found = false;

    if( !m_reader || !m_libModelsCache.empty() )
        return false;

    models_data = FindStream( { "Library", "Models", "Data" } );

    if( !models_data )
        return false;

    ALTIUM_BINARY_PARSER parser( *this, models_data );

    if( parser.GetRemainingBytes() == 0 )
        return false;

    std::vector<AMODEL> models;

    // First, we parse and extract the model data from the Data stream
    while( parser.GetRemainingBytes() >= 4 )
    {
        AMODEL elem( parser );
        models.push_back( elem );
    }

    // Next, we need the model directory entry to read the compressed model streams
    m_reader->EnumFiles( m_reader->GetRootEntry(), 2, [&]( const CFB::COMPOUND_FILE_ENTRY* entry, const CFB::utf16string& dir, int ) -> int
    {
        if( found )
            return 1;

        if( m_reader->IsStream( entry ) )
            return 0;

        std::string dir_str = UTF16ToUTF8( dir.c_str() );
        std::string entry_str = UTF16ToUTF8( entry->name );

        if( dir_str.compare( "Library" ) == 0 && entry_str.compare( "Models" ) == 0 )
        {
            models_root = entry;
            found = true;
            return 1;
        }

        return 0;
    });

    if( !models_root )
        return false;

    m_reader->EnumFiles(
        models_root, 1,
        [&]( const CFB::COMPOUND_FILE_ENTRY* stepEntry, const CFB::utf16string&, int ) -> int
        {
            long     fileNumber;
            wxString fileName = UTF16ToUTF8( stepEntry->name, stepEntry->nameLen );

            if( !fileName.ToLong( &fileNumber ) )
                return 0;

            if( !m_reader->IsStream( stepEntry ) || fileNumber >= long( models.size() ) )
                return 0;

            size_t            stepSize = static_cast<size_t>( stepEntry->size );
            std::vector<char> stepContent( stepSize );

            // read file into buffer
            m_reader->ReadFile( stepEntry, 0, stepContent.data(), stepSize );

            if( stepContent.empty() )
                return 0;

            // We store the models in their original compressed form so as to speed the caching process
            // When we parse an individual footprint, we decompress and recompress the model data into
            // our format
            wxString modelName = models[fileNumber].id;
            m_libModelsCache.emplace( modelName, std::make_pair( std::move( models[fileNumber] ),
                                                                 std::move( stepContent ) ) );
            return 0;
        } );

    return true;
}
