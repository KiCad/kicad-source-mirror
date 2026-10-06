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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <io/easypc/easypc_file.h>

#include <algorithm>
#include <cstring>
#include <set>

#include <boost/endian/conversion.hpp>
#include <compoundfilereader.h>

#include <ki_exception.h>
#include <wx/ffile.h>
#include <wx/translation.h>


namespace EASYPC
{

namespace
{

    constexpr uint32_t NOSTREAM = 0xFFFFFFFF;

    const char* const FILE_TYPE_TABLE[] = { "PCB Design", "Schematic Design", "PCB Symbol Library",
                                            "Schematic Symbol Library", "Component Library" };

    const char16_t ESCAPED_CHARS[] = { u'#', u'!', u'\\', u'/', u':', u'*', u'?', u'"', u'<', u'>', u'|' };


    [[noreturn]] void corrupt( const wxString& aWhat )
    {
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC compound file is malformed: %s." ), aWhat ) );
    }


    /// Run a compoundfilereader call, turning its exceptions into IO_ERROR
    template <typename FN>
    auto guarded( FN aFn ) -> decltype( aFn() )
    {
        try
        {
            return aFn();
        }
        catch( const IO_ERROR& )
        {
            throw;
        }
        catch( const std::exception& e )
        {
            corrupt( wxString::FromUTF8( e.what() ) );
        }
    }

} // namespace


const char* FileKindString( FILE_KIND aKind )
{
    return FILE_TYPE_TABLE[static_cast<size_t>( aKind )];
}


wxString UnescapeItemName( const std::u16string& aName )
{
    wxString result;

    for( char16_t ch : aName )
        result.Append( static_cast<wxUniChar>( ch >= 0x06 && ch <= 0x10 ? ESCAPED_CHARS[ch - 0x06] : ch ) );

    return result;
}


std::vector<uint8_t> ReadWholeFile( const wxString& aPath )
{
    wxFFile      file( aPath, wxS( "rb" ) );
    wxFileOffset len = file.IsOpened() ? file.Length() : -1;

    if( len < 0 )
        THROW_IO_ERROR( wxString::Format( _( "Cannot read file '%s'." ), aPath ) );

    std::vector<uint8_t> data( static_cast<size_t>( len ) );

    if( len > 0 && file.Read( data.data(), data.size() ) != data.size() )
        THROW_IO_ERROR( wxString::Format( _( "Cannot read file '%s'." ), aPath ) );

    return data;
}


COMPOUND_FILE::COMPOUND_FILE( const wxString& aPath ) :
        m_image( ReadWholeFile( aPath ) )
{
    static const uint8_t MAGIC[8] = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };

    if( m_image.size() < 512 || std::memcmp( m_image.data(), MAGIC, 8 ) != 0 )
        THROW_IO_ERROR( _( "Not an Easy-PC / DesignSpark file: no compound file signature." ) );

    m_reader = guarded(
            [&]()
            {
                return std::make_unique<CFB::CompoundFileReader>( m_image.data(), m_image.size() );
            } );

    auto raw = [&]( uint32_t aId )
    {
        const CFB::COMPOUND_FILE_ENTRY* entry = guarded(
                [&]()
                {
                    return m_reader->GetEntry( aId );
                } );

        if( !entry || entry->nameLen < 2 || entry->nameLen > 64 || entry->nameLen % 2 )
            corrupt( wxString::Format( wxS( "directory entry %u" ), aId ) );

        return entry;
    };

    std::set<uint32_t> visited;

    auto add = [&]( uint32_t aId )
    {
        if( !visited.insert( aId ).second )
            corrupt( wxString::Format( wxS( "directory entry %u is reached twice" ), aId ) );

        const CFB::COMPOUND_FILE_ENTRY* entry = raw( aId );
        m_entries[aId] = SOURCE_FB_ENTRY{ aId, std::u16string( entry->name, entry->name + entry->nameLen / 2 - 1 ),
                                          entry->type };
    };

    // Each storage's red-black tree is walked in order, iteratively since the ids come from the file
    std::vector<uint32_t> storages = { 0 };
    add( 0 );

