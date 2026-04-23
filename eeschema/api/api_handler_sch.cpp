/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
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

#include <api/api_handler_sch.h>
#include <api/api_enums.h>
#include <api/api_sch_utils.h>
#include <api/api_utils.h>
#include <api/sch_context.h>
#include <magic_enum.hpp>
#include <base_screen.h>
#include <jobs/job_export_sch_bom.h>
#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_export_sch_plot.h>
#include <kiway.h>
#include <sch_field.h>
#include <sch_group.h>
#include <connection_graph.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_label.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <wx/filename.h>

#include <api/common/types/base_types.pb.h>

using namespace kiapi::common::commands;
using kiapi::common::types::CommandStatus;
using kiapi::common::types::DocumentType;
using kiapi::common::types::ItemRequestStatus;


std::set<KICAD_T> API_HANDLER_SCH::s_allowedTypes = {
    // SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_SHAPE_T,
    SCH_BITMAP_T,
    SCH_TEXTBOX_T,
    SCH_TEXT_T,
    // SCH_TABLE_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_GROUP_T,
    SCH_HIER_LABEL_T,
    SCH_DIRECTIVE_LABEL_T,
    SCH_SYMBOL_T,
    SCH_SHEET_T,
};


HANDLER_RESULT<types::RunJobResponse> ExecuteSchematicJob( KIWAY* aKiway, JOB& aJob )
{
    types::RunJobResponse response;
    WX_STRING_REPORTER reporter;
    int exitCode = aKiway->ProcessJob( KIWAY::FACE_SCH, &aJob, &reporter );

    for( const JOB_OUTPUT& output : aJob.GetOutputs() )
        response.add_output_path( output.m_outputPath.ToUTF8() );

    if( exitCode == 0 )
    {
        response.set_status( types::JobStatus::JS_SUCCESS );
        return response;
    }

    response.set_status( types::JobStatus::JS_ERROR );
    response.set_message( fmt::format( "Schematic export job '{}' failed with exit code {}: {}",
                                       aJob.GetType(), exitCode,
                                       reporter.GetMessages().ToStdString() ) );
    return response;
}


API_HANDLER_SCH::API_HANDLER_SCH( SCH_EDIT_FRAME* aFrame ) :
        API_HANDLER_SCH( CreateSchFrameContext( aFrame ), aFrame )
{
}


API_HANDLER_SCH::API_HANDLER_SCH( std::shared_ptr<SCH_CONTEXT> aContext,
                                  SCH_EDIT_FRAME* aFrame ) :
        API_HANDLER_EDITOR( aFrame ),
        m_frame( aFrame ),
        m_context( std::move( aContext ) )
{
    using namespace kiapi::schematic::jobs;
    using namespace kiapi::schematic::types;

    registerHandler<GetOpenDocuments, GetOpenDocumentsResponse>(
            &API_HANDLER_SCH::handleGetOpenDocuments );
    registerHandler<GetItems, GetItemsResponse>( &API_HANDLER_SCH::handleGetItems );
    registerHandler<GetItemsById, GetItemsResponse>( &API_HANDLER_SCH::handleGetItemsById );

    registerHandler<RunSchematicJobExportSvg, types::RunJobResponse>(
            &API_HANDLER_SCH::handleRunSchematicJobExportSvg );
    registerHandler<RunSchematicJobExportDxf, types::RunJobResponse>(
            &API_HANDLER_SCH::handleRunSchematicJobExportDxf );
    registerHandler<RunSchematicJobExportPdf, types::RunJobResponse>(
            &API_HANDLER_SCH::handleRunSchematicJobExportPdf );
    registerHandler<RunSchematicJobExportPs, types::RunJobResponse>(
            &API_HANDLER_SCH::handleRunSchematicJobExportPs );
    registerHandler<RunSchematicJobExportNetlist, types::RunJobResponse>(
            &API_HANDLER_SCH::handleRunSchematicJobExportNetlist );
    registerHandler<RunSchematicJobExportBOM, types::RunJobResponse>(
            &API_HANDLER_SCH::handleRunSchematicJobExportBOM );
    registerHandler<GetSchematicHierarchy, SchematicHierarchyResponse>( &API_HANDLER_SCH::handleGetSchematicHierarchy );
    registerHandler<GetPageSettings, types::PageSettings>( &API_HANDLER_SCH::handleGetPageSettings );
    registerHandler<SetPageSettings, types::PageSettings>( &API_HANDLER_SCH::handleSetPageSettings );
    registerHandler<GetSchematicNetlist, SchematicNetlistResponse>( &API_HANDLER_SCH::handleGetSchematicNetlist );
}


std::unique_ptr<COMMIT> API_HANDLER_SCH::createCommit()
{
    return std::make_unique<SCH_COMMIT>( m_frame );
}


SCHEMATIC* API_HANDLER_SCH::schematic() const
{
    wxCHECK( m_context, nullptr );
    return m_context->GetSchematic();
}


std::optional<SCH_ITEM*> API_HANDLER_SCH::getItemById( const KIID& aId, SCH_SHEET_PATH* aPathOut ) const
{
    if( !schematic()->HasHierarchy() )
        schematic()->RefreshHierarchy();

    SCH_ITEM* item = schematic()->ResolveItem( aId, aPathOut, true );

    if( !item )
        return std::nullopt;

    return item;
}


