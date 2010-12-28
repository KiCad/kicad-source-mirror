/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
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


LPID::LPID( const STRING& aLPID ) throw( PARSE_ERROR )
{
    const char* rev = EndsWithRev( aLPID );
    size_t      revNdx;
    size_t      partNdx;
    size_t      baseNdx;

    //=====<revision>=========================================
    if( rev )
    {
        revNdx   = rev - aLPID.c_str();
        revision = aLPID.substr( revNdx );
        --revNdx;  // back up to omit the '/' which preceeds the rev
    }
    else
        revNdx = aLPID.size();

    //=====<logical>==========================================
    if( ( partNdx = aLPID.find( ':' ) ) != aLPID.npos )
    {
        logical = aLPID.substr( 0, partNdx );
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
        category = aLPID.substr( partNdx, baseNdx - partNdx );
        ++baseNdx;   // skip '/'
    }
    else
    {
        baseNdx = partNdx;
    }

    //=====<baseName>==========================================
    baseName = aLPID.substr( baseNdx, revNdx - baseNdx );
}


STRING LPID::GetLogicalLib() const
{
    return logical;
}


bool LPID::SetLogicalLib( const STRING& aLogical )
{
    if( aLogical.find_first_of( ":/" ) == STRING::npos )
    {
        logical = aLogical;
        return true;
    }
    return false;
}


STRING LPID::GetCategory() const
{
    return category;
}


bool LPID::SetCategory( const STRING& aCategory )
{
    if( aCategory.find_first_of( ":/" ) == STRING::npos )
    {
        category = aCategory;
        return true;
    }
    return false;
}


STRING LPID::GetBaseName() const
{
    return baseName;
}


bool LPID::SetBaseName( const STRING& aBaseName )
{
    if( aBaseName.find_first_of( ":/" ) == STRING::npos )
    {
        baseName = aBaseName;
        return true;
    }
    return false;
}


STRING LPID::GetPartName() const
{
    STRING ret;

    // return [category/]baseName
    if( category.size() )
    {
        ret += category;
        ret += '/';
    }

    ret += baseName;

    return ret;
}


STRING LPID::GetRevision() const
{
    return revision;
}


bool LPID::SetRevision( const STRING& aRevision )
{
    STRING rev;

    rev += "x/";
    rev += aRevision;

    if( EndsWithRev( rev ) )
    {
        revision = aRevision;
        return true;
    }
    return false;
}


STRING LPID::GetFullText() const
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


#if 1 && defined(DEBUG)

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
        printf( "input:'%s'  full:'%s'  base:'%s'  partName:'%s'  cat:'%s'\n",
            lpids[i],
            lpid.GetFullText().c_str(),
            lpid.GetBaseName().c_str(),
            lpid.GetPartName().c_str(),
            lpid.GetCategory().c_str()
            );
    }
}


int main( int argc, char** argv )
{
    LPID::Test();

    return 0;
}

#endif
