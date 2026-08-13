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

#include <set>
#include <vector>

#include <dialogs/dialog_fields_table.h>
#include <fields_view_controls_grid_data_model.h>
#include <board.h>
#include <footprint_fields_data_model.h>

wxDECLARE_EVENT( EDA_EVT_CLOSE_DIALOG_FOOTPRINT_FIELDS_TABLE, wxCommandEvent );

class PCBNEW_SETTINGS;
class PCB_EDIT_FRAME;
class JOB_EXPORT_BOM;


class DIALOG_FOOTPRINT_FIELDS_TABLE : public DIALOG_FIELDS_TABLE, public BOARD_LISTENER
{
public:
    DIALOG_FOOTPRINT_FIELDS_TABLE( PCB_EDIT_FRAME* parent, JOB_EXPORT_BOM* aJob = nullptr );
    ~DIALOG_FOOTPRINT_FIELDS_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void SetupColumnProperties( int aCol ) override;
    void SetupAllColumnProperties();

    void setScope( FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE aScope );

    /**
     * Construct the rows of m_fieldsCtrl and the columns of m_dataModel from a union of all
     * field names in use.
     */
    void LoadFieldNames();

    void OnColMove( wxGridEvent& aEvent );
    void OnTableRangeSelected( wxGridRangeSelectEvent& aEvent );

    void OnScope( wxCommandEvent& event ) override;
    void OnMenu( wxCommandEvent& event ) override;

    void OnTableCellClick( wxGridEvent& event ) override;

    void OnExport( wxCommandEvent& aEvent ) override;
    void OnSaveAndContinue( wxCommandEvent& aEvent ) override;
    void OnCancel( wxCommandEvent& aEvent ) override;
    void OnOk( wxCommandEvent& aEvent ) override;
    void OnClose( wxCloseEvent& aEvent ) override;

    void OnOutputFileBrowseClicked( wxCommandEvent& event ) override;


    // BOARD listener event handlers
    void OnBoardItemsAdded( BOARD& aPcb, std::vector<BOARD_ITEM*>& aPcbItem ) override;
    void OnBoardItemsRemoved( BOARD& aPcb, std::vector<BOARD_ITEM*>& aPcbItem ) override;
    void OnBoardItemsChanged( BOARD& aPcb, std::vector<BOARD_ITEM*>& aPcbItem ) override;

    void OnBoardItemAdded( BOARD& aPcb, BOARD_ITEM* aPcbItem ) override
    {
        std::vector<BOARD_ITEM*> items{ aPcbItem };
        OnBoardItemsAdded( aPcb, items );
    }

    void OnBoardItemRemoved( BOARD& aPcb, BOARD_ITEM* aPcbItem ) override
    {
        std::vector<BOARD_ITEM*> items{ aPcbItem };
        OnBoardItemsRemoved( aPcb, items );
    }

    void OnBoardItemChanged( BOARD& aPcb, BOARD_ITEM* aPcbItem ) override
    {
        std::vector<BOARD_ITEM*> items{ aPcbItem };
        OnBoardItemsChanged( aPcb, items );
    }

    void OnBoardCompositeUpdate( BOARD& aPcb, std::vector<BOARD_ITEM*>& aAdded,
                                 std::vector<BOARD_ITEM*>& aRemoved,
                                 std::vector<BOARD_ITEM*>& aChanged ) override
    {
        // Be careful here because footprint exchanging will give a footprint
        // the same UUID but a different pointer. Remove first so we get
        // rid of the old footprint/pointer from the data model and the fp ref list
        // before we add the new-same-UUID footprint.
        if( !aRemoved.empty() ) OnBoardItemsRemoved( aPcb, aRemoved );
        if( !aAdded.empty() ) OnBoardItemsAdded( aPcb, aAdded );
        if( !aChanged.empty() ) OnBoardItemsChanged( aPcb, aChanged );
    }

    void OnCurrentSchematicSheetChanged( wxCommandEvent& aEvent );

    void EnableSelectionEvents();
    void DisableSelectionEvents();

    /**
     * Saves the current grid selection as a set of footprint full paths for later restoration.
     */
    std::set<KIID> SaveGridSelection();

    /**
     * Restores the grid selection from a previously saved set of footprint full paths.
     */
    void RestoreGridSelection( const std::set<KIID>& aKIIDs );

private:
    FIELDS_TABLE_DATA_MODEL_BASE* getDataModel() const override { return m_dataModel; }

    void doApplyBomPreset( const BOM_PRESET& aPreset ) override;
    void savePresetsToBoard();

    void onAddVariant( wxCommandEvent& aEvent ) override;
    void onDeleteVariant( wxCommandEvent& aEvent ) override;
    void onRenameVariant( wxCommandEvent& aEvent ) override;
    void onCopyVariant( wxCommandEvent& aEvent ) override;
    void onEditVariantDescription( wxCommandEvent& aEvent ) override;
    void onVariantSelectionChange( wxCommandEvent& aEvent ) override;

    void updateVariantButtonStates();

    wxString resolveVariant() const;

private:
    PCB_EDIT_FRAME* m_parent;

    FOOTPRINT_REFERENCE_LIST                 m_footprintsList;
    FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel = nullptr;

    BOARD_DESIGN_SETTINGS& m_boardSettings;

    JOB_EXPORT_BOM* m_job;

    bool m_aborted = false;

public:
    bool WasAborted() const { return m_aborted; }
};