tl::expected<bool, ApiResponseStatus>
API_HANDLER_SCH::validateDocumentInternal( const DocumentSpecifier& aDocument ) const
{
    if( aDocument.type() != DocumentType::DOCTYPE_SCHEMATIC )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "the requested document is not a schematic" );
        return tl::unexpected( e );
    }

    const PROJECT& prj = m_context->Prj();

    if( aDocument.project().name().compare( prj.GetProjectName().ToUTF8() ) != 0 )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "the requested project {} is not open",
                                          aDocument.project().name() ) );
        return tl::unexpected( e );
    }

    if( aDocument.project().path().compare( prj.GetProjectPath().ToUTF8() ) != 0 )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "the requested project {} is not open at path {}",
                                          aDocument.project().name(),
                                          aDocument.project().path() ) );
        return tl::unexpected( e );
    }

    if( aDocument.has_sheet_path() )
    {
        KIID_PATH path = UnpackSheetPath( aDocument.sheet_path() );

        if( !schematic()->Hierarchy().HasPath( path ) )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( fmt::format( "the requested sheet path {} is not valid for this schematic",
                                              path.AsString().ToStdString() ) );
            return tl::unexpected( e );
        }
    }

    return true;
}


HANDLER_RESULT<GetOpenDocumentsResponse> API_HANDLER_SCH::handleGetOpenDocuments(
        const HANDLER_CONTEXT<GetOpenDocuments>& aCtx )
{
    if( aCtx.Request.type() != DocumentType::DOCTYPE_SCHEMATIC )
    {
        ApiResponseStatus e;

        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    GetOpenDocumentsResponse response;
    common::types::DocumentSpecifier doc;

    wxFileName fn( m_context->GetCurrentFileName() );

    doc.set_type( DocumentType::DOCTYPE_SCHEMATIC );

    if( std::optional<SCH_SHEET_PATH> path = m_context->GetCurrentSheet() )
        PackSheetPath( *doc.mutable_sheet_path(), path->Path() );

    PackProject( *doc.mutable_project(), m_context->Prj() );

    response.mutable_documents()->Add( std::move( doc ) );
    return response;
}


void API_HANDLER_SCH::filterValidSchTypes( std::set<KICAD_T>& aTypeList )
{
    std::erase_if( aTypeList,
                   []( KICAD_T aType )
                   {
                       return !s_allowedTypes.contains( aType );
                   } );
}


HANDLER_RESULT<GetItemsResponse> API_HANDLER_SCH::handleGetItems( const HANDLER_CONTEXT<GetItems>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( HANDLER_RESULT<std::optional<KIID>> valid = validateItemHeaderDocument( aCtx.Request.header() );
        !valid.has_value() )
    {
        return tl::unexpected( valid.error() );
    }

    std::set<KICAD_T> typesRequested, typesInserted;

    for( KICAD_T type : parseRequestedItemTypes( aCtx.Request.types() ) )
        typesRequested.insert( type );

    filterValidSchTypes( typesRequested );

    if( typesRequested.empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "none of the requested types are valid for a Schematic object" );
        return tl::unexpected( e );
    }

    SCH_SHEET_LIST hierarchy = schematic()->Hierarchy();
    std::optional<SCH_SHEET_PATH> pathFilter;

    if( aCtx.Request.header().document().has_sheet_path() )
    {
        KIID_PATH kp = UnpackSheetPath( aCtx.Request.header().document().sheet_path() );
        pathFilter = hierarchy.GetSheetPathByKIIDPath( kp );
    }

    std::map<KICAD_T, std::vector<std::pair<EDA_ITEM*, SCH_SHEET_PATH>>> itemMap;

    auto processScreen =
        [&]( const SCH_SHEET_PATH& aPath )
        {
            const SCH_SCREEN* aScreen = aPath.LastScreen();

            for( SCH_ITEM* aItem : aScreen->Items() )
            {
                itemMap[ aItem->Type() ].emplace_back( aItem, aPath );

                aItem->RunOnChildren(
                        [&]( SCH_ITEM* aChild )
                        {
                            itemMap[ aChild->Type() ].emplace_back( aChild, aPath );
                        },
                        RECURSE_MODE::NO_RECURSE );
            }
        };

    if( pathFilter )
    {
        processScreen( *pathFilter );
    }
    else
    {
        for( const SCH_SHEET_PATH& path : hierarchy )
            processScreen( path );
    }

    GetItemsResponse response;
    google::protobuf::Any any;

    for( KICAD_T type : parseRequestedItemTypes( aCtx.Request.types() ) )
    {
        if( !s_allowedTypes.contains( type ) )
            continue;

        if( typesInserted.contains( type ) )
            continue;

        for( const auto& [item, itemPath] : itemMap[type] )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                kiapi::schematic::types::SchematicSymbolInstance symbol;

                if( !PackSymbol( &symbol, static_cast<SCH_SYMBOL*>( item ), itemPath ) )
                    continue;

                any.PackFrom( symbol );
            }
            else if( item->Type() == SCH_SHEET_T )
            {
                kiapi::schematic::types::SheetSymbol sheet;

                if( !PackSheet( &sheet, static_cast<SCH_SHEET*>( item ), itemPath ) )
                    continue;

                any.PackFrom( sheet );
            }
            else
            {
                item->Serialize( any );
            }

            response.mutable_items()->Add( std::move( any ) );
        }
    }

    response.set_status( ItemRequestStatus::IRS_OK );
    return response;
}


