/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
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

#include <magic_enum.hpp>
#include <memory>
#include <properties/property.h>

#include <common.h>
#include <api/api_handler_pcb.h>
#include <api/api_pcb_utils.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <base_screen.h>
#include <board_commit.h>
#include <board_connected_item.h>
#include <board_design_settings.h>
#include <core/kicad_algo.h>
#include <footprint.h>
#include <kicad_clipboard.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_edit_frame.h>
#include <pcb_group.h>
#include <pcb_reference_image.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <pcbnew_id.h>
#include <pcb_marker.h>
#include <kiway.h>
#include <drc/drc_item.h>
#include <jobs/job_export_pcb_3d.h>
#include <jobs/job_export_pcb_dxf.h>
#include <jobs/job_export_pcb_drill.h>
#include <jobs/job_export_pcb_gencad.h>
#include <jobs/job_export_pcb_gerber.h>
#include <jobs/job_export_pcb_gerbers.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <jobs/job_export_pcb_ipcd356.h>
#include <jobs/job_export_pcb_odb.h>
#include <jobs/job_export_pcb_pdf.h>
#include <jobs/job_export_pcb_pos.h>
#include <jobs/job_export_pcb_ps.h>
#include <jobs/job_export_pcb_stats.h>
#include <jobs/job_export_pcb_svg.h>
#include <jobs/job_pcb_render.h>
#include <layer_ids.h>
#include <netlist_reader/board_netlist_updater.h>
#include <netlist_reader/pcb_netlist.h>
#include <project.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/zone_filler_tool.h>
#include <zone.h>
#include <zone_filler.h>

#include <api/common/types/base_types.pb.h>
#include <connectivity/connectivity_data.h>
#include <drc/drc_rule_condition.h>
#include <drc/drc_rule_parser.h>
#include <widgets/appearance_controls.h>
#include <widgets/report_severity.h>
#include <drc/rule_editor/drc_re_rule_loader.h>
#include <wx/ffile.h>

using namespace kiapi::common::commands;
using types::CommandStatus;
using types::DocumentType;
using types::ItemRequestStatus;


API_HANDLER_PCB::API_HANDLER_PCB( PCB_EDIT_FRAME* aFrame ) :
        API_HANDLER_PCB( CreatePcbFrameContext( aFrame ), aFrame )
{
}


API_HANDLER_PCB::API_HANDLER_PCB( std::shared_ptr<PCB_CONTEXT> aContext, PCB_EDIT_FRAME* aFrame ) :
        API_HANDLER_BOARD( std::move( aContext ), aFrame )
{
    registerHandler<GetOpenDocuments, GetOpenDocumentsResponse>(
            &API_HANDLER_PCB::handleGetOpenDocuments );
    registerHandler<SaveDocument, Empty>( &API_HANDLER_PCB::handleSaveDocument );
    registerHandler<SaveCopyOfDocument, Empty>( &API_HANDLER_PCB::handleSaveCopyOfDocument );
    registerHandler<RevertDocument, Empty>( &API_HANDLER_PCB::handleRevertDocument );

    registerHandler<GetItems, GetItemsResponse>( &API_HANDLER_PCB::handleGetItems );

    registerHandler<SetBoardEnabledLayers, BoardEnabledLayersResponse>( &API_HANDLER_PCB::handleSetBoardEnabledLayers );
    registerHandler<GetBoardDesignRules, BoardDesignRulesResponse>( &API_HANDLER_PCB::handleGetBoardDesignRules );
    registerHandler<SetBoardDesignRules, BoardDesignRulesResponse>( &API_HANDLER_PCB::handleSetBoardDesignRules );
    registerHandler<GetCustomDesignRules, CustomRulesResponse>( &API_HANDLER_PCB::handleGetCustomDesignRules );
    registerHandler<SetCustomDesignRules, CustomRulesResponse>( &API_HANDLER_PCB::handleSetCustomDesignRules );
    registerHandler<GetBoardOrigin, types::Vector2>( &API_HANDLER_PCB::handleGetBoardOrigin );
    registerHandler<SetBoardOrigin, Empty>( &API_HANDLER_PCB::handleSetBoardOrigin );
    registerHandler<GetBoardLayerName, BoardLayerNameResponse>( &API_HANDLER_PCB::handleGetBoardLayerName );

    registerHandler<GetNets, NetsResponse>( &API_HANDLER_PCB::handleGetNets );
    registerHandler<GetConnectedItems, GetItemsResponse>( &API_HANDLER_PCB::handleGetConnectedItems );
    registerHandler<GetItemsByNet, GetItemsResponse>( &API_HANDLER_PCB::handleGetItemsByNet );
    registerHandler<GetItemsByNetClass, GetItemsResponse>( &API_HANDLER_PCB::handleGetItemsByNetClass );
    registerHandler<GetNetClassForNets, NetClassForNetsResponse>(
            &API_HANDLER_PCB::handleGetNetClassForNets );
    registerHandler<RefillZones, Empty>( &API_HANDLER_PCB::handleRefillZones );
    registerHandler<ImportNetlist, ImportNetlistResponse>( &API_HANDLER_PCB::handleImportNetlist );

    registerHandler<GetBoardEditorAppearanceSettings, BoardEditorAppearanceSettings>(
            &API_HANDLER_PCB::handleGetBoardEditorAppearanceSettings );
    registerHandler<SetBoardEditorAppearanceSettings, Empty>(
            &API_HANDLER_PCB::handleSetBoardEditorAppearanceSettings );
    registerHandler<InjectDrcError, InjectDrcErrorResponse>(
            &API_HANDLER_PCB::handleInjectDrcError );

    registerHandler<RunBoardJobExport3D, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExport3D );
    registerHandler<RunBoardJobExportRender, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportRender );
    registerHandler<RunBoardJobExportSvg, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportSvg );
    registerHandler<RunBoardJobExportDxf, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportDxf );
    registerHandler<RunBoardJobExportPdf, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportPdf );
    registerHandler<RunBoardJobExportPs, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportPs );
    registerHandler<RunBoardJobExportGerbers, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportGerbers );
    registerHandler<RunBoardJobExportDrill, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportDrill );
    registerHandler<RunBoardJobExportPosition, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportPosition );
    registerHandler<RunBoardJobExportGencad, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportGencad );
    registerHandler<RunBoardJobExportIpc2581, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportIpc2581 );
    registerHandler<RunBoardJobExportIpcD356, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportIpcD356 );
    registerHandler<RunBoardJobExportODB, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportODB );
    registerHandler<RunBoardJobExportStats, types::RunJobResponse>(
            &API_HANDLER_PCB::handleRunBoardJobExportStats );

    registerHandler<GetPageSettings, types::PageSettings>( &API_HANDLER_PCB::handleGetPageSettings );
    registerHandler<SetPageSettings, types::PageSettings>( &API_HANDLER_PCB::handleSetPageSettings );
}


PCB_EDIT_FRAME* API_HANDLER_PCB::frame() const
{
    return static_cast<PCB_EDIT_FRAME*>( m_frame );
}


HANDLER_RESULT<GetOpenDocumentsResponse> API_HANDLER_PCB::handleGetOpenDocuments(
        const HANDLER_CONTEXT<GetOpenDocuments>& aCtx )
{
    if( aCtx.Request.type() != DocumentType::DOCTYPE_PCB )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    GetOpenDocumentsResponse response;
    common::types::DocumentSpecifier doc;

    wxFileName fn( pcbContext()->GetCurrentFileName() );

    doc.set_type( DocumentType::DOCTYPE_PCB );
    doc.set_board_filename( fn.GetFullName() );

    doc.mutable_project()->set_name( project().GetProjectName().ToStdString() );
    doc.mutable_project()->set_path( project().GetProjectDirectory().ToStdString() );

    response.mutable_documents()->Add( std::move( doc ) );
    return response;
}


HANDLER_RESULT<Empty> API_HANDLER_PCB::handleSaveDocument(
        const HANDLER_CONTEXT<SaveDocument>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    pcbContext()->SaveBoard();
    return Empty();
}


HANDLER_RESULT<Empty> API_HANDLER_PCB::handleSaveCopyOfDocument(
        const HANDLER_CONTEXT<SaveCopyOfDocument>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    wxFileName boardPath( project().AbsolutePath( wxString::FromUTF8( aCtx.Request.path() ) ) );

    if( !boardPath.IsOk() || !boardPath.IsDirWritable() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "save path '{}' could not be opened",
                                          boardPath.GetFullPath().ToStdString() ) );
        return tl::unexpected( e );
    }

    if( boardPath.FileExists()
        && ( !boardPath.IsFileWritable() || !aCtx.Request.options().overwrite() ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "save path '{}' exists and cannot be overwritten",
                                          boardPath.GetFullPath().ToStdString() ) );
        return tl::unexpected( e );
    }

    if( boardPath.GetExt() != FILEEXT::KiCadPcbFileExtension )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "save path '{}' must have a kicad_pcb extension",
                                          boardPath.GetFullPath().ToStdString() ) );
        return tl::unexpected( e );
    }

    BOARD* board = this->board();

    if( board->GetFileName().Matches( boardPath.GetFullPath() ) )
    {
        pcbContext()->SaveBoard();
        return Empty();
    }

    bool includeProject = true;

    if( aCtx.Request.has_options() )
        includeProject = aCtx.Request.options().include_project();

    pcbContext()->SavePcbCopy( boardPath.GetFullPath(), includeProject, /* aHeadless = */ true );

    return Empty();
}


HANDLER_RESULT<Empty> API_HANDLER_PCB::handleRevertDocument(
        const HANDLER_CONTEXT<RevertDocument>& aCtx )
{
    if( std::optional<ApiResponseStatus> headless = checkForHeadless( "RevertDocument" ) )
        return tl::unexpected( *headless );

    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    wxFileName fn = project().AbsolutePath( board()->GetFileName() );

    frame()->GetScreen()->SetContentModified( false );
    frame()->ReleaseFile();
    frame()->OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ), KICTL_REVERT );

    return Empty();
}


tl::expected<bool, ApiResponseStatus> API_HANDLER_PCB::validateDocumentInternal( const DocumentSpecifier& aDocument ) const
{
    if( aDocument.type() != DocumentType::DOCTYPE_PCB )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "the requested document is not a board" );
        return tl::unexpected( e );
    }

    wxFileName fn( pcbContext()->GetCurrentFileName() );

    if( aDocument.board_filename().compare( fn.GetFullName() ) != 0 )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "the requested document {} is not open",
                                          aDocument.board_filename() ) );
        return tl::unexpected( e );
    }

    return true;
}


