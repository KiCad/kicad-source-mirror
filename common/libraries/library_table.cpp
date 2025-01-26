/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unordered_set>

#include <boost/lexical_cast.hpp>

#include <lib_table_base.h>
#include <libraries/library_table.h>
#include <libraries/library_table_parser.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <wx_filename.h>


std::map<std::string, UTF8> LIBRARY_TABLE_ROW::GetOptionsMap() const
{
    return LIB_TABLE::ParseOptions( TO_UTF8( m_options ) );
}


LIBRARY_TABLE::LIBRARY_TABLE( const wxFileName &aPath, LIBRARY_TABLE_SCOPE aScope ) :
        m_scope( aScope )
{
    LIBRARY_TABLE_PARSER parser;

    wxFileName file( aPath );
    WX_FILENAME::ResolvePossibleSymlinks( file );
    m_path = file.GetAbsolutePath();

    tl::expected<LIBRARY_TABLE_IR, LIBRARY_PARSE_ERROR> ir =
            parser.Parse( m_path.ToStdString() );

    if( ir.has_value() )
    {
        m_ok = initFromIR( *ir );
    }
    else
    {
        m_ok = false;
        m_errorDescription = ir.error().description;
    }
}


LIBRARY_TABLE::LIBRARY_TABLE( const wxString &aBuffer, LIBRARY_TABLE_SCOPE aScope ) :
        m_scope( aScope )
{
    m_ok = false;
    // TODO
}


bool LIBRARY_TABLE::initFromIR( const LIBRARY_TABLE_IR& aIR )
{
    m_type = aIR.type;

    try
    {
        m_version = boost::lexical_cast<int>( aIR.version );
    }
    catch( const boost::bad_lexical_cast & )
    {
        m_version = std::nullopt;
    }

    for( const LIBRARY_TABLE_ROW_IR& row : aIR.rows )
        addRowFromIR( row );

    return true;
}


bool LIBRARY_TABLE::addRowFromIR( const LIBRARY_TABLE_ROW_IR& aIR )
{
    LIBRARY_TABLE_ROW row;

    row.m_nickname = wxString::FromUTF8( aIR.nickname );
    row.m_uri = wxString::FromUTF8( aIR.uri );
    row.m_type = wxString::FromUTF8( aIR.type );
    row.m_options = wxString::FromUTF8( aIR.options );
    row.m_description = wxString::FromUTF8( aIR.description );
    row.m_ok = true;
    row.m_scope = m_scope;

    m_rows.emplace_back( row );
    return true;
}


// TODO(JE) this shouldn't be called from ctor and probably shouldn't be in LIBRARY_TABLE at all,
// but instead in LIBRARY_MANAGER - just opening one LIBRARY_TABLE shouldn't necessarily attempt a
// load on all the child tables
void LIBRARY_TABLE::LoadNestedTables()
{
    std::unordered_set<wxString> seenTables;

    std::function<void(LIBRARY_TABLE&)> processOneTable =
        [&]( LIBRARY_TABLE& aTable )
        {
            seenTables.insert( aTable.Path() );

            for( LIBRARY_TABLE_ROW& row : aTable.m_rows )
            {
                if( row.m_type == wxT( "Table" ) )
                {
                    wxFileName file( row.m_uri );

                    // URI may be relative to parent
                    file.MakeAbsolute( wxFileName( aTable.Path() ).GetPath() );

                    WX_FILENAME::ResolvePossibleSymlinks( file );
                    wxString src = file.GetFullPath();

                    if( seenTables.contains( src ) )
                    {
                        wxLogTrace( traceLibraries, "Library table %s has already been loaded!",
                                    src );
                        row.m_ok = false;
                        row.m_errorDescription = _( "A reference to this library table already exists" );
                        continue;
                    }

                    auto child = std::make_unique<LIBRARY_TABLE>( file, m_scope );

                    processOneTable( *child );

                    aTable.m_children.insert( { row.m_nickname, std::move( child ) } );
                }
            }
        };

    processOneTable( *this );
}
