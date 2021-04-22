/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2010-2020 KiCad Developers, see change_log.txt for contributors.
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
#include <wx/wx.h>      // _()

#include <macros.h>     // TO_UTF8()
#include <lib_id.h>
#include <kicad_string.h>


static inline bool isDigit( char c )
{
    return c >= '0' && c <= '9';
}


const char* EndsWithRev( const char* start, const char* tail, char separator )
{
    bool    sawDigit = false;

    while( tail > start && isDigit( *--tail ) )
    {
        sawDigit = true;
    }

    // if sawDigit, tail points to the 'v' here.

    if( sawDigit && tail-3 >= start )
    {
        tail -= 3;

        if( tail[0]==separator && tail[1]=='r' && tail[2]=='e' && tail[3]=='v' )
        {
            return tail+1;  // omit separator, return "revN[N..]"
        }
    }

    return nullptr;
}


#if 0   // Not used
int RevCmp( const char* s1, const char* s2 )
{
    int r = strncmp( s1, s2, 3 );

    if( r || strlen(s1)<4 || strlen(s2)<4 )
    {
        return r;
    }

    int rnum1 = atoi( s1+3 );
    int rnum2 = atoi( s2+3 );

    return -(rnum1 - rnum2);    // swap the sign, higher revs first
}
#endif

//----<Policy and field test functions>-------------------------------------


static inline int okLogical( const UTF8& aField )
{
    // std::string::npos is largest positive number, casting to int makes it -1.
    // Returning that means success.
    return int( aField.find_first_of( ":" ) );
}


static int okRevision( const UTF8& aField )
{
    char rev[32];  // C string for speed

    if( aField.size() >= 4 )
    {
        strncpy( rev, "x/", sizeof( rev ) );
        strncat( rev, aField.c_str(), sizeof(rev)-strlen(rev)-1 );

        if( EndsWithRev( rev, rev + strlen(rev), '/' ) == rev+2 )
            return -1;    // success
    }

    return 0; // first character position "is in error", is best we can do.
}


//----</Policy and field test functions>-------------------------------------


void LIB_ID::clear()
{
    m_libraryName.clear();
    m_itemName.clear();
    m_revision.clear();
}


int LIB_ID::Parse( const UTF8& aId, bool aFix )
{
    clear();

    const char* buffer = aId.c_str();
    const char* rev = EndsWithRev( buffer, buffer+aId.length(), '/' );
    size_t      revNdx;
    size_t      partNdx;
    int         offset = -1;

    //=====<revision>=====================================
    // in a LIB_ID like discret:R3/rev4
    if( rev )
    {
        revNdx = rev - buffer;

        // no need to check revision, EndsWithRev did that.
        m_revision = aId.substr( revNdx );
        --revNdx;  // back up to omit the '/' which precedes the rev
    }
    else
    {
        revNdx = aId.size();
    }

    //=====<name>=========================================
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
    if( partNdx >= revNdx )
        return partNdx;     // Error: no library item name.

    UTF8 fpname = aId.substr( partNdx, revNdx-partNdx );

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


LIB_ID::LIB_ID( const wxString& aLibraryName, const wxString& aItemName,
                const wxString& aRevision ) :
        m_libraryName( aLibraryName ),
        m_itemName( aItemName ),
        m_revision( aRevision )
{
}


int LIB_ID::SetLibNickname( const UTF8& aLogical )
{
    int offset = okLogical( aLogical );

    if( offset == -1 )
        m_libraryName = aLogical;

    return offset;
}


int LIB_ID::SetLibItemName( const UTF8& aLibItemName, bool aTestForRev )
{
    int separation = int( aLibItemName.find_first_of( "/" ) );

    if( aTestForRev && separation != -1 )
    {
        m_itemName = aLibItemName.substr( 0, separation-1 );
        return separation;
    }
    else
    {
        m_itemName = aLibItemName;
    }

    return -1;
}


int LIB_ID::SetRevision( const UTF8& aRevision )
{
    int offset = okRevision( aRevision );

    if( offset == -1 )
        m_revision = aRevision;

    return offset;
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

    if( m_revision.size() )
    {
        ret += '/';
        ret += m_revision;
    }

    return ret;
}


UTF8 LIB_ID::GetLibItemNameAndRev() const
{
    UTF8 ret = m_itemName;

    if( m_revision.size() )
    {
        ret += '/';
        ret += m_revision;
    }

    return ret;
}


UTF8 LIB_ID::Format( const UTF8& aLibraryName, const UTF8& aLibItemName, const UTF8& aRevision )
{
    UTF8    ret;
    int     offset;

    if( aLibraryName.size() )
    {
        offset = okLogical( aLibraryName );

        if( offset != -1 )
        {
            THROW_PARSE_ERROR( _( "Illegal character found in logical library name" ),
                               wxString::FromUTF8( aLibraryName.c_str() ), aLibraryName.c_str(), 0, offset );
        }

        ret += aLibraryName;
        ret += ':';
    }

    ret += aLibItemName;    // TODO: Add validity test.

    if( aRevision.size() )
    {
        offset = okRevision( aRevision );

        if( offset != -1 )
        {
            THROW_PARSE_ERROR( _( "Illegal character found in revision" ),
                               wxString::FromUTF8( aRevision.c_str() ),
                               aRevision.c_str(),
                               0,
                               offset );
        }

        ret += '/';
        ret += aRevision;
    }

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

    retv = m_itemName.compare( aLibId.m_itemName );

    if( retv != 0 )
        return retv;

    return m_revision.compare( aLibId.m_revision );
}


int LIB_ID::HasIllegalChars( const UTF8& aLibItemName )
{
    int offset = 0;

    for( auto ch : aLibItemName )
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

    if( aUniChar < ' ' )
        return false;

    // This list of characters is also duplicated in validators.cpp and footprint.cpp
    // TODO: Unify forbidden character lists
    switch( aUniChar )
    {
    case ':':
    case '/':
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


#if 0

// @todo Move this test into the unit test framework.

void LIB_ID::Test()
{
    static const char* lpids[] = {
        "smt:R_0805/rev0",
        "mysmt:R_0805/rev2",
        "device:AXIAL-0500",
    };

    for( unsigned i=0;  i<sizeof(lpids)/sizeof(lpids[0]);  ++i )
    {
        // test some round tripping

        LIB_ID lpid( lpids[i] );  // parse

        // format
        printf( "input:'%s'  full:'%s'  nickname: %s  m_itemName:'%s' rev:'%s'\n",
                lpids[i],
                lpid.Format().c_str(),
                lpid.GetLibNickname().c_str(),
                lpid.GetLibItemName().c_str(),
                lpid.GetRevision().c_str() );
    }
}

#endif
