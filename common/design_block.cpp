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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <design_block.h>

#include <wx/tokenzr.h>


std::vector<SEARCH_TERM>& DESIGN_BLOCK::GetSearchTerms()
{
    m_searchTerms.clear();
    m_searchTerms.reserve( 6 );

    m_searchTerms.emplace_back( SEARCH_TERM( GetLibNickname(), 4 ) );
    m_searchTerms.emplace_back( SEARCH_TERM( GetName(), 8 ) );
    m_searchTerms.emplace_back( SEARCH_TERM( GetLIB_ID().Format(), 16 ) );

    wxStringTokenizer keywordTokenizer( GetKeywords(), wxS( " " ), wxTOKEN_STRTOK );

    while( keywordTokenizer.HasMoreTokens() )
        m_searchTerms.emplace_back( SEARCH_TERM( keywordTokenizer.GetNextToken(), 4 ) );

    // Also include keywords as one long string, just in case
    m_searchTerms.emplace_back( SEARCH_TERM( GetKeywords(), 1 ) );
    m_searchTerms.emplace_back( SEARCH_TERM( GetDesc(), 1 ) );

    return m_searchTerms;
}
