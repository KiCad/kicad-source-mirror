/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 Benjamin Chung (ckfinite@gmail.com)
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

#ifndef KICAD_API_HANDLER_FOOTPRINT_H
#define KICAD_API_HANDLER_FOOTPRINT_H

#include <api/api_handler_board.h>
#include <api/footprint_context.h>
#include <api/common/commands/project_commands.pb.h>


class FOOTPRINT;
class FOOTPRINT_EDIT_FRAME;


class API_HANDLER_FOOTPRINT : public API_HANDLER_BOARD
{
public:
    API_HANDLER_FOOTPRINT( FOOTPRINT_EDIT_FRAME* aFrame );
    API_HANDLER_FOOTPRINT( std::shared_ptr<FOOTPRINT_CONTEXT> aContext,
                           FOOTPRINT_EDIT_FRAME* aFrame = nullptr );

private:
    HANDLER_RESULT<Empty> handleOpenLibraryItem( const HANDLER_CONTEXT<commands::OpenLibraryItem>& aCtx );

    HANDLER_RESULT<commands::GetOpenDocumentsResponse> handleGetOpenDocuments(
            const HANDLER_CONTEXT<commands::GetOpenDocuments>& aCtx );

    HANDLER_RESULT<Empty> handleSaveDocument( const HANDLER_CONTEXT<commands::SaveDocument>& aCtx );

    HANDLER_RESULT<Empty> handleSaveCopyOfDocument(
            const HANDLER_CONTEXT<commands::SaveCopyOfDocument>& aCtx );

    HANDLER_RESULT<Empty> handleRevertDocument(
            const HANDLER_CONTEXT<commands::RevertDocument>& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse> handleGetItems(
            const HANDLER_CONTEXT<commands::GetItems>& aCtx );

protected:
    kiapi::common::types::DocumentType thisDocumentType() const override
    {
        return kiapi::common::types::DOCTYPE_FOOTPRINT;
    }

    tl::expected<bool, ApiResponseStatus> validateDocumentInternal( const DocumentSpecifier& aDocument ) const override;

    BOARD_ITEM_CONTAINER* getDefaultContainer() override;

private:
    FOOTPRINT_CONTEXT* footprintContext() const
    {
        return static_cast<FOOTPRINT_CONTEXT*>( context() );
    }

    FOOTPRINT_EDIT_FRAME* frame() const;

    HANDLER_RESULT<FOOTPRINT*> validateAndGetFootprint( const DocumentSpecifier& aDocument );
};


#endif //KICAD_API_HANDLER_FOOTPRINT_H