HANDLER_RESULT<GetItemsResponse> API_HANDLER_PCB::handleGetItems( const HANDLER_CONTEXT<GetItems>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    GetItemsResponse response;

    BOARD* board = this->board();
    std::vector<BOARD_ITEM*> items;
    std::set<KICAD_T> typesRequested, typesInserted;
    bool handledAnything = false;

    for( KICAD_T type : parseRequestedItemTypes( aCtx.Request.types() ) )
    {
        typesRequested.emplace( type );

        if( typesInserted.count( type ) )
            continue;

        switch( type )
        {
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
            handledAnything = true;
            std::copy( board->Tracks().begin(), board->Tracks().end(),
                       std::back_inserter( items ) );
            typesInserted.insert( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } );
            break;

        case PCB_PAD_T:
        {
            handledAnything = true;

            for( FOOTPRINT* fp : board->Footprints() )
            {
                std::copy( fp->Pads().begin(), fp->Pads().end(),
                           std::back_inserter( items ) );
            }

            typesInserted.insert( PCB_PAD_T );
            break;
        }

        case PCB_FOOTPRINT_T:
        {
            handledAnything = true;

            std::copy( board->Footprints().begin(), board->Footprints().end(),
                       std::back_inserter( items ) );

            typesInserted.insert( PCB_FOOTPRINT_T );
            break;
        }

        case PCB_SHAPE_T:
        case PCB_TEXT_T:
        case PCB_TEXTBOX_T:
        case PCB_BARCODE_T:
        case PCB_REFERENCE_IMAGE_T:
        {
            handledAnything = true;
            bool inserted = false;

            for( BOARD_ITEM* item : board->Drawings() )
            {
                if( item->Type() == type )
                {
                    items.emplace_back( item );
                    inserted = true;
                }
            }

            if( inserted )
                typesInserted.insert( type );

            break;
        }

        case PCB_DIMENSION_T:
        {
            handledAnything = true;
            bool inserted = false;

            for( BOARD_ITEM* item : board->Drawings() )
            {
                switch (item->Type()) {
                    case PCB_DIM_ALIGNED_T:
                    case PCB_DIM_CENTER_T:
                    case PCB_DIM_RADIAL_T:
                    case PCB_DIM_ORTHOGONAL_T:
                    case PCB_DIM_LEADER_T:
                        items.emplace_back( item );
                        inserted = true;
                        break;
                    default:
                        break;
                }
            }
            // we have to add the dimension subtypes to the requested to get them out
            typesRequested.insert( {PCB_DIM_ALIGNED_T, PCB_DIM_CENTER_T, PCB_DIM_RADIAL_T, PCB_DIM_ORTHOGONAL_T, PCB_DIM_LEADER_T } );

            if( inserted )
                typesInserted.insert( {PCB_DIM_ALIGNED_T, PCB_DIM_CENTER_T, PCB_DIM_RADIAL_T, PCB_DIM_ORTHOGONAL_T, PCB_DIM_LEADER_T } );

            break;
        }

        case PCB_ZONE_T:
        {
            handledAnything = true;

            std::copy( board->Zones().begin(), board->Zones().end(),
                       std::back_inserter( items ) );

            typesInserted.insert( PCB_ZONE_T );
            break;
        }

        case PCB_GROUP_T:
        {
            handledAnything = true;

            std::copy( board->Groups().begin(), board->Groups().end(),
                       std::back_inserter( items ) );

            typesInserted.insert( PCB_GROUP_T );
            break;
        }
        default:
            break;
        }
    }

    if( !handledAnything )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "none of the requested types are valid for a Board object" );
        return tl::unexpected( e );
    }

    for( const BOARD_ITEM* item : items )
    {
        if( !typesRequested.count( item->Type() ) )
            continue;

        google::protobuf::Any itemBuf;
        item->Serialize( itemBuf );
        response.mutable_items()->Add( std::move( itemBuf ) );
    }

    response.set_status( ItemRequestStatus::IRS_OK );
    return response;
}


HANDLER_RESULT<BoardEnabledLayersResponse> API_HANDLER_PCB::handleSetBoardEnabledLayers(
        const HANDLER_CONTEXT<SetBoardEnabledLayers>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    if( aCtx.Request.copper_layer_count() % 2 != 0 )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "copper_layer_count must be an even number" );
        return tl::unexpected( e );
    }

    if( aCtx.Request.copper_layer_count() > MAX_CU_LAYERS )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "copper_layer_count must be below %d", MAX_CU_LAYERS ) );
        return tl::unexpected( e );
    }

    int copperLayerCount = static_cast<int>( aCtx.Request.copper_layer_count() );
    LSET enabled = board::UnpackLayerSet( aCtx.Request.layers() );

    // Sanitize the input
    enabled |= LSET( { Edge_Cuts, Margin, F_CrtYd, B_CrtYd } );
    enabled &= ~LSET::AllCuMask();
    enabled |= LSET::AllCuMask( copperLayerCount );

    BOARD* board = this->board();

    LSET previousEnabled = board->GetEnabledLayers();
    LSET changedLayers = enabled ^ previousEnabled;

    board->SetEnabledLayers( enabled );
    board->SetVisibleLayers( board->GetVisibleLayers() | changedLayers );

    LSEQ removedLayers;

    for( PCB_LAYER_ID layer_id : previousEnabled )
    {
        if( !enabled[layer_id] && board->HasItemsOnLayer( layer_id ) )
            removedLayers.push_back( layer_id );
    }

    bool modified = false;

    if( !removedLayers.empty() )
    {
        toolManager()->RunAction( PCB_ACTIONS::selectionClear );

        for( PCB_LAYER_ID layer_id : removedLayers )
            modified |= board->RemoveAllItemsOnLayer( layer_id );
    }

    if( enabled != previousEnabled )
        frame()->UpdateUserInterface();

    if( modified )
        frame()->OnModify();

    BoardEnabledLayersResponse response;

    response.set_copper_layer_count( copperLayerCount );
    board::PackLayerSet( *response.mutable_layers(), enabled );

    return response;
}


