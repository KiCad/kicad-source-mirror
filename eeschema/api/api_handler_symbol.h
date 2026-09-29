/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef KICAD_API_HANDLER_SYMBOL_H
#define KICAD_API_HANDLER_SYMBOL_H

#include <google/protobuf/empty.pb.h>

#include <api/api_handler_editor.h>
#include <api/common/commands/editor_commands.pb.h>
#include <api/common/commands/project_commands.pb.h>
#include <api/symbol_editor_context.h>

using google::protobuf::Empty;

class LIB_SYMBOL;
class SCH_ITEM;
class SYMBOL_EDIT_FRAME;


class API_HANDLER_SYMBOL : public API_HANDLER_EDITOR
{
public:
    API_HANDLER_SYMBOL( SYMBOL_EDIT_FRAME* aFrame );
    API_HANDLER_SYMBOL( std::shared_ptr<SYMBOL_EDITOR_CONTEXT> aContext,
                        SYMBOL_EDIT_FRAME* aFrame = nullptr );

protected:
    std::unique_ptr<COMMIT> createCommit() override;

    kiapi::common::types::DocumentType thisDocumentType() const override
    {
        return kiapi::common::types::DOCTYPE_SYMBOL;
    }

    const EDA_IU_SCALE& getIuScale() const override { return schIUScale; }

    tl::expected<bool, ApiResponseStatus> validateDocumentInternal( const DocumentSpecifier& aDocument ) const override;

    std::optional<EDA_ITEM*> getItemFromDocument( const DocumentSpecifier& aDocument, const KIID& aId ) override;

    HANDLER_RESULT<types::ItemRequestStatus> handleCreateUpdateItemsInternal( bool aCreate,
            const std::string& aClientName,
            const types::ItemHeader &aHeader,
            const google::protobuf::RepeatedPtrField<google::protobuf::Any>& aItems,
            std::function<void( commands::ItemStatus, google::protobuf::Any )> aItemHandler )
            override;

    void deleteItemsInternal( std::map<KIID, ItemDeletionStatus>& aItemsToDelete,
                              const std::string& aClientName ) override;

    void onModified() override;

    void pushCurrentCommit( const std::string& aClientName, const wxString& aMessage ) override;

    HANDLER_RESULT<commands::GetDocumentModifiedStateResponse>
    handleGetDocumentModifiedState( const HANDLER_CONTEXT<commands::GetDocumentModifiedState>& aCtx ) override;

    SYMBOL_EDITOR_CONTEXT* context() const { return m_context.get(); }

    LIB_SYMBOL* symbol() const { return context()->GetCurSymbol(); }

    SYMBOL_EDIT_FRAME* frame() const;

private:
    HANDLER_RESULT<Empty> handleOpenLibraryItem( const HANDLER_CONTEXT<commands::OpenLibraryItem>& aCtx );

    HANDLER_RESULT<commands::GetOpenDocumentsResponse> handleGetOpenDocuments(
            const HANDLER_CONTEXT<commands::GetOpenDocuments>& aCtx );

    HANDLER_RESULT<Empty> handleSaveDocument( const HANDLER_CONTEXT<commands::SaveDocument>& aCtx );

    HANDLER_RESULT<Empty> handleSaveCopyOfDocument(
            const HANDLER_CONTEXT<commands::SaveCopyOfDocument>& aCtx );

    HANDLER_RESULT<Empty> handleRevertDocument( const HANDLER_CONTEXT<commands::RevertDocument>& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse> handleGetItems(
            const HANDLER_CONTEXT<commands::GetItems>& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse> handleGetItemsById(
            const HANDLER_CONTEXT<commands::GetItemsById>& aCtx );

    std::shared_ptr<SYMBOL_EDITOR_CONTEXT> m_context;
    static std::set<KICAD_T>               s_allowedTypes;
};


#endif //KICAD_API_HANDLER_SYMBOL_H