HANDLER_RESULT<GetItemsResponse> API_HANDLER_SCH::handleGetItemsById( const HANDLER_CONTEXT<GetItemsById>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    SCH_SHEET_LIST hierarchy = schematic()->Hierarchy();
    std::optional<SCH_SHEET_PATH> pathFilter;

    if( aCtx.Request.header().document().has_sheet_path() )
    {
        KIID_PATH kp = UnpackSheetPath( aCtx.Request.header().document().sheet_path() );
        pathFilter = hierarchy.GetSheetPathByKIIDPath( kp );
    }

    GetItemsResponse response;
    SCH_ITEM* item = nullptr;
    google::protobuf::Any any;

    for( const types::KIID& idProto : aCtx.Request.items() )
    {
        KIID id( idProto.value() );

        SCH_SHEET_PATH itemPath;

        if( pathFilter )
        {
            item = pathFilter->ResolveItem( id );
            itemPath = *pathFilter;
        }
        else
        {
            item = hierarchy.ResolveItem( id, &itemPath, true );
        }

        if( !item || !s_allowedTypes.contains( item->Type() ) )
            continue;

        if( item->Type() == SCH_SYMBOL_T )
        {
            kiapi::schematic::types::SchematicSymbolInstance symbol;

            if( !PackSymbol( &symbol, static_cast<SCH_SYMBOL*>( item ), itemPath ) )
                continue;

            any.PackFrom( symbol );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            kiapi::schematic::types::SheetSymbol sheet;

            if( !PackSheet( &sheet, static_cast<SCH_SHEET*>( item ), itemPath ) )
                continue;

            any.PackFrom( sheet );
        }
        else
        {
            item->Serialize( any );
        }

        response.mutable_items()->Add( std::move( any ) );
    }

    if( response.items().empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "none of the requested IDs were found or valid" );
        return tl::unexpected( e );
    }

    response.set_status( ItemRequestStatus::IRS_OK );
    return response;
}


HANDLER_RESULT<std::unique_ptr<EDA_ITEM>> API_HANDLER_SCH::createItemForType( KICAD_T aType, EDA_ITEM* aContainer )
{
    if( !aContainer )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "Tried to create an item in a null container" );
        return tl::unexpected( e );
    }

    if( !s_allowedTypes.contains( aType ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "type {} is not supported by the schematic API handler",
                                          magic_enum::enum_name( aType ) ) );
        return tl::unexpected( e );
    }

    if( aType == SCH_PIN_T && !dynamic_cast<SCH_SYMBOL*>( aContainer ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "Tried to create a pin in {}, which is not a symbol",
                                          aContainer->GetFriendlyName().ToStdString() ) );
        return tl::unexpected( e );
    }
    else if( aType == SCH_SHEET_T && !dynamic_cast<SCH_SCREEN*>( aContainer ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "Tried to create a sheet symbol in {}, which is not a "
                                          "schematic sheet",
                                          aContainer->GetFriendlyName().ToStdString() ) );
        return tl::unexpected( e );
    }
    else if( aType == SCH_SYMBOL_T && !dynamic_cast<SCH_SCREEN*>( aContainer ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "Tried to create a symbol in {}, which is not a "
                                          "schematic sheet",
                                          aContainer->GetFriendlyName().ToStdString() ) );
        return tl::unexpected( e );
    }

    std::unique_ptr<EDA_ITEM> created = CreateItemForType( aType, aContainer );

    if( created && !created->GetParent() )
        created->SetParent( aContainer );

    if( !created )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "Tried to create an item of type {}, which is unhandled",
                                          magic_enum::enum_name( aType ) ) );
        return tl::unexpected( e );
    }

    return created;
}