HANDLER_RESULT<BoardDesignRulesResponse> API_HANDLER_PCB::handleGetBoardDesignRules(
        const HANDLER_CONTEXT<GetBoardDesignRules>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    const BOARD_DESIGN_SETTINGS& bds = board()->GetDesignSettings();
    BoardDesignRulesResponse     response;
    kiapi::board::BoardDesignRules* rules = response.mutable_rules();

    kiapi::board::MinimumConstraints* constraints = rules->mutable_constraints();

    constraints->mutable_min_clearance()->set_value_nm( bds.m_MinClearance );
    constraints->mutable_min_groove_width()->set_value_nm( bds.m_MinGrooveWidth );
    constraints->mutable_min_connection_width()->set_value_nm( bds.m_MinConn );
    constraints->mutable_min_track_width()->set_value_nm( bds.m_TrackMinWidth );
    constraints->mutable_min_via_annular_width()->set_value_nm( bds.m_ViasMinAnnularWidth );
    constraints->mutable_min_via_size()->set_value_nm( bds.m_ViasMinSize );
    constraints->mutable_min_through_drill()->set_value_nm( bds.m_MinThroughDrill );
    constraints->mutable_min_microvia_size()->set_value_nm( bds.m_MicroViasMinSize );
    constraints->mutable_min_microvia_drill()->set_value_nm( bds.m_MicroViasMinDrill );
    constraints->mutable_copper_edge_clearance()->set_value_nm( bds.m_CopperEdgeClearance );
    constraints->mutable_hole_clearance()->set_value_nm( bds.m_HoleClearance );
    constraints->mutable_hole_to_hole_min()->set_value_nm( bds.m_HoleToHoleMin );
    constraints->mutable_silk_clearance()->set_value_nm( bds.m_SilkClearance );
    constraints->set_min_resolved_spokes( bds.m_MinResolvedSpokes );
    constraints->mutable_min_silk_text_height()->set_value_nm( bds.m_MinSilkTextHeight );
    constraints->mutable_min_silk_text_thickness()->set_value_nm( bds.m_MinSilkTextThickness );

    kiapi::board::PredefinedSizes* sizes = rules->mutable_predefined_sizes();

    for( size_t ii = 1; ii < bds.m_TrackWidthList.size(); ++ii )
        sizes->add_tracks()->mutable_width()->set_value_nm( bds.m_TrackWidthList[ii] );

    for( size_t ii = 1; ii < bds.m_ViasDimensionsList.size(); ++ii )
    {
        kiapi::board::PresetViaDimension* via = sizes->add_vias();
        via->mutable_diameter()->set_value_nm( bds.m_ViasDimensionsList[ii].m_Diameter );
        via->mutable_drill()->set_value_nm( bds.m_ViasDimensionsList[ii].m_Drill );
    }

    for( size_t ii = 1; ii < bds.m_DiffPairDimensionsList.size(); ++ii )
    {
        kiapi::board::PresetDiffPairDimension* pair = sizes->add_diff_pairs();
        pair->mutable_width()->set_value_nm( bds.m_DiffPairDimensionsList[ii].m_Width );
        pair->mutable_gap()->set_value_nm( bds.m_DiffPairDimensionsList[ii].m_Gap );
        pair->mutable_via_gap()->set_value_nm( bds.m_DiffPairDimensionsList[ii].m_ViaGap );
    }

    kiapi::board::SolderMaskPasteDefaults* maskPaste = rules->mutable_solder_mask_paste();

    maskPaste->mutable_mask_expansion()->set_value_nm( bds.m_SolderMaskExpansion );
    maskPaste->mutable_mask_min_width()->set_value_nm( bds.m_SolderMaskMinWidth );
    maskPaste->mutable_mask_to_copper_clearance()->set_value_nm( bds.m_SolderMaskToCopperClearance );
    maskPaste->mutable_paste_margin()->set_value_nm( bds.m_SolderPasteMargin );
    maskPaste->set_paste_margin_ratio( bds.m_SolderPasteMarginRatio );
    maskPaste->set_allow_soldermask_bridges_in_footprints( bds.m_AllowSoldermaskBridgesInFPs );

    kiapi::board::TeardropDefaults* teardrops = rules->mutable_teardrops();

    teardrops->set_target_vias( bds.m_TeardropParamsList.m_TargetVias );
    teardrops->set_target_pth_pads( bds.m_TeardropParamsList.m_TargetPTHPads );
    teardrops->set_target_smd_pads( bds.m_TeardropParamsList.m_TargetSMDPads );
    teardrops->set_target_track_to_track( bds.m_TeardropParamsList.m_TargetTrack2Track );
    teardrops->set_use_round_shapes_only( bds.m_TeardropParamsList.m_UseRoundShapesOnly );

    TEARDROP_PARAMETERS_LIST& tdList = const_cast<TEARDROP_PARAMETERS_LIST&>( bds.m_TeardropParamsList );

    for( int target = TARGET_ROUND; target <= TARGET_TRACK; ++target )
    {
        const TEARDROP_PARAMETERS* params = tdList.GetParameters( static_cast<TARGET_TD>( target ) );
        kiapi::board::TeardropTargetEntry* entry = teardrops->add_target_params();

        entry->set_target( ToProtoEnum<TARGET_TD, kiapi::board::TeardropTarget>(
            static_cast<TARGET_TD>( target ) ) );
        entry->mutable_params()->set_enabled( params->m_Enabled );
        entry->mutable_params()->mutable_max_length()->set_value_nm( params->m_TdMaxLen );
        entry->mutable_params()->mutable_max_width()->set_value_nm( params->m_TdMaxWidth );
        entry->mutable_params()->set_best_length_ratio( params->m_BestLengthRatio );
        entry->mutable_params()->set_best_width_ratio( params->m_BestWidthRatio );
        entry->mutable_params()->set_width_to_size_filter_ratio( params->m_WidthtoSizeFilterRatio );
        entry->mutable_params()->set_curved_edges( params->m_CurvedEdges );
        entry->mutable_params()->set_allow_two_tracks( params->m_AllowUseTwoTracks );
        entry->mutable_params()->set_on_pads_in_zones( params->m_TdOnPadsInZones );
    }

    kiapi::board::ViaProtectionDefaults* viaProtection = rules->mutable_via_protection();

    viaProtection->set_tent_front( bds.m_TentViasFront );
    viaProtection->set_tent_back( bds.m_TentViasBack );
    viaProtection->set_cover_front( bds.m_CoverViasFront );
    viaProtection->set_cover_back( bds.m_CoverViasBack );
    viaProtection->set_plug_front( bds.m_PlugViasFront );
    viaProtection->set_plug_back( bds.m_PlugViasBack );
    viaProtection->set_cap( bds.m_CapVias );
    viaProtection->set_fill( bds.m_FillVias );

    for( const auto& [errorCode, severity] : bds.m_DRCSeverities )
    {
        board::DrcSeveritySetting* setting = rules->add_severities();
        setting->set_rule_type(
                ToProtoEnum<PCB_DRC_CODE, board::DrcErrorType>( static_cast<PCB_DRC_CODE>( errorCode ) ) );
        setting->set_severity( ToProtoEnum<SEVERITY, types::RuleSeverity>( severity ) );
    }

    for( const wxString& serialized : bds.m_DrcExclusions )
    {
        kiapi::board::DrcExclusion* exclusion = rules->add_exclusions();
        exclusion->mutable_marker()->mutable_id()->set_opaque_id( serialized.ToStdString() );

        auto it = bds.m_DrcExclusionComments.find( serialized );

        if( it != bds.m_DrcExclusionComments.end() )
            exclusion->set_comment( it->second.ToStdString() );
    }

    response.set_custom_rules_status( CRS_NONE );

    wxString rulesPath = board()->GetDesignRulesPath();

    if( !rulesPath.IsEmpty() && wxFileName::IsFileReadable( rulesPath ) )
    {
        wxFFile file( rulesPath, "r" );
        wxString content;
        std::vector<std::shared_ptr<DRC_RULE>> parsedRules;

        if( !file.IsOpened() )
        {
            response.set_custom_rules_status( CRS_INVALID );
            return response;
        }

        file.ReadAll( &content );
        file.Close();

        try
        {
            DRC_RULES_PARSER parser( content, "File" );
            parser.Parse( parsedRules, nullptr );
            response.set_custom_rules_status( CRS_VALID );
        }
        catch( const IO_ERROR& )
        {
            response.set_custom_rules_status( CRS_INVALID );
        }
    }

    return response;
}


HANDLER_RESULT<BoardDesignRulesResponse> API_HANDLER_PCB::handleSetBoardDesignRules(
        const HANDLER_CONTEXT<SetBoardDesignRules>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    BOARD_DESIGN_SETTINGS newSettings( board()->GetDesignSettings() );
    const kiapi::board::BoardDesignRules& rules = aCtx.Request.rules();

    if( rules.has_constraints() )
    {
        const kiapi::board::MinimumConstraints& constraints = rules.constraints();

        newSettings.m_MinClearance = constraints.min_clearance().value_nm();
        newSettings.m_MinGrooveWidth = constraints.min_groove_width().value_nm();
        newSettings.m_MinConn = constraints.min_connection_width().value_nm();
        newSettings.m_TrackMinWidth = constraints.min_track_width().value_nm();
        newSettings.m_ViasMinAnnularWidth = constraints.min_via_annular_width().value_nm();
        newSettings.m_ViasMinSize = constraints.min_via_size().value_nm();
        newSettings.m_MinThroughDrill = constraints.min_through_drill().value_nm();
        newSettings.m_MicroViasMinSize = constraints.min_microvia_size().value_nm();
        newSettings.m_MicroViasMinDrill = constraints.min_microvia_drill().value_nm();
        newSettings.m_CopperEdgeClearance = constraints.copper_edge_clearance().value_nm();
        newSettings.m_HoleClearance = constraints.hole_clearance().value_nm();
        newSettings.m_HoleToHoleMin = constraints.hole_to_hole_min().value_nm();
        newSettings.m_SilkClearance = constraints.silk_clearance().value_nm();
        newSettings.m_MinResolvedSpokes = constraints.min_resolved_spokes();
        newSettings.m_MinSilkTextHeight = constraints.min_silk_text_height().value_nm();
        newSettings.m_MinSilkTextThickness = constraints.min_silk_text_thickness().value_nm();
    }

    if( rules.has_predefined_sizes() )
    {
        newSettings.m_TrackWidthList.clear();
        newSettings.m_TrackWidthList.emplace_back( 0 );

        for( const kiapi::board::PresetTrackWidth& track : rules.predefined_sizes().tracks() )
            newSettings.m_TrackWidthList.emplace_back( track.width().value_nm() );

        newSettings.m_ViasDimensionsList.clear();
        newSettings.m_ViasDimensionsList.emplace_back( 0, 0 );

        for( const kiapi::board::PresetViaDimension& via : rules.predefined_sizes().vias() )
        {
            newSettings.m_ViasDimensionsList.emplace_back( static_cast<int>( via.diameter().value_nm() ),
                                                           static_cast<int>( via.drill().value_nm() ) );
        }

        newSettings.m_DiffPairDimensionsList.clear();
        newSettings.m_DiffPairDimensionsList.emplace_back( 0, 0, 0 );

        for( const kiapi::board::PresetDiffPairDimension& pair : rules.predefined_sizes().diff_pairs() )
        {
            newSettings.m_DiffPairDimensionsList.emplace_back(
                    static_cast<int>( pair.width().value_nm() ),
                    static_cast<int>( pair.gap().value_nm() ),
                    static_cast<int>( pair.via_gap().value_nm() ) );
        }
    }

    if( rules.has_solder_mask_paste() )
    {
        const kiapi::board::SolderMaskPasteDefaults& maskPaste = rules.solder_mask_paste();

        newSettings.m_SolderMaskExpansion = maskPaste.mask_expansion().value_nm();
        newSettings.m_SolderMaskMinWidth = maskPaste.mask_min_width().value_nm();
        newSettings.m_SolderMaskToCopperClearance = maskPaste.mask_to_copper_clearance().value_nm();
        newSettings.m_SolderPasteMargin = maskPaste.paste_margin().value_nm();
        newSettings.m_SolderPasteMarginRatio = maskPaste.paste_margin_ratio();
        newSettings.m_AllowSoldermaskBridgesInFPs =
                maskPaste.allow_soldermask_bridges_in_footprints();
    }

    if( rules.has_teardrops() )
    {
        const kiapi::board::TeardropDefaults& teardrops = rules.teardrops();

        newSettings.m_TeardropParamsList.m_TargetVias = teardrops.target_vias();
        newSettings.m_TeardropParamsList.m_TargetPTHPads = teardrops.target_pth_pads();
        newSettings.m_TeardropParamsList.m_TargetSMDPads = teardrops.target_smd_pads();
        newSettings.m_TeardropParamsList.m_TargetTrack2Track = teardrops.target_track_to_track();
        newSettings.m_TeardropParamsList.m_UseRoundShapesOnly = teardrops.use_round_shapes_only();

        for( const kiapi::board::TeardropTargetEntry& entry : teardrops.target_params() )
        {
            if( entry.target() == kiapi::board::TeardropTarget::TDT_UNKNOWN )
                continue;

            TARGET_TD target = FromProtoEnum<TARGET_TD, kiapi::board::TeardropTarget>(
                    entry.target() );

            TEARDROP_PARAMETERS* params = newSettings.m_TeardropParamsList.GetParameters( target );

            params->m_Enabled = entry.params().enabled();
            params->m_TdMaxLen = entry.params().max_length().value_nm();
            params->m_TdMaxWidth = entry.params().max_width().value_nm();
            params->m_BestLengthRatio = entry.params().best_length_ratio();
            params->m_BestWidthRatio = entry.params().best_width_ratio();
            params->m_WidthtoSizeFilterRatio = entry.params().width_to_size_filter_ratio();
            params->m_CurvedEdges = entry.params().curved_edges();
            params->m_AllowUseTwoTracks = entry.params().allow_two_tracks();
            params->m_TdOnPadsInZones = entry.params().on_pads_in_zones();
        }
    }

    if( rules.has_via_protection() )
    {
        const kiapi::board::ViaProtectionDefaults& viaProtection = rules.via_protection();

        newSettings.m_TentViasFront = viaProtection.tent_front();
        newSettings.m_TentViasBack = viaProtection.tent_back();
        newSettings.m_CoverViasFront = viaProtection.cover_front();
        newSettings.m_CoverViasBack = viaProtection.cover_back();
        newSettings.m_PlugViasFront = viaProtection.plug_front();
        newSettings.m_PlugViasBack = viaProtection.plug_back();
        newSettings.m_CapVias = viaProtection.cap();
        newSettings.m_FillVias = viaProtection.fill();
    }

    if( rules.severities_size() > 0 )
    {
        newSettings.m_DRCSeverities.clear();

        for( const kiapi::board::DrcSeveritySetting& severitySetting : rules.severities() )
        {
            PCB_DRC_CODE ruleType =
                    FromProtoEnum<PCB_DRC_CODE, kiapi::board::DrcErrorType>( severitySetting.rule_type() );

            const std::unordered_set<SEVERITY> permitted( { RPT_SEVERITY_ERROR, RPT_SEVERITY_WARNING, RPT_SEVERITY_IGNORE } );
            SEVERITY setting = FromProtoEnum<SEVERITY, kiapi::common::types::RuleSeverity>( severitySetting.severity() );

            if( !permitted.contains( setting ) )
            {
                ApiResponseStatus e;
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message( fmt::format( "DRC severity must be error, warning, or ignore" ) );
                return tl::unexpected( e );
            }

            newSettings.m_DRCSeverities[ruleType] = setting;
        }
    }

    if( rules.exclusions_size() > 0 )
    {
        newSettings.m_DrcExclusions.clear();
        newSettings.m_DrcExclusionComments.clear();

        for( const kiapi::board::DrcExclusion& exclusion : rules.exclusions() )
        {
            wxString serialized = wxString::FromUTF8( exclusion.marker().id().opaque_id() );

            if( serialized.IsEmpty() )
            {
                ApiResponseStatus e;
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message( "DrcExclusion marker id must not be empty" );
                return tl::unexpected( e );
            }

            newSettings.m_DrcExclusions.insert( serialized );
            newSettings.m_DrcExclusionComments[serialized] = wxString::FromUTF8( exclusion.comment() );
        }
    }

    std::vector<BOARD_DESIGN_SETTINGS::VALIDATION_ERROR> errors = newSettings.ValidateDesignRules();

    if( !errors.empty() )
    {
        const BOARD_DESIGN_SETTINGS::VALIDATION_ERROR& error = errors.front();

        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "Invalid board design rules: {}: {}",
                                          error.setting_name.ToStdString(),
                                          error.error_message.ToStdString() ) );
        return tl::unexpected( e );
    }

    board()->SetDesignSettings( newSettings );

    if( frame() )
    {
        frame()->OnModify();
        frame()->UpdateUserInterface();
    }

    HANDLER_CONTEXT<GetBoardDesignRules> getCtx = { aCtx.ClientName, GetBoardDesignRules() };
    *getCtx.Request.mutable_board() = aCtx.Request.board();

    return handleGetBoardDesignRules( getCtx );
}


