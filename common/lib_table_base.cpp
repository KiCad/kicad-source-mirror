/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2017 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2012-2017 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <wx/filename.h>

#include <set>

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <kiface_i.h>
#include <lib_table_lexer.h>
#include <lib_table_base.h>


#define OPT_SEP     '|'         ///< options separator character


using namespace LIB_TABLE_T;


LIB_TABLE_ROW* new_clone( const LIB_TABLE_ROW& aRow )
{
    return aRow.clone();
}


void LIB_TABLE_ROW::setProperties( PROPERTIES* aProperties )
{
    properties.reset( aProperties );
}


void LIB_TABLE_ROW::SetFullURI( const wxString& aFullURI )
{
    uri_user = aFullURI;

#if !FP_LATE_ENVVAR
    uri_expanded = FP_LIB_TABLE::ExpandSubstitutions( aFullURI );
#endif
}


const wxString LIB_TABLE_ROW::GetFullURI( bool aSubstituted ) const
{
    if( aSubstituted )
    {
#if !FP_LATE_ENVVAR         // early expansion
        return uri_expanded;

#else   // late expansion
        return LIB_TABLE::ExpandSubstitutions( uri_user );
#endif
    }

    return uri_user;
}


void LIB_TABLE_ROW::Format( OUTPUTFORMATTER* out, int nestLevel ) const
{
    // In Kicad, we save path and file names using the Unix notation (separator = '/')
    // So ensure separator is always '/' is saved URI string
    wxString uri = GetFullURI();
    uri.Replace( '\\', '/' );

    out->Print( nestLevel, "(lib (name %s)(type %s)(uri %s)(options %s)(descr %s))\n",
                out->Quotew( GetNickName() ).c_str(),
                out->Quotew( GetType() ).c_str(),
                out->Quotew( uri ).c_str(),
                out->Quotew( GetOptions() ).c_str(),
                out->Quotew( GetDescr() ).c_str()
                );
}


void LIB_TABLE_ROW::Parse( std::unique_ptr< LIB_TABLE_ROW >& aRow, LIB_TABLE_LEXER* in )
{
    /*
     * (lib (name NICKNAME)(descr DESCRIPTION)(type TYPE)(full_uri FULL_URI)(options OPTIONS))
     *
     *  Elements after (name) are order independent.
     */

    T tok = in->NextTok();

    if( tok != T_lib )
        in->Expecting( T_lib );

    // (name NICKNAME)
    in->NeedLEFT();

    if( ( tok = in->NextTok() ) != T_name )
        in->Expecting( T_name );

    in->NeedSYMBOLorNUMBER();

    aRow->SetNickName( in->FromUTF8() );

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
            // Saved path and file names use the Unix notation (separator = '/')
            // However old files, and files edited by hands can use the woindows
            // separator. Force the unix notation
            // (It works on windows, and moreover, wxFileName and wxDir takes care to that
            // on windows)
            // moreover, URLs use the '/' as separator
            {
            wxString uri = in->FromUTF8();
            uri.Replace( '\\', '/' );
            aRow->SetFullURI( uri );
            }
            break;

        case T_type:
            if( sawType )
                in->Duplicate( tok );
            sawType = true;
            in->NeedSYMBOLorNUMBER();
            aRow->SetType( in->FromUTF8() );
            break;

        case T_options:
            if( sawOpts )
                in->Duplicate( tok );
            sawOpts = true;
            in->NeedSYMBOLorNUMBER();
            aRow->SetOptions( in->FromUTF8() );
            break;

        case T_descr:
            if( sawDesc )
                in->Duplicate( tok );
            sawDesc = true;
            in->NeedSYMBOLorNUMBER();
            aRow->SetDescr( in->FromUTF8() );
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
}


bool LIB_TABLE_ROW::operator==( const LIB_TABLE_ROW& r ) const
{
    return nickName == r.nickName
        && uri_user == r.uri_user
        && options == r.options
        && description == r.description;
}


void LIB_TABLE_ROW::SetOptions( const wxString& aOptions )
{
    options = aOptions;

    // set PROPERTIES* from options
    setProperties( LIB_TABLE::ParseOptions( TO_UTF8( aOptions ) ) );
}


LIB_TABLE::LIB_TABLE( LIB_TABLE* aFallBackTable ) :
    fallBack( aFallBackTable )
{
    // not copying fall back, simply search aFallBackTable separately
    // if "nickName not found".
}


LIB_TABLE::~LIB_TABLE()
{
    // *fallBack is not owned here.
}


bool LIB_TABLE::IsEmpty( bool aIncludeFallback )
{
    if( !aIncludeFallback || !fallBack )
        return rows.empty();

    return rows.empty() && fallBack->IsEmpty( true );
}


const wxString LIB_TABLE::GetDescription( const wxString& aNickname )
{
    // use "no exception" form of find row:
    const LIB_TABLE_ROW* row = findRow( aNickname );

    if( row )
        return row->GetDescr();
    else
        return wxEmptyString;
}


