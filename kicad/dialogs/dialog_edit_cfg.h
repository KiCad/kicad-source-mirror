/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/filename.h>

class WX_GRID;

class DIALOG_EDIT_CFG : public wxDialog
{
public:
    DIALOG_EDIT_CFG( wxWindow* aParent );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void saveSettings();
    void OnCellChange( wxGridEvent& aEvent );
    void OnCellRightClick( wxGridEvent& aEvent );
    void OnResetDefault( wxCommandEvent& aEvent );
    void updateRowAppearance( int aRow );

private:
    WX_GRID*   m_grid;
    wxFileName m_cfgFile;
    int        m_contextRow;
};
