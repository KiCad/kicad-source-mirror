/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-12 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
    // if "nickName not found".
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

        ROW     row( this );

        row.SetNickName( in->FromUTF8() );

        in->NeedRIGHT();

        // (type "TYPE")
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_type )
            in->Expecting( T_type );

        in->NeedSYMBOLorNUMBER();

        row.SetType( in->FromUTF8() );

        in->NeedRIGHT();

        // (uri "FULL_URI")
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_full_uri )
            in->Expecting( T_full_uri );

        in->NeedSYMBOLorNUMBER();

        row.SetFullURI( in->FromUTF8() );

        in->NeedRIGHT();

        // (options "OPTIONS")
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_options )
            in->Expecting( T_options );

        in->NeedSYMBOLorNUMBER();

        row.SetOptions( in->FromUTF8() );

        in->NeedRIGHT();
        in->NeedRIGHT();            // terminate the (lib..)

        // all nickNames within this table fragment must be unique, so we do not
        // use doReplace in InsertRow().  However a fallBack table can have a
        // conflicting nickName and ours will supercede that one since in
        // FindLib() we search this table before any fall back.
        if( !InsertRow( row ) )
        {
            wxString msg = wxString::Format(
                                _( "'%s' is a duplicate footprint library nickName" ),
                                GetChars( row.nickName )
                                );
            THROW_IO_ERROR( msg );
        }
    }
}


void FP_LIB_TABLE::Format( OUTPUTFORMATTER* out, int nestLevel ) const
    throw( IO_ERROR )
{
    out->Print( nestLevel, "(fp_lib_table\n" );

    for( ROWS_CITER it = rows.begin();  it != rows.end();  ++it )
        it->Format( out, nestLevel+1 );

    out->Print( nestLevel, ")\n" );
}


void FP_LIB_TABLE::ROW::Format( OUTPUTFORMATTER* out, int nestLevel ) const
    throw( IO_ERROR )
{
    out->Print( nestLevel, "(lib (name %s)(type %s)(full_uri %s)(options %s))\n",
                out->Quotew( nickName ).c_str(),
                out->Quotew( type ).c_str(),
                out->Quotew( uri ).c_str(),
                out->Quotew( options ).c_str()
                );
}


std::vector<wxString> FP_LIB_TABLE::GetLogicalLibs()
{
    // Only return unique logical library names.  Use std::set::insert() to
    // quietly reject any duplicates, which can happen when encountering a duplicate
    // nickname from one of the fall back table(s).

    std::set<wxString>          unique;
    std::vector<wxString>       ret;
    const FP_LIB_TABLE*         cur = this;

    do
    {
        for( ROWS_CITER it = cur->rows.begin();  it!=cur->rows.end();  ++it )
        {
            unique.insert( it->nickName );
        }

    } while( ( cur = cur->fallBack ) != 0 );

    // return a sorted, unique set of nicknames in a std::vector<wxString> to caller
    for( std::set<wxString>::const_iterator it = unique.begin();  it!=unique.end();  ++it )
        ret.push_back( *it );

    return ret;
}


#if 0       // will need PLUGIN_RELEASER.
MODULE* FP_LIB_TABLE::LookupFootprint( const FP_LIB_ID& aFootprintId )
    throw( IO_ERROR )
{
    PLUGIN* plugin = lookupLib( aFootprintId );

    return plugin->FootprintLoad( FROM_UTF8( aFootprintId.GetBaseName().c_str() ),
                                  FROM_UTF8( aFootprintId.GetLogicalLib().c_str() ) );
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
#endif


FP_LIB_TABLE::ROW* FP_LIB_TABLE::FindRow( const wxString& aNickName ) const
{
    // this function must be *super* fast, so therefore should not instantiate
    // anything which would require using the heap.
    const FP_LIB_TABLE* cur = this;

    do
    {
        INDEX_CITER  it = cur->nickIndex.find( aNickName );

        if( it != cur->nickIndex.end() )
        {
            return (FP_LIB_TABLE::ROW*) &cur->rows[it->second];  // found
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return 0;   // not found
}


bool FP_LIB_TABLE::InsertRow( const ROW& aRow, bool doReplace )
{
    // this does not need to be super fast.

    INDEX_CITER it = nickIndex.find( aRow.nickName );

    if( it == nickIndex.end() )
    {
        rows.push_back( aRow );
        nickIndex.insert( INDEX_VALUE( aRow.nickName, rows.size() - 1 ) );
        return true;
    }

    if( doReplace )
    {
        rows[it->second] = aRow;
        return true;
    }

    return false;
}

