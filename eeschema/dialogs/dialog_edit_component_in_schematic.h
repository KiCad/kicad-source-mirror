/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef _DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_H_
#define _DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_H_

#include <dialog_edit_component_in_schematic_base.h>

#include <fields_grid_table.h>


class SCH_EDIT_FRAME;
class LIB_PART;


/**
 * Dialog used to edit #SCH_COMPONENT objects in a schematic.
 *
 * This is derived from DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE which is maintained by
 * wxFormBuilder.
 */
class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC : public DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE
{
public:
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( SCH_EDIT_FRAME* aParent, SCH_COMPONENT* aComponent );
    ~DIALOG_EDIT_COMPONENT_IN_SCHEMATIC() override;

    SCH_EDIT_FRAME* GetParent();

private:
    wxConfigBase* m_config;

    SCH_COMPONENT* m_cmp;
    LIB_PART*      m_part;

    int      m_width;
    int      m_delayedFocusRow;
    int      m_delayedFocusColumn;
    wxString m_shownColumns;

    FIELDS_GRID_TABLE<SCH_FIELD>* m_fields;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    // event handlers
    void UpdateFieldsFromLibrary( wxCommandEvent& event ) override;
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnMoveUp( wxCommandEvent& event ) override;
    void OnMoveDown( wxCommandEvent& event ) override;
    void OnBrowseLibrary( wxCommandEvent& event ) override;
    void OnEditSpiceModel( wxCommandEvent& event ) override;
    void OnSizeGrid( wxSizeEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnCancelButtonClick( wxCommandEvent& event ) override;
    void OnInitDlg( wxInitDialogEvent& event ) override;

    void AdjustGridColumns( int aWidth );
};

#endif // _DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_H_
