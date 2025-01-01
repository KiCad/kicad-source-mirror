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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef KICAD_DIALOG_IMPORT_SETTINGS_H
#define KICAD_DIALOG_IMPORT_SETTINGS_H

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

    /**
     * Enable or disable the "Import Settings" button.
     *
     * This dialog defaults to all import selections cleared, and the "Import
     * Settings" button disabled.  The user must check at least one of the import
     * selection checkboxes for the "Import Settings" button to be enabled.
     *
     * @return the "Import Settings" button enable state.
     */
    bool UpdateImportSettingsButton();

    /**
     * Update "Select All" button label as appropriate
     */
    void UpdateSelectAllButton();

    wxString GetFilePath() { return m_filePath; }

protected:
    PCB_EDIT_FRAME* m_frame;
    static wxString m_filePath;

private:
    /**
     * Store state used to toggle button between "Select All" and "Deselect All"
     */
    bool m_showSelectAllOnBtn;
};

#endif //KICAD_DIALOG_IMPORT_SETTINGS_H