HANDLER_RESULT<ItemRequestStatus> API_HANDLER_SCH::handleCreateUpdateItemsInternal( bool aCreate,
        const std::string& aClientName,
        const types::ItemHeader &aHeader,
        const google::protobuf::RepeatedPtrField<google::protobuf::Any>& aItems,
        std::function<void( ItemStatus, google::protobuf::Any )> aItemHandler )
{
    ApiResponseStatus e;

    auto containerResult = validateItemHeaderDocument( aHeader );

    if( !containerResult && containerResult.error().status() == ApiStatusCode::AS_UNHANDLED )
    {
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }
    else if( !containerResult )
    {
        e.CopyFrom( containerResult.error() );
        return tl::unexpected( e );
    }

    SCH_SHEET_LIST hierarchy = schematic()->Hierarchy();
    SCH_SCREEN* targetScreen = schematic()->GetCurrentScreen();
    SCH_SHEET_PATH targetPath = m_context->GetCurrentSheet().value_or( *hierarchy.begin() );

    if( aHeader.document().has_sheet_path() )
    {
        KIID_PATH kp = UnpackSheetPath( aHeader.document().sheet_path() );
        if( std::optional<SCH_SHEET_PATH> path = hierarchy.GetSheetPathByKIIDPath( kp ) )
        {
            targetPath = *path;
            targetScreen = targetPath.LastScreen();
        }
    }

    SCH_COMMIT* commit = static_cast<SCH_COMMIT*>( getCurrentCommit( aClientName ) );

    for( const google::protobuf::Any& anyItem : aItems )
    {
        ItemStatus status;
        std::optional<KICAD_T> type = TypeNameFromAny( anyItem );

        if( !type )
        {
            status.set_code( ItemStatusCode::ISC_INVALID_TYPE );
            status.set_error_message( fmt::format( "Could not decode a valid type from {}",
                                                   anyItem.type_url() ) );
            aItemHandler( status, anyItem );
            continue;
        }

        EDA_ITEM* container = targetScreen;

        HANDLER_RESULT<std::unique_ptr<EDA_ITEM>> creationResult = createItemForType( *type, container );

        if( !creationResult )
        {
            status.set_code( ItemStatusCode::ISC_INVALID_TYPE );
            status.set_error_message( creationResult.error().error_message() );
            aItemHandler( status, anyItem );
            continue;
        }

        std::unique_ptr<EDA_ITEM> item( std::move( *creationResult ) );

        bool unpacked = false;

        if( *type == SCH_SYMBOL_T )
        {
            kiapi::schematic::types::SchematicSymbolInstance symbol;
            unpacked = anyItem.UnpackTo( &symbol )
                       && UnpackSymbol( static_cast<SCH_SYMBOL*>( item.get() ), symbol );
        }
        else if( *type == SCH_SHEET_T )
        {
            kiapi::schematic::types::SheetSymbol sheetProto;
            unpacked = anyItem.UnpackTo( &sheetProto );

            if( unpacked )
            {
                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item.get() );

                if( tl::expected<bool, ApiResponseStatus> result = UnpackSheet( sheet, sheetProto );
                    result.has_value() )
                {
                    unpacked = *result;
                    SCH_SHEET_INSTANCE instance;

                    if( !sheet->GetInstances().empty() )
                        instance = *sheet->GetInstances().begin();

                    if( instance.m_PageNumber.IsEmpty() )
                        instance.m_PageNumber = hierarchy.GetNextPageNumber();

                    if( instance.m_Path.empty() )
                    {
                        SCH_SHEET_PATH newPath( targetPath );
                        newPath.push_back( sheet );
                        instance.m_Path = newPath.Path();
                    }

                    sheet->AddInstance( instance );
                }
                else
                {
                    return tl::unexpected( result.error() );
                }
            }
        }
        else
        {
            unpacked = item->Deserialize( anyItem );
        }

        if( !unpacked )
        {
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( fmt::format( "could not unpack {} from request",
                                              item->GetClass().ToStdString() ) );
            return tl::unexpected( e );
        }

        SCH_ITEM* existingItem = nullptr;
        SCH_SHEET_PATH existingPath;

        existingItem = targetPath.ResolveItem( item->m_Uuid );

        if( existingItem )
            existingPath = targetPath;

        if( aCreate && existingItem )
        {
            status.set_code( ItemStatusCode::ISC_EXISTING );
            status.set_error_message( fmt::format( "an item with UUID {} already exists",
                                                   item->m_Uuid.AsStdString() ) );
            aItemHandler( status, anyItem );
            continue;
        }
        else if( !aCreate && !existingItem )
        {
            status.set_code( ItemStatusCode::ISC_NONEXISTENT );
            status.set_error_message( fmt::format( "an item with UUID {} does not exist",
                                                   item->m_Uuid.AsStdString() ) );
            aItemHandler( status, anyItem );
            continue;
        }

        if( !aCreate )
        {
            SCH_SCREEN* itemScreen = existingPath.LastScreen();

            if( itemScreen != targetScreen )
            {
                status.set_code( ItemStatusCode::ISC_INVALID_DATA );
                status.set_error_message( fmt::format( "item {} exists on a different sheet than targeted",
                                                       item->m_Uuid.AsStdString() ) );
                aItemHandler( status, anyItem );
                continue;
            }
        }

        if( *type == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item.get() );

            if( aCreate && !sheet->GetScreen() )
                sheet->SetScreen( new SCH_SCREEN( schematic() ) );

            SCH_SHEET_PATH parentPath;

            if( aCreate )
                parentPath = targetPath;
            else
                parentPath = existingPath;

            wxString destFilePath = parentPath.LastScreen()->GetFileName();

            if( !destFilePath.IsEmpty() )
            {
                SCH_SHEET_LIST schematicSheets = schematic()->Hierarchy();
                SCH_SHEET_LIST loadedSheets( sheet );

                if( schematicSheets.TestForRecursion( loadedSheets, destFilePath ) )
                {
                    status.set_code( ItemStatusCode::ISC_INVALID_DATA );
                    status.set_error_message( "sheet update would create recursive hierarchy" );
                    aItemHandler( status, anyItem );
                    continue;
                }
            }
        }

        status.set_code( ItemStatusCode::ISC_OK );
        google::protobuf::Any newItem;

        if( aCreate )
        {
            SCH_ITEM* createdItem = static_cast<SCH_ITEM*>( item.release() );
            commit->Add( createdItem, targetScreen );

            if( !createdItem )
            {
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message( "could not add the requested item to its parent container" );
                return tl::unexpected( e );
            }

            if( createdItem->Type() == SCH_SYMBOL_T )
            {
                kiapi::schematic::types::SchematicSymbolInstance symbol;

                if( PackSymbol( &symbol, static_cast<SCH_SYMBOL*>( createdItem ), targetPath ) )
                    newItem.PackFrom( symbol );
            }
            else if( createdItem->Type() == SCH_SHEET_T )
            {
                kiapi::schematic::types::SheetSymbol sheet;

                if( PackSheet( &sheet, static_cast<SCH_SHEET*>( createdItem ), targetPath ) )
                    newItem.PackFrom( sheet );
            }
            else
            {
                createdItem->Serialize( newItem );
            }
        }
        else
        {
            commit->Modify( existingItem, targetScreen );
            existingItem->SwapItemData( static_cast<SCH_ITEM*>( item.get() ) );

            if( existingItem->Type() == SCH_SYMBOL_T )
            {
                SCH_SHEET_PATH path = existingPath;
                kiapi::schematic::types::SchematicSymbolInstance symbol;

                if( PackSymbol( &symbol, static_cast<SCH_SYMBOL*>( existingItem ), path ) )
                    newItem.PackFrom( symbol );
            }
            else if( existingItem->Type() == SCH_SHEET_T )
            {
                SCH_SHEET_PATH path = existingPath;
                kiapi::schematic::types::SheetSymbol sheet;

                if( PackSheet( &sheet, static_cast<SCH_SHEET*>( existingItem ), path ) )
                    newItem.PackFrom( sheet );
            }
            else
            {
                existingItem->Serialize( newItem );
            }
        }

        aItemHandler( status, newItem );
    }

    if( !m_activeClients.contains( aClientName ) )
    {
        pushCurrentCommit( aClientName, aCreate ? _( "Created items via API" )
                                                : _( "Modified items via API" ) );
    }


    return ItemRequestStatus::IRS_OK;
}


