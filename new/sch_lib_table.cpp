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

#include <set>
#include <assert.h>

#include <sch_lib_table_lexer.h>
#include <sch_lpid.h>
#include <sch_lib_table.h>
#include <sch_dir_lib_source.h>

//using namespace std;    // screws up Doxygen
using namespace SCH;


LIB_TABLE::LIB_TABLE( LIB_TABLE* aFallBackTable ) :
    fallBack( aFallBackTable )
{
    // not copying fall back, simply search aFallBackTable separately
    // if "logicalName not found".
}


void LIB_TABLE::Parse( SCH_LIB_TABLE_LEXER* in ) throw( IO_ERROR )
{
    /*  grammar:

        (lib_table
          (lib (logical LOGICAL)(type TYPE)(full_uri FULL_URI)(options OPTIONS))
          (lib (logical LOGICAL)(type TYPE)(full_uri FULL_URI)(options OPTIONS))
          (lib (logical LOGICAL)(type TYPE)(full_uri FULL_URI)(options OPTIONS))
         )

         note: "(lib_table" has already been read in.
    */

    ELT_T   tok;

    while( ( tok = in->NextTok() ) != T_RIGHT && tok != T_EOF )
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

        std::auto_ptr<ROW> row( new ROW( this ) );

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
        in->NeedRIGHT();            // terminate the (lib..)

        // all logicalNames within this table fragment must be unique, so we do not
        // use doReplace in InsertRow().  However a fallBack table can have a
        // conflicting logicalName and ours will supercede that one since in
        // FindLib() we search this table before any fall back.
        if( !InsertRow( row ) )
        {
            STRING msg;

            msg += '\'';
            msg += row->logicalName;
            msg += '\'';
            msg += " is a duplicate logical lib name";
            THROW_IO_ERROR( msg );
        }
    }
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
    out->Print( nestLevel, "(lib (logical %s)(type %s)(full_uri %s)(options %s))\n",
        out->Quoted( logicalName ).c_str(),
        out->Quoted( libType ).c_str(),
        out->Quoted( fullURI ).c_str(),
        out->Quoted( options ).c_str()
        );
}


STRINGS LIB_TABLE::GetLogicalLibs()
{
    // Only return unique logical library names.  Use std::set::insert() to
    // quietly reject any duplicates, which can happen when encountering a duplicate
    // logical lib name from one of the fall back table(s).

    std::set<STRING>    unique;
    STRINGS             ret;
    const LIB_TABLE*    cur = this;

    do
    {
        for( ROWS_CITER it = cur->rows.begin();  it!=cur->rows.end();  ++it )
        {
            unique.insert( it->second->logicalName );
        }

    } while( ( cur = cur->fallBack ) != 0 );

    // return a sorted, unique set of logical lib name STRINGS to caller
    for( std::set<STRING>::const_iterator it = unique.begin();  it!=unique.end();  ++it )
        ret.push_back( *it );

    return ret;
}


PART* LIB_TABLE::LookupPart( const LPID& aLPID, LIB* aLocalLib )
    throw( IO_ERROR )
{
    LIB* lib = lookupLib( aLPID, aLocalLib );

    return lib->LookupPart( aLPID, this );
}


LIB* LIB_TABLE::lookupLib( const LPID& aLPID, LIB* aFallBackLib )
    throw( IO_ERROR )
{
    if( aLPID.GetLogicalLib().size() )
    {
        ROW* row = FindRow( aLPID.GetLogicalLib() );
        if( !row )
        {
            STRING msg = "lib table contains no logical lib '";
            msg += aLPID.GetLogicalLib();
            msg += '\'';
            THROW_IO_ERROR( msg );
        }

        if( !row->lib )
        {
            loadLib( row );
        }

        assert( row->lib );     // fix loadLib() to throw if cannot load

        return row->lib;
    }

    if( aFallBackLib )
    {
        return aFallBackLib;
    }

    STRING msg = "lookupLib() requires logicalLibName or a fallback lib";
    THROW_IO_ERROR( msg );
}


void LIB_TABLE::loadLib( ROW* aRow ) throw( IO_ERROR )
{
    assert( !aRow->lib );   // caller should know better.

    const STRING& libType = aRow->GetType();

    if( !libType.compare( "dir" ) )
    {
        // autor_ptr wrap source while we create sink, in case sink throws.
        std::auto_ptr<LIB_SOURCE>   source(
            new DIR_LIB_SOURCE(
                aRow->GetFullURI(),
                aRow->GetOptions() ) );

        /* @todo load LIB_SINK
        std::auto_ptr<LIB_SINK>     sink(
            new DIR_LIB_SINK(
                aRow->GetFullURI(),
                aRow->GetOptions() ) );
        */

        // LIB::LIB( const STRING& aLogicalLibrary, LIB_SOURCE* aSource, LIB_SINK* aSink = NULL );
        aRow->lib = new LIB( aRow->GetLogicalName(), source.release(), NULL );
    }

/*
    else if( !libType.compare( "schematic" ) )
    {
        // @todo code and load SCHEMATIC_LIB_SOURCE
    }

    else if( !libType.compare( "subversion" ) )
    {
        // @todo code and load SVN_LIB_SOURCE
    }

    else if( !libType.compare( "http" ) )
    {
        // @todo code and load HTTP_LIB_SOURCE
    }
*/
    else
    {
        STRING msg = "cannot load unknown libType: '";
        msg += libType;
        msg += '\'';
        THROW_IO_ERROR( msg );
    }
}


LIB_TABLE::ROW* LIB_TABLE::FindRow( const STRING& aLogicalName ) const
{
    // this function must be *super* fast, so therefore should not instantiate
    // anything which would require using the heap.  This function is the reason
    // ptr_map<> was used instead of ptr_set<>, which would have required
    // instantiating a ROW just to find a ROW.
    const LIB_TABLE*  cur = this;

    do
    {
        ROWS_CITER  it = cur->rows.find( aLogicalName );

        if( it != cur->rows.end() )
        {
            // reference: http://myitcorner.com/blog/?p=361
            return (LIB_TABLE::ROW*) it->second;  // found
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return 0;   // not found
}


bool LIB_TABLE::InsertRow( std::auto_ptr<ROW>& aRow, bool doReplace )
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

    if( doReplace )
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
    // A pair of double quotes alone, is not really a legal DSN token since
    // any duplicated double quote ("") is assumed to be a single double quote (")
    // in any DSN lexer.
    // To pass an empty string, we can pass " " to (options " "), or if you passed
    // """" this would show up as "" with quotes present in the parser.  The parser
    // probably doesn't want a pair of double quotes, strlen() = 2.
    SCH_LIB_TABLE_LEXER  slr(
        "(lib_table \n"
        "  (lib (logical www)           (type http)     (full_uri http://kicad.org/libs)    (options \" \"))\n"
        "  (lib (logical meparts)       (type dir)      (full_uri /tmp/eeschema-lib)        (options useVersioning))\n"
        "  (lib (logical old-project)   (type schematic)(full_uri /tmp/old-schematic.sch)   (options \" \"))\n"
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
    LPID    lpid( "meparts:tigers/ears/rev10" );

    LookupPart( lpid );
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
        printf( "%s", ioe.inputLine.c_str() );      // rare not to have \n at end
        printf( "%*s^\n", ioe.byteIndex-1, " " );
    }
    catch( IO_ERROR& ioe )
    {
        printf( "%s\n", (const char*) ioe.errorText.ToUTF8() );
    }

    return 0;
}

#endif
