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


#include <wx/config.h>      // wxExpandEnvVars()

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
    /*
        (fp_lib_table
            (lib (name NICKNAME)(descr DESCRIPTION)(type TYPE)(full_uri FULL_URI)(options OPTIONS))
            :
        )

        Elements after (name) are order independent.
    */

    T       tok;

    // This table may be nested within a larger s-expression, or not.
    // Allow for parser of that optional containing s-epression to have looked ahead.
    if( in->CurTok() != T_fp_lib_table )
    {
        in->NeedLEFT();
        if( ( tok = in->NextTok() ) != T_fp_lib_table )
            in->Expecting( T_fp_lib_table );
    }

    while( ( tok = in->NextTok() ) != T_RIGHT )
    {
        ROW     row;        // reconstructed for each row in input stream.

        if( tok == T_EOF )
            in->Expecting( T_RIGHT );

        if( tok != T_LEFT )
            in->Expecting( T_LEFT );

        // in case there is a "row integrity" error, tell where later.
        int lineNum = in->CurLineNumber();
        int offset  = in->CurOffset();

        if( ( tok = in->NextTok() ) != T_lib )
            in->Expecting( T_lib );

        // (name NICKNAME)
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_name )
            in->Expecting( T_name );

        in->NeedSYMBOLorNUMBER();

        row.SetNickName( in->FromUTF8() );

        in->NeedRIGHT();

        // After (name), remaining (lib) elements are order independent, and in
        // some cases optional.

        bool    sawType = false;
        bool    sawOpts = false;
        bool    sawDesc = false;
        bool    sawUri  = false;

        while( ( tok = in->NextTok() ) != T_RIGHT )
        {
            if( tok == T_EOF )
                in->Unexpected( T_EOF );

            if( tok != T_LEFT )
                in->Expecting( T_LEFT );

            tok = in->NeedSYMBOLorNUMBER();

            switch( tok )
            {
            case T_uri:
                if( sawUri )
                    in->Duplicate( tok );
                sawUri = true;
                in->NeedSYMBOLorNUMBER();
                row.SetFullURI( in->FromUTF8() );
                break;

            case T_type:
                if( sawType )
                    in->Duplicate( tok );
                sawType = true;
                in->NeedSYMBOLorNUMBER();
                row.SetType( in->FromUTF8() );
                break;

            case T_options:
                if( sawOpts )
                    in->Duplicate( tok );
                sawOpts = true;
                in->NeedSYMBOLorNUMBER();
                row.SetOptions( in->FromUTF8() );
                break;

            case T_descr:
                if( sawDesc )
                    in->Duplicate( tok );
                sawDesc = true;
                in->NeedSYMBOLorNUMBER();
                row.SetDescr( in->FromUTF8() );
                break;

            default:
                in->Unexpected( tok );
            }

            in->NeedRIGHT();
        }

        if( !sawType )
            in->Expecting( T_type );

        if( !sawUri )
            in->Expecting( T_uri );

        // all nickNames within this table fragment must be unique, so we do not
        // use doReplace in InsertRow().  (However a fallBack table can have a
        // conflicting nickName and ours will supercede that one since in
        // FindLib() we search this table before any fall back.)
        if( !InsertRow( row ) )
        {
            wxString msg = wxString::Format(
                                _( "'%s' is a duplicate footprint library nickName" ),
                                GetChars( row.nickName ) );
            THROW_PARSE_ERROR( msg, in->CurSource(), in->CurLine(), lineNum, offset );
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
    out->Print( nestLevel, "(lib (name %s)(type %s)(uri %s)(options %s)(descr %s))\n",
                out->Quotew( GetNickName() ).c_str(),
                out->Quotew( GetType() ).c_str(),
                out->Quotew( GetFullURI() ).c_str(),
                out->Quotew( GetOptions() ).c_str(),
                out->Quotew( GetDescr() ).c_str()
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


const FP_LIB_TABLE::ROW* FP_LIB_TABLE::findRow( const wxString& aNickName )
{
    FP_LIB_TABLE* cur = this;

    do
    {
        cur->ensureIndex();

        INDEX_CITER  it = cur->nickIndex.find( aNickName );

        if( it != cur->nickIndex.end() )
        {
            return &cur->rows[it->second];  // found
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return 0;   // not found
}


bool FP_LIB_TABLE::InsertRow( const ROW& aRow, bool doReplace )
{
    ensureIndex();

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


const FP_LIB_TABLE::ROW* FP_LIB_TABLE::FindRow( const wxString& aLibraryNickName )
    throw( IO_ERROR )
{
    const ROW* row = findRow( aLibraryNickName );

    if( !row )
    {
        wxString msg = wxString::Format( _("lib table contains no logical lib '%s'" ),
                            GetChars( aLibraryNickName ) );
        THROW_IO_ERROR( msg );
    }

    return row;
}


PLUGIN* FP_LIB_TABLE::PluginFind( const wxString& aLibraryNickName )
    throw( IO_ERROR )
{
    const ROW* row = FindRow( aLibraryNickName );

    // row will never be NULL here.

    PLUGIN* plugin = IO_MGR::PluginFind( row->type );

    return plugin;
}


const wxString FP_LIB_TABLE::ExpandSubtitutions( const wxString aString )
{
    // We reserve the right to do this another way, by providing our own member
    // function.
    return wxExpandEnvVars( aString );
}


#if 0  // don't know that this is needed yet
MODULE* FP_LIB_TABLE::LookupFootprint( const FP_LIB_ID& aFootprintId )
    throw( IO_ERROR )
{
    const ROW* row = FindRow( aFootprintId.GetLibraryNickName() );

    // row will never be NULL here.

    PLUGIN::RELEASER pi( PluginFind( row->type ) );

    return pi->FootprintLoad(   aLibraryPath->GetFullURI() ),
                                aFootprintId.GetFootprintName(),

                                // fetch a PROPERTIES instance on stack here
                                row->GetPropertiesFromOptions()
                                );
}
#endif
