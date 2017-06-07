/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file eeschema/dialogs/dialog_bom.cpp
 * @brief Dialog box for creating bom and other documents from generic netlist.
 */

#ifndef EESCHEMA_DIALOGS_DIALOG_BOM_EDITOR_H_
#define EESCHEMA_DIALOGS_DIALOG_BOM_EDITOR_H_

#include <wx/dataview.h>

#include <schframe.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <invoke_sch_dialog.h>
#include <dialog_bom_editor_base.h>
#include <class_netlist_object.h>
#include <sch_reference_list.h>
#include <vector>

#include <bom_table_model.h>

class DIALOG_BOM_EDITOR : public DIALOG_BOM_EDITOR_BASE
{

public:
    DIALOG_BOM_EDITOR( SCH_EDIT_FRAME* parent );
    virtual ~DIALOG_BOM_EDITOR();

private:
    //! Parent object (Schematic)
    SCH_EDIT_FRAME* m_parent;

    BOM_TABLE_MODEL::MODEL_PTR m_bom;

    void LoadComponents();

    void LoadColumnNames();
    void ReloadColumns();

    void ApplyAllChanges();

    // Checkbox event callbacks
    virtual void OnColumnItemToggled( wxDataViewEvent& event ) override;
    virtual void OnGroupComponentsToggled( wxCommandEvent& event ) override;

    virtual void OnRevertFieldChanges( wxCommandEvent& event ) override;

    virtual void OnApplyFieldChanges( wxCommandEvent& event ) override;

    virtual void OnRegroupComponents( wxCommandEvent& event ) override;

    // Called after a value in the table has changed
    virtual void OnTableValueChanged( wxDataViewEvent& event ) override;

    // Called when a cell is left-clicked
    virtual void OnTableItemActivated( wxDataViewEvent& event ) override;

    // Called when a cell is right-clicked
    virtual void OnTableItemContextMenu( wxDataViewEvent& event ) override;

    // Called when the dialog is closed
    virtual void OnDialogClosed( wxCloseEvent& event ) override;
    virtual void OnCloseButton( wxCommandEvent& event ) override;

    bool CanCloseDialog();

    void UpdateTitle( void );

    virtual void OnUpdateUI( wxUpdateUIEvent& event ) override;

};

#endif /* EESCHEMA_DIALOGS_DIALOG_BOM_EDITOR_H_ */
