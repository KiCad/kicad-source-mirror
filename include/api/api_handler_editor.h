/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_API_HANDLER_EDITOR_H
#define KICAD_API_HANDLER_EDITOR_H

#include <api/api_handler.h>
#include <api/common/commands/editor_commands.pb.h>
#include <commit.h>
#include <kiid.h>

using namespace kiapi::common;
using kiapi::common::types::DocumentSpecifier;
using kiapi::common::types::ItemRequestStatus;
using kiapi::common::commands::ItemDeletionStatus;

class EDA_BASE_FRAME;

/**
 * Base class for API handlers related to editor frames
 */
class API_HANDLER_EDITOR : public API_HANDLER
{
public:
    API_HANDLER_EDITOR( EDA_BASE_FRAME* aFrame = nullptr );

protected:
    /// If the header is valid, returns the item container
    HANDLER_RESULT<std::optional<KIID>> validateItemHeaderDocument(
            const kiapi::common::types::ItemHeader& aHeader );

    HANDLER_RESULT<bool> validateDocument( const DocumentSpecifier& aDocument );

    /**
     * Checks if the editor can accept commands
     * @return an error status if busy, std::nullopt if not busy
     */
    virtual std::optional<ApiResponseStatus> checkForBusy();

    HANDLER_RESULT<commands::BeginCommitResponse> handleBeginCommit( commands::BeginCommit& aMsg,
                                                                     const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::EndCommitResponse> handleEndCommit( commands::EndCommit& aMsg,
                                                                 const HANDLER_CONTEXT& aCtx );

    COMMIT* getCurrentCommit( const HANDLER_CONTEXT& aCtx );

    virtual void pushCurrentCommit( const HANDLER_CONTEXT& aCtx, const wxString& aMessage );

    HANDLER_RESULT<commands::CreateItemsResponse> handleCreateItems( commands::CreateItems& aMsg,
                                                                     const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::UpdateItemsResponse> handleUpdateItems( commands::UpdateItems& aMsg,
                                                                     const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::DeleteItemsResponse> handleDeleteItems( commands::DeleteItems& aMsg,
                                                                     const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::HitTestResponse> handleHitTest( commands::HitTest& aMsg,
                                                             const HANDLER_CONTEXT& aCtx );

    /**
     * Override this to create an appropriate COMMIT subclass for the frame in question
     * @return a new COMMIT, bound to the editor frame
     */
    virtual std::unique_ptr<COMMIT> createCommit() = 0;

    /**
     * Override this to specify which document type this editor handles
     */
    virtual kiapi::common::types::DocumentType thisDocumentType() const = 0;

    /**
     * @return true if the given document is valid for this editor and is currently open
     */
    virtual bool validateDocumentInternal( const DocumentSpecifier& aDocument ) const = 0;

    virtual HANDLER_RESULT<ItemRequestStatus> handleCreateUpdateItemsInternal( bool aCreate,
        const HANDLER_CONTEXT& aCtx,
        const types::ItemHeader &aHeader,
        const google::protobuf::RepeatedPtrField<google::protobuf::Any>& aItems,
        std::function<void( commands::ItemStatus, google::protobuf::Any )> aItemHandler ) = 0;

    virtual void deleteItemsInternal( std::map<KIID, ItemDeletionStatus>& aItemsToDelete,
                                      const HANDLER_CONTEXT& aCtx ) = 0;

    virtual std::optional<EDA_ITEM*> getItemFromDocument( const DocumentSpecifier& aDocument,
                                                          const KIID& aId ) = 0;

protected:
    std::map<std::string, std::pair<KIID, std::unique_ptr<COMMIT>>> m_commits;

    std::set<std::string> m_activeClients;

    EDA_BASE_FRAME* m_frame;
};

#endif //KICAD_API_HANDLER_EDITOR_H
