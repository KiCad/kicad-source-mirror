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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_API_HANDLER_PCB_H
#define KICAD_API_HANDLER_PCB_H

#include <api/api_handler_board.h>
#include <api/pcb_context.h>
#include <api/board/board_jobs.pb.h>
#include <api/common/commands/project_commands.pb.h>
#include <properties/property_mgr.h>

using namespace kiapi::board::jobs;


class PCB_EDIT_FRAME;
class PCB_TRACK;
class PROPERTY_BASE;


class API_HANDLER_PCB : public API_HANDLER_BOARD
{
public:
    API_HANDLER_PCB( PCB_EDIT_FRAME* aFrame );
    API_HANDLER_PCB( std::shared_ptr<PCB_CONTEXT> aContext, PCB_EDIT_FRAME* aFrame = nullptr );

private:
    typedef std::map<std::string, PROPERTY_BASE*> PROTO_PROPERTY_MAP;

    HANDLER_RESULT<commands::GetOpenDocumentsResponse> handleGetOpenDocuments(
            const HANDLER_CONTEXT<commands::GetOpenDocuments>& aCtx );

    HANDLER_RESULT<Empty> handleSaveDocument( const HANDLER_CONTEXT<commands::SaveDocument>& aCtx );

    HANDLER_RESULT<Empty> handleSaveCopyOfDocument(
            const HANDLER_CONTEXT<commands::SaveCopyOfDocument>& aCtx );

    HANDLER_RESULT<Empty> handleRevertDocument(
            const HANDLER_CONTEXT<commands::RevertDocument>& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse> handleGetItems(
            const HANDLER_CONTEXT<commands::GetItems>& aCtx );

    HANDLER_RESULT<BoardEnabledLayersResponse> handleSetBoardEnabledLayers(
            const HANDLER_CONTEXT<SetBoardEnabledLayers>& aCtx );

    HANDLER_RESULT<BoardDesignRulesResponse> handleGetBoardDesignRules(
            const HANDLER_CONTEXT<GetBoardDesignRules>& aCtx );

    HANDLER_RESULT<BoardDesignRulesResponse> handleSetBoardDesignRules(
            const HANDLER_CONTEXT<SetBoardDesignRules>& aCtx );

    HANDLER_RESULT<CustomRulesResponse> handleGetCustomDesignRules(
            const HANDLER_CONTEXT<GetCustomDesignRules>& aCtx );

    HANDLER_RESULT<CustomRulesResponse> handleSetCustomDesignRules(
            const HANDLER_CONTEXT<SetCustomDesignRules>& aCtx );

    HANDLER_RESULT<types::Vector2> handleGetBoardOrigin(
            const HANDLER_CONTEXT<GetBoardOrigin>& aCtx );

    HANDLER_RESULT<Empty> handleSetBoardOrigin( const HANDLER_CONTEXT<SetBoardOrigin>& aCtx );

    HANDLER_RESULT<BoardLayerNameResponse> handleGetBoardLayerName(
            const HANDLER_CONTEXT<GetBoardLayerName>& aCtx );

    HANDLER_RESULT<NetsResponse> handleGetNets( const HANDLER_CONTEXT<GetNets>& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse> handleGetConnectedItems(
            const HANDLER_CONTEXT<GetConnectedItems>& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse> handleGetItemsByNet(
            const HANDLER_CONTEXT<GetItemsByNet>& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse> handleGetItemsByNetClass(
            const HANDLER_CONTEXT<GetItemsByNetClass>& aCtx );

    HANDLER_RESULT<NetClassForNetsResponse> handleGetNetClassForNets(
            const HANDLER_CONTEXT<GetNetClassForNets>& aCtx );

    HANDLER_RESULT<Empty> handleRefillZones( const HANDLER_CONTEXT<RefillZones>& aCtx );

    HANDLER_RESULT<ImportNetlistResponse> handleImportNetlist(
            const HANDLER_CONTEXT<ImportNetlist>& aCtx );

    HANDLER_RESULT<BoardEditorAppearanceSettings> handleGetBoardEditorAppearanceSettings(
            const HANDLER_CONTEXT<GetBoardEditorAppearanceSettings>& aCtx );

    HANDLER_RESULT<Empty> handleSetBoardEditorAppearanceSettings(
            const HANDLER_CONTEXT<SetBoardEditorAppearanceSettings>& aCtx );

    HANDLER_RESULT<InjectDrcErrorResponse> handleInjectDrcError(
            const HANDLER_CONTEXT<InjectDrcError>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExport3D(
            const HANDLER_CONTEXT<RunBoardJobExport3D>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportRender(
            const HANDLER_CONTEXT<RunBoardJobExportRender>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportSvg(
            const HANDLER_CONTEXT<RunBoardJobExportSvg>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportDxf(
            const HANDLER_CONTEXT<RunBoardJobExportDxf>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportPdf(
            const HANDLER_CONTEXT<RunBoardJobExportPdf>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportPs(
            const HANDLER_CONTEXT<RunBoardJobExportPs>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportGerbers(
            const HANDLER_CONTEXT<RunBoardJobExportGerbers>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportDrill(
            const HANDLER_CONTEXT<RunBoardJobExportDrill>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportPosition(
            const HANDLER_CONTEXT<RunBoardJobExportPosition>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportGencad(
            const HANDLER_CONTEXT<RunBoardJobExportGencad>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportIpc2581(
            const HANDLER_CONTEXT<RunBoardJobExportIpc2581>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportIpcD356(
            const HANDLER_CONTEXT<RunBoardJobExportIpcD356>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportODB(
            const HANDLER_CONTEXT<RunBoardJobExportODB>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunBoardJobExportStats(
            const HANDLER_CONTEXT<RunBoardJobExportStats>& aCtx );

protected:
    kiapi::common::types::DocumentType thisDocumentType() const override
    {
        return kiapi::common::types::DOCTYPE_PCB;
    }

    tl::expected<bool, ApiResponseStatus> validateDocumentInternal( const DocumentSpecifier& aDocument ) const override;

    std::optional<TITLE_BLOCK*> getTitleBlock() override;

    std::optional<PAGE_INFO> getPageSettings() override;

    bool setPageSettings( const PAGE_INFO& aPageInfo ) override;

    wxString getDrawingSheetFileName() override;

    void setDrawingSheetFileName( const wxString& aFileName ) override;

    void onModified() override;

private:
    PCB_CONTEXT* pcbContext() const { return static_cast<PCB_CONTEXT*>( context() ); }

    PCB_EDIT_FRAME* frame() const;
};

#endif //KICAD_API_HANDLER_PCB_H
