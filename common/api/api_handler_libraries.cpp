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

#include <magic_enum.hpp>
#include <ranges>

#include <api/api_enums.h>
#include <api/api_utils.h>
#include <libraries/library_manager.h>
#include <kiway.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <api/common/types/base_types.pb.h>
#include <api/common/commands/editor_commands.pb.h>
#include <api/common/types/library_types.pb.h>

#include <design_block_library_adapter.h>

using namespace kiapi::common::commands;
using kiapi::common::types::LibraryCommandStatus;
using kiapi::common::types::LibraryLoadStatus;
using kiapi::common::types::LibraryTableScope;
using kiapi::common::types::LibraryType;

API_HANDLER_LIBRARIES::API_HANDLER_LIBRARIES( LIBRARY_TABLE_TYPE aType ) :
        m_type( aType )
{
    registerHandler<GetLibraryItems, LibraryItemsResponse>( &API_HANDLER_LIBRARIES::handleGetLibraryItems );
    registerHandler<GetLibraryStatuses, LibraryStatusResponse>(
            &API_HANDLER_LIBRARIES::handleGetLibraryStatuses );
    registerHandler<ReloadLibrary, LibraryCommandStatus>( &API_HANDLER_LIBRARIES::handleReloadLibrary );
    registerHandler<GetItemsFromLibrary, GetItemsResponse>( &API_HANDLER_LIBRARIES::handleGetItemsFromLibrary );

    // Only the design-block instance registers LoadAllLibraries: it is the dispatcher that
    // lazily loads the schematic and PCB kifaces and forwards to their per-type loads.  If
    // the per-type (SYMBOL/FOOTPRINT) handlers registered it too, the server's handler set
    // (ordered by pointer, not registration order) could route the command to an instance
    // without a KIWAY, which would fail the request.
    if( aType == LIBRARY_TABLE_TYPE::DESIGN_BLOCK )
        registerHandler<LoadAllLibraries, LibraryCommandStatus>(
                &API_HANDLER_LIBRARIES::handleLoadAllLibraries );
}


API_HANDLER_LIBRARIES::~API_HANDLER_LIBRARIES() = default;


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


static void packTableEntry( kiapi::common::types::LibraryTableEntry& aEntry, LIBRARY_TABLE_TYPE aTableType,
                            const LIBRARY_TABLE_ROW& aRow )
{
    aEntry.set_nickname( aRow.Nickname().ToUTF8() );
    aEntry.set_uri( aRow.URI().ToUTF8() );
    aEntry.set_type( ToProtoEnum<LIBRARY_TABLE_TYPE, LibraryType>( aTableType ) );
    aEntry.set_scope( ToProtoEnum<LIBRARY_TABLE_SCOPE, LibraryTableScope>( aRow.Scope() ) );
    aEntry.set_description( aRow.Description().ToUTF8() );

    for( const auto& [key, value] : aRow.GetOptionsMap() )
        ( *aEntry.mutable_options() )[key] = value;

    if( std::optional<LIBRARY_MANAGER_ADAPTER*> adapter = Pgm().GetLibraryManager().Adapter( aTableType ) )
        aEntry.set_writable( ( *adapter )->IsWritable( aRow.Nickname() ) );

    aEntry.set_disabled( aRow.Disabled() );
    aEntry.set_hidden( aRow.Hidden() );
}


static void packLibraryStatusEntry( kiapi::common::types::LibraryStatusEntry* aEntry, LIBRARY_TABLE_TYPE aTableType,
                                    const LIBRARY_TABLE_ROW& aRow, const LIB_STATUS& aStatus )
{
    packTableEntry( *aEntry->mutable_entry(), aTableType, aRow );
    aEntry->set_status( ToProtoEnum<LOAD_STATUS, LibraryLoadStatus>( aStatus.load_status ) );

    if( aStatus.error )
        aEntry->set_error_message( aStatus.error->message.ToUTF8() );
}


