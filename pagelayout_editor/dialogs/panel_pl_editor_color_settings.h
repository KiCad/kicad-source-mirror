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

#ifndef KICAD_PANEL_PL_EDITOR_COLOR_SETTINGS_H
#define KICAD_PANEL_PL_EDITOR_COLOR_SETTINGS_H

#include "panel_pl_editor_color_settings_base.h"


class PANEL_PL_EDITOR_COLOR_SETTINGS : public PANEL_PL_EDITOR_COLOR_SETTINGS_BASE
{
public:
    PANEL_PL_EDITOR_COLOR_SETTINGS( wxWindow* aParent );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ResetPanel() override;
};


#endif //KICAD_PANEL_PL_EDITOR_COLOR_SETTINGS_H
