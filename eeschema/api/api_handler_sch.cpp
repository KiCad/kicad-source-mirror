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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <api/api_handler_sch.h>
#include <api/api_enums.h>
#include <api/api_sch_utils.h>
#include <api/api_utils.h>
#include <api/cross_probe_client.h>
#include <api/sch_context.h>
#include <fmt.h>
#include <wx/log.h>
#include <magic_enum.hpp>
#include <base_screen.h>
#include <jobs/job_export_bom.h>
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
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>
#include <project.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>

#include <api/common/types/base_types.pb.h>
#include <trace_helpers.h>

using namespace kiapi::common::commands;
using kiapi::common::types::CommandStatus;
using kiapi::common::types::DocumentType;
using kiapi::common::types::ItemRequestStatus;


std::set<KICAD_T> API_HANDLER_SCH::s_allowedTypes = {
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_SHAPE_T,
    SCH_RULE_AREA_T,
    SCH_BITMAP_T,
    SCH_TEXTBOX_T,
    SCH_TEXT_T,
    SCH_TABLE_T,
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
    using namespace kiapi::schematic::commands;

    registerHandler<GetOpenDocuments, GetOpenDocumentsResponse>(
            &API_HANDLER_SCH::handleGetOpenDocuments );
    registerHandler<SaveDocument, google::protobuf::Empty>(
            &API_HANDLER_SCH::handleSaveDocument );
    registerHandler<SaveCopyOfDocument, google::protobuf::Empty>(
            &API_HANDLER_SCH::handleSaveCopyOfDocument );

    registerHandler<GetItems, GetItemsResponse>( &API_HANDLER_SCH::handleGetItems );
    registerHandler<GetItemsById, GetItemsResponse>( &API_HANDLER_SCH::handleGetItemsById );

    registerHandler<GetSelection, SelectionResponse>( &API_HANDLER_SCH::handleGetSelection );
    registerHandler<ClearSelection, Empty>( &API_HANDLER_SCH::handleClearSelection );
    registerHandler<AddToSelection, SelectionResponse>( &API_HANDLER_SCH::handleAddToSelection );
    registerHandler<RemoveFromSelection, SelectionResponse>(
            &API_HANDLER_SCH::handleRemoveFromSelection );

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

    registerHandler<CrossProbeAnnounce, CrossProbeAnnounceResponse>( &API_HANDLER_SCH::handleCrossProbeAnnounce );
    registerHandler<SyncSelection, SyncSelectionResponse>( &API_HANDLER_SCH::handleSyncSelection );
    registerHandler<HighlightNets, HighlightNetsResponse>( &API_HANDLER_SCH::handleHighlightNets );
}


std::unique_ptr<COMMIT> API_HANDLER_SCH::createCommit()
{
    if( m_frame )
        return std::make_unique<SCH_COMMIT>( m_frame );

    return std::make_unique<SCH_COMMIT>( toolManager() );
}


SCHEMATIC* API_HANDLER_SCH::schematic() const
{
    wxCHECK( m_context, nullptr );
    return m_context->GetSchematic();
}


std::optional<ApiResponseStatus> API_HANDLER_SCH::checkForHeadless( const std::string& aCommandName ) const
{
    if( m_frame )
        return std::nullopt;

    ApiResponseStatus e;
    e.set_status( ApiStatusCode::AS_UNIMPLEMENTED );
    e.set_error_message( fmt::format( "{} is not available in headless mode", aCommandName ) );
    return e;
}


bool API_HANDLER_SCH::packSchItem( google::protobuf::Any& aOut, SCH_ITEM* aItem,
                                   const SCH_SHEET_PATH& aPath )
{
    if( aItem->Type() == SCH_SYMBOL_T )
    {
        kiapi::schematic::types::SchematicSymbolInstance symbol;

        if( !PackSymbol( &symbol, static_cast<SCH_SYMBOL*>( aItem ), aPath ) )
            return false;

        aOut.PackFrom( symbol );
    }
    else if( aItem->Type() == SCH_SHEET_T )
    {
        kiapi::schematic::types::SheetSymbol sheet;

        if( !PackSheet( &sheet, static_cast<SCH_SHEET*>( aItem ), aPath ) )
            return false;

        aOut.PackFrom( sheet );
    }
    else
    {
        aItem->Serialize( aOut );
    }

    return true;
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


HANDLER_RESULT<google::protobuf::Empty> API_HANDLER_SCH::handleSaveDocument( const HANDLER_CONTEXT<SaveDocument>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    if( !context()->SaveSchematic() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "failed to save schematic" );
        return tl::unexpected( e );
    }

    return google::protobuf::Empty();
}