HANDLER_RESULT<LibraryStatusResponse>
API_HANDLER_LIBRARIES::handleGetLibraryStatuses( const HANDLER_CONTEXT<GetLibraryStatuses>& aCtx )
{
    LIBRARY_TABLE_SCOPE scope = FromProtoEnum<LIBRARY_TABLE_SCOPE>( aCtx.Request.scope() );

    if( scope == LIBRARY_TABLE_SCOPE::UNINITIALIZED )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "unknown library table scope" );
        return tl::unexpected( e );
    }

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();
    LibraryStatusResponse response;

    std::vector types = { LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_TYPE::FOOTPRINT, LIBRARY_TABLE_TYPE::DESIGN_BLOCK };

    if( !aCtx.Request.types().empty() )
    {
        types.clear();

        for( int typeProto : aCtx.Request.types() )
        {
            LIBRARY_TABLE_TYPE type = FromProtoEnum<LIBRARY_TABLE_TYPE>( static_cast<LibraryType>( typeProto ) );

            if( type == LIBRARY_TABLE_TYPE::UNINITIALIZED )
            {
                ApiResponseStatus e;
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message( wxString::Format( "invalid library table type %d", typeProto ).ToUTF8() );
                return tl::unexpected( e );
            }

            types.push_back( type );
        }
    }

    for( LIBRARY_TABLE_TYPE tableType : types )
    {
        LIBRARY_MANAGER_ADAPTER* adapter = nullptr;

        if( std::optional<LIBRARY_MANAGER_ADAPTER*> optAdapter = manager.Adapter( tableType ) )
            adapter = *optAdapter;

        for( const LIBRARY_TABLE_ROW* row : manager.Rows( tableType, scope ) )
        {
            if( row->Disabled() )
                continue;

            kiapi::common::types::LibraryStatusEntry* entry = response.add_libraries();

            if( adapter )
            {
                if( std::optional<LIB_STATUS> status = adapter->GetLibraryStatus( row->Nickname() ) )
                {
                    packLibraryStatusEntry( entry, tableType, *row, *status );
                    continue;
                }
            }

            packLibraryStatusEntry( entry, tableType, *row, LIB_STATUS{} );
        }
    }

    return response;
}


HANDLER_RESULT<LibraryCommandStatus>
API_HANDLER_LIBRARIES::handleReloadLibrary( const HANDLER_CONTEXT<ReloadLibrary>& aCtx )
{
    auto makeStatus =
            []( LibraryCommandStatus::Code aCode, const wxString& aMessage = wxEmptyString )
            {
                LibraryCommandStatus status;
                status.set_code( aCode );

                if( !aMessage.empty() )
                    status.set_error_message( aMessage.ToUTF8() );

                return status;
            };

    if( aCtx.Request.type() == LibraryType::LT_UNKNOWN )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "a valid library type is required" );
        return tl::unexpected( e );
    }

    if( aCtx.Request.scope() == LibraryTableScope::LTS_BOTH || aCtx.Request.scope() == LibraryTableScope::LTS_UNKNOWN )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "LTS_BOTH is invalid for ReloadLibrary; choose one table" );
        return tl::unexpected( e );
    }

    const LIBRARY_TABLE_TYPE  tableType = FromProtoEnum<LIBRARY_TABLE_TYPE, LibraryType>( aCtx.Request.type() );
    const LIBRARY_TABLE_SCOPE scope = ( aCtx.Request.scope() == LibraryTableScope::LTS_GLOBAL )
                                              ? LIBRARY_TABLE_SCOPE::GLOBAL
                                              : LIBRARY_TABLE_SCOPE::PROJECT;

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    if( !manager.Adapter( tableType ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    std::vector<wxString> nicknames;

    if( aCtx.Request.nickname_size() > 0 )
    {
        for( const std::string& nickname : aCtx.Request.nickname() )
            nicknames.emplace_back( wxString::FromUTF8( nickname ) );
    }
    else
    {
        for( const LIBRARY_TABLE_ROW* row : manager.Rows( tableType, scope ) )
        {
            if( !row->Disabled() )
                nicknames.emplace_back( row->Nickname() );
        }
    }

    if( nicknames.empty() )
        return makeStatus( LibraryCommandStatus::LCS_NOT_FOUND, "No libraries found to reload" );

    for( const wxString& nickname : nicknames )
    {
        if( !manager.GetRow( tableType, nickname, scope ) )
        {
            return makeStatus( LibraryCommandStatus::LCS_NOT_FOUND,
                               wxString::Format( "Library '%s' not found", nickname ) );
        }
    }

    for( const wxString& nickname : nicknames )
        manager.ReloadLibraryEntry( tableType, nickname, scope );

    return makeStatus( LibraryCommandStatus::LCS_OK );
}


HANDLER_RESULT<LibraryCommandStatus>
API_HANDLER_LIBRARIES::handleLoadAllLibraries( const HANDLER_CONTEXT<LoadAllLibraries>& aCtx )
{
    auto makeStatus = []( LibraryCommandStatus::Code aCode, const wxString& aMessage = wxEmptyString )
    {
        LibraryCommandStatus status;
        status.set_code( aCode );

        if( !aMessage.empty() )
            status.set_error_message( aMessage.ToUTF8() );

        return status;
    };

    if( Pgm().IsGUI() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNIMPLEMENTED );
        e.set_error_message( "LoadAllLibraries is not available in GUI mode" );
        return tl::unexpected( e );
    }

    std::map<LIBRARY_TABLE_TYPE, KIWAY::FACE_T> typesToLoad;

    if( aCtx.Request.type_size() == 0 )
    {
        typesToLoad = { { LIBRARY_TABLE_TYPE::SYMBOL, KIWAY::FACE_SCH },
                        { LIBRARY_TABLE_TYPE::FOOTPRINT, KIWAY::FACE_PCB },
                        { LIBRARY_TABLE_TYPE::DESIGN_BLOCK, KIWAY::KIWAY_FACE_COUNT } };
    }
    else
    {
        for( int protoRaw : aCtx.Request.type() )
        {
            LibraryType protoType = static_cast<LibraryType>( protoRaw );
            LIBRARY_TABLE_TYPE type =
                    FromProtoEnum<LIBRARY_TABLE_TYPE, LibraryType>( static_cast<LibraryType>( protoType ) );

            switch( type )
            {
            case LIBRARY_TABLE_TYPE::SYMBOL:        typesToLoad.emplace( type, KIWAY::FACE_SCH );         break;
            case LIBRARY_TABLE_TYPE::FOOTPRINT:     typesToLoad.emplace( type, KIWAY::FACE_PCB );         break;
            case LIBRARY_TABLE_TYPE::DESIGN_BLOCK:  typesToLoad.emplace( type, KIWAY::KIWAY_FACE_COUNT ); break;

            default:
            {
                ApiResponseStatus e;
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message(
                        wxString::Format( "invalid library type %s", magic_enum::enum_name( protoType ) ).ToUTF8() );
                return tl::unexpected( e );
            }
            }
        }
    }

    // should always be set in this path
    if( !m_kiway )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_NOT_READY );
        e.set_error_message( "internal error while attempting to load all libraries" );
        return tl::unexpected( e );
    }

    KIWAY& kiway = *m_kiway;

    for( const auto& [type, face] : typesToLoad )
    {
        if( type == LIBRARY_TABLE_TYPE::DESIGN_BLOCK )
        {
            loadAllLibraries();
            continue;
        }

        KIFACE* kiface = kiway.KiFACE( face );

        wxCHECK2( kiface, continue );

        if( m_handlerRegisterCallback )
            m_handlerRegisterCallback( kiface );

        kiface->LoadAllLibraries();
    }

    return makeStatus( LibraryCommandStatus::LCS_OK );
}