HANDLER_RESULT<CustomRulesResponse> API_HANDLER_PCB::handleGetCustomDesignRules(
        const HANDLER_CONTEXT<GetCustomDesignRules>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    CustomRulesResponse response;
    response.set_status( CRS_NONE );

    wxString rulesPath = board()->GetDesignRulesPath();

    if( rulesPath.IsEmpty() || !wxFileName::IsFileReadable( rulesPath ) )
        return response;

    wxFFile file( rulesPath, "r" );

    if( !file.IsOpened() )
    {
        response.set_status( CRS_INVALID );
        response.set_error_text( "Failed to open custom rules file" );
        return response;
    }

    wxString content;
    file.ReadAll( &content );
    file.Close();

    std::vector<std::shared_ptr<DRC_RULE>> parsedRules;

    try
    {
        DRC_RULES_PARSER parser( content, "File" );
        parser.Parse( parsedRules, nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        response.set_status( CRS_INVALID );
        response.set_error_text( ioe.What().ToStdString() );
        return response;
    }

    for( const std::shared_ptr<DRC_RULE>& rule : parsedRules )
    {
        // TODO(JE) since we now need this for both here and the rules editor, maybe it's time
        // to just make comment parsing part of the parser?
        wxString text = DRC_RULE_LOADER::ExtractRuleText( content, rule->m_Name );
        wxString comment = DRC_RULE_LOADER::ExtractRuleComment( text );

        kiapi::board::CustomRule* customRule = response.add_rules();

        if( rule->m_Condition )
            customRule->set_condition( rule->m_Condition->GetExpression().ToUTF8() );

        for( const DRC_CONSTRAINT& constraint : rule->m_Constraints )
        {
            board::CustomRuleConstraint* constraintProto = customRule->add_constraints();
            constraint.ToProto( *constraintProto );
        }

        customRule->set_severity( ToProtoEnum<SEVERITY, types::RuleSeverity>( rule->m_Severity ) );
        customRule->set_name( rule->m_Name.ToUTF8() );

        if( rule->m_LayerSource.CmpNoCase( wxS( "outer" ) ) == 0 )
        {
            customRule->set_layer_mode( kiapi::board::CRLM_OUTER );
        }
        else if( rule->m_LayerSource.CmpNoCase( wxS( "inner" ) ) == 0 )
        {
            customRule->set_layer_mode( kiapi::board::CRLM_INNER );
        }
        else if( !rule->m_LayerSource.IsEmpty() )
        {
            int layer = LSET::NameToLayer( rule->m_LayerSource );

            if( layer != UNDEFINED_LAYER && layer != UNSELECTED_LAYER && layer < PCB_LAYER_ID_COUNT )
            {
                customRule->set_single_layer(
                        ToProtoEnum<PCB_LAYER_ID, board::types::BoardLayer>( static_cast<PCB_LAYER_ID>( layer ) ) );
            }
        }

        if( !comment.IsEmpty() )
            customRule->set_comments( comment );
    }

    response.set_status( CRS_VALID );
    return response;
}


HANDLER_RESULT<CustomRulesResponse> API_HANDLER_PCB::handleSetCustomDesignRules(
        const HANDLER_CONTEXT<SetCustomDesignRules>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    wxString rulesPath = board()->GetDesignRulesPath();

    if( aCtx.Request.rules_size() == 0 )
    {
        if( wxFileName::FileExists( rulesPath ) )
        {
            if( !wxRemoveFile( rulesPath ) )
            {
                CustomRulesResponse response;
                response.set_status( CRS_INVALID );
                response.set_error_text( "Failed to remove custom rules file" );
                return response;
            }
        }

        CustomRulesResponse response;
        response.set_status( CRS_NONE );
        return response;
    }

    wxString rulesText;
    rulesText << "(version 2)\n";

    for( const board::CustomRule& rule : aCtx.Request.rules() )
    {
        wxString serializationError;
        wxString serializedRule = DRC_RULE::FormatRuleFromProto( rule, &serializationError );

        if( serializedRule.IsEmpty() )
        {
            CustomRulesResponse response;
            response.set_status( CRS_INVALID );

            if( serializationError.IsEmpty() )
                response.set_error_text( "Failed to serialize custom rule" );
            else
                response.set_error_text( serializationError.ToUTF8() );

            return response;
        }

        rulesText << "\n" << serializedRule;
    }

    // Validate generated file text before writing so callers get parser errors in response.
    try
    {
        std::vector<std::shared_ptr<DRC_RULE>> parsedRules;
        DRC_RULES_PARSER parser( rulesText, "SetCustomDesignRules" );
        parser.Parse( parsedRules, nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        CustomRulesResponse response;
        response.set_status( CRS_INVALID );
        response.set_error_text( ioe.What().ToStdString() );
        return response;
    }

    wxFFile file( rulesPath, "w" );

    if( !file.IsOpened() )
    {
        CustomRulesResponse response;
        response.set_status( CRS_INVALID );
        response.set_error_text( "Failed to open custom rules file for writing" );
        return response;
    }

    if( !file.Write( rulesText ) )
    {
        file.Close();

        CustomRulesResponse response;
        response.set_status( CRS_INVALID );
        response.set_error_text( "Failed to write custom rules file" );
        return response;
    }

    file.Close();

    HANDLER_CONTEXT<GetCustomDesignRules> getCtx = { aCtx.ClientName, GetCustomDesignRules() };
    *getCtx.Request.mutable_board() = aCtx.Request.board();
    return handleGetCustomDesignRules( getCtx );
}


HANDLER_RESULT<types::Vector2> API_HANDLER_PCB::handleGetBoardOrigin(
        const HANDLER_CONTEXT<GetBoardOrigin>& aCtx )
{
    if( HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );
        !documentValidation )
    {
        return tl::unexpected( documentValidation.error() );
    }

    VECTOR2I origin;
    const BOARD_DESIGN_SETTINGS& settings = board()->GetDesignSettings();

    switch( aCtx.Request.type() )
    {
    case BOT_GRID:
        origin = settings.GetGridOrigin();
        break;

    case BOT_DRILL:
        origin = settings.GetAuxOrigin();
        break;

    default:
    case BOT_UNKNOWN:
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "Unexpected origin type" );
        return tl::unexpected( e );
    }
    }

    types::Vector2 reply;
    PackVector2( reply, origin );
    return reply;
}

HANDLER_RESULT<Empty> API_HANDLER_PCB::handleSetBoardOrigin(
        const HANDLER_CONTEXT<SetBoardOrigin>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );
        !documentValidation )
    {
        return tl::unexpected( documentValidation.error() );
    }

    VECTOR2I origin = UnpackVector2( aCtx.Request.origin() );

    switch( aCtx.Request.type() )
    {
    case BOT_GRID:
    {
        PCB_EDIT_FRAME* f = frame();

        frame()->CallAfter( [f, origin]()
                            {
                                // gridSetOrigin takes ownership and frees this
                                VECTOR2D* dorigin = new VECTOR2D( origin );
                                TOOL_MANAGER* mgr = f->GetToolManager();
                                mgr->RunAction( PCB_ACTIONS::gridSetOrigin, dorigin );
                                f->Refresh();
                            } );
        break;
    }

    case BOT_DRILL:
    {
        PCB_EDIT_FRAME* f = frame();

        frame()->CallAfter( [f, origin]()
                            {
                                TOOL_MANAGER* mgr = f->GetToolManager();
                                mgr->RunAction( PCB_ACTIONS::drillSetOrigin, origin );
                                f->Refresh();
                            } );
        break;
    }

    default:
    case BOT_UNKNOWN:
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "Unexpected origin type" );
        return tl::unexpected( e );
    }
    }

    return Empty();
}