void API_HANDLER_SCH::deleteItemsInternal( std::map<KIID, ItemDeletionStatus>& aItemsToDelete,
                                           const std::string& aClientName )
{
    SCH_SHEET_LIST hierarchy = schematic()->Hierarchy();
    COMMIT* commit = getCurrentCommit( aClientName );

    for( auto& [id, status] : aItemsToDelete )
    {
        SCH_SHEET_PATH path;
        SCH_ITEM* item = hierarchy.ResolveItem( id, &path, true );

        if( !item )
            continue;

        if( !s_allowedTypes.contains( item->Type() ) )
        {
            status = ItemDeletionStatus::IDS_IMMUTABLE;
            continue;
        }

        commit->Remove( item, path.LastScreen() );
        status = ItemDeletionStatus::IDS_OK;
    }

    if( !m_activeClients.contains( aClientName ) )
        pushCurrentCommit( aClientName, _( "Deleted items via API" ) );
}


std::optional<EDA_ITEM*> API_HANDLER_SCH::getItemFromDocument( const DocumentSpecifier& aDocument, const KIID& aId )
{
    if( !validateDocument( aDocument ) )
        return std::nullopt;

    SCH_ITEM* item = schematic()->Hierarchy().ResolveItem( aId, nullptr, true );

    if( !item)
        return std::nullopt;

    return item;
}


std::optional<TITLE_BLOCK*> API_HANDLER_SCH::getTitleBlock()
{
    wxCHECK( m_context->GetCurrentSheet(), std::nullopt );
    return &m_context->GetCurrentSheet()->LastScreen()->GetTitleBlock();
}


std::optional<PAGE_INFO> API_HANDLER_SCH::getPageSettings()
{
    wxCHECK( m_context->GetCurrentSheet(), std::nullopt );
    return m_context->GetCurrentSheet()->LastScreen()->GetPageSettings();
}


bool API_HANDLER_SCH::setPageSettings( const PAGE_INFO& aPageInfo )
{
    wxCHECK( m_context->GetCurrentSheet(), false );
    m_context->GetCurrentSheet()->LastScreen()->SetPageSettings( aPageInfo );
    return true;
}


wxString API_HANDLER_SCH::getDrawingSheetFileName()
{
    return BASE_SCREEN::m_DrawingSheetFileName;
}


void API_HANDLER_SCH::setDrawingSheetFileName( const wxString& aFileName )
{
    BASE_SCREEN::m_DrawingSheetFileName = aFileName;
    schematic()->Settings().m_SchDrawingSheetFileName = aFileName;

    if( m_frame )
        m_frame->LoadDrawingSheet();
}


