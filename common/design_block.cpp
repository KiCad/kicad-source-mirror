/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <design_block.h>

#include <wx/tokenzr.h>
#include <api/api_utils.h>


std::vector<SEARCH_TERM>& DESIGN_BLOCK::GetSearchTerms()
{
    m_searchTerms.clear();
    m_searchTerms.reserve( 6 );

    m_searchTerms.emplace_back( SEARCH_TERM( GetLibNickname(), 4 ) );
    m_searchTerms.emplace_back( SEARCH_TERM( GetName(), 8, true ) );
    m_searchTerms.emplace_back( SEARCH_TERM( GetLIB_ID().Format(), 16, true ) );

    wxStringTokenizer keywordTokenizer( GetKeywords(), wxS( " " ), wxTOKEN_STRTOK );

    while( keywordTokenizer.HasMoreTokens() )
        m_searchTerms.emplace_back( SEARCH_TERM( keywordTokenizer.GetNextToken(), 4 ) );

    // Also include keywords as one long string, just in case
    m_searchTerms.emplace_back( SEARCH_TERM( GetKeywords(), 1 ) );
    m_searchTerms.emplace_back( SEARCH_TERM( GetDesc(), 1 ) );

    return m_searchTerms;
}


void DESIGN_BLOCK::Serialize( kiapi::common::types::DesignBlock& aDesignBlock ) const
{
    using namespace kiapi::common;
    using namespace kiapi::common::types;

    DesignBlock& def = aDesignBlock;
    def.Clear();
    PackLibId( def.mutable_id(), m_lib_id );

    def.set_schematic_filename( m_schematicFile.ToUTF8() );
    def.set_board_filename( m_boardFile.ToUTF8() );
    def.set_lib_description( m_libDescription.ToUTF8() );
    def.set_keywords( m_keywords.ToUTF8() );

    for( const auto& [name, value] : m_fields )
    {
        types::CustomProperty* field = def.mutable_fields()->Add();
        field->set_key( name.ToUTF8() );
        field->set_value( value.ToUTF8() );
    }
}


void DESIGN_BLOCK::Serialize( google::protobuf::Any &aContainer ) const
{
    kiapi::common::types::DesignBlock designBlock;
    Serialize( designBlock );

    if( !aContainer.PackFrom( designBlock ) )
        wxFAIL_MSG( wxT( "Failed to pack DESIGN_BLOCK" ) );
}


bool DESIGN_BLOCK::Deserialize( const kiapi::common::types::DesignBlock& aDesignBlock )
{
    using namespace kiapi::common;
    using namespace kiapi::common::types;

    const DesignBlock& def = aDesignBlock;

    m_lib_id = UnpackLibId( def.id() );

    m_schematicFile = def.schematic_filename();
    m_boardFile = def.board_filename();
    m_libDescription = def.lib_description();
    m_keywords = def.keywords();

    m_fields.clear();

    for( const types::CustomProperty& prop : def.fields() )
        m_fields[wxString::FromUTF8( prop.key() )] = wxString::FromUTF8( prop.value() );

    return true;
}


bool DESIGN_BLOCK::Deserialize( const google::protobuf::Any& aContainer )
{
    kiapi::common::types::DesignBlock designBlock;

    if( !aContainer.UnpackTo( &designBlock ) )
        return false;

    return Deserialize( designBlock );
}
