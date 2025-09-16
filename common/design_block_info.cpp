/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/*
 * Functions to read design block libraries and fill m_design_blocks by available design blocks
 * names and their documentation (comments and keywords).
 */

#include <design_block_info.h>
#include <fp_lib_table.h>
#include <dialogs/html_message_box.h>
#include <string_utils.h>
#include <kiface_ids.h>
#include <kiway.h>
#include <lib_id.h>
#include <thread>
#include <utility>
#include <wx/tokenzr.h>
#include <kiface_base.h>

DESIGN_BLOCK_INFO* DESIGN_BLOCK_LIST::GetDesignBlockInfo( const wxString& aLibNickname,
                                                          const wxString& aDesignBlockName )
{
    if( aDesignBlockName.IsEmpty() )
        return nullptr;

    for( std::unique_ptr<DESIGN_BLOCK_INFO>& db : m_list )
    {
        if( aLibNickname == db->GetLibNickname() && aDesignBlockName == db->GetDesignBlockName() )
            return db.get();
    }

    return nullptr;
}


DESIGN_BLOCK_INFO* DESIGN_BLOCK_LIST::GetDesignBlockInfo( const wxString& aDesignBlockName )
{
    if( aDesignBlockName.IsEmpty() )
        return nullptr;

    LIB_ID dbid;

    wxCHECK_MSG( dbid.Parse( aDesignBlockName ) < 0, nullptr,
                 wxString::Format( wxT( "'%s' is not a valid LIB_ID." ), aDesignBlockName ) );

    return GetDesignBlockInfo( dbid.GetLibNickname(), dbid.GetLibItemName() );
}


std::vector<SEARCH_TERM> DESIGN_BLOCK_INFO::GetSearchTerms()
{
    std::vector<SEARCH_TERM> terms;

    terms.emplace_back( SEARCH_TERM( GetLibNickname(), 4 ) );
    terms.emplace_back( SEARCH_TERM( GetName(), 8 ) );
    terms.emplace_back( SEARCH_TERM( GetLIB_ID().Format(), 16 ) );

    wxStringTokenizer keywordTokenizer( GetKeywords(), " \t\r\n", wxTOKEN_STRTOK );

    while( keywordTokenizer.HasMoreTokens() )
        terms.emplace_back( SEARCH_TERM( keywordTokenizer.GetNextToken(), 4 ) );

    // Also include keywords as one long string, just in case
    terms.emplace_back( SEARCH_TERM( GetKeywords(), 1 ) );
    terms.emplace_back( SEARCH_TERM( GetDesc(), 1 ) );

    return terms;
}


bool DESIGN_BLOCK_INFO::InLibrary( const wxString& aLibrary ) const
{
    return aLibrary == m_nickname;
}


bool operator<( const DESIGN_BLOCK_INFO& lhs, const DESIGN_BLOCK_INFO& rhs )
{
    int retv = StrNumCmp( lhs.m_nickname, rhs.m_nickname, false );

    if( retv != 0 )
        return retv < 0;

    // Technically design block names are not case sensitive because the file name is used
    // as the design block name.  On windows this would be problematic because windows does
    // not support case sensitive file names by default.  This should not cause any issues
    // and allow for a future change to use the name defined in the design block file.
    return StrNumCmp( lhs.m_dbname, rhs.m_dbname, false ) < 0;
}
