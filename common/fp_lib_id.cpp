/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
#include <wx/wx.h>      // _()

#include <fp_lib_id.h>


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

// These all return -1 on success, or >= 0 if there is an error at a
// particular character offset into their respective arguments.  If >=0,
// then that return value gives the character offset of the error.


static inline int okLogical( const std::string& aField )
{
    // std::string::npos is largest positive number, casting to int makes it -1.
    // Returning that means success.
    return int( aField.find_first_of( ":/" ) );
}


static inline int okBase( const std::string& aField )
{
    int offset = int( aField.find_first_of( ":/" ) );

    if( offset != -1 )
        return offset;

    // cannot be empty
    if( !aField.size() )
        return 0;

    return offset;  // ie. -1
}


static int okRevision( const std::string& aField )
{
    char  rev[32];  // C string for speed

    if( aField.size() >= 4 )
    {
        strcpy( rev, "x/" );
        strcat( rev, aField.c_str() );

        if( EndsWithRev( rev, rev + strlen(rev) ) == rev+2 )
            return -1;    // success
    }

    return 0; // first character position "is in error", is best we can do.
}

//----</Policy and field test functions>-------------------------------------


void FP_LIB_ID::clear()
{
    logical.clear();
    baseName.clear();
    footprintName.clear();
    revision.clear();
}


int FP_LIB_ID::Parse( const std::string& aId )
{
    clear();

    const char* rev = EndsWithRev( aId );
    size_t      revNdx;
    size_t      partNdx;
    size_t      baseNdx;
    int         offset;

    //=====<revision>=========================================
    if( rev )
    {
        revNdx = rev - aId.c_str();

        // no need to check revision, EndsWithRev did that.
        revision = aId.substr( revNdx );
        --revNdx;  // back up to omit the '/' which preceeds the rev
    }
    else
        revNdx = aId.size();

    //=====<logical>==========================================
    if( ( partNdx = aId.find( ':' ) ) != aId.npos )
    {
        offset = SetLogicalLib( aId.substr( 0, partNdx ) );
        if( offset > -1 )
        {
            return offset;
        }
        ++partNdx;  // skip ':'
    }
    else
        partNdx = 0;

    //=====<baseName>==========================================
    offset = SetBaseName( aId.substr( baseNdx, revNdx - baseNdx ) );

    if( offset > -1 )
    {
        return offset + baseNdx;
    }

    return -1;
}


FP_LIB_ID::FP_LIB_ID( const std::string& aId ) throw( PARSE_ERROR )
{
    int offset = Parse( aId );

    if( offset != -1 )
    {
        THROW_PARSE_ERROR( _( "Illegal character found in FP_LIB_ID string" ),
                           wxString::FromUTF8( aId.c_str() ),
                           aId.c_str(),
                           0,
                           offset );
    }
}


int FP_LIB_ID::SetLogicalLib( const std::string& aLogical )
{
    int offset = okLogical( aLogical );

    if( offset == -1 )
    {
        logical = aLogical;
    }

    return offset;
}


int FP_LIB_ID::SetBaseName( const std::string& aBaseName )
{
    int offset = okBase( aBaseName );

    if( offset == -1 )
    {
        baseName = aBaseName;
    }

    return offset;
}


int FP_LIB_ID::SetFootprintName( const std::string& aFootprintName )
{
    std::string  base;
    int          offset;
    int          separation = int( aFootprintName.find_first_of( "/" ) );

    if( separation != -1 )
    {
        base = aFootprintName.substr( separation+1 );
    }
    else
    {
        base = aFootprintName;
    }

    if( (offset = SetBaseName( base )) != -1 )
    {
        return offset + separation + 1;
    }

    return -1;
}


int FP_LIB_ID::SetRevision( const std::string& aRevision )
{
    int offset = okRevision( aRevision );

    if( offset == -1 )
    {
        revision = aRevision;
    }

    return offset;
}


std::string FP_LIB_ID::Format() const
{
    std::string  ret;

    if( logical.size() )
    {
        ret += logical;
        ret += ':';
    }

    ret += baseName;

    if( revision.size() )
    {
        ret += '/';
        ret += revision;
    }

    return ret;
}


std::string FP_LIB_ID::GetFootprintNameAndRev() const
{
    std::string ret;

    ret += baseName;

    if( revision.size() )
    {
        ret += '/';
        ret += revision;
    }

    return ret;
}


std::string FP_LIB_ID::Format( const std::string& aLogicalLib, const std::string& aFootprintName,
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
            THROW_PARSE_ERROR( _( "Illegal character found in logical lib name" ),
                               wxString::FromUTF8( aLogicalLib.c_str() ),
                               aLogicalLib.c_str(),
                               0,
                               offset );
        }

        ret += aLogicalLib;
        ret += ':';
    }

    {
        std::string  base;

        int separation = int( aFootprintName.find_first_of( "/" ) );

        if( separation != -1 )
        {
            base = aFootprintName.substr( separation+1 );
        }
        else
        {
            base = aFootprintName;
        }


        if( (offset = okBase( base )) != -1 )
        {
            THROW_PARSE_ERROR( _( "Illegal character found in base name" ),
                               wxString::FromUTF8( aRevision.c_str() ),
                               aRevision.c_str(),
                               0,
                               offset + separation + 1 );
        }

        ret += base;
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

void FP_LIB_ID::Test()
{
    static const char* lpids[] = {
        "/R/rev0",
        "passives/R/rev2",
        ":passives/R/rev3",
        "C/rev22",
        "passives/C22",
        "R",
        "me:R",
        // most difficult:
        "me:/R/rev0",
        "me:R/rev0",
    };

    for( unsigned i=0;  i<sizeof(lpids)/sizeof(lpids[0]);  ++i )
    {
        // test some round tripping

        FP_LIB_ID lpid( lpids[i] );  // parse

        // format
        printf( "input:'%s'  full:'%s'  base:'%s'  footprintName:'%s' rev:'%s'\n",
                lpids[i],
                lpid.Format().c_str(),
                lpid.GetBaseName().c_str(),
                lpid.GetFootprintName().c_str(),
                lpid.GetRevision().c_str() );
    }
}


int main( int argc, char** argv )
{
    FP_LIB_ID::Test();

    return 0;
}

#endif
