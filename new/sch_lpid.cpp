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

#include <sch_lpid.h>

using namespace SCH;

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

static inline int okLogical( const STRING& aField )
{
    // std::string::npos is largest positive number, casting to int makes it -1.
    // Returning that means success.
    return int( aField.find_first_of( ":/" ) );
}

static inline int okBase( const STRING& aField )
{
    int offset = int( aField.find_first_of( ":/" ) );
    if( offset != -1 )
        return offset;

    // cannot be empty
    if( !aField.size() )
        return 0;

    return offset;  // ie. -1
}

static inline int okCategory( const STRING& aField )
{
    return int( aField.find_first_of( ":/" ) );
}

static int okRevision( const STRING& aField )
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


void LPID::clear()
{
    logical.clear();
    category.clear();
    baseName.clear();
    partName.clear();
    revision.clear();
}


int LPID::Parse( const STRING& aLPID )
{
    clear();

    const char* rev = EndsWithRev( aLPID );
    size_t      revNdx;
    size_t      partNdx;
    size_t      baseNdx;
    int         offset;

    //=====<revision>=========================================
    if( rev )
    {
        revNdx = rev - aLPID.c_str();

        // no need to check revision, EndsWithRev did that.
        revision = aLPID.substr( revNdx );
        --revNdx;  // back up to omit the '/' which preceeds the rev
    }
    else
        revNdx = aLPID.size();

    //=====<logical>==========================================
    if( ( partNdx = aLPID.find( ':' ) ) != aLPID.npos )
    {
        offset = SetLogicalLib( aLPID.substr( 0, partNdx ) );
        if( offset > -1 )
        {
            return offset;
        }
        ++partNdx;  // skip ':'
    }
    else
        partNdx = 0;

    //=====<rawName && category>==============================
    // "length limited" search:
    const char* base = (const char*) memchr( aLPID.c_str() + partNdx, '/', revNdx - partNdx );

    if( base )
    {
        baseNdx  = base - aLPID.c_str();
        offset  = SetCategory( aLPID.substr( partNdx, baseNdx - partNdx ) );
        if( offset > -1 )
        {
            return offset + partNdx;
        }
        ++baseNdx;   // skip '/'
    }
    else
    {
        baseNdx = partNdx;
    }

    //=====<baseName>==========================================
    offset = SetBaseName( aLPID.substr( baseNdx, revNdx - baseNdx ) );
    if( offset > -1 )
    {
        return offset + baseNdx;
    }

    return -1;
}


LPID::LPID( const STRING& aLPID ) throw( PARSE_ERROR )
{
    int offset = Parse( aLPID );

    if( offset != -1 )
    {
        THROW_PARSE_ERROR(
                _( "Illegal character found in LPID string" ),
                wxString::FromUTF8( aLPID.c_str() ),
                aLPID.c_str(),
                0,
                offset
                );
    }
}


int LPID::SetLogicalLib( const STRING& aLogical )
{
    int offset = okLogical( aLogical );
    if( offset == -1 )
    {
        logical = aLogical;
    }
    return offset;
}


int LPID::SetCategory( const STRING& aCategory )
{
    int offset = okCategory( aCategory );
    if( offset == -1 )
    {
        category = aCategory;

        // set the partName too
        if( category.size() )
        {
            partName = category;
            partName += '/';
            partName += baseName;
        }
        else
            partName = baseName;

    }
    return offset;
}


int LPID::SetBaseName( const STRING& aBaseName )
{
    int offset = okBase( aBaseName );
    if( offset == -1 )
    {
        baseName = aBaseName;

        // set the partName too
        if( category.size() )
        {
            partName = category;
            partName += '/';
            partName += baseName;
        }
        else
            partName = baseName;
    }
    return offset;
}


