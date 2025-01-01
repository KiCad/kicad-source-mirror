/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
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

#ifndef DIALOG_GRID_SETTINGS_H
#define DIALOG_GRID_SETTINGS_H

#include <dialog_grid_settings_base.h>
#include <widgets/unit_binder.h>

struct GRID;

class DIALOG_GRID_SETTINGS : public DIALOG_GRID_SETTINGS_BASE
{
public:
    DIALOG_GRID_SETTINGS( wxWindow* aParent, wxWindow* aEventSource, UNITS_PROVIDER* aProvider,
                          GRID& aGrid );
    ~DIALOG_GRID_SETTINGS() override = default;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

protected:
    void OnLinkedChecked( wxCommandEvent& event ) override;

private:
    UNITS_PROVIDER* m_unitsProvider;
    GRID&           m_grid;
    UNIT_BINDER     m_gridSizeX;
    UNIT_BINDER     m_gridSizeY;
};

#endif // DIALOG_GRID_SETTINGS_H
