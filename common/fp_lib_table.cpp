/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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

#include <io_mgr.h>

#include <fp_lib_table_lexer.h>
#include <fp_lib_table.h>


using namespace FP_LIB_TABLE_T;


FP_LIB_TABLE::FP_LIB_TABLE( FP_LIB_TABLE* aFallBackTable ) :
    fallBack( aFallBackTable )
{
    // not copying fall back, simply search aFallBackTable separately
    // if "logicalName not found".
}


void FP_LIB_TABLE::Parse( FP_LIB_TABLE_LEXER* in ) throw( IO_ERROR, PARSE_ERROR )
{
    T tok;

    while( ( tok = in->NextTok() ) != T_RIGHT )
    {
        // (lib (name "LOGICAL")(type "TYPE")(full_uri "FULL_URI")(options "OPTIONS"))

        if( tok == T_EOF )
            in->Expecting( T_RIGHT );

        if( tok != T_LEFT )
            in->Expecting( T_LEFT );

        if( ( tok = in->NextTok() ) != T_fp_lib )
            in->Expecting( T_fp_lib );

        // (name "LOGICAL_NAME")
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_name )
            in->Expecting( T_name );

        in->NeedSYMBOLorNUMBER();

        std::auto_ptr<ROW> row( new ROW( this ) );

        row->SetLogicalName( in->CurText() );

        in->NeedRIGHT();

        // (type "TYPE")
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_type )
            in->Expecting( T_type );

        in->NeedSYMBOLorNUMBER();

        row->SetType( in->CurText() );

        in->NeedRIGHT();

        // (uri "FULL_URI")
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_full_uri )
            in->Expecting( T_full_uri );

        in->NeedSYMBOLorNUMBER();

        row->SetFullURI( in->CurText() );

        in->NeedRIGHT();

        // (options "OPTIONS")
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
            std::string msg;

            msg += '\'';
            msg += row->logicalName;
            msg += '\'';
            msg += " is a duplicate logical footprint library name";
            THROW_IO_ERROR( msg );
        }
    }
}


void FP_LIB_TABLE::Format( OUTPUTFORMATTER* out, int nestLevel ) const
    throw( IO_ERROR )
{
    out->Print( nestLevel, "(fp_lib_table\n" );

    for( ROWS_CITER it = rows.begin();  it != rows.end();  ++it )
        it->second->Format( out, nestLevel+1 );

    out->Print( nestLevel, ")\n" );
}


void FP_LIB_TABLE::ROW::Format( OUTPUTFORMATTER* out, int nestLevel ) const
    throw( IO_ERROR )
{
    out->Print( nestLevel, "(lib (logical %s)(type %s)(full_uri %s)(options %s))\n",
                out->Quotes( logicalName ).c_str(),
                out->Quotes( type ).c_str(),
                out->Quotes( uri ).c_str(),
                out->Quotes( options ).c_str() );
}


std::vector<std::string> FP_LIB_TABLE::GetLogicalLibs()
{
    // Only return unique logical library names.  Use std::set::insert() to
    // quietly reject any duplicates, which can happen when encountering a duplicate
    // logical lib name from one of the fall back table(s).

    std::set<std::string>      unique;
    std::vector<std::string>   ret;
    const FP_LIB_TABLE*        cur = this;

    do
    {
        for( ROWS_CITER it = cur->rows.begin();  it!=cur->rows.end();  ++it )
        {
            unique.insert( it->second->logicalName );
        }

    } while( ( cur = cur->fallBack ) != 0 );

    // return a sorted, unique set of logical lib name std::vector<std::string> to caller
    for( std::set<std::string>::const_iterator it = unique.begin();  it!=unique.end();  ++it )
        ret.push_back( *it );

    return ret;
}


MODULE* FP_LIB_TABLE::LookupFootprint( const FP_LIB_ID& aFootprintId )
    throw( IO_ERROR )
{
    PLUGIN* plugin = lookupLib( aFootprintId );

    return plugin->FootprintLoad( wxString( aFootprintId.GetBaseName().c_str() ),
                                  wxString( aFootprintId.GetLogicalLib().c_str() ) );
}


PLUGIN* FP_LIB_TABLE::lookupLib( const FP_LIB_ID& aFootprintId )
    throw( IO_ERROR )
{
    if( aFootprintId.GetLogicalLib().size() )
    {
        ROW* row = FindRow( aFootprintId.GetLogicalLib() );

        if( !row )
        {
            std::string msg = "lib table contains no logical lib '";
            msg += aFootprintId.GetLogicalLib();
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

    std::string msg = "lookupLib() requires logicalLibName";
    THROW_IO_ERROR( msg );
}


void FP_LIB_TABLE::loadLib( ROW* aRow ) throw( IO_ERROR )
{
    assert( !aRow->lib );   // caller should know better.

    const std::string& type = aRow->GetType();

    if( !type.compare( "dir" ) )
    {
        // @todo Look up plug in here.
    }

/*
    else if( !type.compare( "schematic" ) )
    {
        // @todo code and load SCHEMATIC_LIB_SOURCE
    }

    else if( !type.compare( "subversion" ) )
    {
        // @todo code and load SVN_LIB_SOURCE
    }

    else if( !type.compare( "http" ) )
    {
        // @todo code and load HTTP_LIB_SOURCE
    }
*/
    else
    {
        std::string msg = "cannot load unknown footprint library type: '";
        msg += type;
        msg += '\'';
        THROW_IO_ERROR( msg );
    }
}


FP_LIB_TABLE::ROW* FP_LIB_TABLE::FindRow( const std::string& aLogicalName ) const
{
    // this function must be *super* fast, so therefore should not instantiate
    // anything which would require using the heap.  This function is the reason
    // ptr_map<> was used instead of ptr_set<>, which would have required
    // instantiating a ROW just to find a ROW.
    const FP_LIB_TABLE* cur = this;

    do
    {
        ROWS_CITER  it = cur->rows.find( aLogicalName );

        if( it != cur->rows.end() )
        {
            // reference: http://myitcorner.com/blog/?p=361
            return (FP_LIB_TABLE::ROW*) it->second;  // found
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return 0;   // not found
}


bool FP_LIB_TABLE::InsertRow( std::auto_ptr<ROW>& aRow, bool doReplace )
{
    // this does not need to be super fast.

    ROWS_CITER it = rows.find( aRow->logicalName );

    if( it == rows.end() )
    {
        // be careful here, key is needed because aRow can be
        // release()ed before logicalName is captured.
        const std::string& key = aRow->logicalName;
        rows.insert( key, aRow );
        return true;
    }

    if( doReplace )
    {
        rows.erase( aRow->logicalName );

        // be careful here, key is needed because aRow can be
        // release()ed before logicalName is captured.
        const std::string&   key = aRow->logicalName;
        rows.insert( key, aRow );
        return true;
    }

    return false;
}

