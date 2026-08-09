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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <widgets/unit_binder.h>
#include <panel_setup_symbol_parity_base.h>

class SCH_EDIT_FRAME;
class SCHEMATIC_SETTINGS;


class PANEL_SETUP_SYMBOL_PARITY : public PANEL_SETUP_SYMBOL_PARITY_BASE
{
public:
    PANEL_SETUP_SYMBOL_PARITY( wxWindow* aWindow, SCH_EDIT_FRAME* aFrame  );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ImportSettingsFrom( SYMBOL_PARITY_SETTINGS& aSettings );

private:
    SCH_EDIT_FRAME*    m_frame;
};


