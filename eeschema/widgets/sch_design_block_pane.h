/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
#ifndef SCH_DESIGN_BLOCK_PANE_H
#define SCH_DESIGN_BLOCK_PANE_H

#include <widgets/design_block_pane.h>
#include <design_block_tree_model_adapter.h>
#include <widgets/html_window.h>
#include <widgets/wx_panel.h>
#include <wx/checkbox.h>
#include <wx/filedlgcustomize.h>
#include <eeschema_settings.h>


class SCH_EDIT_FRAME;
class PANEL_DESIGN_BLOCK_CHOOSER;


class SCH_DESIGN_BLOCK_PANE : public DESIGN_BLOCK_PANE
{
public:
    SCH_DESIGN_BLOCK_PANE( SCH_EDIT_FRAME* aParent, const LIB_ID* aPreselect, std::vector<LIB_ID>& aHistoryList );

    /* Handler for checkbox events */
    void OnCheckBox( wxCommandEvent& aEvent );
    void UpdateCheckboxes();

    void ProjectChanged() override;

protected:
    void setLabelsAndTooltips() override;

protected:
    wxCheckBox* m_repeatedPlacement;
    wxCheckBox* m_placeAsGroup;
    wxCheckBox* m_placeAsSheet;
    wxCheckBox* m_keepAnnotations;
};


// This is a helper class for the file dialog to allow the user to choose similar options
// as the design block chooser when importing a sheet.
class FILEDLG_IMPORT_SHEET_CONTENTS : public wxFileDialogCustomizeHook
{
public:
    FILEDLG_IMPORT_SHEET_CONTENTS( EESCHEMA_SETTINGS* aSettings );

    void AddCustomControls( wxFileDialogCustomize& customizer ) override;

    void TransferDataFromCustomControls() override;

private:
    EESCHEMA_SETTINGS* m_settings;

    wxFileDialogCheckBox* m_cbRepeatedPlacement;
    wxFileDialogCheckBox* m_cbPlaceAsGroup;
    wxFileDialogCheckBox* m_cbPlaceAsSheet;
    wxFileDialogCheckBox* m_cbKeepAnnotations;

    wxDECLARE_NO_COPY_CLASS( FILEDLG_IMPORT_SHEET_CONTENTS );
};

#endif
