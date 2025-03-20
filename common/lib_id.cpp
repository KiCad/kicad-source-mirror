/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <cstring>
#include <memory>
#include <wx/translation.h>
#include <fmt/format.h>

#include <ki_exception.h>
#include <macros.h>     // TO_UTF8()
#include <lib_id.h>


static int checkLibNickname( const UTF8& aField )
{
    // std::string::npos is largest positive number, casting to int makes it -1.
    // Returning that means success.
    return int( aField.find_first_of( ":" ) );
}


void LIB_ID::clear()
{
    m_libraryName.clear();
    m_itemName.clear();
    m_subLibraryName.clear();
}


int LIB_ID::Parse( const UTF8& aId, bool aFix )
{
    clear();

    size_t partNdx;
    int    offset = -1;

    //=====<library nickname>=============================
    if( ( partNdx = aId.find( ':' ) ) != aId.npos )
    {
        offset = SetLibNickname( aId.substr( 0, partNdx ) );

        if( offset > -1 )
            return offset;

        ++partNdx;  // skip ':'
    }
    else
    {
        partNdx = 0;
    }

    //=====<item name>====================================
    UTF8 fpname = aId.substr( partNdx );

    // Be sure the item name is valid.
    // Some chars can be found in legacy files converted files from other EDA tools.
    if( aFix )
        fpname = FixIllegalChars( fpname, false );
    else
        offset = HasIllegalChars( fpname );

    if( offset > -1 )
        return offset;

    SetLibItemName( fpname );

    return -1;
}


LIB_ID::LIB_ID( const wxString& aLibraryName, const wxString& aItemName ) :
        m_libraryName( aLibraryName ),
        m_itemName( aItemName )
{
}


int LIB_ID::SetLibNickname( const UTF8& aLibNickname )
{
    int offset = checkLibNickname( aLibNickname );

    if( offset == -1 )
        m_libraryName = aLibNickname;

    return offset;
}


int LIB_ID::SetLibItemName( const UTF8& aLibItemName )
{
    m_itemName = aLibItemName;

    return -1;
}


UTF8 LIB_ID::Format() const
{
    UTF8    ret;

    if( m_libraryName.size() )
    {
        ret += m_libraryName;
        ret += ':';
    }

    ret += m_itemName;

    return ret;
}


UTF8 LIB_ID::Format( const UTF8& aLibraryName, const UTF8& aLibItemName )
{
    UTF8    ret;
    int     offset;

    if( aLibraryName.size() )
    {
        offset = checkLibNickname( aLibraryName );

        if( offset != -1 )
        {
            THROW_PARSE_ERROR( _( "Illegal character found in library nickname" ),
                               wxString::FromUTF8( aLibraryName.c_str() ), aLibraryName.c_str(),
                               0, offset );
        }

        ret += aLibraryName;
        ret += ':';
    }

    ret += aLibItemName;

    return ret;
}


int LIB_ID::compare( const LIB_ID& aLibId ) const
{
    // Don't bother comparing the same object.
    if( this == &aLibId )
        return 0;

    int retv = m_libraryName.compare( aLibId.m_libraryName );

    if( retv != 0 )
        return retv;

    return m_itemName.compare( aLibId.m_itemName );
}


int LIB_ID::HasIllegalChars( const UTF8& aLibItemName )
{
    int offset = 0;

    for( auto& ch : aLibItemName )
    {
        if( !isLegalChar( ch ) )
            return offset;
        else
            ++offset;
    }

    return -1;
}


UTF8 LIB_ID::FixIllegalChars( const UTF8& aLibItemName, bool aLib )
{
    UTF8 fixedName;

    for( UTF8::uni_iter chIt = aLibItemName.ubegin(); chIt < aLibItemName.uend(); ++chIt )
    {
        auto ch = *chIt;
        if( aLib )
            fixedName += isLegalLibraryNameChar( ch ) ? ch : '_';
        else
            fixedName += isLegalChar( ch ) ? ch : '_';
    }

    return fixedName;
}


bool LIB_ID::isLegalChar( unsigned aUniChar )
{
    bool const space_allowed = true;
    bool const illegal_filename_chars_allowed = false;

    // This list of characters is also duplicated in validators.cpp and footprint.cpp
    // TODO: Unify forbidden character lists - Warning, invalid filename characters are not the same
    // as invalid LIB_ID characters.  We will need to separate the FP filenames from FP names
    // before this can be unified
    switch( aUniChar )
    {
    case ':':
    case '\t':
    case '\n':
    case '\r':
        return false;

    case '\\':
    case '<':
    case '>':
    case '"':
        return illegal_filename_chars_allowed;

    case ' ':
        return space_allowed;

    default:
        return true;
    }
}


unsigned LIB_ID::FindIllegalLibraryNameChar( const UTF8& aLibraryName )
{
    for( unsigned ch : aLibraryName )
    {
        if( !isLegalLibraryNameChar( ch ) )
            return ch;
    }

    return 0;
}


bool LIB_ID::isLegalLibraryNameChar( unsigned aUniChar )
{
    bool const space_allowed = true;

    if( aUniChar < ' ' )
        return false;

    switch( aUniChar )
    {
    case '\\':
    case ':':
        return false;

    case ' ':
        return space_allowed;

    default:
        return true;
    }
}


const wxString LIB_ID::GetFullLibraryName() const
{
    if( m_subLibraryName.empty() )
        return m_libraryName;

    return wxString::Format( wxS( "%s - %s" ), m_libraryName.c_str(), m_subLibraryName.c_str() );
}