HANDLER_RESULT<BoardLayerNameResponse> API_HANDLER_PCB::handleGetBoardLayerName(
            const HANDLER_CONTEXT<GetBoardLayerName>& aCtx )
{
    if( HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );
        !documentValidation )
    {
        return tl::unexpected( documentValidation.error() );
    }

    BoardLayerNameResponse response;

    PCB_LAYER_ID id = FromProtoEnum<PCB_LAYER_ID>( aCtx.Request.layer() );

    response.set_name( board()->GetLayerName( id ) );

    return response;
}


std::optional<TITLE_BLOCK*> API_HANDLER_PCB::getTitleBlock()
{
    return &context()->GetBoard()->GetTitleBlock();
}


std::optional<PAGE_INFO> API_HANDLER_PCB::getPageSettings()
{
    return context()->GetBoard()->GetPageSettings();
}


bool API_HANDLER_PCB::setPageSettings( const PAGE_INFO& aPageInfo )
{
    context()->GetBoard()->SetPageSettings( aPageInfo );
    return true;
}


wxString API_HANDLER_PCB::getDrawingSheetFileName()
{
    return BASE_SCREEN::m_DrawingSheetFileName;
}


void API_HANDLER_PCB::setDrawingSheetFileName( const wxString& aFileName )
{
    BASE_SCREEN::m_DrawingSheetFileName = aFileName;

    if( frame() )
        frame()->LoadDrawingSheet();
}


void API_HANDLER_PCB::onModified()
{
    if( frame() )
    {
        frame()->Refresh();
        frame()->OnModify();
        frame()->UpdateUserInterface();
    }
}


HANDLER_RESULT<NetsResponse> API_HANDLER_PCB::handleGetNets( const HANDLER_CONTEXT<GetNets>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    NetsResponse response;
    BOARD* board = this->board();

    std::set<wxString> netclassFilter;

    for( const std::string& nc : aCtx.Request.netclass_filter() )
        netclassFilter.insert( wxString( nc.c_str(), wxConvUTF8 ) );

    for( NETINFO_ITEM* net : board->GetNetInfo() )
    {
        NETCLASS* nc = net->GetNetClass();

        if( !netclassFilter.empty() && nc )
        {
            bool inClass = false;

            for( const wxString& filter : netclassFilter )
            {
                if( nc->ContainsNetclassWithName( filter ) )
                {
                    inClass = true;
                    break;
                }
            }

            if( !inClass )
                continue;
        }

        board::types::Net* netProto = response.add_nets();
        netProto->set_name( net->GetNetname() );
        netProto->mutable_code()->set_value( net->GetNetCode() );
    }

    return response;
}


HANDLER_RESULT<GetItemsResponse> API_HANDLER_PCB::handleGetConnectedItems(
        const HANDLER_CONTEXT<GetConnectedItems>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    std::vector<KICAD_T> types = parseRequestedItemTypes( aCtx.Request.types() );
    const bool filterByType = aCtx.Request.types_size() > 0;

    if( filterByType && types.empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "none of the requested types are valid for a Board object" );
        return tl::unexpected( e );
    }

    std::set<KICAD_T> typeFilter( types.begin(), types.end() );
    std::vector<BOARD_CONNECTED_ITEM*> sourceItems;

    for( const types::KIID& id : aCtx.Request.items() )
    {
        if( std::optional<BOARD_ITEM*> item = getItemById( KIID( id.value() ) ) )
        {
            if( BOARD_CONNECTED_ITEM* connected = dynamic_cast<BOARD_CONNECTED_ITEM*>( *item ) )
                sourceItems.emplace_back( connected );
        }
    }

    if( sourceItems.empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "none of the requested IDs were found or valid connected items" );
        return tl::unexpected( e );
    }

    GetItemsResponse response;
    std::shared_ptr<CONNECTIVITY_DATA> conn = board()->GetConnectivity();
    std::set<KIID> insertedItems;

    for( BOARD_CONNECTED_ITEM* source : sourceItems )
    {
        for( BOARD_CONNECTED_ITEM* connected : conn->GetConnectedItems( source ) )
        {
            if( filterByType && !typeFilter.contains( connected->Type() ) )
                continue;

            if( !insertedItems.insert( connected->m_Uuid ).second )
                continue;

            connected->Serialize( *response.add_items() );
        }
    }

    response.set_status( ItemRequestStatus::IRS_OK );
    return response;
}


HANDLER_RESULT<GetItemsResponse> API_HANDLER_PCB::handleGetItemsByNet(
        const HANDLER_CONTEXT<GetItemsByNet>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    std::vector<KICAD_T> types = parseRequestedItemTypes( aCtx.Request.types() );
    const bool filterByType = aCtx.Request.types_size() > 0;

    if( filterByType && types.empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "none of the requested types are valid for a Board object" );
        return tl::unexpected( e );
    }

    if( !filterByType )
        types.assign( { PCB_PAD_T, PCB_VIA_T, PCB_TRACE_T, PCB_ARC_T, PCB_SHAPE_T, PCB_ZONE_T } );

    GetItemsResponse response;
    BOARD* board = this->board();
    std::shared_ptr<CONNECTIVITY_DATA> conn = board->GetConnectivity();
    std::set<KIID> insertedItems;

    const NETINFO_LIST& nets = board->GetNetInfo();

    for( const board::types::Net& net : aCtx.Request.nets() )
    {
        NETINFO_ITEM* netInfo = nets.GetNetItem( wxString::FromUTF8( net.name() ) );

        if( !netInfo )
            continue;

        for( BOARD_CONNECTED_ITEM* item : conn->GetNetItems( netInfo->GetNetCode(), types ) )
        {
            if( !insertedItems.insert( item->m_Uuid ).second )
                continue;

            item->Serialize( *response.add_items() );
        }
    }

    response.set_status( ItemRequestStatus::IRS_OK );
    return response;
}


HANDLER_RESULT<GetItemsResponse> API_HANDLER_PCB::handleGetItemsByNetClass(
        const HANDLER_CONTEXT<GetItemsByNetClass>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    std::vector<KICAD_T> types = parseRequestedItemTypes( aCtx.Request.types() );
    const bool filterByType = aCtx.Request.types_size() > 0;

    if( filterByType && types.empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "none of the requested types are valid for a Board object" );
        return tl::unexpected( e );
    }

    if( !filterByType )
        types.assign( { PCB_PAD_T, PCB_VIA_T, PCB_TRACE_T, PCB_ARC_T, PCB_SHAPE_T, PCB_ZONE_T } );

    std::set<wxString> requestedClasses;

    for( const std::string& netClass : aCtx.Request.net_classes() )
        requestedClasses.insert( wxString( netClass.c_str(), wxConvUTF8 ) );

    GetItemsResponse response;
    BOARD* board = this->board();
    std::shared_ptr<CONNECTIVITY_DATA> conn = board->GetConnectivity();
    std::set<KIID> insertedItems;

    for( NETINFO_ITEM* net : board->GetNetInfo() )
    {
        if( !net )
            continue;

        NETCLASS* nc = net->GetNetClass();

        if( !requestedClasses.empty() )
        {
            if( !nc )
                continue;

            bool inClass = false;

            for( const wxString& filter : requestedClasses )
            {
                if( nc->ContainsNetclassWithName( filter ) )
                {
                    inClass = true;
                    break;
                }
            }

            if( !inClass )
                continue;
        }

        for( BOARD_CONNECTED_ITEM* item : conn->GetNetItems( net->GetNetCode(), types ) )
        {
            if( !insertedItems.insert( item->m_Uuid ).second )
                continue;

            item->Serialize( *response.add_items() );
        }
    }

    response.set_status( ItemRequestStatus::IRS_OK );
    return response;
}


HANDLER_RESULT<NetClassForNetsResponse> API_HANDLER_PCB::handleGetNetClassForNets(
            const HANDLER_CONTEXT<GetNetClassForNets>& aCtx )
{
    NetClassForNetsResponse response;

    BOARD* board = this->board();
    const NETINFO_LIST& nets = board->GetNetInfo();
    google::protobuf::Any any;

    for( const board::types::Net& net : aCtx.Request.net() )
    {
        NETINFO_ITEM* netInfo = nets.GetNetItem( wxString::FromUTF8( net.name() ) );

        if( !netInfo )
            continue;

        netInfo->GetNetClass()->Serialize( any );
        auto [pair, rc] = response.mutable_classes()->insert( { net.name(), {} } );
        any.UnpackTo( &pair->second );
    }

    return response;
}


HANDLER_RESULT<Empty> API_HANDLER_PCB::handleRefillZones( const HANDLER_CONTEXT<RefillZones>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    if( aCtx.Request.zones().empty() )
    {
        TOOL_MANAGER* mgr = toolManager();

        if( frame() )
        {
            frame()->CallAfter( [mgr]()
                                {
                                    mgr->RunAction( PCB_ACTIONS::zoneFillAll );
                                } );
        }
        else
        {
            // Headless sessions have no event loop to defer to; fill synchronously through the
            // same tool the CLI jobs use.
            if( !mgr->FindTool( ZONE_FILLER_TOOL_NAME ) )
                mgr->RegisterTool( new ZONE_FILLER_TOOL );

            mgr->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( nullptr, nullptr, true );
        }
    }
    else
    {
        std::vector<ZONE*> toFill;

        for( const types::KIID& id : aCtx.Request.zones() )
        {
            std::optional<BOARD_ITEM*> item = getItemById( KIID( id.value() ) );

            if( !item || ( *item )->Type() != PCB_ZONE_T )
            {
                ApiResponseStatus e;
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message( fmt::format( "zone with ID {} not found on the board", id.value() ) );
                return tl::unexpected( e );
            }

            ZONE* zone = static_cast<ZONE*>( *item );

            // The filler silently skips rule areas, which would turn this into a false success
            if( zone->GetIsRuleArea() )
            {
                ApiResponseStatus e;
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message( fmt::format( "zone with ID {} is a rule area and cannot be filled",
                                                  id.value() ) );
                return tl::unexpected( e );
            }

            // A repeated id would enqueue concurrent fill tasks for the same zone
            if( !alg::contains( toFill, zone ) )
                toFill.push_back( zone );
        }

        std::unique_ptr<COMMIT> commit = createCommit();
        ZONE_FILLER             filler( board(), commit.get() );

        if( !filler.Fill( toFill ) )
        {
            commit->Revert();

            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_UNKNOWN );
            e.set_error_message( "zone fill failed" );
            return tl::unexpected( e );
        }

        commit->Push( _( "Fill Zone(s)" ), SKIP_CONNECTIVITY | ZONE_FILL_OP );
        board()->BuildConnectivity();
        toolManager()->PostEvent( EVENTS::ConnectivityChangedEvent );

        if( frame() )
        {
            frame()->GetCanvas()->RedrawRatsnest();
            frame()->GetCanvas()->Refresh();
        }
    }

    return Empty();
}


