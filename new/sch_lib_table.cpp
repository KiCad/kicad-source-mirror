/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010-2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


#include <sch_lib_table.h>
#include <sch_lib_table_lexer.h>

using namespace std;
using namespace SCH;


LIB_TABLE::LIB_TABLE( LIB_TABLE* aFallBackTable )
{
    if( aFallBackTable )
    {
        const ROWS& t = aFallBackTable->rows;

        for( ROWS_CITER it = t.begin();  it != t.end();  ++it )
        {
            // our rows are empty, expect no collisions here
            auto_ptr<ROW>   row( new ROW( *it ) );

            row->owner = this;
            rows.insert( row.release() );
        }
    }
}


void LIB_TABLE::Parse( SCH_LIB_TABLE_LEXER* in ) throw( IO_ERROR )
{
    /*  grammar:

        (lib_table
          (lib (logical "LOGICAL")(type "TYPE")(full_uri "FULL_URI")(options "OPTIONS"))
          (lib (logical "LOGICAL")(type "TYPE")(full_uri "FULL_URI")(options "OPTIONS"))
          (lib (logical "LOGICAL")(type "TYPE")(full_uri "FULL_URI")(options "OPTIONS"))
         )

         note: "(lib_table" has already been read in.
    */

    ELT_T   tok;

    while( (tok = in->NextTok() ) != T_RIGHT && tok != T_EOF )
    {
        // (lib (logical "LOGICAL")(type "TYPE")(full_uri "FULL_URI")(options "OPTIONS"))

        if( tok != T_LEFT )
            in->Expecting( T_LEFT );

        if( ( tok = in->NextTok() ) != T_lib )
            in->Expecting( T_lib );

        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_logical )
            in->Expecting( T_logical );

        in->NeedSYMBOLorNUMBER();

        auto_ptr<ROW>   row( new ROW( this ) );

        row->SetLogicalName( in->CurText() );

        in->NeedRIGHT();
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_type )
            in->Expecting( T_type );

        in->NeedSYMBOLorNUMBER();

        // verify that type is one of: {dir, schematic, subversion, http}
        if( strcmp( in->CurText(), "dir" ) &&
            strcmp( in->CurText(), "schematic" ) &&
            strcmp( in->CurText(), "subversion" ) &&
            strcmp( in->CurText(), "http" ) )
        {
            in->Expecting( wxT( "dir|schematic|subversion|http" ) );
        }

        row->SetType( in->CurText() );

        in->NeedRIGHT();
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_full_uri )
            in->Expecting( T_full_uri );

        in->NeedSYMBOLorNUMBER();

        row->SetFullURI( in->CurText() );

        in->NeedRIGHT();
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_options )
            in->Expecting( T_options );

        in->NeedSYMBOLorNUMBER();

        row->SetOptions( in->CurText() );

        in->NeedRIGHT();
        in->NeedRIGHT();            // teriminate the (lib..)

        rows.insert( row.release() );
    }
    return;
}


#if 1

int main( int argc, char** argv )
{
    // the null string is not really a legal DSN token since any double quotes
    // as assumed to be a single quote.  To pass an empty string, we pass " "
    // to (options " ")
    SCH_LIB_TABLE_LEXER  slr(
        "(lib_table \n"
        "  (lib (logical meparts)       (type dir)      (full_uri /tmp/eeschema-lib)        (options \" \"))\n"
        "  (lib (logical old-project)   (type schematic)(full_uri /tmp/old-schematic.sch)   (options \" \"))\n"
        "  (lib (logical www)           (type http)     (full_uri http://kicad.org/libs)    (options \" \"))\n",

        wxT( "inline text" )        // source
        );

    LIB_TABLE   lib_table;

    // read the "( lib_table" pair of tokens

    try
    {
        slr.NextTok();
        slr.NextTok();

        lib_table.Parse( &slr );
    }

    catch( std::exception& ex )
    {
        printf( "std::exception\n" );
    }

    catch( IO_ERROR ioe )
    {
        printf( "caught\n" );
        printf( "exception: %s\n", (const char*) wxConvertWX2MB( ioe.errorText ) );
    }

    lib_table.Show();

    return 0;
}

#endif
