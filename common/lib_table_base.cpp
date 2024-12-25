/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2012-2022 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <wx/debug.h>
#include <wx/filename.h>
#include <set>
#include <common.h>
#include <kiface_base.h>
#include <lib_table_base.h>
#include <lib_table_lexer.h>
#include <macros.h>
#include <string_utils.h>
#include <build_version.h>

#define OPT_SEP     '|'         ///< options separator character


using namespace LIB_TABLE_T;


LIB_TABLE_ROW* new_clone( const LIB_TABLE_ROW& aRow )
{
    return aRow.clone();
}


void LIB_TABLE_ROW::setProperties( STRING_UTF8_MAP* aProperties )
{
    properties.reset( aProperties );
}


void LIB_TABLE_ROW::SetFullURI( const wxString& aFullURI )
{
    uri_user = aFullURI;
}


const wxString LIB_TABLE_ROW::GetFullURI( bool aSubstituted ) const
{
    if( aSubstituted )
    {
        return ExpandEnvVarSubstitutions( uri_user, nullptr );
    }

    return uri_user;
}


void LIB_TABLE_ROW::Format( OUTPUTFORMATTER* out, int nestLevel ) const
{
    // In Kicad, we save path and file names using the Unix notation (separator = '/')
    // So ensure separator is always '/' is saved URI string
    wxString uri = GetFullURI();
    uri.Replace( '\\', '/' );

    wxString extraOptions;

    if( !GetIsEnabled() )
        extraOptions += "(disabled)";

    if( !GetIsVisible() )
        extraOptions += "(hidden)";

    out->Print( nestLevel, "(lib (name %s)(type %s)(uri %s)(options %s)(descr %s)%s)\n",
                out->Quotew( GetNickName() ).c_str(),
                out->Quotew( GetType() ).c_str(),
                out->Quotew( uri ).c_str(),
                out->Quotew( GetOptions() ).c_str(),
                out->Quotew( GetDescr() ).c_str(),
                extraOptions.ToStdString().c_str() );
}


bool LIB_TABLE_ROW::operator==( const LIB_TABLE_ROW& r ) const
{
    return nickName == r.nickName
        && uri_user == r.uri_user
        && options == r.options
        && description == r.description
        && enabled == r.enabled
        && visible == r.visible;
}


void LIB_TABLE_ROW::SetOptions( const wxString& aOptions )
{
    options = aOptions;

    // set PROPERTIES* from options
    setProperties( LIB_TABLE::ParseOptions( TO_UTF8( aOptions ) ) );
}


LIB_TABLE::LIB_TABLE( LIB_TABLE* aFallBackTable ) :
    m_fallBack( aFallBackTable ), m_version( 0 )
{
    // not copying fall back, simply search aFallBackTable separately
    // if "nickName not found".
}


LIB_TABLE::~LIB_TABLE()
{
    // *fallBack is not owned here.
}


void LIB_TABLE::Clear()
{
    m_rows.clear();
    m_rowsMap.clear();
}


bool LIB_TABLE::IsEmpty( bool aIncludeFallback )
{
    if( !aIncludeFallback || !m_fallBack )
        return m_rows.empty();

    return m_rows.empty() && m_fallBack->IsEmpty( true );
}


const wxString LIB_TABLE::GetDescription( const wxString& aNickname )
{
    // Use "no exception" form of find row and ignore disabled flag.
    const LIB_TABLE_ROW* row = findRow( aNickname );

    if( row )
        return row->GetDescr();
    else
        return wxEmptyString;
}


bool LIB_TABLE::HasLibrary( const wxString& aNickname, bool aCheckEnabled ) const
{
    const LIB_TABLE_ROW* row = findRow( aNickname, aCheckEnabled );

    if( row == nullptr )
        return false;

    return true;
}


bool LIB_TABLE::HasLibraryWithPath( const wxString& aPath ) const
{
    for( const LIB_TABLE_ROW& row : m_rows )
    {
        if( row.GetFullURI() == aPath )
            return true;
    }

    return false;
}


wxString LIB_TABLE::GetFullURI( const wxString& aNickname, bool aExpandEnvVars ) const
{
    const LIB_TABLE_ROW* row = findRow( aNickname, true );

    wxString retv;

    if( row )
        retv = row->GetFullURI( aExpandEnvVars );

    return retv;
}