HANDLER_RESULT<ImportNetlistResponse> API_HANDLER_PCB::handleImportNetlist( const HANDLER_CONTEXT<ImportNetlist>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    wxFileName netlistPath( project().AbsolutePath( wxString::FromUTF8( aCtx.Request.netlist_path() ) ) );

    if( !netlistPath.IsOk() || !netlistPath.FileExists() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message(
                fmt::format( "netlist file '{}' could not be opened", netlistPath.GetFullPath().ToStdString() ) );
        return tl::unexpected( e );
    }

    PCB_CONTEXT*       ctx = pcbContext();
    WX_STRING_REPORTER reporter;

    const bool lookupByTimestamp = aCtx.Request.match_mode() != NetlistMatchMode::NMM_REFERENCE;

    NETLIST netlist;
    netlist.SetFindByTimeStamp( lookupByTimestamp );
    netlist.SetReplaceFootprints( aCtx.Request.update_footprints() );

    if( !ctx->ReadNetlistFromFile( netlistPath.GetFullPath(), netlist, reporter ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "unable to handle netlist file '{}': {}",
                                          netlistPath.GetFullPath().ToStdString(),
                                          reporter.GetMessages().ToStdString() ) );
        return tl::unexpected( e );
    }

    std::unique_ptr<BOARD_NETLIST_UPDATER> updater = ctx->MakeNetlistUpdater();

    updater->SetReporter( &reporter );
    updater->SetIsDryRun( aCtx.Request.dry_run() );
    updater->SetLookupByTimestamp( lookupByTimestamp );
    updater->SetDeleteUnusedFootprints( aCtx.Request.delete_extra_footprints() );
    updater->SetReplaceFootprints( aCtx.Request.update_footprints() );
    updater->SetTransferGroups( aCtx.Request.transfer_groups() );
    updater->SetOverrideLocks( aCtx.Request.override_locks() );
    updater->SetUpdateFields( true );

    const bool success = updater->UpdateNetlist( netlist );

    if( !aCtx.Request.dry_run() && success )
        ctx->OnNetlistChanged( *updater );

    ImportNetlistResponse response;
    response.set_report( reporter.GetMessages().ToUTF8() );
    response.set_error_count( updater->GetErrorCount() );
    response.set_warning_count( updater->GetWarningCount() );
    response.set_new_footprint_count( updater->GetNewFootprintCount() );
    return response;
}


HANDLER_RESULT<BoardEditorAppearanceSettings> API_HANDLER_PCB::handleGetBoardEditorAppearanceSettings(
        const HANDLER_CONTEXT<GetBoardEditorAppearanceSettings>& aCtx )
{
    if( std::optional<ApiResponseStatus> headless = checkForHeadless( "GetBoardEditorAppearanceSettings" ) )
        return tl::unexpected( *headless );

    BoardEditorAppearanceSettings reply;

    // TODO: might be nice to put all these things in one place and have it derive SERIALIZABLE

    const PCB_DISPLAY_OPTIONS& displayOptions = frame()->GetDisplayOptions();

    reply.set_inactive_layer_display( ToProtoEnum<HIGH_CONTRAST_MODE, InactiveLayerDisplayMode>(
            displayOptions.m_ContrastModeDisplay ) );
    reply.set_net_color_display(
            ToProtoEnum<NET_COLOR_MODE, NetColorDisplayMode>( displayOptions.m_NetColorMode ) );

    reply.set_board_flip( frame()->GetCanvas()->GetView()->IsMirroredX()
                                  ? BoardFlipMode::BFM_FLIPPED_X
                                  : BoardFlipMode::BFM_NORMAL );

    PCBNEW_SETTINGS* editorSettings = frame()->GetPcbNewSettings();

    reply.set_ratsnest_display( ToProtoEnum<RATSNEST_MODE, RatsnestDisplayMode>(
            editorSettings->m_Display.m_RatsnestMode ) );

    return reply;
}


HANDLER_RESULT<Empty> API_HANDLER_PCB::handleSetBoardEditorAppearanceSettings(
        const HANDLER_CONTEXT<SetBoardEditorAppearanceSettings>& aCtx )
{
    if( std::optional<ApiResponseStatus> headless = checkForHeadless( "SetBoardEditorAppearanceSettings" ) )
        return tl::unexpected( *headless );

    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    PCB_DISPLAY_OPTIONS options = frame()->GetDisplayOptions();
    KIGFX::PCB_VIEW* view = frame()->GetCanvas()->GetView();
    PCBNEW_SETTINGS* editorSettings = frame()->GetPcbNewSettings();
    const BoardEditorAppearanceSettings& newSettings = aCtx.Request.settings();

    options.m_ContrastModeDisplay =
            FromProtoEnum<HIGH_CONTRAST_MODE>( newSettings.inactive_layer_display() );
    options.m_NetColorMode =
            FromProtoEnum<NET_COLOR_MODE>( newSettings.net_color_display() );

    bool flip = newSettings.board_flip() == BoardFlipMode::BFM_FLIPPED_X;

    if( flip != view->IsMirroredX() )
    {
        view->SetMirror( !view->IsMirroredX(), view->IsMirroredY() );
        view->RecacheAllItems();
    }

    editorSettings->m_Display.m_RatsnestMode =
            FromProtoEnum<RATSNEST_MODE>( newSettings.ratsnest_display() );

    frame()->SetDisplayOptions( options );
    frame()->GetCanvas()->GetView()->UpdateAllLayersColor();
    frame()->GetCanvas()->Refresh();

    return Empty();
}


HANDLER_RESULT<InjectDrcErrorResponse> API_HANDLER_PCB::handleInjectDrcError(
        const HANDLER_CONTEXT<InjectDrcError>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    SEVERITY severity = FromProtoEnum<SEVERITY>( aCtx.Request.severity() );
    int      layer = severity == RPT_SEVERITY_WARNING ? LAYER_DRC_WARNING : LAYER_DRC_ERROR;
    int      code = severity == RPT_SEVERITY_WARNING ? DRCE_GENERIC_WARNING : DRCE_GENERIC_ERROR;

    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( code );

    drcItem->SetErrorMessage( wxString::FromUTF8( aCtx.Request.message() ) );

    RC_ITEM::KIIDS ids;

    for( const auto& id : aCtx.Request.items() )
        ids.emplace_back( KIID( id.value() ) );

    if( !ids.empty() )
        drcItem->SetItems( ids );

    const auto& pos = aCtx.Request.position();
    VECTOR2I    position( static_cast<int>( pos.x_nm() ), static_cast<int>( pos.y_nm() ) );

    PCB_MARKER* marker = new PCB_MARKER( drcItem, position, layer );

    COMMIT* commit = getCurrentCommit( aCtx.ClientName );
    commit->Add( marker );
    commit->Push( wxS( "API injected DRC marker" ) );

    InjectDrcErrorResponse response;
    response.mutable_marker()->set_value( marker->GetUUID().AsStdString() );

    return response;
}


std::optional<ApiResponseStatus> ValidateUnitsInchMm( types::Units aUnits,
                                                      const std::string& aCommandName )
{
    if( aUnits == types::Units::U_INCH || aUnits == types::Units::U_MM
        || aUnits == types::Units::U_UNKNOWN )
    {
        return std::nullopt;
    }

    ApiResponseStatus e;
    e.set_status( ApiStatusCode::AS_BAD_REQUEST );
    e.set_error_message( fmt::format( "{} supports only inch and mm units", aCommandName ) );
    return e;
}


std::optional<ApiResponseStatus>
ValidatePaginationModeForSingleOrPerFile( kiapi::board::jobs::BoardJobPaginationMode aMode,
                                          const std::string& aCommandName )
{
    if( aMode == kiapi::board::jobs::BoardJobPaginationMode::BJPM_UNKNOWN
        || aMode == kiapi::board::jobs::BoardJobPaginationMode::BJPM_ALL_LAYERS_ONE_PAGE
        || aMode == kiapi::board::jobs::BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_FILE )
    {
        return std::nullopt;
    }

    ApiResponseStatus e;
    e.set_status( ApiStatusCode::AS_BAD_REQUEST );
    e.set_error_message( fmt::format( "{} does not support EACH_LAYER_OWN_PAGE pagination mode",
                                      aCommandName ) );
    return e;
}


