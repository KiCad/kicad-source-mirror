/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <sch_lib_table.h>
#include <sch_lib_table_lexer.h>
#include <sch_lpid.h>
#include <sch_part.h>

using namespace SCH;


// If LIB_TABLE::Test() is conditioned on DEBUG being defined, build with
// CMAKE_BUILD_TYPE=Debug, otherwise don't worry about it, because it will work
// on any CMAKE_BUILD_TYPE, including Release.
void LIB_TABLE::Test()
{
    SCH_LIB_TABLE_LEXER  slr(
        "(lib_table \n"
        "  (lib (logical www)           (type http)     (full_uri http://kicad.org/libs)    (options \"\"))\n"
        "  (lib (logical meparts)       (type dir)      (full_uri /tmp/eeschema-lib)        (options useVersioning))\n"
//        "  (lib (logical meparts)       (type dir)      (full_uri /tmp/eeschema-lib)        (options \"\"))\n"
        "  (lib (logical old-project)   (type schematic)(full_uri /tmp/old-schematic.sch)   (options \"\"))\n"
        ")\n"
        ,

        wxT( "inline text" )        // source
        );

    // read the "( lib_table" pair of tokens
    slr.NextTok();
    slr.NextTok();

    // parse the rest of input to slr
    Parse( &slr );

    STRING_FORMATTER    sf;

    // format this whole table into sf, it will be sorted by logicalName.
    Format( &sf, 0 );

    printf( "test 'Parse() <-> Format()' round tripping:\n" );
    printf( "%s", sf.GetString().c_str() );

    printf( "\ntest a lookup of 'www':\n" );
    const LIB_TABLE::ROW* www = FindRow( "www" );
    if( www )
    {
        // found, print it just to prove it.
        sf.Clear();

        www->Format( &sf, 1 );
        printf( "%s", sf.GetString().c_str() );
    }
    else
        printf( "not found\n" );

    printf( "\nlist of logical libraries:\n" );

    STRINGS logNames = GetLogicalLibs();
    for( STRINGS::const_iterator it = logNames.begin(); it!=logNames.end();  ++it )
    {
        printf( "logicalName: %s\n", it->c_str() );
    }

    // find a part
    LPID    lpid( "meparts:tigers/ears" );

    PART*   part = LookupPart( lpid );

    sf.Clear();
    part->Format( &sf, 0, 0 );

    printf( "%s", sf.GetString().c_str() );
}


int main( int argc, char** argv )
{
    LIB_TABLE   lib_table;

    try
    {
        // test exceptions:
        // THROW_PARSE_ERROR( wxT("test problem"), wxT("input"), 23, 46 );
        //THROW_IO_ERROR( wxT("io test") );

        lib_table.Test();
    }
    catch( PARSE_ERROR& ioe )   // most derived class first
    {
        printf( "%s\n", (const char*) ioe.errorText.ToUTF8() );

        printf( "%s%s",
                ioe.inputLine.c_str(),
                ioe.inputLine.empty() || *ioe.inputLine.rbegin() != '\n' ?
                        "\n" : "" );      // rare not to have \n at end

        printf( "%*s^\n", ioe.byteIndex>=1 ? ioe.byteIndex-1 : 0, "" );
    }
    catch( const IO_ERROR& ioe )
    {
        printf( "%s\n", (const char*) ioe.errorText.ToUTF8() );
    }

    return 0;
}

