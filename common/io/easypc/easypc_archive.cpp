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

#include <io/easypc/easypc_archive.h>

#include <algorithm>
#include <cstring>

#include <boost/endian/conversion.hpp>

#include <ki_exception.h>
#include <wx/debug.h>
#include <wx/strconv.h>
#include <wx/translation.h>


namespace EASYPC
{

namespace
{

    constexpr uint16_t NEW_CLASS_TAG = 0xFFFF;
    constexpr uint16_t CLASS_TAG = 0x8000;
    constexpr uint32_t BIG_CLASS_TAG = 0x80000000;
    constexpr uint16_t BIG_OBJECT_TAG = 0x7FFF;

    // Class names are shorter than 64 bytes
    constexpr uint16_t MAX_CLASS_NAME = 64;


    const REGISTRY& registry()
    {
        static const REGISTRY reg = []()
        {
            REGISTRY r;
            RegisterRootClasses( r );
            RegisterStyleClasses( r );
            RegisterGeometryClasses( r );
            RegisterLibraryClasses( r );
            RegisterConnectivityClasses( r );
            return r;
        }();

        return reg;
    }


    LOAD_TASK readRoot( ARCHIVE& aAr, OBJECT*& aResult )
    {
        aResult = co_await aAr.Object();
    }


    LOAD_TASK embedRoot( ARCHIVE& aAr, OBJECT& aObject, const char* aClass )
    {
        co_await aAr.Embedded( aObject, aClass );
    }

} // namespace


LOAD_TASK OBJECT::Load( ARCHIVE& aAr )
{
    co_return;
}


int OBJECT::ItemFormat( const ARCHIVE& aAr ) const
{
    return aAr.CurrentReadFormat();
}


ARCHIVE::ARCHIVE( const uint8_t* aData, size_t aSize, DOC_KIND aKind, int32_t aProduct ) :
        m_data( aData ),
        m_size( aSize ),
        m_kind( aKind ),
        m_product( aProduct )
{
    m_load.emplace_back();
}


void ARCHIVE::Require( size_t aLen ) const
{
    if( aLen > m_size - m_pos )
    {
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC archive ends at offset %zu, %zu bytes short of a %zu byte "
                                             "read." ),
                                          m_pos, aLen - ( m_size - m_pos ), aLen ) );
    }
}


void ARCHIVE::Skip( size_t aLen )
{
    Require( aLen );
    m_pos += aLen;
}


uint8_t ARCHIVE::U8()
{
    Require( 1 );
    return m_data[m_pos++];
}


bool ARCHIVE::InBool()
{
    return m_boolIsByte ? U8() != 0 : U32() != 0;
}


uint16_t ARCHIVE::U16()
{
    Require( 2 );
    m_pos += 2;
    return boost::endian::load_little_u16( m_data + m_pos - 2 );
}


uint32_t ARCHIVE::U32()
{
    Require( 4 );
    m_pos += 4;
    return boost::endian::load_little_u32( m_data + m_pos - 4 );
}


uint64_t ARCHIVE::U64()
{
    Require( 8 );
    m_pos += 8;
    return boost::endian::load_little_u64( m_data + m_pos - 8 );
}


double ARCHIVE::F64()
{
    uint64_t bits = U64();
    double   v;
    std::memcpy( &v, &bits, sizeof( v ) );
    return v;
}


uint32_t ARCHIVE::Count()
{
    uint16_t n = U16();
    return n != 0xFFFF ? n : U32();
}


uint32_t ARCHIVE::stringLength( bool& aUnicode )
{
    // The length is a byte; 0xFF escapes to a WORD, whose 0xFFFE marks UTF-16 and restarts the length
    aUnicode = false;
    uint32_t len = U8();

    if( len == 0xFF )
    {
        len = U16();

        if( len == 0xFFFE )
        {
            aUnicode = true;
            len = U8();

            if( len == 0xFF )
                len = U16();
        }

        if( len == 0xFFFF )
            len = U32();
    }

    return len;
}


wxString ARCHIVE::ReadString()
{
    bool     unicode;
    uint32_t len = stringLength( unicode );
    size_t   bytes = unicode ? static_cast<size_t>( len ) * 2 : len;

    Require( bytes );

    const char* text = reinterpret_cast<const char*>( m_data + m_pos );
    m_pos += bytes;

    if( unicode )
        return wxString( text, wxMBConvUTF16LE(), bytes );

    static const wxCSConv cp1252( wxFONTENCODING_CP1252 );
    return wxString( text, cp1252, bytes );
}