HANDLER_RESULT<google::protobuf::Empty>
API_HANDLER_SCH::handleSaveCopyOfDocument( const HANDLER_CONTEXT<SaveCopyOfDocument>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    wxFileName schematicPath( project().AbsolutePath( wxString::FromUTF8( aCtx.Request.path() ) ) );

    if( !schematicPath.IsOk() || !schematicPath.IsDirWritable() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message(
                fmt::format( "save path '{}' could not be opened", schematicPath.GetFullPath().ToStdString() ) );
        return tl::unexpected( e );
    }

    if( schematicPath.FileExists() && ( !schematicPath.IsFileWritable() || !aCtx.Request.options().overwrite() ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "save path '{}' exists and cannot be overwritten",
                                          schematicPath.GetFullPath().ToStdString() ) );
        return tl::unexpected( e );
    }

    if( schematicPath.GetExt() != FILEEXT::KiCadSchematicFileExtension )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "save path '{}' must have a kicad_sch extension",
                                          schematicPath.GetFullPath().ToStdString() ) );
        return tl::unexpected( e );
    }

    bool includeProject = true;

    if( aCtx.Request.has_options() )
        includeProject = aCtx.Request.options().include_project();

    if( !context()->SaveSchematicCopy( schematicPath.GetFullPath(), includeProject ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "failed to save schematic copy" );
        return tl::unexpected( e );
    }

    return google::protobuf::Empty();
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
            if( packSchItem( any, static_cast<SCH_ITEM*>( item ), itemPath ) )
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


HANDLER_RESULT<SelectionResponse>
API_HANDLER_SCH::handleGetSelection( const HANDLER_CONTEXT<GetSelection>& aCtx )
{
    if( std::optional<ApiResponseStatus> headless = checkForHeadless( "GetSelection" ) )
        return tl::unexpected( *headless );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    std::set<KICAD_T> filter;

    for( KICAD_T type : parseRequestedItemTypes( aCtx.Request.types() ) )
        filter.insert( type );

    SCH_SELECTION_TOOL* tool = m_context->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_SHEET_PATH path = m_context->GetCurrentSheet().value_or( SCH_SHEET_PATH() );

    SelectionResponse response;
    google::protobuf::Any any;

    for( EDA_ITEM* item : tool->GetSelection() )
    {
        if( filter.empty() || filter.contains( item->Type() ) )
        {
            if( packSchItem( any, static_cast<SCH_ITEM*>( item ), path ) )
                response.mutable_items()->Add( std::move( any ) );
        }
    }

    return response;
}


HANDLER_RESULT<Empty>
API_HANDLER_SCH::handleClearSelection( const HANDLER_CONTEXT<ClearSelection>& aCtx )
{
    if( std::optional<ApiResponseStatus> headless = checkForHeadless( "ClearSelection" ) )
        return tl::unexpected( *headless );

    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    m_context->GetToolManager()->RunAction( ACTIONS::selectionClear );
    m_frame->Refresh();

    return Empty();
}


HANDLER_RESULT<SelectionResponse>
API_HANDLER_SCH::handleAddToSelection( const HANDLER_CONTEXT<AddToSelection>& aCtx )
{
    if( std::optional<ApiResponseStatus> headless = checkForHeadless( "AddToSelection" ) )
        return tl::unexpected( *headless );

    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    SCH_SELECTION_TOOL* tool = m_context->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_SHEET_PATH current = m_context->GetCurrentSheet().value_or( SCH_SHEET_PATH() );

    EDA_ITEMS toAdd;

    for( const types::KIID& id : aCtx.Request.items() )
    {
        SCH_SHEET_PATH itemPath;

        // Selection only operates on the currently-displayed sheet; off-sheet items are skipped
        if( std::optional<SCH_ITEM*> item = getItemById( KIID( id.value() ), &itemPath );
            item && itemPath == current )
        {
            toAdd.push_back( *item );
        }
    }

    tool->AddItemsToSel( &toAdd );
    m_frame->Refresh();

    SelectionResponse response;
    google::protobuf::Any any;

    for( EDA_ITEM* item : tool->GetSelection() )
    {
        if( packSchItem( any, static_cast<SCH_ITEM*>( item ), current ) )
            response.mutable_items()->Add( std::move( any ) );
    }

    return response;
}