int LPID::SetPartName( const STRING& aPartName )
{
    STRING  category;
    STRING  base;
    int     offset;
    int     separation = int( aPartName.find_first_of( "/" ) );

    if( separation != -1 )
    {
        category = aPartName.substr( 0, separation );
        base     = aPartName.substr( separation+1 );
    }
    else
    {
        // leave category empty
        base = aPartName;
    }

    if( (offset = SetCategory( category )) != -1 )
        return offset;

    if( (offset = SetBaseName( base )) != -1 )
    {
        return offset + separation + 1;
    }

    return -1;
}


int LPID::SetRevision( const STRING& aRevision )
{
    int offset = okRevision( aRevision );
    if( offset == -1 )
    {
        revision = aRevision;
    }
    return offset;
}


STRING LPID::Format() const
{
    STRING  ret;

    if( logical.size() )
    {
        ret += logical;
        ret += ':';
    }

    if( category.size() )
    {
        ret += category;
        ret += '/';
    }

    ret += baseName;

    if( revision.size() )
    {
        ret += '/';
        ret += revision;
    }

    return ret;
}


STRING LPID::GetPartNameAndRev() const
{
    STRING ret;

    if( category.size() )
    {
        ret += category;
        ret += '/';
    }

    ret += baseName;

    if( revision.size() )
    {
        ret += '/';
        ret += revision;
    }

    return ret;
}


STRING LPID::Format( const STRING& aLogicalLib, const STRING& aPartName, const STRING& aRevision )
    throw( PARSE_ERROR )
{
    STRING  ret;
    int     offset;

    if( aLogicalLib.size() )
    {
        offset = okLogical( aLogicalLib );
        if( offset != -1 )
        {
            THROW_PARSE_ERROR(
                    _( "Illegal character found in logical lib name" ),
                    wxString::FromUTF8( aLogicalLib.c_str() ),
                    aLogicalLib.c_str(),
                    0,
                    offset
                    );
        }
        ret += aLogicalLib;
        ret += ':';
    }

    {
        STRING  category;
        STRING  base;

        int separation = int( aPartName.find_first_of( "/" ) );

        if( separation != -1 )
        {
            category = aPartName.substr( 0, separation );
            base     = aPartName.substr( separation+1 );
        }
        else
        {
            // leave category empty
            base = aPartName;
        }

        if( (offset = okCategory( category )) != -1 )
        {
            THROW_PARSE_ERROR(
                    _( "Illegal character found in category" ),
                    wxString::FromUTF8( aRevision.c_str() ),
                    aRevision.c_str(),
                    0,
                    offset
                    );
        }

        if( (offset = okBase( base )) != -1 )
        {
            THROW_PARSE_ERROR(
                    _( "Illegal character found in base name" ),
                    wxString::FromUTF8( aRevision.c_str() ),
                    aRevision.c_str(),
                    0,
                    offset + separation + 1
                    );
        }

        if( category.size() )
        {
            ret += category;
            ret += '/';
        }

        ret += base;
    }

    if( aRevision.size() )
    {
        offset = okRevision( aRevision );
        if( offset != -1 )
        {
            THROW_PARSE_ERROR(
                    _( "Illegal character found in revision" ),
                    wxString::FromUTF8( aRevision.c_str() ),
                    aRevision.c_str(),
                    0,
                    offset
                    );
        }

        ret += '/';
        ret += aRevision;
    }

    return ret;
}


#if 0 && defined(DEBUG)

// build this with Debug CMAKE_BUILD_TYPE

void LPID::Test()
{
    static const char* lpids[] = {
        "me:passives/R/rev0",
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

        LPID lpid( lpids[i] );  // parse

        // format
        printf( "input:'%s'  full:'%s'  base:'%s'  partName:'%s'  cat:'%s'  rev:'%s'\n",
            lpids[i],
            lpid.Format().c_str(),
            lpid.GetBaseName().c_str(),
            lpid.GetPartName().c_str(),
            lpid.GetCategory().c_str(),
            lpid.GetRevision().c_str()
            );
    }
}


int main( int argc, char** argv )
{
    LPID::Test();

    return 0;
}

#endif