void API_HANDLER_SCH::onModified()
{
    if( m_frame )
    {
        m_frame->Refresh();
        m_frame->OnModify();
    }
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_SCH::handleRunSchematicJobExportSvg(
        const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportSvg>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    auto plotJob = std::make_unique<JOB_EXPORT_SCH_PLOT_SVG>();
    plotJob->m_filename = m_context->GetCurrentFileName();

    if( !aCtx.Request.job_settings().output_path().empty() )
        plotJob->SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    const kiapi::schematic::jobs::SchematicPlotSettings& settings = aCtx.Request.plot_settings();

    plotJob->m_drawingSheet = wxString::FromUTF8( settings.drawing_sheet() );
    plotJob->m_defaultFont = wxString::FromUTF8( settings.default_font() );
    plotJob->m_variant = wxString::FromUTF8( settings.variant() );
    plotJob->m_plotAll = settings.plot_all();
    plotJob->m_plotDrawingSheet = settings.plot_drawing_sheet();
    plotJob->m_show_hop_over = settings.show_hop_over();
    plotJob->m_blackAndWhite = settings.black_and_white();
    plotJob->m_useBackgroundColor = settings.use_background_color();
    plotJob->m_minPenWidth = settings.min_pen_width();
    plotJob->m_theme = wxString::FromUTF8( settings.theme() );

    plotJob->m_plotPages.clear();

    for( const std::string& page : settings.plot_pages() )
        plotJob->m_plotPages.push_back( wxString::FromUTF8( page ) );

    if( aCtx.Request.plot_settings().page_size() != kiapi::schematic::jobs::SchematicJobPageSize::SJPS_UNKNOWN )
    {
        plotJob->m_pageSizeSelect = FromProtoEnum<JOB_PAGE_SIZE>( aCtx.Request.plot_settings().page_size() );
    }

    return ExecuteSchematicJob( m_context->GetKiway(), *plotJob );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_SCH::handleRunSchematicJobExportDxf(
        const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportDxf>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    auto plotJob = std::make_unique<JOB_EXPORT_SCH_PLOT_DXF>();
    plotJob->m_filename = m_context->GetCurrentFileName();

    if( !aCtx.Request.job_settings().output_path().empty() )
        plotJob->SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    const kiapi::schematic::jobs::SchematicPlotSettings& settings = aCtx.Request.plot_settings();

    plotJob->m_drawingSheet = wxString::FromUTF8( settings.drawing_sheet() );
    plotJob->m_defaultFont = wxString::FromUTF8( settings.default_font() );
    plotJob->m_variant = wxString::FromUTF8( settings.variant() );
    plotJob->m_plotAll = settings.plot_all();
    plotJob->m_plotDrawingSheet = settings.plot_drawing_sheet();
    plotJob->m_show_hop_over = settings.show_hop_over();
    plotJob->m_blackAndWhite = settings.black_and_white();
    plotJob->m_useBackgroundColor = settings.use_background_color();
    plotJob->m_minPenWidth = settings.min_pen_width();
    plotJob->m_theme = wxString::FromUTF8( settings.theme() );

    plotJob->m_plotPages.clear();

    for( const std::string& page : settings.plot_pages() )
        plotJob->m_plotPages.push_back( wxString::FromUTF8( page ) );

    if( aCtx.Request.plot_settings().page_size() != kiapi::schematic::jobs::SchematicJobPageSize::SJPS_UNKNOWN )
    {
        plotJob->m_pageSizeSelect = FromProtoEnum<JOB_PAGE_SIZE>( aCtx.Request.plot_settings().page_size() );
    }

    return ExecuteSchematicJob( m_context->GetKiway(), *plotJob );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_SCH::handleRunSchematicJobExportPdf(
        const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportPdf>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    auto plotJob = std::make_unique<JOB_EXPORT_SCH_PLOT_PDF>( false );
    plotJob->m_filename = m_context->GetCurrentFileName();

    if( !aCtx.Request.job_settings().output_path().empty() )
        plotJob->SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    const kiapi::schematic::jobs::SchematicPlotSettings& settings = aCtx.Request.plot_settings();

    plotJob->m_drawingSheet = wxString::FromUTF8( settings.drawing_sheet() );
    plotJob->m_defaultFont = wxString::FromUTF8( settings.default_font() );
    plotJob->m_variant = wxString::FromUTF8( settings.variant() );
    plotJob->m_plotAll = settings.plot_all();
    plotJob->m_plotDrawingSheet = settings.plot_drawing_sheet();
    plotJob->m_show_hop_over = settings.show_hop_over();
    plotJob->m_blackAndWhite = settings.black_and_white();
    plotJob->m_useBackgroundColor = settings.use_background_color();
    plotJob->m_minPenWidth = settings.min_pen_width();
    plotJob->m_theme = wxString::FromUTF8( settings.theme() );

    plotJob->m_plotPages.clear();

    for( const std::string& page : settings.plot_pages() )
        plotJob->m_plotPages.push_back( wxString::FromUTF8( page ) );

    if( aCtx.Request.plot_settings().page_size() != kiapi::schematic::jobs::SchematicJobPageSize::SJPS_UNKNOWN )
    {
        plotJob->m_pageSizeSelect = FromProtoEnum<JOB_PAGE_SIZE>( aCtx.Request.plot_settings().page_size() );
    }

    plotJob->m_PDFPropertyPopups = aCtx.Request.property_popups();
    plotJob->m_PDFHierarchicalLinks = aCtx.Request.hierarchical_links();
    plotJob->m_PDFMetadata = aCtx.Request.include_metadata();

    return ExecuteSchematicJob( m_context->GetKiway(), *plotJob );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_SCH::handleRunSchematicJobExportPs(
        const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportPs>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    auto plotJob = std::make_unique<JOB_EXPORT_SCH_PLOT_PS>();
    plotJob->m_filename = m_context->GetCurrentFileName();

    if( !aCtx.Request.job_settings().output_path().empty() )
        plotJob->SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    const kiapi::schematic::jobs::SchematicPlotSettings& settings = aCtx.Request.plot_settings();

    plotJob->m_drawingSheet = wxString::FromUTF8( settings.drawing_sheet() );
    plotJob->m_defaultFont = wxString::FromUTF8( settings.default_font() );
    plotJob->m_variant = wxString::FromUTF8( settings.variant() );
    plotJob->m_plotAll = settings.plot_all();
    plotJob->m_plotDrawingSheet = settings.plot_drawing_sheet();
    plotJob->m_show_hop_over = settings.show_hop_over();
    plotJob->m_blackAndWhite = settings.black_and_white();
    plotJob->m_useBackgroundColor = settings.use_background_color();
    plotJob->m_minPenWidth = settings.min_pen_width();
    plotJob->m_theme = wxString::FromUTF8( settings.theme() );

    plotJob->m_plotPages.clear();

    for( const std::string& page : settings.plot_pages() )
        plotJob->m_plotPages.push_back( wxString::FromUTF8( page ) );

    if( aCtx.Request.plot_settings().page_size() != kiapi::schematic::jobs::SchematicJobPageSize::SJPS_UNKNOWN )
    {
        plotJob->m_pageSizeSelect = FromProtoEnum<JOB_PAGE_SIZE>( aCtx.Request.plot_settings().page_size() );
    }

    return ExecuteSchematicJob( m_context->GetKiway(), *plotJob );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_SCH::handleRunSchematicJobExportNetlist(
        const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportNetlist>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    if( aCtx.Request.format() == kiapi::schematic::jobs::SchematicNetlistFormat::SNF_UNKNOWN )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "RunSchematicJobExportNetlist requires a valid format" );
        return tl::unexpected( e );
    }

    JOB_EXPORT_SCH_NETLIST netlistJob;
    netlistJob.m_filename = m_context->GetCurrentFileName();

    if( !aCtx.Request.job_settings().output_path().empty() )
        netlistJob.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    netlistJob.format = FromProtoEnum<JOB_EXPORT_SCH_NETLIST::FORMAT>( aCtx.Request.format() );

    if( !aCtx.Request.variant_name().empty() )
        netlistJob.m_variantNames.emplace_back( wxString::FromUTF8( aCtx.Request.variant_name() ) );

    return ExecuteSchematicJob( m_context->GetKiway(), netlistJob );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_SCH::handleRunSchematicJobExportBOM(
        const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportBOM>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_SCH_BOM bomJob;
    bomJob.m_filename = m_context->GetCurrentFileName();

    if( !aCtx.Request.job_settings().output_path().empty() )
        bomJob.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    bomJob.m_bomFmtPresetName = wxString::FromUTF8( aCtx.Request.format().preset_name() );
    bomJob.m_fieldDelimiter = wxString::FromUTF8( aCtx.Request.format().field_delimiter() );
    bomJob.m_stringDelimiter = wxString::FromUTF8( aCtx.Request.format().string_delimiter() );
    bomJob.m_refDelimiter = wxString::FromUTF8( aCtx.Request.format().ref_delimiter() );
    bomJob.m_refRangeDelimiter = wxString::FromUTF8( aCtx.Request.format().ref_range_delimiter() );
    bomJob.m_keepTabs = aCtx.Request.format().keep_tabs();
    bomJob.m_keepLineBreaks = aCtx.Request.format().keep_line_breaks();

    bomJob.m_bomPresetName = wxString::FromUTF8( aCtx.Request.fields().preset_name() );
    bomJob.m_sortField = wxString::FromUTF8( aCtx.Request.fields().sort_field() );
    bomJob.m_filterString = wxString::FromUTF8( aCtx.Request.fields().filter() );

    if( aCtx.Request.fields().sort_direction() == kiapi::schematic::jobs::BOMSortDirection::BSD_ASCENDING )
    {
        bomJob.m_sortAsc = true;
    }
    else if( aCtx.Request.fields().sort_direction() == kiapi::schematic::jobs::BOMSortDirection::BSD_DESCENDING )
    {
        bomJob.m_sortAsc = false;
    }

    for( const kiapi::schematic::jobs::BOMField& field : aCtx.Request.fields().fields() )
    {
        bomJob.m_fieldsOrdered.emplace_back( wxString::FromUTF8( field.name() ) );
        bomJob.m_fieldsLabels.emplace_back( wxString::FromUTF8( field.label() ) );

        if( field.group_by() )
            bomJob.m_fieldsGroupBy.emplace_back( wxString::FromUTF8( field.name() ) );
    }

    bomJob.m_excludeDNP = aCtx.Request.exclude_dnp();
    bomJob.m_groupSymbols = aCtx.Request.group_symbols();

    if( !aCtx.Request.variant_name().empty() )
        bomJob.m_variantNames.emplace_back( wxString::FromUTF8( aCtx.Request.variant_name() ) );

    return ExecuteSchematicJob( m_context->GetKiway(), bomJob );
}


void API_HANDLER_SCH::packSheetInstance( kiapi::schematic::types::SheetInstance* aInstance, SCH_SHEET_PATH& aPath,
                                          SCH_SHEET* aSheet )
{
    aPath.push_back( aSheet );

    PackSheetPath( *aInstance->mutable_path(), aPath.Path() );

    wxString sheetName = aSheet->GetShownName( false );

    if( sheetName.IsEmpty() && aSheet->GetScreen() )
    {
        wxFileName fn( aSheet->GetScreen()->GetFileName() );
        sheetName = fn.GetName();
    }

    aInstance->set_name( sheetName.ToUTF8() );
    aInstance->set_filename( aSheet->GetFileName().ToUTF8() );
    aInstance->set_page_number( aPath.GetPageNumber().ToUTF8() );

    if( aSheet->GetScreen() )
    {
        std::vector<SCH_ITEM*> childSheets;
        aSheet->GetScreen()->GetSheets( &childSheets );

        std::ranges::sort( childSheets,
                           [&]( SCH_ITEM* a, SCH_ITEM* b )
                           {
                               SCH_SHEET_PATH pathA = aPath;
                               pathA.push_back( static_cast<SCH_SHEET*>( a ) );

                               SCH_SHEET_PATH pathB = aPath;
                               pathB.push_back( static_cast<SCH_SHEET*>( b ) );

                               return pathA.ComparePageNum( pathB ) < 0;
                           } );

        for( SCH_ITEM* childItem : childSheets )
        {
            SCH_SHEET* childSheet = static_cast<SCH_SHEET*>( childItem );
            kiapi::schematic::types::SheetInstance* childInstance = aInstance->add_children();
            packSheetInstance( childInstance, aPath, childSheet );
        }
    }

    aPath.pop_back();
}


HANDLER_RESULT<kiapi::schematic::types::SchematicHierarchyResponse> API_HANDLER_SCH::handleGetSchematicHierarchy(
        const HANDLER_CONTEXT<kiapi::schematic::types::GetSchematicHierarchy>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    kiapi::schematic::types::SchematicHierarchyResponse response;
    response.mutable_document()->CopyFrom( aCtx.Request.document() );

    if( !schematic()->HasHierarchy() )
        schematic()->RefreshHierarchy();

    SCH_SHEET_PATH path;
    std::vector<SCH_SHEET*> topLevelSheets = schematic()->GetTopLevelSheets();

    std::ranges::sort( topLevelSheets,
               [&]( SCH_SHEET* a, SCH_SHEET* b )
               {
                   SCH_SHEET_PATH pathA;
                   pathA.push_back( a );

                   SCH_SHEET_PATH pathB;
                   pathB.push_back( b );

                   return pathA.ComparePageNum( pathB ) < 0;
               } );

    for( SCH_SHEET* topSheet : topLevelSheets )
    {
        kiapi::schematic::types::SheetInstance* instance = response.add_top_level_sheets();
        packSheetInstance( instance, path, topSheet );
    }

    return response;
}


HANDLER_RESULT<kiapi::schematic::types::SchematicNetlistResponse>
API_HANDLER_SCH::handleGetSchematicNetlist( const HANDLER_CONTEXT<kiapi::schematic::types::GetSchematicNetlist>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    std::vector<KICAD_T> types = parseRequestedItemTypes( aCtx.Request.types() );
    const bool filterByType = aCtx.Request.types_size() > 0;

    if( filterByType && types.empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "none of the requested types are valid for a Schematic object" );
        return tl::unexpected( e );
    }

    std::set<KICAD_T> typeFilter( types.begin(), types.end() );

    CONNECTION_GRAPH* connectionGraph = schematic()->ConnectionGraph();

    if( !connectionGraph )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "schematic has no connection graph" );
        return tl::unexpected( e );
    }

    kiapi::schematic::types::SchematicNetlistResponse response;
    response.mutable_document()->CopyFrom( aCtx.Request.document() );

    for( const auto& [key, subgraphList] : connectionGraph->GetNetMap() )
    {
        if( subgraphList.empty() )
            continue;

        CONNECTION_SUBGRAPH* firstSubgraph = subgraphList[0];

        if( firstSubgraph->GetDriverConnection() && firstSubgraph->GetDriverConnection()->IsBus() )
            continue;

        if( firstSubgraph->GetDriverPriority() < CONNECTION_SUBGRAPH::PRIORITY::PIN )
            continue;

        kiapi::schematic::types::SchematicNet* net = response.add_nets();
        net->set_name( key.Name.ToUTF8() );

        for( CONNECTION_SUBGRAPH* subGraph : subgraphList )
        {
            kiapi::schematic::types::SchematicNetSheetContents* sheetContents = net->add_sheets();
            PackSheetPath( *sheetContents->mutable_path(), subGraph->GetSheet().Path() );

            for( SCH_ITEM* item : subGraph->GetItems() )
            {
                if( filterByType && !typeFilter.contains( item->Type() ) )
                    continue;

                sheetContents->add_items()->set_value( item->m_Uuid.AsStdString() );
            }
        }
    }

    return response;
}
