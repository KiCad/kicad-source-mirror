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

#ifndef DIALOG_LIB_FIELDS_H
#define DIALOG_LIB_FIELDS_H

#include <dialog_lib_fields_base.h>
#include <sch_reference_list.h>

class SYMBOL_EDIT_FRAME;
class LIB_FIELDS_EDITOR_GRID_DATA_MODEL;
class LIB_SYMBOL;

class DIALOG_LIB_FIELDS : public DIALOG_LIB_FIELDS_BASE
{
public:
    DIALOG_LIB_FIELDS( SYMBOL_EDIT_FRAME* parent, wxString libId );
    ~DIALOG_LIB_FIELDS() override;

    void OnInit();

protected:
    void OnClose( wxCloseEvent& event ) override;
    void OnColumnItemToggled( wxDataViewEvent& event ) override;
    void OnFieldsCtrlSelectionChanged( wxDataViewEvent& event ) override;
    void OnSizeFieldList( wxSizeEvent& event ) override;
    void OnAddField( wxCommandEvent& event ) override;
    void OnRenameField( wxCommandEvent& event ) override;
    void OnRemoveField( wxCommandEvent& event ) override;
    void OnFilterMouseMoved( wxMouseEvent& event ) override;
    void OnFilterText( wxCommandEvent& event ) override;
    void OnRegroupSymbols( wxCommandEvent& event ) override;
    void OnTableValueChanged( wxGridEvent& event ) override;
    void OnTableCellClick( wxGridEvent& event ) override;
    void OnTableItemContextMenu( wxGridEvent& event ) override;
    void OnTableColSize( wxGridSizeEvent& event ) override;
    void OnCancel( wxCommandEvent& event ) override;
    void OnOk( wxCommandEvent& event ) override;
    void OnApply( wxCommandEvent& event ) override;
    bool TransferDataFromWindow() override;

private:
    void UpdateFieldList();
    void AddField( const wxString& aFieldName, const wxString& aLabelValue, bool show, bool groupBy, bool addedByUser = false, bool aIsCheckbox = false );
    void RemoveField( const wxString& fieldName );
    void RenameField( const wxString& oldName, const wxString& newName );
    void RegroupSymbols();

    void OnColSort( wxGridEvent& aEvent );
    void OnColMove( wxGridEvent& aEvent );
    void OnColLabelChange( wxDataViewEvent& aEvent );
    void SetupColumnProperties( int aCol );
    void SetupAllColumnProperties();

    void loadSymbols();

    wxString                           m_libId;
    SYMBOL_EDIT_FRAME*                 m_parent;

    int                                m_fieldNameColWidth;
    int                                m_labelColWidth;
    int                                m_showColWidth;
    int                                m_groupByColWidth;

    LIB_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
    std::vector<LIB_SYMBOL*>           m_symbolsList;
};


#endif // DIALOG_LIB_FIELDS_H