void ARCHIVE::SkipString()
{
    bool     unicode;
    uint32_t len = stringLength( unicode );
    Skip( unicode ? static_cast<size_t>( len ) * 2 : len );
}


OBJECT* ARCHIVE::readTag( bool& aIsNew )
{
    aIsNew = false;

    const size_t   tag_pos = m_pos;
    const uint16_t short_tag = U16();
    uint32_t       tag = short_tag;

    if( short_tag == BIG_OBJECT_TAG )
        tag = U32();
    else if( short_tag & CLASS_TAG )
        tag = BIG_CLASS_TAG | ( short_tag & ~CLASS_TAG );

    const char* class_name = nullptr;
    uint16_t    class_schema = 0;

    if( short_tag == NEW_CLASS_TAG )
    {
        class_schema = U16();
        uint16_t name_length = U16();

        if( name_length >= MAX_CLASS_NAME )
            THROW_IO_ERROR(
                    wxString::Format( _( "Easy-PC class name of %u bytes at offset %zu." ), name_length, tag_pos ) );

        Require( name_length );
        std::string_view name( reinterpret_cast<const char*>( m_data + m_pos ), name_length );
        m_pos += name_length;

        auto it = registry().find( name );

        if( it == registry().end() )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unsupported Easy-PC object '%s' at offset %zu." ),
                                              wxString::FromUTF8( name.data(), name.size() ), tag_pos ) );
        }

        class_name = it->first.c_str();
        m_load.push_back( SLOT{ class_name, class_schema, nullptr } );
    }
    else if( tag & BIG_CLASS_TAG )
    {
        uint32_t index = tag & ~BIG_CLASS_TAG;

        if( index >= m_load.size() || !m_load[index].ClassName || m_load[index].Object )
        {
            THROW_IO_ERROR( wxString::Format( _( "Easy-PC class reference to slot %u at offset %zu is not a "
                                                 "class." ),
                                              index, tag_pos ) );
        }

        class_name = m_load[index].ClassName;
        class_schema = m_load[index].Schema;
    }
    else
    {
        if( tag == 0 )
            return nullptr;

        if( tag >= m_load.size() || !m_load[tag].Object )
        {
            THROW_IO_ERROR( wxString::Format( _( "Easy-PC object reference to slot %u at offset %zu is not an "
                                                 "object." ),
                                              tag, tag_pos ) );
        }

        return m_load[tag].Object;
    }

    // Nested objects are heap frames, so this only bounds what a crafted stream can allocate
    if( m_depth + 1 > MAX_DEPTH )
    {
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC archive nests deeper than %d objects at offset %zu." ), MAX_DEPTH,
                                          tag_pos ) );
    }

    std::unique_ptr<OBJECT> created = registry().find( class_name )->second();
    OBJECT*                 obj = created.get();
    obj->ClassName = class_name;
    obj->Schema = class_schema;
    obj->Offset = tag_pos;

    // The new object takes its slot before its fields, so references to it from inside resolve
    m_load.push_back( SLOT{ class_name, class_schema, obj } );
    m_objects.push_back( std::move( created ) );

    aIsNew = true;
    return obj;
}


bool ARCHIVE::OBJECT_AWAITER::await_ready()
{
    bool isNew = false;
    m_object = m_archive.readTag( isNew );
    return !isNew;
}


void ARCHIVE::OBJECT_AWAITER::await_suspend( std::coroutine_handle<> aAwaiting )
{
    m_archive.pushObjectLoad( *m_object, m_object->Schema );
}


void ARCHIVE::EMBEDDED_AWAITER::await_suspend( std::coroutine_handle<> aAwaiting )
{
    m_object.Offset = m_archive.m_pos;

    // An embedded member keeps the schema of the object around it
    m_archive.pushObjectLoad( m_object, m_archive.m_objectSchema );
}


void LOAD_TASK::await_suspend( std::coroutine_handle<> aAwaiting )
{
    m_handle.promise().m_archive->pushTask( Release() );
}


void ARCHIVE::pushObjectLoad( OBJECT& aObject, uint16_t aSchema )
{
    if( m_depth + 1 > MAX_DEPTH )
    {
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC archive nests deeper than %d objects at offset %zu." ), MAX_DEPTH,
                                          m_pos ) );
    }

    m_frames.push_back( FRAME{ aObject.Load( *this ).Release(), &aObject, m_objectSchema, true } );
    m_objectSchema = aSchema;
    ++m_depth;
}


