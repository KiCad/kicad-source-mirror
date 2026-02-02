/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 2013-2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
 * Functions to read footprint libraries and fill m_footprints by available footprints names
 * and their documentation (comments and keywords)
 */

#include <footprint_info.h>
#include <dialogs/html_message_box.h>
#include <string_utils.h>
#include <lib_id.h>
#include <wx/tokenzr.h>

FOOTPRINT_INFO* FOOTPRINT_LIST::GetFootprintInfo( const wxString& aLibNickname,
                                                  const wxString& aFootprintName )
{
    if( aFootprintName.IsEmpty() )
        return nullptr;

    for( std::unique_ptr<FOOTPRINT_INFO>& fp : m_list )
    {
        if( aLibNickname == fp->GetLibNickname() && aFootprintName == fp->GetFootprintName() )
            return fp.get();
    }

    return nullptr;
}


FOOTPRINT_INFO* FOOTPRINT_LIST::GetFootprintInfo( const wxString& aFootprintName )
{
    if( aFootprintName.IsEmpty() )
        return nullptr;

    LIB_ID fpid;

    wxCHECK_MSG( fpid.Parse( aFootprintName ) < 0, nullptr,
                 wxString::Format( wxT( "'%s' is not a valid LIB_ID." ), aFootprintName ) );

    return GetFootprintInfo( fpid.GetLibNickname(), fpid.GetLibItemName() );
}


std::vector<SEARCH_TERM> FOOTPRINT_INFO::GetSearchTerms()
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


bool FOOTPRINT_INFO::InLibrary( const wxString& aLibrary ) const
{
    return aLibrary == m_nickname;
}


bool operator<( const FOOTPRINT_INFO& lhs, const FOOTPRINT_INFO& rhs )
{
    int retv = StrNumCmp( lhs.m_nickname, rhs.m_nickname, false );

    if( retv != 0 )
        return retv < 0;

    // Technically footprint names are not case sensitive because the file name is used
    // as the footprint name.  On windows this would be problematic because windows does
    // not support case sensitive file names by default.  This should not cause any issues
    // and allow for a future change to use the name defined in the footprint file.
    return StrNumCmp( lhs.m_fpname, rhs.m_fpname, false ) < 0;
}


wxString FOOTPRINT_LIST::GetErrorMessages()
{
    wxString messages;

    while( std::unique_ptr<IO_ERROR> error = PopError() )
    {
        if( !messages.IsEmpty() )
            messages += wxS( "\n" );

        messages += error->Problem();
    }

    return messages;
}
