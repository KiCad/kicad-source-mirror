/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
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

#ifndef PANEL_EESCHEMA_BOM_PRESETS_H
#define PANEL_EESCHEMA_BOM_PRESETS_H

#include <panel_bom_presets_base.h>
#include <schematic_settings.h>

class SCH_EDIT_FRAME;


class PANEL_BOM_PRESETS : public PANEL_BOM_PRESETS_BASE
{
public:
    PANEL_BOM_PRESETS( wxWindow* aWindow, SCHEMATIC_SETTINGS& aSettings );

    void ImportBomPresetsFrom( SCHEMATIC_SETTINGS& aSettings );
    void ImportBomFmtPresetsFrom( SCHEMATIC_SETTINGS& aSettings );

protected:
    void OnDeleteBomPreset( wxCommandEvent& event ) override;
    void OnDeleteBomFmtPreset( wxCommandEvent& event ) override;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    void BuildGrid();

protected:
    SCHEMATIC_SETTINGS&         m_settings;
    std::vector<BOM_PRESET>     m_bomPresets;
    std::vector<BOM_FMT_PRESET> m_bomFmtPresets;
};


#endif //PANEL_EESCHEMA_DEFUALT_FIELDS_H