LIB_TABLE_ROW* LIB_TABLE::findRow( const wxString& aNickName, bool aCheckIfEnabled ) const
{
    LIB_TABLE_ROW* row = nullptr;
    LIB_TABLE* cur = (LIB_TABLE*) this;

    do
    {
        std::shared_lock<std::shared_mutex> lock( cur->m_mutex );

        if( cur->m_rowsMap.count( aNickName ) )
            row = &*cur->m_rowsMap.at( aNickName );

        if( row )
        {
            if( !aCheckIfEnabled || row->GetIsEnabled() )
                return row;
            else
                return nullptr; // We found it, but it's disabled
        }

        // Repeat, this time looking for names that were "fixed" by legacy versions because
        // the old eeschema file format didn't support spaces in tokens.
        for( const std::pair<const wxString, LIB_TABLE_ROWS_ITER>& entry : cur->m_rowsMap )
        {
            wxString legacyLibName = entry.first;
            legacyLibName.Replace( " ", "_" );

            if( legacyLibName == aNickName )
            {
                row = &*entry.second;

                if( !aCheckIfEnabled || row->GetIsEnabled() )
                    return row;
            }
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->m_fallBack ) != nullptr );

    return nullptr; // not found
}


const LIB_TABLE_ROW* LIB_TABLE::FindRowByURI( const wxString& aURI )
{
    LIB_TABLE* cur = this;

    do
    {
        for( unsigned i = 0; i < cur->m_rows.size(); i++ )
        {
            wxString tmp = cur->m_rows[i].GetFullURI( true );

            if( tmp.Find( "://" ) != wxNOT_FOUND )
            {
                if( tmp == aURI )
                    return &cur->m_rows[i];  // found as URI
            }
            else
            {
                wxFileName fn = aURI;

                // This will also test if the file is a symlink so if we are comparing
                // a symlink to the same real file, the comparison will be true.  See
                // wxFileName::SameAs() in the wxWidgets source.
                if( fn == wxFileName( tmp ) )
                    return &cur->m_rows[i];  // found as full path and file name
            }
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->m_fallBack ) != nullptr );

    return nullptr; // not found
}


std::vector<wxString> LIB_TABLE::GetLogicalLibs()
{
    // Only return unique logical library names.  Use std::set::insert() to quietly reject any
    // duplicates (usually due to encountering a duplicate nickname in a fallback table).

    std::set<wxString>    unique;
    std::vector<wxString> ret;
    const LIB_TABLE*      cur = this;

    do
    {
        for( const LIB_TABLE_ROW& row : cur->m_rows )
        {
            if( row.GetIsEnabled() )
                unique.insert( row.GetNickName() );
        }

    } while( ( cur = cur->m_fallBack ) != nullptr );

    ret.reserve( unique.size() );

    // return a sorted, unique set of nicknames in a std::vector<wxString> to caller
    for( std::set< wxString >::const_iterator it = unique.begin();  it!=unique.end();  ++it )
        ret.push_back( *it );

    // We want to allow case-sensitive duplicates but sort by case-insensitive ordering
    std::sort( ret.begin(), ret.end(),
            []( const wxString& lhs, const wxString& rhs )
            {
                return StrNumCmp( lhs, rhs, true /* ignore case */ ) < 0;
            } );

    return ret;
}


bool LIB_TABLE::InsertRow( LIB_TABLE_ROW* aRow, bool doReplace )
{
    std::lock_guard<std::shared_mutex> lock( m_mutex );

    auto it = m_rowsMap.find( aRow->GetNickName() );

    if( it != m_rowsMap.end() )
    {
        if( !doReplace )
            return false;

        m_rows.replace( it->second, aRow );
    }
    else
    {
        m_rows.push_back( aRow );
    }

    aRow->SetParent( this );
    reindex();
    return true;
}


bool LIB_TABLE::RemoveRow( const LIB_TABLE_ROW* aRow )
{
    std::lock_guard<std::shared_mutex> lock( m_mutex );

    bool found = false;
    auto it = m_rowsMap.find( aRow->GetNickName() );

    if( it != m_rowsMap.end() )
    {
        if( &*it->second == aRow )
        {
            found = true;
            m_rows.erase( it->second );
        }
    }

    if( !found )
    {
        // Bookkeeping got messed up...
        for( int i = (int)m_rows.size() - 1; i >= 0; --i )
        {
            if( &m_rows[i] == aRow )
            {
                m_rows.erase( m_rows.begin() + i );
                found = true;
                break;
            }
        }
    }

    if( found )
        reindex();

    return found;
}