    while( !storages.empty() )
    {
        uint32_t storage = storages.back();
        storages.pop_back();

        std::vector<uint32_t>& members = m_children[storage];
        std::vector<uint32_t>  stack;
        uint32_t               id = raw( storage )->childID;

        while( id != NOSTREAM || !stack.empty() )
        {
            for( ; id != NOSTREAM; id = raw( id )->leftSiblingID )
            {
                stack.push_back( id );
                add( id );
            }

            uint32_t node = stack.back();
            stack.pop_back();
            members.push_back( node );

            if( m_entries.at( node ).IsStorage() )
                storages.push_back( node );

            id = raw( node )->rightSiblingID;
        }
    }
}


COMPOUND_FILE::~COMPOUND_FILE() = default;


std::vector<const SOURCE_FB_ENTRY*> COMPOUND_FILE::Children( const SOURCE_FB_ENTRY& aStorage ) const
{
    std::vector<const SOURCE_FB_ENTRY*> out;

    if( auto it = m_children.find( aStorage.Id ); it != m_children.end() )
    {
        for( uint32_t id : it->second )
            out.push_back( &m_entries.at( id ) );
    }

    return out;
}


const SOURCE_FB_ENTRY* COMPOUND_FILE::FindChild( const SOURCE_FB_ENTRY& aStorage, const std::u16string& aName ) const
{
    for( const SOURCE_FB_ENTRY* entry : Children( aStorage ) )
    {
        if( entry->Name == aName )
            return entry;
    }

    return nullptr;
}


std::vector<uint8_t> COMPOUND_FILE::ReadStream( const SOURCE_FB_ENTRY& aStream ) const
{
    if( !aStream.IsStream() )
        corrupt( wxString::Format( wxS( "directory entry %u is not a stream" ), aStream.Id ) );

    const CFB::COMPOUND_FILE_ENTRY* entry = m_reader->GetEntry( aStream.Id );
    uint64_t                        size = m_reader->GetStreamSize( entry );

    if( size > m_image.size() )
        corrupt( wxString::Format( wxS( "stream %u is larger than the file" ), aStream.Id ) );

    std::vector<uint8_t> out( static_cast<size_t>( size ) );

    if( !out.empty() )
        guarded(
                [&]()
                {
                    m_reader->ReadFile( entry, 0, reinterpret_cast<char*>( out.data() ), out.size() );
                } );

    return out;
}


FILE_KIND COMPOUND_FILE::FileKind() const
{
    const SOURCE_FB_ENTRY* entry = FindChild( Root(), u"\x05"
                                                      u"FileType" );

    if( !entry )
        THROW_IO_ERROR( _( "Easy-PC / DesignSpark file has no FileType stream." ) );

    // The type ends at a newline; libraries from about 2001 have none
    std::vector<uint8_t> data = ReadStream( *entry );
    std::string          text( data.begin(), std::find( data.begin(), data.end(), '\n' ) );

    for( size_t i = 0; i < std::size( FILE_TYPE_TABLE ); ++i )
    {
        if( text == FILE_TYPE_TABLE[i] )
            return static_cast<FILE_KIND>( i );
    }

    THROW_IO_ERROR( wxString::Format( _( "Easy-PC / DesignSpark file type '%s' is not supported." ),
                                      wxString::From8BitData( text.c_str() ) ) );
}


int32_t COMPOUND_FILE::LibraryProduct() const
{
    const SOURCE_FB_ENTRY* entry = FindChild( Root(), u"\x05"
                                                      u"LibFormat" );
    std::vector<uint8_t>   data = entry ? ReadStream( *entry ) : std::vector<uint8_t>();
    int32_t                value = 0;

    // A first int, then a second read into the same variable; a 5 to 7 byte stream overwrites only low bytes
    if( data.size() >= 4 )
    {
        uint8_t bytes[4];
        std::memcpy( bytes, data.data(), 4 );
        std::memcpy( bytes, data.data() + 4, std::min<size_t>( data.size() - 4, 4 ) );
        value = boost::endian::load_little_s32( bytes );
    }

    return value == 0 ? 1 : value;
}

} // namespace EASYPC