std::optional<ApiResponseStatus> ApplyBoardPlotSettings( const BoardPlotSettings& aSettings,
                                                         JOB_EXPORT_PCB_PLOT& aJob )
{
    for( int layer : aSettings.layers() )
    {
        PCB_LAYER_ID layerId = FromProtoEnum<PCB_LAYER_ID, board::types::BoardLayer>(
                static_cast<board::types::BoardLayer>( layer ) );

        if( layerId == PCB_LAYER_ID::UNDEFINED_LAYER )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "Board plot settings contain an invalid layer" );
            return e;
        }

        aJob.m_plotLayerSequence.push_back( layerId );
    }

    for( int layer : aSettings.common_layers() )
    {
        PCB_LAYER_ID layerId = FromProtoEnum<PCB_LAYER_ID, board::types::BoardLayer>(
                static_cast<board::types::BoardLayer>( layer ) );

        if( layerId == PCB_LAYER_ID::UNDEFINED_LAYER )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "Board plot settings contain an invalid common layer" );
            return e;
        }

        aJob.m_plotOnAllLayersSequence.push_back( layerId );
    }

    aJob.m_colorTheme = wxString::FromUTF8( aSettings.color_theme() );
    aJob.m_drawingSheet = wxString::FromUTF8( aSettings.drawing_sheet() );
    aJob.m_variant = wxString::FromUTF8( aSettings.variant() );

    aJob.m_mirror = aSettings.mirror();
    aJob.m_blackAndWhite = aSettings.black_and_white();
    aJob.m_negative = aSettings.negative();
    aJob.m_scale = aSettings.scale();

    aJob.m_sketchPadsOnFabLayers = aSettings.sketch_pads_on_fab_layers();
    aJob.m_hideDNPFPsOnFabLayers = aSettings.hide_dnp_footprints_on_fab_layers();
    aJob.m_sketchDNPFPsOnFabLayers = aSettings.sketch_dnp_footprints_on_fab_layers();
    aJob.m_crossoutDNPFPsOnFabLayers = aSettings.crossout_dnp_footprints_on_fab_layers();

    aJob.m_plotFootprintValues = aSettings.plot_footprint_values();
    aJob.m_plotRefDes = aSettings.plot_reference_designators();
    aJob.m_plotDrawingSheet = aSettings.plot_drawing_sheet();
    aJob.m_subtractSolderMaskFromSilk = aSettings.subtract_solder_mask_from_silk();
    aJob.m_plotPadNumbers = aSettings.plot_pad_numbers();

    aJob.m_drillShapeOption = FromProtoEnum<DRILL_MARKS>( aSettings.drill_marks() );

    aJob.m_useDrillOrigin = aSettings.use_drill_origin();
    aJob.m_checkZonesBeforePlot = aSettings.check_zones_before_plot();

    return std::nullopt;
}


