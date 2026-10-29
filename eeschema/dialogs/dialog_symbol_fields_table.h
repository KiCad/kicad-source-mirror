/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <set>
#include <vector>

#include <dialogs/dialog_fields_table.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>
#include <fields_view_controls_grid_data_model.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <symbol_fields_data_model.h>

wxDECLARE_EVENT( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, wxCommandEvent );

class SCH_EDIT_FRAME;
class JOB_EXPORT_BOM;
class MDW_SOURCE;
class PANEL_SYMBOL_FIELDS_WORKSPACE;
struct MDW_PROJECT_STATE;
class TEMPLATES;


class DIALOG_SYMBOL_FIELDS_TABLE : public DIALOG_FIELDS_TABLE, public SCHEMATIC_LISTENER
{
public:
    DIALOG_SYMBOL_FIELDS_TABLE( SCH_EDIT_FRAME* aParent, JOB_EXPORT_BOM* aJob = nullptr );
    ~DIALOG_SYMBOL_FIELDS_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    /// Take the schematic's variant list again and show its current variant.
    void ReloadVariants();

    /// Show the schematic's current variant, which the toolbar or an undo may have just changed.
    void FollowCurrentVariant();

    /// The schematic is about to be freed or the table closed, so stop listening to it and stop every request.
    void DetachFromSchematic();

private:
    wxGridCellEditor* createDatasheetEditor() override;
    wxGridCellEditor* createFootprintEditor() override;

    void setScope( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE aScope );
    void updateSelectionItems();

    /**
     * Construct the rows of m_fieldsCtrl and the columns of m_dataModel from a union of all
     * field names in use.
     */
    void LoadFieldNames();

    void OnTableSelectionChanged( const std::set<int>& aRows ) override;

    void OnScope( wxCommandEvent& aEvent ) override;
    void OnMenu( wxCommandEvent& aEvent ) override;

    void OnSaveAndContinue( wxCommandEvent& aEvent ) override;
    void OnCancel( wxCommandEvent& aEvent ) override;
    void OnOk( wxCommandEvent& aEvent ) override;
    void OnClose( wxCloseEvent& aEvent ) override;

    // Schematic listener event handlers
    void OnSchItemsAdded( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchItemsRemoved( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchItemsChanged( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchSheetChanged( SCHEMATIC& aSch ) override;
    void OnSchSelectionChanged( SCHEMATIC& aSch ) override;

    void OnPageChanged( wxNotebookEvent& aEvent ) override;

    /// Add the Workspace page when the project's source keeps a BOM
    void addWorkspacePage();
    /// aLink is why aDesign names no item, when it names none
    void showWorkspacePage( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_PROJECT_STATE& aDesign,
                            const MDW_RESULT& aLink = MDW_RESULT::Ok() );

    /// Probe the schematic for the symbols at aPaths, as a selection on the Edit page does
    void crossProbePaths( const std::vector<wxString>& aPaths );

    void crossProbe( const std::set<SCH_REFERENCE>& aRefs );

    void rebuildRowsPreservingSelection();
    void rebuildRowsPreservingSelection( const std::set<KIID_PATH>& aSavedSelection );

    FIELDS_TABLE_DATA_MODEL_BASE* getDataModel() const override { return m_dataModel; }

private:
    SCH_REFERENCE_LIST getSymbolReferences( SCH_SYMBOL* aSymbol, SCH_REFERENCE_LIST& aCachedRefs );
    SCH_REFERENCE_LIST getSheetSymbolReferences( SCH_SHEET& aSheet );

    void onAddVariant( wxCommandEvent& aEvent ) override;
    void onDeleteVariant( wxCommandEvent& aEvent ) override;
    void onRenameVariant( wxCommandEvent& aEvent ) override;
    void onCopyVariant( wxCommandEvent& aEvent ) override;
    void onEditVariantDescription( wxCommandEvent& aEvent ) override;
    void onVariantSelectionChange( wxCommandEvent& aEvent ) override;

    /// Show the list box's selected variant in the grid without touching the schematic.
    void showSelectedVariant();

    /// Select \a aVariant, or the default when it is gone, rebuilding rows unless \a aShown stays.
    void selectVariant( const wxString& aVariant, const wxString& aShown );

    void updateVariantButtonStates();

    wxString resolveVariant() const override;
    bool     resolveTextVar( wxString* aToken ) const override;

private:
    SCH_EDIT_FRAME*                    m_parent;

    SCH_REFERENCE_LIST                 m_symbolsList;
    SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel = nullptr;

    TEMPLATES&                         m_templateFieldNames;

    PANEL_SYMBOL_FIELDS_WORKSPACE*     m_workspacePage = nullptr;
    MDW_OP                             m_projectStateOp; ///< Destroyed with the dialog, which cancels it
    MDW_OP                             m_projectRefOp;

    bool m_aborted = false;

public:
    bool WasAborted() const { return m_aborted; }
};
