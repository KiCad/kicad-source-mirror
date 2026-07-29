/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) Mike Williams
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <dialog_fields_table_base.h>

class KICOMMON_API DIALOG_FIELDS_TABLE : public DIALOG_FIELDS_TABLE_BASE
{
public:
    using DIALOG_FIELDS_TABLE_BASE::DIALOG_FIELDS_TABLE_BASE;

    void ShowEditTab();
    void ShowExportTab();

    /**
     * Derive the default BOM output file name from the input file name by swapping the
     * extension to CSV. Returns an empty string when the input has no name (unsaved), so
     * callers can distinguish "use the default" from "no destination is available".
     */
    static wxString GetDefaultBomFileName( const wxString& aInputFileName );

protected:
    // Set bitmap and tooltip according to left panel visibility
    void setSideBarButtonLook( bool aIsLeftPanelCollapsed );

    void OnTableValueChanged( wxGridEvent& event ) override;
    void OnTableColSize( wxGridSizeEvent& event ) override;
    void OnSizeViewControlsGrid( wxSizeEvent& event ) override;

    void OnFilterMouseMoved( wxMouseEvent& event ) override;
};
