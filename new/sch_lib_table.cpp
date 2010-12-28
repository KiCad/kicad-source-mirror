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


LIB_TABLE::LIB_TABLE( LIB_TABLE* aFallBackTable ) :
    fallBack( aFallBackTable )
{
    /* not copying fall back, simply search aFallBackTable separately if "logicalName not found".
    if( aFallBackTable )
    {
        const ROWS& t = aFallBackTable->rows;

        for( ROWS_CITER it = t.begin();  it != t.end();  ++it )
        {
            // our rows are empty, expect no collisions here
            auto_ptr<ROW> row( new ROW( *it->second ) );
            row->owner = this;
            insert( row );
        }
    }
    */
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

        // all logicalNames within this table fragment must be unique, so we do not
        // replace.  However a fallBack table can have a conflicting logicalName
        // and ours will supercede that one since in FindLib() we search this table
        // before any fall back.
        if( !InsertRow( row ) )
        {
            STRING msg;

            msg += '\'';
            msg += row->logicalName;
            msg += '\'';
            msg += " is a duplicate logical lib name";
            throw IO_ERROR( msg );
        }
    }
    return;
}


void LIB_TABLE::Format( OUTPUTFORMATTER* out, int nestLevel ) const
    throw( IO_ERROR )
{
    out->Print( nestLevel, "(lib_table\n" );
    for( ROWS_CITER it = rows.begin();  it != rows.end();  ++it )
        it->second->Format( out, nestLevel+1 );
    out->Print( nestLevel, ")\n" );
}

void LIB_TABLE::ROW::Format( OUTPUTFORMATTER* out, int nestLevel ) const
    throw( IO_ERROR )
{
    out->Print( nestLevel, "(lib (logical \"%s\")(type \"%s\")(full_uri \"%s\")(options \"%s\"))\n",
        logicalName.c_str(), libType.c_str(), fullURI.c_str(), options.c_str() );
}


const LIB_TABLE::ROW* LIB_TABLE::FindRow( const STRING& aLogicalName )
{
    // this function must be *super* fast, so therefore should not instantiate
    // anything which would require using the heap.  This function is the reason
    // ptr_map<> was used instead of ptr_set<>, which would have required
    // instantiating a ROW just to find a ROW.
    LIB_TABLE*  cur = this;

    do
    {
        ROWS_CITER  it = cur->rows.find( aLogicalName );

        if( it != cur->rows.end() )
        {
            // reference: http://myitcorner.com/blog/?p=361
            return (const LIB_TABLE::ROW*) it->second;  // found
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return 0;   // not found
}


bool LIB_TABLE::InsertRow( auto_ptr<ROW>& aRow, bool doReplace )
{
    // this does not need to be super fast.

    ROWS_CITER it = rows.find( aRow->logicalName );

    if( it == rows.end() )
    {
        // be careful here, key is needed because aRow can be
        // release()ed before logicalName is captured.
        const STRING&   key = aRow->logicalName;
        rows.insert( key, aRow );
        return true;
    }
    else if( doReplace )
    {
        rows.erase( aRow->logicalName );

        // be careful here, key is needed because aRow can be
        // release()ed before logicalName is captured.
        const STRING&   key = aRow->logicalName;
        rows.insert( key, aRow );
        return true;
    }

    return false;
}


#if 1 && defined(DEBUG)

// build this with a Debug CMAKE_BUILD_TYPE

void LIB_TABLE::Test()
{
    // the null string is not really a legal DSN token since any duplicated
    // double quote ("") is assumed to be a single double quote (").
    // To pass an empty string, we can pass " " to (options " ")
    SCH_LIB_TABLE_LEXER  slr(
        "(lib_table \n"
        "  (lib (logical meparts)       (type dir)      (full_uri /tmp/eeschema-lib)        (options \" \"))\n"
        "  (lib (logical old-project)   (type schematic)(full_uri /tmp/old-schematic.sch)   (options \" \"))\n"
        "  (lib (logical www)           (type http)     (full_uri http://kicad.org/libs)    (options \" \"))\n",

        wxT( "inline text" )        // source
        );

    try
    {
        // read the "( lib_table" pair of tokens
        slr.NextTok();
        slr.NextTok();

        // parse the rest of input to slr
        Parse( &slr );
    }

    catch( std::exception& ex )
    {
        printf( "std::exception\n" );
    }

    catch( IO_ERROR ioe )
    {
        printf( "exception: %s\n", (const char*) wxConvertWX2MB( ioe.errorText ) );
    }

    STRING_FORMATTER    sf;

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
}


int main( int argc, char** argv )
{
    LIB_TABLE   lib_table;

    lib_table.Test();

    return 0;
}

#endif
