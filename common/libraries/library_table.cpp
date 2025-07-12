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

#include <boost/lexical_cast.hpp>

#include <lib_table_base.h>
#include <libraries/library_table.h>
#include <libraries/library_table_parser.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <wx_filename.h>
#include <xnode.h>
#include <libraries/library_manager.h>


const wxString LIBRARY_TABLE_ROW::TABLE_TYPE_NAME = wxT( "Table" );


bool LIBRARY_TABLE_ROW::operator==( const LIBRARY_TABLE_ROW& aOther ) const
{
    return m_scope == aOther.m_scope
            && m_nickname == aOther.m_nickname
            && m_uri == aOther.m_uri
            && m_type == aOther.m_type
            && m_options == aOther.m_options
            && m_description == aOther.m_description
            && m_disabled == aOther.m_disabled
            && m_hidden == aOther.m_hidden;
}


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

    if( !file.FileExists() )
    {
        m_ok = false;
        m_errorDescription = wxString::Format( _( "The library table path '%s' does not exist" ),
                                               file.GetFullPath() );
        return;
    }

    tl::expected<LIBRARY_TABLE_IR, LIBRARY_PARSE_ERROR> ir = parser.Parse( m_path.ToStdString() );

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
        m_path( wxEmptyString ),
        m_scope( aScope )
{
    LIBRARY_TABLE_PARSER parser;

    tl::expected<LIBRARY_TABLE_IR, LIBRARY_PARSE_ERROR> ir =
            parser.ParseBuffer( aBuffer.ToStdString() );

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


bool LIBRARY_TABLE::operator==( const LIBRARY_TABLE& aOther ) const
{
    return m_path == aOther.m_path
            && m_scope == aOther.m_scope
            && m_type == aOther.m_type
            && m_version == aOther.m_version
            && m_rows == aOther.m_rows;
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
    row.m_hidden = aIR.hidden;
    row.m_disabled = aIR.disabled;
    row.m_ok = true;
    row.m_scope = m_scope;

    m_rows.emplace_back( row );
    return true;
}


void LIBRARY_TABLE::Format( OUTPUTFORMATTER* aOutput ) const
{
    static const std::map<LIBRARY_TABLE_TYPE, wxString> types = {
        { LIBRARY_TABLE_TYPE::SYMBOL, "sym_lib_table" },
        { LIBRARY_TABLE_TYPE::FOOTPRINT, "fp_lib_table" },
        { LIBRARY_TABLE_TYPE::DESIGN_BLOCK, "design_block_lib_table" }
    };

    wxCHECK( types.contains( Type() ), /* void */ );

    XNODE self( wxXML_ELEMENT_NODE, types.at( Type() ) );

    // TODO(JE) library tables - version management?
    self.AddAttribute( "version", 7 );

    for( const LIBRARY_TABLE_ROW& row : Rows() )
    {
        if( !row.IsOk() )
            continue;

        wxString uri = row.URI();
        uri.Replace( '\\', '/' );

        XNODE* rowNode = new XNODE( wxXML_ELEMENT_NODE, "lib" );
        rowNode->AddAttribute( "name", row.Nickname() );
        rowNode->AddAttribute( "type", row.Type() );
        rowNode->AddAttribute( "uri", uri );
        rowNode->AddAttribute( "options", row.Options() );
        rowNode->AddAttribute( "descr", row.Description() );

        if( row.Disabled() )
            rowNode->AddChild( new XNODE( wxXML_ELEMENT_NODE, "disabled" ) );

        if( row.Hidden() )
            rowNode->AddChild( new XNODE( wxXML_ELEMENT_NODE, "hidden" ) );

        self.AddChild( rowNode );
    }

    self.Format( aOutput );
}


LIBRARY_TABLE_ROW LIBRARY_TABLE::MakeRow() const
{
    LIBRARY_TABLE_ROW row = {};

    row.SetScope( m_scope );
    row.SetOk();

    return row;
}


LIBRARY_TABLE_ROW& LIBRARY_TABLE::InsertRow()
{
    return Rows().emplace_back( MakeRow() );
}


bool LIBRARY_TABLE::HasRow( const wxString& aNickname ) const
{
    for( const LIBRARY_TABLE_ROW& row : m_rows )
    {
        if( row.Nickname() == aNickname )
            return true;
    }

    return false;
}


bool LIBRARY_TABLE::HasRowWithURI( const wxString& aUri, const PROJECT& aProject,
                                   bool aSubstituted ) const
{
    for( const LIBRARY_TABLE_ROW& row : m_rows )
    {
        if( !aSubstituted && row.URI() == aUri )
            return true;

        if( aSubstituted && LIBRARY_MANAGER::ExpandURI( row.URI(), aProject ) == aUri )
            return true;
    }

    return false;
}


std::optional<LIBRARY_TABLE_ROW*> LIBRARY_TABLE::Row( const wxString& aNickname )
{
    for( LIBRARY_TABLE_ROW& row : m_rows )
    {
        if( row.Nickname() == aNickname )
            return &row;
    }

    return std::nullopt;
}


std::optional<const LIBRARY_TABLE_ROW*> LIBRARY_TABLE::Row( const wxString& aNickname ) const
{
    for( const LIBRARY_TABLE_ROW& row : m_rows )
    {
        if( row.Nickname() == aNickname )
            return &row;
    }

    return std::nullopt;
}
