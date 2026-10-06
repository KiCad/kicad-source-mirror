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

#include <io/easypc/easypc_design_settings.h>

#include <cmath>
#include <cstring>
#include <map>

#include <boost/endian/conversion.hpp>

#include <io/easypc/easypc_document.h>
#include <ki_exception.h>
#include <title_block.h>
#include <wx/datetime.h>
#include <wx/strconv.h>
#include <wx/translation.h>


namespace EASYPC
{

namespace
{

    // FMTID_SummaryInformation {F29F85E0-4FF9-1068-AB91-08002B27B3D9} as stored
    const uint8_t FMTID_SUMMARY[16] = { 0xE0, 0x85, 0x9F, 0xF2, 0xF9, 0x4F, 0x68, 0x10,
                                        0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9 };

    constexpr uint32_t VT_LPSTR = 30;
    constexpr uint32_t VT_FILETIME = 64;
    constexpr uint32_t PID_LASTSAVE_DTM = 13;

    // Seconds between the FILETIME epoch (1601) and the Unix epoch
    constexpr int64_t FILETIME_UNIX_OFFSET = 11644473600LL;

} // namespace


void FillTitleBlock( const DESIGN_DOCUMENT& aDoc, TITLE_BLOCK& aTitleBlock )
{
    // An OLE property set holding the design's document properties
    const std::vector<uint8_t>& data = aDoc.SummaryInformation;

    if( data.empty() )
        return;

    auto bytes = [&]( size_t aOffset, size_t aLen ) -> const uint8_t*
    {
        if( aOffset > data.size() || data.size() - aOffset < aLen )
            THROW_IO_ERROR( wxString::Format( _( "Easy-PC SummaryInformation is cut short at %zu." ), aOffset ) );

        return data.data() + aOffset;
    };

    auto u32 = [&]( size_t aOffset )
    {
        return boost::endian::load_little_u32( bytes( aOffset, 4 ) );
    };

    if( ( u32( 0 ) & 0xFFFF ) != 0xFFFE )
        THROW_IO_ERROR( _( "Easy-PC SummaryInformation has no byte order mark." ) );

    size_t section = 0;

    for( uint32_t i = 0; i < u32( 24 ) && !section; ++i )
    {
        if( std::memcmp( bytes( 28 + size_t( i ) * 20, 16 ), FMTID_SUMMARY, 16 ) == 0 )
            section = u32( 28 + size_t( i ) * 20 + 16 );
    }

    if( !section )
        return;

    static const wxCSConv        cp1252( wxFONTENCODING_CP1252 );
    std::map<uint32_t, wxString> text;
    wxString                     saved;

    for( uint32_t i = 0; i < u32( section + 4 ); ++i )
    {
        size_t   entry = section + 8 + size_t( i ) * 8;
        uint32_t pid = u32( entry );
        size_t   prop = section + u32( entry + 4 );
        uint32_t type = u32( prop ) & 0xFFFF;

        if( type == VT_LPSTR )
        {
            // The length counts the terminating NUL
            uint32_t    len = u32( prop + 4 );
            const char* chars = reinterpret_cast<const char*>( bytes( prop + 8, len ) );
            text[pid] = wxString( chars, cp1252, strnlen( chars, len ) );
        }
        else if( type == VT_FILETIME && pid == PID_LASTSAVE_DTM )
        {
            uint64_t ft = u32( prop + 4 ) | ( uint64_t( u32( prop + 8 ) ) << 32 );

            if( ft )
                saved = wxDateTime( time_t( int64_t( ft / 10000000ULL ) - FILETIME_UNIX_OFFSET ) ).FormatISODate();
        }
    }

    aTitleBlock.SetTitle( text[2] );

    if( !saved.IsEmpty() )
        aTitleBlock.SetDate( saved );

    // Subject, author, keywords and comments
    for( int i = 0; i < 4; ++i )
        aTitleBlock.SetComment( i, text[3 + i] );
}


PAGE_INFO PageForExtents( const BOX2L& aExtentsNm )
{
    const double needW = aExtentsNm.GetWidth() / 1e6 + 2 * PAGE_MARGIN_MM;
    const double needH = aExtentsNm.GetHeight() / 1e6 + 2 * PAGE_MARGIN_MM;

    for( PAGE_SIZE_TYPE type :
         { PAGE_SIZE_TYPE::A4, PAGE_SIZE_TYPE::A3, PAGE_SIZE_TYPE::A2, PAGE_SIZE_TYPE::A1, PAGE_SIZE_TYPE::A0,
           PAGE_SIZE_TYPE::A, PAGE_SIZE_TYPE::B, PAGE_SIZE_TYPE::C, PAGE_SIZE_TYPE::D, PAGE_SIZE_TYPE::E } )
    {
        for( bool portrait : { false, true } )
        {
            PAGE_INFO page( type, portrait );

            if( page.GetWidthMM() >= needW && page.GetHeightMM() >= needH )
                return page;
        }
    }

    PAGE_INFO user( PAGE_SIZE_TYPE::User );
    user.SetWidthMM( std::ceil( needW ) );
    user.SetHeightMM( std::ceil( needH ) );
    return user;
}

} // namespace EASYPC
