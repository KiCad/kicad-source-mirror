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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <api/api_handler_libraries.h>

#include <api/api_enums.h>
#include <api/api_utils.h>
#include <libraries/library_manager.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <api/common/types/library_types.pb.h>

#include <design_block_library_adapter.h>

using namespace kiapi::common::commands;
using kiapi::common::types::LibraryType;


API_HANDLER_LIBRARIES::API_HANDLER_LIBRARIES( LIBRARY_TABLE_TYPE aType ) :
        m_type( aType )
{
    registerHandler<GetLibraryItems, LibraryItemsResponse>( &API_HANDLER_LIBRARIES::handleGetLibraryItems );
}


LIBRARY_MANAGER_ADAPTER* API_HANDLER_LIBRARIES::adapterForProject( PROJECT& aProject ) const
{
    return aProject.DesignBlockLibs();
}


std::vector<wxString> API_HANDLER_LIBRARIES::getItemNames( LIBRARY_MANAGER_ADAPTER& aAdapter,
                                                           const wxString& aNickname ) const
{
    return static_cast<DESIGN_BLOCK_LIBRARY_ADAPTER&>( aAdapter ).GetDesignBlockNames( aNickname );
}


HANDLER_RESULT<LibraryItemsResponse>
API_HANDLER_LIBRARIES::handleGetLibraryItems( const HANDLER_CONTEXT<GetLibraryItems>& aCtx )
{
    if( aCtx.Request.type() != ToProtoEnum<LIBRARY_TABLE_TYPE, LibraryType>( m_type ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    PROJECT& project = Pgm().GetSettingsManager().Prj();
    LIBRARY_MANAGER_ADAPTER* adapter = adapterForProject( project );

    if( !adapter )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "no library adapter is available for this library type" );
        return tl::unexpected( e );
    }

    std::vector<wxString> nicknames;

    if( aCtx.Request.nickname_size() > 0 )
    {
        for( const std::string& nickname : aCtx.Request.nickname() )
        {
            wxString nicknameStr = wxString::FromUTF8( nickname );

            // TODO(JE) decide whether or not to do blocking loads for explicit requests
            adapter->LoadLibraryEntry( nicknameStr );

            nicknames.emplace_back( nicknameStr );
        }
    }
    else
    {
        for( const LIBRARY_TABLE_ROW* row : adapter->Rows() )
            nicknames.emplace_back( row->Nickname() );
    }

    LibraryItemsResponse response;

    for( const wxString& nickname : nicknames )
    {
        for( const wxString& itemName : getItemNames( *adapter, nickname ) )
        {
            kiapi::common::types::LibraryIdentifier* id = response.add_items();
            id->set_library_nickname( nickname.ToUTF8() );
            id->set_entry_name( itemName.ToUTF8() );
        }
    }

    return response;
}
