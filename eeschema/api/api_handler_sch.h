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

#ifndef KICAD_API_HANDLER_SCH_H
#define KICAD_API_HANDLER_SCH_H

#include <api/api_handler_editor.h>
#include <api/sch_context.h>
#include <api/common/commands/editor_commands.pb.h>
#include <api/schematic/schematic_commands.pb.h>
#include <api/schematic/schematic_jobs.pb.h>
#include <kiid.h>

using namespace kiapi;
using namespace kiapi::common;

class SCH_EDIT_FRAME;
class SCH_ITEM;
class SCH_SHEET;


class API_HANDLER_SCH : public API_HANDLER_EDITOR
{
public:
    API_HANDLER_SCH( SCH_EDIT_FRAME* aFrame );
    API_HANDLER_SCH( std::shared_ptr<SCH_CONTEXT> aContext, SCH_EDIT_FRAME* aFrame = nullptr );

protected:
    std::unique_ptr<COMMIT> createCommit() override;

    kiapi::common::types::DocumentType thisDocumentType() const override
    {
        return kiapi::common::types::DOCTYPE_SCHEMATIC;
    }

    const EDA_IU_SCALE& getIuScale() const override { return schIUScale; }

    tl::expected<bool, ApiResponseStatus> validateDocumentInternal( const DocumentSpecifier& aDocument ) const override;

    std::optional<SCH_ITEM*> getItemById( const KIID& aId, SCH_SHEET_PATH* aPathOut = nullptr ) const;

    HANDLER_RESULT<std::unique_ptr<EDA_ITEM>> createItemForType( KICAD_T aType,
                                                                 EDA_ITEM* aContainer );

    HANDLER_RESULT<types::ItemRequestStatus> handleCreateUpdateItemsInternal( bool aCreate,
            const std::string& aClientName,
            const types::ItemHeader &aHeader,
            const google::protobuf::RepeatedPtrField<google::protobuf::Any>& aItems,
            std::function<void(commands::ItemStatus, google::protobuf::Any)> aItemHandler )
            override;

    void deleteItemsInternal( std::map<KIID, ItemDeletionStatus>& aItemsToDelete,
                              const std::string& aClientName ) override;

    std::optional<EDA_ITEM*> getItemFromDocument( const DocumentSpecifier& aDocument,
                                                  const KIID& aId ) override;

    std::optional<TITLE_BLOCK*> getTitleBlock() override;

    std::optional<PAGE_INFO> getPageSettings() override;

    bool setPageSettings( const PAGE_INFO& aPageInfo ) override;

    wxString getDrawingSheetFileName() override;

    void setDrawingSheetFileName( const wxString& aFileName ) override;

    void onModified() override;

private:
    HANDLER_RESULT<commands::GetOpenDocumentsResponse>
    handleGetOpenDocuments( const HANDLER_CONTEXT<commands::GetOpenDocuments>& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse> handleGetItems( const HANDLER_CONTEXT<commands::GetItems>& aCtx );

    HANDLER_RESULT<commands::GetItemsResponse>
    handleGetItemsById( const HANDLER_CONTEXT<commands::GetItemsById>& aCtx );

    HANDLER_RESULT<types::RunJobResponse>
    handleRunSchematicJobExportSvg( const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportSvg>& aCtx );

    HANDLER_RESULT<types::RunJobResponse>
    handleRunSchematicJobExportDxf( const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportDxf>& aCtx );

    HANDLER_RESULT<types::RunJobResponse>
    handleRunSchematicJobExportPdf( const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportPdf>& aCtx );

    HANDLER_RESULT<types::RunJobResponse>
    handleRunSchematicJobExportPs( const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportPs>& aCtx );

    HANDLER_RESULT<types::RunJobResponse> handleRunSchematicJobExportNetlist(
            const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportNetlist>& aCtx );

    HANDLER_RESULT<types::RunJobResponse>
    handleRunSchematicJobExportBOM( const HANDLER_CONTEXT<kiapi::schematic::jobs::RunSchematicJobExportBOM>& aCtx );

    HANDLER_RESULT<kiapi::schematic::types::SchematicHierarchyResponse>
    handleGetSchematicHierarchy( const HANDLER_CONTEXT<kiapi::schematic::types::GetSchematicHierarchy>& aCtx );

    void packSheetInstance( kiapi::schematic::types::SheetInstance* aInstance, SCH_SHEET_PATH& aPath,
                            SCH_SHEET* aSheet );

    HANDLER_RESULT<kiapi::schematic::types::SchematicNetlistResponse>
    handleGetSchematicNetlist( const HANDLER_CONTEXT<kiapi::schematic::types::GetSchematicNetlist>& aCtx );

    SCHEMATIC* schematic() const;

    void filterValidSchTypes( std::set<KICAD_T>& aTypeList );

    SCH_EDIT_FRAME*              m_frame;
    std::shared_ptr<SCH_CONTEXT> m_context;
    static std::set<KICAD_T>     s_allowedTypes;
};


#endif //KICAD_API_HANDLER_SCH_H
