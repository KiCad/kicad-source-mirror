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

#include <api/api_handler.h>

#include <api/common/commands/editor_commands.pb.h>

#include <properties/property_mgr.h>

using namespace kiapi;
using namespace kiapi::common;

using google::protobuf::Empty;


class BOARD_COMMIT;
class BOARD_ITEM;
class BOARD_ITEM_CONTAINER;
class EDA_ITEM;
class PCB_EDIT_FRAME;
class PCB_TRACK;
class PROPERTY_BASE;


class API_HANDLER_PCB : public API_HANDLER
{
public:
    API_HANDLER_PCB( PCB_EDIT_FRAME* aFrame );

private:
    typedef std::map<std::string, PROPERTY_BASE*> PROTO_PROPERTY_MAP;

    static std::unique_ptr<BOARD_ITEM> createItemForType( KICAD_T aType,
                                                          BOARD_ITEM_CONTAINER* aContainer );

    HANDLER_RESULT<commands::RunActionResponse> handleRunAction( commands::RunAction& aMsg );

    HANDLER_RESULT<commands::GetOpenDocumentsResponse> handleGetOpenDocuments(
            commands::GetOpenDocuments& aMsg );

    HANDLER_RESULT<commands::BeginCommitResponse> handleBeginCommit( commands::BeginCommit& aMsg );
    HANDLER_RESULT<commands::EndCommitResponse> handleEndCommit( commands::EndCommit& aMsg );

    HANDLER_RESULT<commands::CreateItemsResponse> handleCreateItems( commands::CreateItems& aMsg );
    HANDLER_RESULT<commands::GetItemsResponse> handleGetItems( commands::GetItems& aMsg );
    HANDLER_RESULT<commands::UpdateItemsResponse> handleUpdateItems( commands::UpdateItems& aMsg );
    HANDLER_RESULT<commands::DeleteItemsResponse> handleDeleteItems( commands::DeleteItems& aMsg );

private:

    bool validateItemHeaderDocument( const common::types::ItemHeader& aHeader );

    BOARD_COMMIT* getCurrentCommit();

    void pushCurrentCommit( const std::string& aMessage );

    PCB_EDIT_FRAME* m_frame;

    std::unique_ptr<BOARD_COMMIT> m_commit;

    bool m_transactionInProgress;
};

#endif //KICAD_API_HANDLER_PCB_H
