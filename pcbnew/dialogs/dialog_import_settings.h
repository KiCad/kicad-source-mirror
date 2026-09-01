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


#ifndef KICAD_DIALOG_IMPORT_SETTINGS_H
#define KICAD_DIALOG_IMPORT_SETTINGS_H

#include <vector>

#include "dialog_import_settings_base.h"


class PCB_EDIT_FRAME;


class DIALOG_IMPORT_SETTINGS : public DIALOG_IMPORT_SETTINGS_BASE
{
public:
    DIALOG_IMPORT_SETTINGS( wxWindow* aParent, PCB_EDIT_FRAME* aFrame );

    void OnBrowseClicked( wxCommandEvent& event ) override;
    void OnSelectAll( wxCommandEvent& event ) override;
    void OnCheckboxClicked( wxCommandEvent& event ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    wxString GetFilePath() const { return m_filePathCtrl->GetValue(); }

protected:
    PCB_EDIT_FRAME* m_frame;

private:
    void onFilePathChanged( wxCommandEvent& aEvent );

    bool anyOptionSelected() const;

    /**
     * Enable the "Import Settings" button, which needs both a source file and at least one
     * checked import selection.
     */
    void updateImportSettingsButton();

    /**
     * Label the toggle for what it will actually do to the current selection.
     */
    void updateSelectAllButton();

    /**
     * Every import selection checkbox, so that the enable, select-all and deselect-all paths
     * cannot drift apart as options are added.
     */
    std::vector<wxCheckBox*> m_options;
};

#endif //KICAD_DIALOG_IMPORT_SETTINGS_H