bool LIB_TABLE::ReplaceRow( size_t aIndex, LIB_TABLE_ROW* aRow )
{
    std::lock_guard<std::shared_mutex> lock( m_mutex );

    if( aIndex >= m_rows.size() )
        return false;

    m_rowsMap.erase( m_rows[aIndex].GetNickName() );

    m_rows.replace( aIndex, aRow );
    reindex();
    return true;
}


bool LIB_TABLE::ChangeRowOrder( size_t aIndex, int aOffset )
{
    std::lock_guard<std::shared_mutex> lock( m_mutex );

    if( aIndex >= m_rows.size() )
        return false;

    int newPos = static_cast<int>( aIndex ) + aOffset;

    if( newPos < 0 || newPos > static_cast<int>( m_rows.size() ) - 1 )
        return false;

    auto element = m_rows.release( m_rows.begin() + aIndex );

    m_rows.insert( m_rows.begin() + newPos, element.release() );
    reindex();

    return true;
}


void LIB_TABLE::TransferRows( LIB_TABLE_ROWS& aRowsList )
{
    std::lock_guard<std::shared_mutex> lock( m_mutex );

    m_rows.transfer( m_rows.end(), aRowsList.begin(), aRowsList.end(), aRowsList );

    reindex();
}


void LIB_TABLE::reindex()
{
    m_rowsMap.clear();

    for( LIB_TABLE_ROWS_ITER it = m_rows.begin(); it != m_rows.end(); ++it )
    {
        it->SetParent( this );
        m_rowsMap[it->GetNickName()] = it;
    }
}


bool LIB_TABLE::migrate()
{
    bool table_updated = false;

    for( LIB_TABLE_ROW& row : m_rows )
    {
        bool     row_updated = false;
        wxString uri = row.GetFullURI( true );

        // If the uri still has a variable in it, that means that the user does not have
        // these vars defined.  We update the old vars to the current versions on load

        static wxString fmtStr = wxS( "${KICAD%d_" );
        int version = 0;
        std::tie(version, std::ignore, std::ignore) = GetMajorMinorPatchTuple();

        for( int ii = 5; ii < version - 1; ++ii )
        {
            row_updated |= ( uri.Replace( wxString::Format( fmtStr, ii ),
                                          wxString::Format( fmtStr, version ), false ) > 0 );
        }

        if( row_updated )
        {
            row.SetFullURI( uri );
            table_updated = true;
        }
    }

    return table_updated;
}


void LIB_TABLE::Load( const wxString& aFileName )
{
    // It's OK if footprint library tables are missing.
    if( wxFileName::IsFileReadable( aFileName ) )
    {
        FILE_LINE_READER reader( aFileName );
        LIB_TABLE_LEXER  lexer( &reader );

        Parse( &lexer );

        if( m_version != 7 && migrate() && wxFileName::IsFileWritable( aFileName ) )
            Save( aFileName );
    }
}


void LIB_TABLE::Save( const wxString& aFileName ) const
{
    FILE_OUTPUTFORMATTER sf( aFileName );

    // Force the lib table version to 7 before saving
    m_version = 7;
    Format( &sf, 0 );
}


STRING_UTF8_MAP* LIB_TABLE::ParseOptions( const std::string& aOptionsList )
{
    if( aOptionsList.size() )
    {
        const char* cp  = &aOptionsList[0];
        const char* end = cp + aOptionsList.size();

        STRING_UTF8_MAP props;
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
                {
                    pair += *cp++;
                }
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
                {
                    props[pair] = "";       // property is present, but with no value.
                }
            }
        }

        if( props.size() )
            return new STRING_UTF8_MAP( props );
    }

    return nullptr;
}


UTF8 LIB_TABLE::FormatOptions( const STRING_UTF8_MAP* aProperties )
{
    UTF8 ret;

    if( aProperties )
    {
        for( STRING_UTF8_MAP::const_iterator it = aProperties->begin(); it != aProperties->end(); ++it )
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