void ARCHIVE::pushTask( LOAD_TASK::HANDLE aHandle )
{
    OBJECT* owner = m_frames.empty() ? nullptr : m_frames.back().Object;
    m_frames.push_back( FRAME{ aHandle, owner, m_objectSchema, false } );
}


void ARCHIVE::drive( LOAD_TASK aRoot )
{
    const size_t base = m_frames.size();
    pushTask( aRoot.Release() );

    auto pop = [&]()
    {
        FRAME frame = m_frames.back();
        m_frames.pop_back();
        frame.Handle.destroy();
        m_objectSchema = frame.SavedSchema;

        if( frame.ObjectLoad )
            --m_depth;
    };

    while( m_frames.size() > base )
    {
        // Resuming may push frames, so the top is fetched afresh each time round
        LOAD_TASK::HANDLE top = m_frames.back().Handle;

        if( !top.done() )
        {
            top.resume();
            continue;
        }

        std::exception_ptr error = top.promise().m_error;

        if( !error )
        {
            pop();
            continue;
        }

        // An embedded owner lives in a loader's frame, so it is described before the frames are destroyed
        const OBJECT* owner = m_frames.back().Object;
        wxString      ownerName = owner ? wxString::FromUTF8( owner->ClassName ) : wxString();
        size_t        ownerOffset = owner ? owner->Offset : 0;

        while( m_frames.size() > base )
            pop();

        try
        {
            std::rethrow_exception( error );
        }
        catch( const IO_ERROR& e )
        {
            // Only the innermost object is named
            if( m_errorHasContext || !owner )
                throw;

            m_errorHasContext = true;
            THROW_IO_ERROR( wxString::Format( wxS( "%s at offset 0x%zX: %s" ), ownerName, ownerOffset, e.Problem() ) );
        }
    }
}


OBJECT* ARCHIVE::ReadObject()
{
    OBJECT* result = nullptr;
    drive( readRoot( *this, result ) );
    return result;
}


void ARCHIVE::LoadEmbedded( OBJECT& aObject, const char* aClass )
{
    drive( embedRoot( *this, aObject, aClass ) );
}


void ARCHIVE::MapObject( OBJECT* aObject )
{
    m_load.push_back( SLOT{ aObject ? aObject->ClassName : nullptr, 0, aObject } );
}


std::unique_ptr<OBJECT> ARCHIVE::Create( const char* aClass )
{
    auto it = registry().find( std::string_view( aClass ) );

    if( it == registry().end() )
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC class '%s' is not registered." ), aClass ) );

    std::unique_ptr<OBJECT> obj = it->second();
    obj->ClassName = it->first.c_str();
    return obj;
}


LOAD_TASK SkipFields( ARCHIVE& aAr, std::string_view aOps )
{
    for( size_t i = 0; i < aOps.size(); )
    {
        char   op = aOps[i++];
        size_t count = 0;

        // A space separates ops; read as one it would take a following '1' op as its count
        if( op == ' ' )
            continue;

        for( ; i < aOps.size() && aOps[i] >= '0' && aOps[i] <= '9'; ++i )
            count = count * 10 + ( aOps[i] - '0' );

        for( count = std::max<size_t>( count, 1 ); count > 0; --count )
        {
            switch( op )
            {
            case 'b': aAr.InBool(); break;
            case '1': aAr.Skip( 1 ); break;
            case 'i': aAr.Skip( 4 ); break;
            case 'f': aAr.Skip( 8 ); break;
            case 's': aAr.SkipString(); break;
            case 'o': co_await aAr.Object(); break;
            case 'a': aAr.Skip( 4 * size_t( aAr.Count() ) ); break;
            default: wxFAIL_MSG( wxS( "Bad SkipFields op" ) );
            }
        }
    }
}


LOAD_TASK SkipRows( ARCHIVE& aAr, int aVersion, std::span<const FIELD_ROW> aRows )
{
    for( const FIELD_ROW& row : aRows )
    {
        if( aVersion >= row.Since && aVersion <= row.Until )
            co_await SkipFields( aAr, row.Ops );
    }
}


int ARCHIVE::OrphanFormat() const
{
    // Only a design has a document for a null parent to mean
    return IsDesign() ? m_documentVersion : m_currentReadFormat;
}


void ARCHIVE::throwBadClass( const OBJECT& aObject ) const
{
    THROW_IO_ERROR( wxString::Format( _( "Easy-PC object '%s' at offset %zu is not of the class required there." ),
                                      aObject.ClassName, aObject.Offset ) );
}

} // namespace EASYPC