LibraryCommandStatus API_HANDLER_LIBRARIES::loadAllLibraries()
{
    // This base implementation handles design blocks
    LIBRARY_MANAGER_ADAPTER* adapter = adapterForProject( Pgm().GetSettingsManager().Prj() );

    LibraryCommandStatus status;

    if( !adapter )
        return status;

    adapter->AsyncLoad();
    status.set_code( LibraryCommandStatus::LCS_OK );
    return status;
}


HANDLER_RESULT<GetItemsResponse>
API_HANDLER_LIBRARIES::handleGetItemsFromLibrary( const HANDLER_CONTEXT<GetItemsFromLibrary>& aCtx )
{
    if( aCtx.Request.type() != ToProtoEnum<LIBRARY_TABLE_TYPE, LibraryType>( m_type ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    wxString projectPath = wxString::FromUTF8( aCtx.Request.document().project().path() );
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    PROJECT* project = projectPath.IsEmpty() ? &mgr.Prj() : mgr.GetProjectForPath( projectPath );

    if( !project )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "the requested project is not open" );
        return tl::unexpected( e );
    }

    LIBRARY_MANAGER_ADAPTER* adapter = adapterForProject( *project );

    if( !adapter )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "no library adapter is available for this library type" );
        return tl::unexpected( e );
    }

    GetItemsResponse response;
    response.mutable_header()->mutable_document()->CopyFrom( aCtx.Request.document() );
    google::protobuf::Any any;

    if( aCtx.Request.item_ids().empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "specify at least one lib_id to retrieve" );
        return tl::unexpected( e );
    }

    std::map<wxString, std::vector<wxString>> byNickname;

    for( const kiapi::common::types::LibraryIdentifier& id : aCtx.Request.item_ids() )
    {
        byNickname[wxString::FromUTF8( id.library_nickname() )].emplace_back( wxString::FromUTF8( id.entry_name() ) );
    }

    for( auto& [nickname, entryNames] : byNickname )
    {
        // TODO(JE) decide whether or not to do blocking loads for explicit requests
        adapter->LoadLibraryEntry( nickname );

        for( const wxString& entryName : entryNames )
        {
            if( packLibraryItem( LIB_ID( nickname, entryName ), any ) )
            {
                any.Swap( response.add_items() );
                any.Clear();
            }
        }
    }

    if( response.items().empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "none of the requested items were found" );
        return tl::unexpected( e );
    }

    response.set_status( kiapi::common::types::ItemRequestStatus::IRS_OK );
    return response;
}
