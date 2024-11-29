/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <google/protobuf/empty.pb.h>

#include <api/api_handler_editor.h>
#include <api/board/board_commands.pb.h>
#include <api/board/board_types.pb.h>
#include <api/common/commands/editor_commands.pb.h>
#include <api/common/commands/project_commands.pb.h>
#include <kiid.h>
#include <properties/property_mgr.h>

using namespace kiapi;
using namespace kiapi::common;
using namespace kiapi::board::commands;

using google::protobuf::Empty;


class BOARD_COMMIT;
class BOARD_ITEM;
class BOARD_ITEM_CONTAINER;
class EDA_ITEM;
class PCB_EDIT_FRAME;
class PCB_TRACK;
class PROPERTY_BASE;


class API_HANDLER_PCB : public API_HANDLER_EDITOR
{
public:
    API_HANDLER_PCB( PCB_EDIT_FRAME* aFrame );

private:
    typedef std::map<std::string, PROPERTY_BASE*> PROTO_PROPERTY_MAP;

    static HANDLER_RESULT<std::unique_ptr<BOARD_ITEM>> createItemForType( KICAD_T aType,
                                                          BOARD_ITEM_CONTAINER* aContainer );

    HANDLER_RESULT<commands::RunActionResponse> handleRunAction( commands::RunAction& aMsg,
                                                                 const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::GetOpenDocumentsResponse> handleGetOpenDocuments(
            commands::GetOpenDocuments& aMsg, const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse> handleGetItems( commands::GetItems& aMsg,
                                                               const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<BoardStackupResponse> handleGetStackup( GetBoardStackup& aMsg,
                                                           const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<GraphicsDefaultsResponse> handleGetGraphicsDefaults( GetGraphicsDefaults& aMsg,
            const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::GetBoundingBoxResponse> handleGetBoundingBox( commands::GetBoundingBox& aMsg,
            const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<PadShapeAsPolygonResponse> handleGetPadShapeAsPolygon( GetPadShapeAsPolygon& aMsg,
            const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<types::TitleBlockInfo> handleGetTitleBlockInfo( commands::GetTitleBlockInfo& aMsg,
            const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::ExpandTextVariablesResponse>
    handleExpandTextVariables( commands::ExpandTextVariables& aMsg, const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<Empty> handleInteractiveMoveItems( InteractiveMoveItems& aMsg,
                                                      const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<NetsResponse> handleGetNets( GetNets& aMsg,
                                                const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<Empty> handleRefillZones( RefillZones& aMsg, const HANDLER_CONTEXT& aCtx );

protected:
    std::unique_ptr<COMMIT> createCommit() override;

    kiapi::common::types::DocumentType thisDocumentType() const override
    {
        return kiapi::common::types::DOCTYPE_PCB;
    }

    bool validateDocumentInternal( const DocumentSpecifier& aDocument ) const override;

    void deleteItemsInternal( std::map<KIID, ItemDeletionStatus>& aItemsToDelete,
                              const HANDLER_CONTEXT& aCtx ) override;

    std::optional<EDA_ITEM*> getItemFromDocument( const DocumentSpecifier& aDocument,
                                                  const KIID& aId ) override;

private:
    PCB_EDIT_FRAME* frame() const;

    void pushCurrentCommit( const HANDLER_CONTEXT& aCtx, const wxString& aMessage ) override;

    std::optional<BOARD_ITEM*> getItemById( const KIID& aId ) const;

    HANDLER_RESULT<types::ItemRequestStatus> handleCreateUpdateItemsInternal( bool aCreate,
            const HANDLER_CONTEXT& aCtx,
            const types::ItemHeader &aHeader,
            const google::protobuf::RepeatedPtrField<google::protobuf::Any>& aItems,
            std::function<void(commands::ItemStatus, google::protobuf::Any)> aItemHandler )
            override;
};

#endif //KICAD_API_HANDLER_PCB_H
