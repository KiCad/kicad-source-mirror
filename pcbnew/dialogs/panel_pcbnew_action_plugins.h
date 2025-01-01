/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Andrew Lutsenko, anlutsenko at gmail dot com
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

#include "panel_pcbnew_action_plugins_base.h"

class PLUGINS_GRID_TRICKS;


class PANEL_PCBNEW_ACTION_PLUGINS : public PANEL_PCBNEW_ACTION_PLUGINS_BASE
{
    friend class PLUGINS_GRID_TRICKS;

public:
    PANEL_PCBNEW_ACTION_PLUGINS ( wxWindow* aParent );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;
    ~PANEL_PCBNEW_ACTION_PLUGINS() override;

    /**
     * Selects a whole row
     */
    void OnGridCellClick( wxGridEvent& event ) override;

    /**
     * Moves plugin up in the grid
     */
    void OnMoveUpButtonClick( wxCommandEvent& event ) override;

    /**
     * Moves plugin down in the grid
     */
    void OnMoveDownButtonClick( wxCommandEvent& event ) override;

    /**
     * Opens user's action plugin directory
     */
    void OnOpenDirectoryButtonClick( wxCommandEvent& event ) override;

    /**
     * Reloads plugins and updates grid
     */
    void OnReloadButtonClick( wxCommandEvent& event ) override;

    /**
     * Shows plugin import errors
     */
    void OnShowErrorsButtonClick( wxCommandEvent& event ) override;

private:

    enum GRID_COLUMNS
    {
        COLUMN_ACTION_NAME,
        COLUMN_VISIBLE,
        COLUMN_PLUGIN_NAME,
        COLUMN_DESCRIPTION,
        COLUMN_SETTINGS_IDENTIFIER,
    };

    wxBitmapBundle m_genericIcon;

    void SwapRows( int aRowA, int aRowB );
    void SelectRow( int aRow );
};