HANDLER_RESULT<SelectionResponse>
API_HANDLER_SCH::handleRemoveFromSelection( const HANDLER_CONTEXT<RemoveFromSelection>& aCtx )
{
    if( std::optional<ApiResponseStatus> headless = checkForHeadless( "RemoveFromSelection" ) )
        return tl::unexpected( *headless );

    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    SCH_SELECTION_TOOL* tool = m_context->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_SHEET_PATH current = m_context->GetCurrentSheet().value_or( SCH_SHEET_PATH() );

    EDA_ITEMS toRemove;

    for( const types::KIID& id : aCtx.Request.items() )
    {
        SCH_SHEET_PATH itemPath;

        if( std::optional<SCH_ITEM*> item = getItemById( KIID( id.value() ), &itemPath );
            item && itemPath == current )
        {
            toRemove.push_back( *item );
        }
    }

    tool->RemoveItemsFromSel( &toRemove );
    m_frame->Refresh();

    SelectionResponse response;
    google::protobuf::Any any;

    for( EDA_ITEM* item : tool->GetSelection() )
    {
        if( packSchItem( any, static_cast<SCH_ITEM*>( item ), current ) )
            response.mutable_items()->Add( std::move( any ) );
    }

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
    bool connectivityChanged = false;   // an in-place symbol update invalidated the net graph

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

            if( existingItem->IsConnectable() )
            {
                existingItem->SetConnectivityDirty();
                connectivityChanged = true;
            }

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

    if( m_frame && connectivityChanged )
        m_frame->RecalculateConnections( nullptr, LOCAL_CLEANUP );

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

    JOB_EXPORT_BOM bomJob;
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
    bomJob.m_includeByteOrderMark = aCtx.Request.format().include_byte_order_mark();

    bomJob.m_bomPresetName = wxString::FromUTF8( aCtx.Request.fields().preset_name() );
    bomJob.m_sortField = wxString::FromUTF8( aCtx.Request.fields().sort_field() );
    bomJob.m_filterString = wxString::FromUTF8( aCtx.Request.fields().filter() );

    switch( aCtx.Request.fields().filter_scope() )
    {
    case kiapi::schematic::jobs::BOMFilterScope::BFS_VISIBLE:
        bomJob.m_filterScope = BOM_FILTER_SCOPE::VISIBLE;
        break;

    case kiapi::schematic::jobs::BOMFilterScope::BFS_ALL:
        bomJob.m_filterScope = BOM_FILTER_SCOPE::ALL;
        break;

    case kiapi::schematic::jobs::BOMFilterScope::BFS_REFERENCE:
    default:
        bomJob.m_filterScope = BOM_FILTER_SCOPE::REFERENCE;
        break;
    }

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


HANDLER_RESULT<kiapi::schematic::commands::SchematicHierarchyResponse> API_HANDLER_SCH::handleGetSchematicHierarchy(
        const HANDLER_CONTEXT<kiapi::schematic::commands::GetSchematicHierarchy>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    kiapi::schematic::commands::SchematicHierarchyResponse response;
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


HANDLER_RESULT<kiapi::schematic::commands::SchematicNetlistResponse>
API_HANDLER_SCH::handleGetSchematicNetlist( const HANDLER_CONTEXT<kiapi::schematic::commands::GetSchematicNetlist>& aCtx )
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

    kiapi::schematic::commands::SchematicNetlistResponse response;
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


// TODO(JE) factor out
HANDLER_RESULT<CrossProbeAnnounceResponse> API_HANDLER_SCH::handleCrossProbeAnnounce(
        const HANDLER_CONTEXT<CrossProbeAnnounce>& aCtx )
{
    wxLogTrace( traceApi, "Received announce from frame %d at %s",
                aCtx.Request.frame_type(), aCtx.Request.socket_path() );

    CROSS_PROBE_CLIENT::RegisterPeer( static_cast<FRAME_T>( aCtx.Request.frame_type() ),
                                      aCtx.Request.socket_path() );

    CrossProbeAnnounceResponse response;
    response.set_status( CPS_OK );
    return response;
}


bool findSymbolsAndPins( const SCH_SHEET_LIST& aSchematicSheetList, const SCH_SHEET_PATH& aSheetPath,
                         std::unordered_map<wxString, std::vector<SCH_REFERENCE>>&             aSyncSymMap,
                         std::unordered_map<wxString, std::unordered_map<wxString, SCH_PIN*>>& aSyncPinMap,
                         const wxString& aVariantName = wxEmptyString, bool aRecursive = false )
{
    if( aRecursive )
    {
        // Iterate over children
        for( const SCH_SHEET_PATH& candidate : aSchematicSheetList )
        {
            if( candidate == aSheetPath || !candidate.IsContainedWithin( aSheetPath ) )
                continue;

            findSymbolsAndPins( aSchematicSheetList, candidate, aSyncSymMap, aSyncPinMap, aVariantName, aRecursive );
        }
    }

    SCH_REFERENCE_LIST references;

    aSheetPath.GetSymbols( references, SYMBOL_FILTER_NON_POWER, true );

    for( unsigned ii = 0; ii < references.GetCount(); ii++ )
    {
        SCH_REFERENCE& schRef = references[ii];

        if( schRef.IsSplitNeeded() )
            schRef.Split();

        SCH_SYMBOL* symbol = schRef.GetSymbol();
        wxString    refNum = schRef.GetRefNumber();
        wxString    fullRef = schRef.GetRef() + refNum;

        // Skip power symbols
        if( fullRef.StartsWith( wxS( "#" ) ) )
            continue;

        // Unannotated symbols are not supported
        if( refNum.compare( wxS( "?" ) ) == 0 )
            continue;

        // Look for whole footprint
        auto symMatchIt = aSyncSymMap.find( fullRef );

        if( symMatchIt != aSyncSymMap.end() )
        {
            symMatchIt->second.emplace_back( schRef );

            // Whole footprint was selected, no need to select pins
            continue;
        }

        // Look for pins
        auto symPinMatchIt = aSyncPinMap.find( fullRef );

        if( symPinMatchIt != aSyncPinMap.end() )
        {
            std::unordered_map<wxString, SCH_PIN*>& pinMap = symPinMatchIt->second;
            std::vector<SCH_PIN*>                   pinsOnSheet = symbol->GetPins( &aSheetPath );

            for( SCH_PIN* pin : pinsOnSheet )
            {
                int pinUnit = pin->GetLibPin()->GetUnit();

                if( pinUnit > 0 && pinUnit != schRef.GetUnit() )
                    continue;

                // Reverse-map the requested pad back to the owning pin (issue #2282).  A pin may
                // resolve to several pads via the map; match the first that pcbnew asked for.
                for( const wxString& pad :
                     ExpandStackedPinNotation( pin->GetEffectivePadNumber( aSheetPath, aVariantName ) ) )
                {
                    auto pinIt = pinMap.find( pad );

                    if( pinIt != pinMap.end() )
                    {
                        pinIt->second = pin;
                        break;
                    }
                }
            }
        }
    }

    return false;
}


bool sheetContainsOnlyWantedItems(
        const SCH_SHEET_LIST& aSchematicSheetList, const SCH_SHEET_PATH& aSheetPath,
        std::unordered_map<wxString, std::vector<SCH_REFERENCE>>&             aSyncSymMap,
        std::unordered_map<wxString, std::unordered_map<wxString, SCH_PIN*>>& aSyncPinMap,
        std::unordered_map<SCH_SHEET_PATH, bool>&                             aCache )
{
    auto cacheIt = aCache.find( aSheetPath );

    if( cacheIt != aCache.end() )
        return cacheIt->second;

    // Iterate over children
    for( const SCH_SHEET_PATH& candidate : aSchematicSheetList )
    {
        if( candidate == aSheetPath || !candidate.IsContainedWithin( aSheetPath ) )
            continue;

        bool childRet = sheetContainsOnlyWantedItems( aSchematicSheetList, candidate, aSyncSymMap,
                                                      aSyncPinMap, aCache );

        if( !childRet )
        {
            aCache.emplace( aSheetPath, false );
            return false;
        }
    }

    SCH_REFERENCE_LIST references;
    aSheetPath.GetSymbols( references, SYMBOL_FILTER_NON_POWER, true );

    if( references.GetCount() == 0 )    // Empty sheet, obviously do not contain wanted items
    {
        aCache.emplace( aSheetPath, false );
        return false;
    }

    for( unsigned ii = 0; ii < references.GetCount(); ii++ )
    {
        SCH_REFERENCE& schRef = references[ii];

        if( schRef.IsSplitNeeded() )
            schRef.Split();

        wxString refNum = schRef.GetRefNumber();
        wxString fullRef = schRef.GetRef() + refNum;

        // Skip power symbols
        if( fullRef.StartsWith( wxS( "#" ) ) )
            continue;

        // Unannotated symbols are not supported
        if( refNum.compare( wxS( "?" ) ) == 0 )
            continue;

        if( aSyncSymMap.find( fullRef ) == aSyncSymMap.end() )
        {
            aCache.emplace( aSheetPath, false );
            return false; // Some symbol is not wanted.
        }

        if( aSyncPinMap.find( fullRef ) != aSyncPinMap.end() )
        {
            aCache.emplace( aSheetPath, false );
            return false; // Looking for specific pins, so can't be mapped
        }
    }

    aCache.emplace( aSheetPath, true );
    return true;
}


std::optional<std::tuple<SCH_SHEET_PATH, SCH_ITEM*, std::vector<SCH_ITEM*>>>
findItemsFromSyncSelection( const SCHEMATIC& aSchematic,
                            const kiapi::common::commands::SyncSelection& aSync )
{
    std::unordered_map<wxString, std::vector<SCH_REFERENCE>>             syncSymMap;
    std::unordered_map<wxString, std::unordered_map<wxString, SCH_PIN*>> syncPinMap;
    std::unordered_map<SCH_SHEET_PATH, bool>                             fullyWantedCache;

    std::optional<wxString>                                    focusSymbol;
    std::optional<std::pair<wxString, wxString>>               focusPin;
    std::unordered_map<SCH_SHEET_PATH, std::vector<SCH_ITEM*>> focusItemResults;

    const SCH_SHEET_LIST allSheetsList = aSchematic.Hierarchy();

    // In orderedSheets, the current sheet comes first.
    std::vector<SCH_SHEET_PATH> orderedSheets;
    orderedSheets.reserve( allSheetsList.size() );
    orderedSheets.push_back( aSchematic.CurrentSheet() );

    for( const SCH_SHEET_PATH& sheetPath : allSheetsList )
    {
        if( sheetPath != aSchematic.CurrentSheet() )
            orderedSheets.push_back( sheetPath );
    }

    const bool focusOnFirst = ( aSync.mode() == kiapi::common::commands::SSM_ITEMS_AND_NETS ) && aSync.has_focus_item();

    for( const kiapi::common::commands::SelectionSpec& spec : aSync.items() )
    {
        switch( spec.spec_case() )
        {
        case kiapi::common::commands::SelectionSpec::kFootprint:
        {
            wxString symRef = wxString::FromUTF8( spec.footprint().reference() );
            syncSymMap[symRef] = std::vector<SCH_REFERENCE>();
            break;
        }

        case kiapi::common::commands::SelectionSpec::kPad:
        {
            wxString symRef = wxString::FromUTF8( spec.pad().reference() );
            wxString padNum = wxString::FromUTF8( spec.pad().number() );
            syncPinMap[symRef][padNum] = nullptr;
            break;
        }

        default:
            break;
        }
    }

    if( focusOnFirst )
    {
        const kiapi::common::commands::SelectionSpec& focusSpec = aSync.focus_item();

        if( focusSpec.has_footprint() )
            focusSymbol = wxString::FromUTF8( focusSpec.footprint().reference() );
        else if( focusSpec.has_pad() )
            focusPin = std::make_pair( wxString::FromUTF8( focusSpec.pad().reference() ),
                                       wxString::FromUTF8( focusSpec.pad().number() ) );
    }

    // Lambda definitions
    auto flattenSyncMaps =
            [&syncSymMap, &syncPinMap]() -> std::vector<SCH_ITEM*>
            {
                std::vector<SCH_ITEM*> allVec;

                for( const auto& [symRef, symbols] : syncSymMap )
                {
                    for( const SCH_REFERENCE& ref : symbols )
                        allVec.push_back( ref.GetSymbol() );
                }

                for( const auto& [symRef, pinMap] : syncPinMap )
                {
                    for( const auto& [padNum, pin] : pinMap )
                    {
                        if( pin )
                            allVec.push_back( pin );
                    }
                }

                return allVec;
            };

    auto clearSyncMaps =
            [&syncSymMap, &syncPinMap]()
            {
                for( auto& [symRef, symbols] : syncSymMap )
                    symbols.clear();

                for( auto& [reference, pins] : syncPinMap )
                {
                    for( auto& [number, pin] : pins )
                        pin = nullptr;
                }
            };

    auto syncMapsValuesEmpty =
            [&syncSymMap, &syncPinMap]() -> bool
            {
                for( const auto& [symRef, symbols] : syncSymMap )
                {
                    if( symbols.size() > 0 )
                        return false;
                }

                for( const auto& [symRef, pins] : syncPinMap )
                {
                    for( const auto& [padNum, pin] : pins )
                    {
                        if( pin )
                            return false;
                    }
                }

                return true;
            };

    auto checkFocusItems =
            [&]( const SCH_SHEET_PATH& aSheet )
            {
                if( focusSymbol )
                {
                    auto findIt = syncSymMap.find( *focusSymbol );

                    if( findIt != syncSymMap.end() )
                    {
                        if( findIt->second.size() > 0 )
                            focusItemResults[aSheet].push_back( findIt->second.front().GetSymbol() );
                    }
                }
                else if( focusPin )
                {
                    auto findIt = syncPinMap.find( focusPin->first );

                    if( findIt != syncPinMap.end() )
                    {
                        if( findIt->second[focusPin->second] )
                            focusItemResults[aSheet].push_back( findIt->second[focusPin->second] );
                    }
                }
            };

    auto makeRetForSheet =
            [&]( const SCH_SHEET_PATH& aSheet, SCH_ITEM* aFocusItem )
            {
                clearSyncMaps();

                // Fill sync maps
                findSymbolsAndPins( allSheetsList, aSheet, syncSymMap, syncPinMap, aSchematic.GetCurrentVariant() );
                std::vector<SCH_ITEM*> itemsVector = flattenSyncMaps();

                // Add fully wanted sheets to vector
                for( SCH_ITEM* item : aSheet.LastScreen()->Items().OfType( SCH_SHEET_T ) )
                {
                    KIID_PATH kiidPath = aSheet.Path();
                    kiidPath.push_back( item->m_Uuid );

                    std::optional<SCH_SHEET_PATH> subsheetPath =
                            allSheetsList.GetSheetPathByKIIDPath( kiidPath );

                    if( !subsheetPath )
                        continue;

                    if( sheetContainsOnlyWantedItems( allSheetsList, *subsheetPath, syncSymMap,
                                                      syncPinMap, fullyWantedCache ) )
                    {
                        itemsVector.push_back( item );
                    }
                }

                return std::make_tuple( aSheet, aFocusItem, itemsVector );
            };

    if( focusOnFirst )
    {
        for( const SCH_SHEET_PATH& sheetPath : orderedSheets )
        {
            clearSyncMaps();

            findSymbolsAndPins( allSheetsList, sheetPath, syncSymMap, syncPinMap, aSchematic.GetCurrentVariant() );

            checkFocusItems( sheetPath );
        }

        if( focusItemResults.size() > 0 )
        {
            for( const SCH_SHEET_PATH& sheetPath : orderedSheets )
            {
                const std::vector<SCH_ITEM*>& items = focusItemResults[sheetPath];

                if( !items.empty() )
                    return makeRetForSheet( sheetPath, items.front() );
            }
        }
    }
    else
    {
        for( const SCH_SHEET_PATH& sheetPath : orderedSheets )
        {
            clearSyncMaps();

            findSymbolsAndPins( allSheetsList, sheetPath, syncSymMap, syncPinMap, aSchematic.GetCurrentVariant() );

            if( !syncMapsValuesEmpty() )
            {
                // Something found on sheet
                return makeRetForSheet( sheetPath, nullptr );
            }
        }
    }

    return std::nullopt;
}


HANDLER_RESULT<SyncSelectionResponse> API_HANDLER_SCH::handleSyncSelection(
        const HANDLER_CONTEXT<SyncSelection>& aCtx )
{
    if( std::optional<ApiResponseStatus> headless = checkForHeadless( "SyncSelection" ) )
        return tl::unexpected( *headless );

    SyncSelectionResponse response;

    const CROSS_PROBING_SETTINGS& settings = m_frame->eeconfig()->m_CrossProbing;

    if( !settings.on_selection && aCtx.Request.context() != SyncSelectionContext::SSC_EXPLICIT )
    {
        response.set_status( CPS_DISABLED );
        response.set_message( "implicit selection sync disabled by user" );
        return response;
    }

    // A request carrying no items asks for nothing to be selected, so there is nothing to find.
    if( aCtx.Request.items_size() == 0 )
    {
        m_frame->SetSyncingSelection( true ); // recursion guard

        m_frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>()->SyncSelection( std::nullopt, nullptr, {} );

        m_frame->SetSyncingSelection( false );

        response.set_status( CPS_OK );
        return response;
    }

    std::optional<std::tuple<SCH_SHEET_PATH, SCH_ITEM*, std::vector<SCH_ITEM*>>> findRet =
                    findItemsFromSyncSelection( *schematic(), aCtx.Request );

    if( findRet )
    {
        auto& [sheetPath, focusItem, items] = *findRet;

        m_frame->SetSyncingSelection( true ); // recursion guard

        m_frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>()->SyncSelection( sheetPath, focusItem, items );

        m_frame->SetSyncingSelection( false );

        if( m_frame->eeconfig()->m_CrossProbing.flash_selection )
        {
            wxLogTrace( traceCrossProbeFlash, "MAIL_SELECTION(_FORCE): flash enabled, items=%zu",
                        items.size() );

            if( items.empty() )
            {
                wxLogTrace( traceCrossProbeFlash, "MAIL_SELECTION(_FORCE): nothing to flash" );
            }
            else
            {
                std::vector<SCH_ITEM*> itemPtrs;
                std::copy( items.begin(), items.end(), std::back_inserter( itemPtrs ) );

                m_frame->StartCrossProbeFlash( itemPtrs );
            }
        }
        else
        {
            wxLogTrace( traceCrossProbeFlash, "MAIL_SELECTION(_FORCE): flash disabled" );
        }
    }

    response.set_status( CPS_OK );
    return response;
}


HANDLER_RESULT<HighlightNetsResponse> API_HANDLER_SCH::handleHighlightNets(
        const HANDLER_CONTEXT<HighlightNets>& aCtx )
{
    if( std::optional<ApiResponseStatus> headless = checkForHeadless( "HighlightNets" ) )
        return tl::unexpected( *headless );

    HighlightNetsResponse response;
    CROSS_PROBING_SETTINGS& crossProbingSettings = m_frame->eeconfig()->m_CrossProbing;

    if( aCtx.ClientName == StandaloneCrossProbeClientName
        || aCtx.ClientName == KiwayClientName )
    {
        if( !crossProbingSettings.auto_highlight )
        {
            response.set_status( CPS_DISABLED );
            return response;
        }
    }

    wxString net;

    if( aCtx.Request.net_name_size() > 0 )
        net = wxString::FromUTF8( aCtx.Request.net_name( 0 ) );

    m_frame->HandleRemoteNetHighlight( net );

    response.set_status( CPS_OK );
    return response;
}
