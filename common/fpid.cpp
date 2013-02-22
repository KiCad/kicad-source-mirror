/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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

#include <fpid.h>


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

    return 0;
}


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

//----<Policy and field test functions>-------------------------------------


static inline int okLogical( const std::string& aField )
{
    // std::string::npos is largest positive number, casting to int makes it -1.
    // Returning that means success.
    return int( aField.find_first_of( ":" ) );
}


static int okRevision( const std::string& aField )
{
    char  rev[32];  // C string for speed

    if( aField.size() >= 4 )
    {
        strcpy( rev, "x/" );
        strcat( rev, aField.c_str() );

        if( EndsWithRev( rev, rev + strlen(rev), '/' ) == rev+2 )
            return -1;    // success
    }

    return 0; // first character position "is in error", is best we can do.
}


//----</Policy and field test functions>-------------------------------------


void FPID::clear()
{
    nickname.clear();
    footprint.clear();
    revision.clear();
}


int FPID::Parse( const std::string& aId )
{
    clear();

    size_t      cnt = aId.length() + 1;
    char        tmp[cnt];  // C string for speed

    std::strcpy( tmp, aId.c_str() );

    const char* rev = EndsWithRev( tmp, tmp+aId.length(), '/' );
    size_t      revNdx;
    size_t      partNdx;
    int         offset;

    //=====<revision>=========================================
    if( rev )
    {
        revNdx = rev - aId.c_str();

        // no need to check revision, EndsWithRev did that.
        revision = aId.substr( revNdx );
        --revNdx;  // back up to omit the '/' which precedes the rev
    }
    else
    {
        revNdx = aId.size();
    }

    //=====<nickname>==========================================
    if( ( partNdx = aId.find( ':' ) ) != aId.npos )
    {
        offset = SetLibNickname( aId.substr( 0, partNdx ) );

        if( offset > -1 )
        {
            return offset;
        }

        ++partNdx;  // skip ':'
    }
    else
    {
        partNdx = 0;
    }

    //=====<footprint name>====================================
    if( partNdx >= revNdx )
        return partNdx;

    SetFootprintName( aId.substr( partNdx, revNdx ) );

    return -1;
}


FPID::FPID( const std::string& aId ) throw( PARSE_ERROR )
{
    int offset = Parse( aId );

    if( offset != -1 )
    {
        THROW_PARSE_ERROR( _( "Illegal character found in FPID string" ),
                           wxString::FromUTF8( aId.c_str() ),
                           aId.c_str(),
                           0,
                           offset );
    }
}


int FPID::SetLibNickname( const std::string& aLogical )
{
    int offset = okLogical( aLogical );

    if( offset == -1 )
    {
        nickname = aLogical;
    }

    return offset;
}


int FPID::SetFootprintName( const std::string& aFootprintName )
{
    int          separation = int( aFootprintName.find_first_of( "/" ) );

    if( separation != -1 )
    {
        nickname = aFootprintName.substr( separation+1 );
        return separation + (int) nickname.size() + 1;
    }
    else
    {
        footprint = aFootprintName;
    }

    return -1;
}


int FPID::SetRevision( const std::string& aRevision )
{
    int offset = okRevision( aRevision );

    if( offset == -1 )
    {
        revision = aRevision;
    }

    return offset;
}


std::string FPID::Format() const
{
    std::string  ret;

    if( nickname.size() )
    {
        ret += nickname;
        ret += ':';
    }

    ret += footprint;

    if( revision.size() )
    {
        ret += '/';
        ret += revision;
    }

    return ret;
}


std::string FPID::GetFootprintNameAndRev() const
{
    std::string ret;

    if( revision.size() )
    {
        ret += '/';
        ret += revision;
    }

    return ret;
}


std::string FPID::Format( const std::string& aLogicalLib, const std::string& aFootprintName,
                          const std::string& aRevision )
    throw( PARSE_ERROR )
{
    std::string  ret;
    int     offset;

    if( aLogicalLib.size() )
    {
        offset = okLogical( aLogicalLib );

        if( offset != -1 )
        {
            THROW_PARSE_ERROR( _( "Illegal character found in logical library name" ),
                               wxString::FromUTF8( aLogicalLib.c_str() ),
                               aLogicalLib.c_str(),
                               0,
                               offset );
        }

        ret += aLogicalLib;
        ret += ':';
    }

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


#if 0 && defined(DEBUG)

// build this with Debug CMAKE_BUILD_TYPE

void FPID::Test()
{
    static const char* lpids[] = {
        "smt:R_0805/rev0",
        "mysmt:R_0805/rev2",
        "device:AXIAL-0500",
    };

    for( unsigned i=0;  i<sizeof(lpids)/sizeof(lpids[0]);  ++i )
    {
        // test some round tripping

        FPID lpid( lpids[i] );  // parse

        // format
        printf( "input:'%s'  full:'%s'  nickname: %s  footprint:'%s' rev:'%s'\n",
                lpids[i],
                lpid.Format().c_str(),
                lpid.GetLibNickname().c_str(),
                lpid.GetFootprintName().c_str(),
                lpid.GetRevision().c_str() );
    }
}


int main( int argc, char** argv )
{
    FPID::Test();

    return 0;
}

#endif