LIB_TABLE_ROW* LIB_TABLE::findRow( const wxString& aNickName ) const
{
    LIB_TABLE* cur = (LIB_TABLE*) this;

    do
    {
        cur->ensureIndex();

        INDEX_CITER it = cur->nickIndex.find( aNickName );

        if( it != cur->nickIndex.end() )
        {
            return &cur->rows[it->second];  // found
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return NULL;   // not found
}


const LIB_TABLE_ROW* LIB_TABLE::FindRowByURI( const wxString& aURI )
{
    LIB_TABLE* cur = this;

    do
    {
        cur->ensureIndex();

        for( unsigned i = 0;  i < cur->rows.size();  i++ )
        {
            wxString uri = cur->rows[i].GetFullURI( true );

            if( wxFileName::GetPathSeparator() == wxChar( '\\' ) && uri.Find( wxChar( '/' ) ) >= 0 )
                uri.Replace( "/", "\\" );

            if( (wxFileName::IsCaseSensitive() && uri == aURI)
              || (!wxFileName::IsCaseSensitive() && uri.Upper() == aURI.Upper() ) )
            {
                return &cur->rows[i];  // found
            }
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return NULL;   // not found
}


std::vector<wxString> LIB_TABLE::GetLogicalLibs()
{
    // Only return unique logical library names.  Use std::set::insert() to
    // quietly reject any duplicates, which can happen when encountering a duplicate
    // nickname from one of the fall back table(s).

    std::set< wxString >       unique;
    std::vector< wxString >    ret;
    const LIB_TABLE*           cur = this;

    do
    {
        for( LIB_TABLE_ROWS_CITER it = cur->rows.begin();  it!=cur->rows.end();  ++it )
        {
            unique.insert( it->GetNickName() );
        }

    } while( ( cur = cur->fallBack ) != 0 );

    ret.reserve( unique.size() );

    // return a sorted, unique set of nicknames in a std::vector<wxString> to caller
    for( std::set< wxString >::const_iterator it = unique.begin();  it!=unique.end();  ++it )
    {
        ret.push_back( *it );
    }

    return ret;
}


bool LIB_TABLE::InsertRow( LIB_TABLE_ROW* aRow, bool doReplace )
{
    ensureIndex();

    INDEX_CITER it = nickIndex.find( aRow->GetNickName() );

    if( it == nickIndex.end() )
    {
        rows.push_back( aRow );
        nickIndex.insert( INDEX_VALUE( aRow->GetNickName(), rows.size() - 1 ) );
        return true;
    }

    if( doReplace )
    {
        rows.replace( it->second, aRow );
        return true;
    }

    return false;
}


void LIB_TABLE::Load( const wxString& aFileName )
    throw( IO_ERROR )
{
    // It's OK if footprint library tables are missing.
    if( wxFileName::IsFileReadable( aFileName ) )
    {
        FILE_LINE_READER    reader( aFileName );
        LIB_TABLE_LEXER     lexer( &reader );

        Parse( &lexer );
    }
}


void LIB_TABLE::Save( const wxString& aFileName ) const
    throw( IO_ERROR, boost::interprocess::lock_exception )
{
    FILE_OUTPUTFORMATTER sf( aFileName );
    Format( &sf, 0 );
}


size_t LIB_TABLE::GetEnvVars( wxArrayString& aEnvVars ) const
{
    const LIB_TABLE* cur = this;

    do
    {
        for( unsigned i = 0;  i < cur->rows.size();  i++ )
        {
            wxString uri = cur->rows[i].GetFullURI( false );

            int start = uri.Find( "${" );

            if( start == wxNOT_FOUND )
                continue;

            int end = uri.Find( '}' );

            if( end == wxNOT_FOUND || end < start+2 )
                continue;

            wxString envVar = uri.Mid( start+2, end - (start+2) );

            if( aEnvVars.Index( envVar, false ) == wxNOT_FOUND )
                aEnvVars.Add( envVar );
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return aEnvVars.GetCount();
}


PROPERTIES* LIB_TABLE::ParseOptions( const std::string& aOptionsList )
{
    if( aOptionsList.size() )
    {
        const char* cp  = &aOptionsList[0];
        const char* end = cp + aOptionsList.size();

        PROPERTIES  props;
        std::string pair;

        // Parse all name=value pairs
        while( cp < end )
        {
            pair.clear();

            // Skip leading white space.
            while( cp < end && isspace( *cp )  )
                ++cp;

            // Find the end of pair/field
            while( cp < end )
            {
                if( *cp == '\\'  &&  cp + 1 < end  &&  cp[1] == OPT_SEP  )
                {
                    ++cp;           // skip the escape
                    pair += *cp++;  // add the separator
                }
                else if( *cp == OPT_SEP )
                {
                    ++cp;           // skip the separator
                    break;          // process the pair
                }
                else
                    pair += *cp++;
            }

            // stash the pair
            if( pair.size() )
            {
                // first equals sign separates 'name' and 'value'.
                size_t  eqNdx = pair.find( '=' );

                if( eqNdx != pair.npos )
                {
                    std::string name  = pair.substr( 0, eqNdx );
                    std::string value = pair.substr( eqNdx + 1 );
                    props[name] = value;
                }
                else
                    props[pair] = "";       // property is present, but with no value.
            }
        }

        if( props.size() )
            return new PROPERTIES( props );
    }

    return NULL;
}


UTF8 LIB_TABLE::FormatOptions( const PROPERTIES* aProperties )
{
    UTF8 ret;

    if( aProperties )
    {
        for( PROPERTIES::const_iterator it = aProperties->begin(); it != aProperties->end(); ++it )
        {
            const std::string& name = it->first;

            const UTF8& value = it->second;

            if( ret.size() )
                ret += OPT_SEP;

            ret += name;

            // the separation between name and value is '='
            if( value.size() )
            {
                ret += '=';

                for( std::string::const_iterator si = value.begin();  si != value.end();  ++si )
                {
                    // escape any separator in the value.
                    if( *si == OPT_SEP )
                        ret += '\\';

                    ret += *si;
                }
            }
        }
    }

    return ret;
}


const wxString LIB_TABLE::ExpandSubstitutions( const wxString& aString )
{
    return ExpandEnvVarSubstitutions( aString );
}