HANDLER_RESULT<types::RunJobResponse> ExecuteBoardJob( PCB_CONTEXT* aContext, JOB& aJob )
{
    types::RunJobResponse response;
    WX_STRING_REPORTER reporter;

    if( !aContext || !aContext->GetKiway() )
    {
        response.set_status( types::JobStatus::JS_ERROR );
        response.set_message( "Internal error" );
        wxCHECK_MSG( false, response, "context missing valid kiway in ExecuteBoardJob?" );
        return response;
    }

    int exitCode = aContext->GetKiway()->ProcessJob( KIWAY::FACE_PCB, &aJob, &reporter );

    for( const JOB_OUTPUT& output : aJob.GetOutputs() )
        response.add_output_path( output.m_outputPath.ToUTF8() );

    if( exitCode == 0 )
    {
        response.set_status( types::JobStatus::JS_SUCCESS );
        return response;
    }

    response.set_status( types::JobStatus::JS_ERROR );
    response.set_message( fmt::format( "Board export job '{}' failed with exit code {}: {}",
                                       aJob.GetType(), exitCode,
                                       reporter.GetMessages().ToStdString() ) );
    return response;
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExport3D(
        const HANDLER_CONTEXT<RunBoardJobExport3D>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_3D job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    job.m_format = FromProtoEnum<JOB_EXPORT_PCB_3D::FORMAT>( aCtx.Request.format() );

    job.m_variant = wxString::FromUTF8( aCtx.Request.variant() );
    job.m_3dparams.m_NetFilter = wxString::FromUTF8( aCtx.Request.net_filter() );
    job.m_3dparams.m_ComponentFilter = wxString::FromUTF8( aCtx.Request.component_filter() );

    job.m_hasUserOrigin = aCtx.Request.has_user_origin();
    job.m_3dparams.m_Origin = VECTOR2D( aCtx.Request.origin().x_nm(), aCtx.Request.origin().y_nm() );

    job.m_3dparams.m_Overwrite = aCtx.Request.overwrite();
    job.m_3dparams.m_UseGridOrigin = aCtx.Request.use_grid_origin();
    job.m_3dparams.m_UseDrillOrigin = aCtx.Request.use_drill_origin();
    job.m_3dparams.m_UseDefinedOrigin = aCtx.Request.use_defined_origin() || aCtx.Request.has_user_origin();
    job.m_3dparams.m_UsePcbCenterOrigin = aCtx.Request.use_pcb_center_origin();

    job.m_3dparams.m_IncludeUnspecified = aCtx.Request.include_unspecified();
    job.m_3dparams.m_IncludeDNP = aCtx.Request.include_dnp();
    job.m_3dparams.m_SubstModels = aCtx.Request.substitute_models();

    job.m_3dparams.m_BoardOutlinesChainingEpsilon = aCtx.Request.board_outlines_chaining_epsilon();
    job.m_3dparams.m_BoardOnly = aCtx.Request.board_only();
    job.m_3dparams.m_CutViasInBody = aCtx.Request.cut_vias_in_body();
    job.m_3dparams.m_ExportBoardBody = aCtx.Request.export_board_body();
    job.m_3dparams.m_ExportComponents = aCtx.Request.export_components();
    job.m_3dparams.m_ExportTracksVias = aCtx.Request.export_tracks_and_vias();
    job.m_3dparams.m_ExportPads = aCtx.Request.export_pads();
    job.m_3dparams.m_ExportZones = aCtx.Request.export_zones();
    job.m_3dparams.m_ExportInnerCopper = aCtx.Request.export_inner_copper();
    job.m_3dparams.m_ExportSilkscreen = aCtx.Request.export_silkscreen();
    job.m_3dparams.m_ExportSoldermask = aCtx.Request.export_soldermask();
    job.m_3dparams.m_FuseShapes = aCtx.Request.fuse_shapes();
    job.m_3dparams.m_FillAllVias = aCtx.Request.fill_all_vias();
    job.m_3dparams.m_OptimizeStep = aCtx.Request.optimize_step();
    job.m_3dparams.m_ExtraPadThickness = aCtx.Request.extra_pad_thickness();

    job.m_vrmlUnits = FromProtoEnum<JOB_EXPORT_PCB_3D::VRML_UNITS>( aCtx.Request.vrml_units() );

    job.m_vrmlModelDir = wxString::FromUTF8( aCtx.Request.vrml_model_dir() );
    job.m_vrmlRelativePaths = aCtx.Request.vrml_relative_paths();

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportRender(
        const HANDLER_CONTEXT<RunBoardJobExportRender>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_PCB_RENDER job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    job.m_format = FromProtoEnum<JOB_PCB_RENDER::FORMAT>( aCtx.Request.format() );
    job.m_quality = FromProtoEnum<JOB_PCB_RENDER::QUALITY>( aCtx.Request.quality() );
    job.m_bgStyle = FromProtoEnum<JOB_PCB_RENDER::BG_STYLE>( aCtx.Request.background_style() );

    job.m_width = aCtx.Request.width();
    job.m_height = aCtx.Request.height();
    job.m_appearancePreset = aCtx.Request.appearance_preset();
    job.m_useBoardStackupColors = aCtx.Request.use_board_stackup_colors();

    job.m_side = FromProtoEnum<JOB_PCB_RENDER::SIDE>( aCtx.Request.side() );

    job.m_zoom = aCtx.Request.zoom();
    job.m_perspective = aCtx.Request.perspective();

    job.m_rotation = UnpackVector3D( aCtx.Request.rotation() );
    job.m_pan = UnpackVector3D( aCtx.Request.pan() );
    job.m_pivot = UnpackVector3D( aCtx.Request.pivot() );

    job.m_proceduralTextures = aCtx.Request.procedural_textures();
    job.m_floor = aCtx.Request.floor();
    job.m_antiAlias = aCtx.Request.anti_alias();
    job.m_postProcess = aCtx.Request.post_process();

    job.m_lightTopIntensity = UnpackVector3D( aCtx.Request.light_top_intensity() );
    job.m_lightBottomIntensity = UnpackVector3D( aCtx.Request.light_bottom_intensity() );
    job.m_lightCameraIntensity = UnpackVector3D( aCtx.Request.light_camera_intensity() );
    job.m_lightSideIntensity = UnpackVector3D( aCtx.Request.light_side_intensity() );
    job.m_lightSideElevation = aCtx.Request.light_side_elevation();

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportSvg(
        const HANDLER_CONTEXT<RunBoardJobExportSvg>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_SVG job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    if( std::optional<ApiResponseStatus> err = ApplyBoardPlotSettings( aCtx.Request.plot_settings(), job ) )
        return tl::unexpected( *err );

    job.m_fitPageToBoard = aCtx.Request.fit_page_to_board();
    job.m_precision = aCtx.Request.precision();

        if( std::optional<ApiResponseStatus> paginationError =
            ValidatePaginationModeForSingleOrPerFile( aCtx.Request.page_mode(),
                                                      "RunBoardJobExportSvg" ) )
    {
        return tl::unexpected( *paginationError );
    }

    job.m_genMode = FromProtoEnum<JOB_EXPORT_PCB_SVG::GEN_MODE>( aCtx.Request.page_mode() );

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportDxf(
        const HANDLER_CONTEXT<RunBoardJobExportDxf>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_DXF job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    if( std::optional<ApiResponseStatus> err = ApplyBoardPlotSettings( aCtx.Request.plot_settings(), job ) )
        return tl::unexpected( *err );

    job.m_plotGraphicItemsUsingContours = aCtx.Request.plot_graphic_items_using_contours();
    job.m_polygonMode = aCtx.Request.polygon_mode();

    if( std::optional<ApiResponseStatus> unitError =
            ValidateUnitsInchMm( aCtx.Request.units(), "RunBoardJobExportDxf" ) )
    {
        return tl::unexpected( *unitError );
    }

    job.m_dxfUnits = FromProtoEnum<JOB_EXPORT_PCB_DXF::DXF_UNITS>( aCtx.Request.units() );

        if( std::optional<ApiResponseStatus> paginationError =
            ValidatePaginationModeForSingleOrPerFile( aCtx.Request.page_mode(),
                                                      "RunBoardJobExportDxf" ) )
    {
        return tl::unexpected( *paginationError );
    }

    job.m_genMode = FromProtoEnum<JOB_EXPORT_PCB_DXF::GEN_MODE>( aCtx.Request.page_mode() );

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportPdf(
        const HANDLER_CONTEXT<RunBoardJobExportPdf>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_PDF job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    if( std::optional<ApiResponseStatus> err = ApplyBoardPlotSettings( aCtx.Request.plot_settings(), job ) )
        return tl::unexpected( *err );

    job.m_pdfFrontFPPropertyPopups = aCtx.Request.front_footprint_property_popups();
    job.m_pdfBackFPPropertyPopups = aCtx.Request.back_footprint_property_popups();
    job.m_pdfMetadata = aCtx.Request.include_metadata();
    job.m_pdfSingle = aCtx.Request.single_document();
    job.m_pdfBackgroundColor = wxString::FromUTF8( aCtx.Request.background_color() );

    job.m_pdfGenMode = FromProtoEnum<JOB_EXPORT_PCB_PDF::GEN_MODE>( aCtx.Request.page_mode() );

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportPs(
        const HANDLER_CONTEXT<RunBoardJobExportPs>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_PS job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    if( std::optional<ApiResponseStatus> err = ApplyBoardPlotSettings( aCtx.Request.plot_settings(), job ) )
        return tl::unexpected( *err );

    if( std::optional<ApiResponseStatus> paginationError =
            ValidatePaginationModeForSingleOrPerFile( aCtx.Request.page_mode(),
                                                      "RunBoardJobExportPs" ) )
    {
        return tl::unexpected( *paginationError );
    }

    job.m_genMode = FromProtoEnum<JOB_EXPORT_PCB_PS::GEN_MODE>( aCtx.Request.page_mode() );

    job.m_trackWidthCorrection = aCtx.Request.track_width_correction();
    job.m_XScaleAdjust = aCtx.Request.x_scale_adjust();
    job.m_YScaleAdjust = aCtx.Request.y_scale_adjust();
    job.m_forceA4 = aCtx.Request.force_a4();
    job.m_useGlobalSettings = aCtx.Request.use_global_settings();

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportGerbers(
        const HANDLER_CONTEXT<RunBoardJobExportGerbers>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    if( aCtx.Request.layers().empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "RunBoardJobExportGerbers requires at least one layer" );
        return tl::unexpected( e );
    }

    JOB_EXPORT_PCB_GERBERS job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    for( int layer : aCtx.Request.layers() )
    {
        PCB_LAYER_ID layerId =
                FromProtoEnum<PCB_LAYER_ID, board::types::BoardLayer>(
                        static_cast<board::types::BoardLayer>( layer ) );

        if( layerId == PCB_LAYER_ID::UNDEFINED_LAYER )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "RunBoardJobExportGerbers contains an invalid layer" );
            return tl::unexpected( e );
        }

        job.m_plotLayerSequence.push_back( layerId );
    }

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportDrill(
        const HANDLER_CONTEXT<RunBoardJobExportDrill>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_DRILL job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    job.m_format = FromProtoEnum<JOB_EXPORT_PCB_DRILL::DRILL_FORMAT>( aCtx.Request.format() );

    if( std::optional<ApiResponseStatus> unitError =
            ValidateUnitsInchMm( aCtx.Request.units(), "RunBoardJobExportDrill" ) )
    {
        return tl::unexpected( *unitError );
    }

    job.m_drillUnits = FromProtoEnum<JOB_EXPORT_PCB_DRILL::DRILL_UNITS>( aCtx.Request.units() );
    job.m_drillOrigin = FromProtoEnum<JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN>( aCtx.Request.origin() );
    job.m_zeroFormat = FromProtoEnum<JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT>( aCtx.Request.zeros_format() );

    if( aCtx.Request.has_excellon() )
    {
        const ExcellonFormatOptions& excellonOptions = aCtx.Request.excellon();

        if( excellonOptions.has_mirror_y() )
            job.m_excellonMirrorY = excellonOptions.mirror_y();

        if( excellonOptions.has_minimal_header() )
            job.m_excellonMinimalHeader = excellonOptions.minimal_header();

        if( excellonOptions.has_combine_pth_npth() )
            job.m_excellonCombinePTHNPTH = excellonOptions.combine_pth_npth();

        if( excellonOptions.has_route_oval_holes() )
            job.m_excellonOvalDrillRoute = excellonOptions.route_oval_holes();
    }

    if( aCtx.Request.map_format() != DrillMapFormat::DMF_UNKNOWN )
    {
        job.m_generateMap = true;
        job.m_mapFormat = FromProtoEnum<JOB_EXPORT_PCB_DRILL::MAP_FORMAT>( aCtx.Request.map_format() );
    }

    job.m_gerberPrecision = aCtx.Request.gerber_precision() == DrillGerberPrecision::DGP_4_5 ? 5 : 6;

    if( aCtx.Request.has_gerber_generate_tenting() )
        job.m_generateTenting = aCtx.Request.gerber_generate_tenting();

    if( aCtx.Request.report_format() != DrillReportFormat::DRF_UNKNOWN )
    {
        job.m_generateReport = true;

        if( aCtx.Request.has_report_filename() )
            job.m_reportPath = wxString::FromUTF8( aCtx.Request.report_filename() );
    }

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportPosition(
        const HANDLER_CONTEXT<RunBoardJobExportPosition>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_POS job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    if( aCtx.Request.has_use_drill_place_file_origin() )
        job.m_useDrillPlaceFileOrigin = aCtx.Request.use_drill_place_file_origin();

    job.m_smdOnly = aCtx.Request.smd_only();
    job.m_excludeFootprintsWithTh = aCtx.Request.exclude_footprints_with_th();
    job.m_excludeDNP = aCtx.Request.exclude_dnp();
    job.m_excludeBOM = aCtx.Request.exclude_from_bom();
    job.m_negateBottomX = aCtx.Request.negate_bottom_x();
    job.m_singleFile = aCtx.Request.single_file();
    job.m_nakedFilename = aCtx.Request.naked_filename();
    if( aCtx.Request.has_include_board_edge_for_gerber() )
        job.m_gerberBoardEdge = aCtx.Request.include_board_edge_for_gerber();

    job.m_variant = wxString::FromUTF8( aCtx.Request.variant() );

    job.m_side = FromProtoEnum<JOB_EXPORT_PCB_POS::SIDE>( aCtx.Request.side() );

    if( std::optional<ApiResponseStatus> unitError =
            ValidateUnitsInchMm( aCtx.Request.units(), "RunBoardJobExportPosition" ) )
    {
        return tl::unexpected( *unitError );
    }

    job.m_units = FromProtoEnum<JOB_EXPORT_PCB_POS::UNITS>( aCtx.Request.units() );
    job.m_format = FromProtoEnum<JOB_EXPORT_PCB_POS::FORMAT>( aCtx.Request.format() );

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportGencad(
        const HANDLER_CONTEXT<RunBoardJobExportGencad>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_GENCAD job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    job.m_flipBottomPads = aCtx.Request.flip_bottom_pads();
    job.m_useIndividualShapes = aCtx.Request.use_individual_shapes();
    job.m_storeOriginCoords = aCtx.Request.store_origin_coords();
    job.m_useDrillOrigin = aCtx.Request.use_drill_origin();
    job.m_useUniquePins = aCtx.Request.use_unique_pins();

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportIpc2581(
        const HANDLER_CONTEXT<RunBoardJobExportIpc2581>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_IPC2581 job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    job.m_drawingSheet = wxString::FromUTF8( aCtx.Request.drawing_sheet() );
    job.m_variant = wxString::FromUTF8( aCtx.Request.variant() );
    if( aCtx.Request.has_precision() )
        job.m_precision = aCtx.Request.precision();

    job.m_compress = aCtx.Request.compress();
    job.m_colInternalId = wxString::FromUTF8( aCtx.Request.internal_id_column() );
    job.m_colMfgPn = wxString::FromUTF8( aCtx.Request.manufacturer_part_number_column() );
    job.m_colMfg = wxString::FromUTF8( aCtx.Request.manufacturer_column() );
    job.m_colDistPn = wxString::FromUTF8( aCtx.Request.distributor_part_number_column() );
    job.m_colDist = wxString::FromUTF8( aCtx.Request.distributor_column() );
    job.m_bomRev = wxString::FromUTF8( aCtx.Request.bom_revision() );

    if( std::optional<ApiResponseStatus> unitError =
            ValidateUnitsInchMm( aCtx.Request.units(), "RunBoardJobExportIpc2581" ) )
    {
        return tl::unexpected( *unitError );
    }

    job.m_units = FromProtoEnum<JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS>( aCtx.Request.units() );
    job.m_version = FromProtoEnum<JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION>( aCtx.Request.version() );

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportIpcD356(
        const HANDLER_CONTEXT<RunBoardJobExportIpcD356>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_IPCD356 job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportODB(
        const HANDLER_CONTEXT<RunBoardJobExportODB>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_ODB job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    job.m_drawingSheet = wxString::FromUTF8( aCtx.Request.drawing_sheet() );
    job.m_variant = wxString::FromUTF8( aCtx.Request.variant() );
    if( aCtx.Request.has_precision() )
        job.m_precision = aCtx.Request.precision();

    if( std::optional<ApiResponseStatus> unitError =
            ValidateUnitsInchMm( aCtx.Request.units(), "RunBoardJobExportODB" ) )
    {
        return tl::unexpected( *unitError );
    }

    job.m_units = FromProtoEnum<JOB_EXPORT_PCB_ODB::ODB_UNITS>( aCtx.Request.units() );
    job.m_compressionMode = FromProtoEnum<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>( aCtx.Request.compression() );

    return ExecuteBoardJob( pcbContext(), job );
}


HANDLER_RESULT<types::RunJobResponse> API_HANDLER_PCB::handleRunBoardJobExportStats(
        const HANDLER_CONTEXT<RunBoardJobExportStats>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.job_settings().document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    JOB_EXPORT_PCB_STATS job;
    job.m_filename = pcbContext()->GetCurrentFileName();
    job.SetConfiguredOutputPath( wxString::FromUTF8( aCtx.Request.job_settings().output_path() ) );

    job.m_format = FromProtoEnum<JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT>( aCtx.Request.format() );

    if( std::optional<ApiResponseStatus> unitError =
            ValidateUnitsInchMm( aCtx.Request.units(), "RunBoardJobExportStats" ) )
    {
        return tl::unexpected( *unitError );
    }

    job.m_units = FromProtoEnum<JOB_EXPORT_PCB_STATS::UNITS>( aCtx.Request.units() );

    job.m_excludeFootprintsWithoutPads = aCtx.Request.exclude_footprints_without_pads();
    job.m_subtractHolesFromBoardArea = aCtx.Request.subtract_holes_from_board_area();
    job.m_subtractHolesFromCopperAreas = aCtx.Request.subtract_holes_from_copper_areas();

    return ExecuteBoardJob( pcbContext(), job );
}
