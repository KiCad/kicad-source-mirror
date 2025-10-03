/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <dialog_lib_fields_table_base.h>
#include <sch_reference_list.h>
#include <lib_fields_data_model.h>

class SYMBOL_EDIT_FRAME;
class LIB_SYMBOL;


class DIALOG_LIB_FIELDS_TABLE : public DIALOG_LIB_FIELDS_TABLE_BASE
{
public:
    enum SCOPE : int
    {
        SCOPE_LIBRARY = 0,
        SCOPE_RELATED_SYMBOLS
    };

    DIALOG_LIB_FIELDS_TABLE( SYMBOL_EDIT_FRAME* parent, DIALOG_LIB_FIELDS_TABLE::SCOPE aScope );
    ~DIALOG_LIB_FIELDS_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ShowHideColumn( int aCol, bool aShow );

private:
    void UpdateFieldList();
    void AddField( const wxString& aFieldName, const wxString& aLabelValue, bool show, bool groupBy,
                   bool addedByUser = false, bool aIsCheckbox = false );
    void RemoveField( const wxString& fieldName );
    void RenameField( const wxString& oldName, const wxString& newName );
    void RegroupSymbols();

    void OnColSort( wxGridEvent& aEvent );
    void OnColMove( wxGridEvent& aEvent );
    void SetupColumnProperties( int aCol );
    void SetupAllColumnProperties();
    void setScope( SCOPE aScope );
    // Set bitmap and tooltip according to left panel visibility
    void setSideBarButtonLook( bool aIsLeftPanelCollapsed );

    void loadSymbols( const wxArrayString& aSymbolNames );

    void OnViewControlsCellChanged( wxGridEvent& aEvent ) override;
    void OnSizeViewControlsGrid( wxSizeEvent& event ) override;
    void OnAddField( wxCommandEvent& event ) override;
    void OnRenameField( wxCommandEvent& event ) override;
    void OnRemoveField( wxCommandEvent& event ) override;

    void OnFilterMouseMoved( wxMouseEvent& event ) override;
    void OnFilterText( wxCommandEvent& event ) override;
    void OnScope( wxCommandEvent& event ) override;
    void OnGroupSymbolsToggled( wxCommandEvent& event ) override;
    void OnRegroupSymbols( wxCommandEvent& event ) override;

    void OnTableValueChanged( wxGridEvent& event ) override;
    void OnTableCellClick( wxGridEvent& event ) override;
    void OnTableItemContextMenu( wxGridEvent& event ) override;
    void OnTableColSize( wxGridSizeEvent& event ) override;

    void OnSidebarToggle( wxCommandEvent& event ) override;
    void OnCancel( wxCommandEvent& event ) override;
    void OnOk( wxCommandEvent& event ) override;
    void OnApply( wxCommandEvent& event ) override;
    void OnClose( wxCloseEvent& event ) override;

private:
    SYMBOL_EDIT_FRAME*                 m_parent;
    SCOPE                              m_scope;

    VIEW_CONTROLS_GRID_DATA_MODEL*     m_viewControlsDataModel;

    LIB_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
    std::vector<LIB_SYMBOL*>           m_symbolsList;
};
