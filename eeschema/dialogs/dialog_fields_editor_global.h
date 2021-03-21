/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_FIELDS_EDITOR_GLOBAL_H
#define DIALOG_FIELDS_EDITOR_GLOBAL_H


#include <dialog_fields_editor_global_base.h>
#include <sch_reference_list.h>


class SCH_EDIT_FRAME;
class FIELDS_EDITOR_GRID_DATA_MODEL;


class DIALOG_FIELDS_EDITOR_GLOBAL : public DIALOG_FIELDS_EDITOR_GLOBAL_BASE
{
public:
    DIALOG_FIELDS_EDITOR_GLOBAL( SCH_EDIT_FRAME* parent );
    virtual ~DIALOG_FIELDS_EDITOR_GLOBAL();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void AddField( const wxString& displayName, const wxString& aCanonicalName, bool defaultShow,
                   bool defaultSortBy );
    void LoadFieldNames();

    void OnColSort( wxGridEvent& aEvent );

    void OnColumnItemToggled( wxDataViewEvent& event ) override;
    void OnGroupSymbolsToggled( wxCommandEvent& event ) override;
    void OnRegroupSymbols( wxCommandEvent& aEvent ) override;
    void OnTableValueChanged( wxGridEvent& event ) override;
    void OnTableCellClick( wxGridEvent& event ) override;
    void OnTableItemContextMenu( wxGridEvent& event ) override;
    void OnTableColSize( wxGridSizeEvent& event ) override;
    void OnSizeFieldList( wxSizeEvent& event ) override;
    void OnAddField( wxCommandEvent& event ) override;
    void OnSaveAndContinue( wxCommandEvent& aEvent ) override;
    void OnCancel( wxCommandEvent& event ) override;
    void OnClose( wxCloseEvent& event ) override;

    SCH_EDIT_FRAME*                m_parent;
    int                            m_showColWidth;
    int                            m_groupByColWidth;

    SCH_REFERENCE_LIST             m_symbolsList;
    FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
};

#endif /* DIALOG_FIELDS_EDITOR_GLOBAL_H */
