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

#ifndef KICAD_API_HANDLER_BOARD_H
#define KICAD_API_HANDLER_BOARD_H

#include <memory>

#include <google/protobuf/empty.pb.h>

#include <api/api_handler_editor.h>
#include <api/board_context.h>
#include <api/board/board_commands.pb.h>
#include <api/board/board_types.pb.h>
#include <api/common/commands/editor_commands.pb.h>
#include <api/common/commands/project_commands.pb.h>
#include <kiid.h>

using namespace kiapi;
using namespace kiapi::common;
using namespace kiapi::board::commands;

using google::protobuf::Empty;


class BOARD_COMMIT;
class BOARD_ITEM;
class BOARD_ITEM_CONTAINER;
class EDA_ITEM;


/**
 * Common base class for API handlers that operate on a BOARD via a BOARD_CONTEXT.
 * Shared by both the PCB editor and the footprint editor.
 */
class API_HANDLER_BOARD : public API_HANDLER_EDITOR
{
public:
    API_HANDLER_BOARD( std::shared_ptr<BOARD_CONTEXT> aContext, EDA_BASE_FRAME* aFrame = nullptr );

protected:
    std::unique_ptr<COMMIT> createCommit() override;

    void deleteItemsInternal( std::map<KIID, ItemDeletionStatus>& aItemsToDelete,
                              const std::string& aClientName ) override;

    std::optional<EDA_ITEM*> getItemFromDocument( const DocumentSpecifier& aDocument,
                                                   const KIID& aId ) override;

    void pushCurrentCommit( const std::string& aClientName, const wxString& aMessage ) override;

    HANDLER_RESULT<types::ItemRequestStatus> handleCreateUpdateItemsInternal( bool aCreate,
            const std::string& aClientName,
            const types::ItemHeader &aHeader,
            const google::protobuf::RepeatedPtrField<google::protobuf::Any>& aItems,
            std::function<void(commands::ItemStatus, google::protobuf::Any)> aItemHandler )
            override;

    BOARD_CONTEXT* context() const { return m_context.get(); }

    BOARD* board() const { return context()->GetBoard(); }

    PROJECT& project() const { return context()->Prj(); }

    TOOL_MANAGER* toolManager() const { return context()->GetToolManager(); }

    std::optional<BOARD_ITEM*> getItemById( const KIID& aId ) const;

    static HANDLER_RESULT<std::unique_ptr<BOARD_ITEM>> createItemForType( KICAD_T aType,
                                                          BOARD_ITEM_CONTAINER* aContainer );

    virtual BOARD_ITEM_CONTAINER* getDefaultContainer();

    std::optional<ApiResponseStatus> checkForHeadless( const std::string& aCommandName ) const;

    std::vector<KICAD_T> parseRequestedItemTypes(
            const google::protobuf::RepeatedField<int>& aTypes );

private:
    HANDLER_RESULT<commands::RunActionResponse> handleRunAction(
            const HANDLER_CONTEXT<commands::RunAction>& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse> handleGetItemsById(
            const HANDLER_CONTEXT<commands::GetItemsById>& aCtx );

    HANDLER_RESULT<commands::SelectionResponse> handleGetSelection(
            const HANDLER_CONTEXT<commands::GetSelection>& aCtx );

    HANDLER_RESULT<Empty> handleClearSelection(
            const HANDLER_CONTEXT<commands::ClearSelection>& aCtx );

    HANDLER_RESULT<commands::SelectionResponse> handleAddToSelection(
            const HANDLER_CONTEXT<commands::AddToSelection>& aCtx );

    HANDLER_RESULT<commands::SelectionResponse> handleRemoveFromSelection(
            const HANDLER_CONTEXT<commands::RemoveFromSelection>& aCtx );

    HANDLER_RESULT<BoardStackupResponse> handleGetStackup(
            const HANDLER_CONTEXT<GetBoardStackup>& aCtx );

    HANDLER_RESULT<BoardEnabledLayersResponse> handleGetBoardEnabledLayers(
            const HANDLER_CONTEXT<GetBoardEnabledLayers>& aCtx );

    HANDLER_RESULT<GraphicsDefaultsResponse> handleGetGraphicsDefaults(
            const HANDLER_CONTEXT<GetGraphicsDefaults>& aCtx );

    HANDLER_RESULT<commands::GetBoundingBoxResponse> handleGetBoundingBox(
            const HANDLER_CONTEXT<commands::GetBoundingBox>& aCtx );

    HANDLER_RESULT<PadShapeAsPolygonResponse> handleGetPadShapeAsPolygon(
            const HANDLER_CONTEXT<GetPadShapeAsPolygon>& aCtx );

    HANDLER_RESULT<PadstackPresenceResponse> handleCheckPadstackPresenceOnLayers(
            const HANDLER_CONTEXT<CheckPadstackPresenceOnLayers>& aCtx );

    HANDLER_RESULT<commands::ExpandTextVariablesResponse> handleExpandTextVariables(
            const HANDLER_CONTEXT<commands::ExpandTextVariables>& aCtx );

    HANDLER_RESULT<Empty> handleInteractiveMoveItems(
            const HANDLER_CONTEXT<InteractiveMoveItems>& aCtx );

    HANDLER_RESULT<commands::SavedDocumentResponse> handleSaveDocumentToString(
            const HANDLER_CONTEXT<commands::SaveDocumentToString>& aCtx );

    HANDLER_RESULT<commands::SavedSelectionResponse> handleSaveSelectionToString(
            const HANDLER_CONTEXT<commands::SaveSelectionToString>& aCtx );

    HANDLER_RESULT<commands::CreateItemsResponse> handleParseAndCreateItemsFromString(
            const HANDLER_CONTEXT<commands::ParseAndCreateItemsFromString>& aCtx );

    HANDLER_RESULT<BoardLayers> handleGetVisibleLayers(
            const HANDLER_CONTEXT<GetVisibleLayers>& aCtx );

    HANDLER_RESULT<Empty> handleSetVisibleLayers(
            const HANDLER_CONTEXT<SetVisibleLayers>& aCtx );

    HANDLER_RESULT<BoardLayerResponse> handleGetActiveLayer(
            const HANDLER_CONTEXT<GetActiveLayer>& aCtx );

    HANDLER_RESULT<Empty> handleSetActiveLayer(
            const HANDLER_CONTEXT<SetActiveLayer>& aCtx );

    std::shared_ptr<BOARD_CONTEXT> m_context;
};

#endif //KICAD_API_HANDLER_BOARD_H
