/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_COLOR_SETTINGS_H
#define PANEL_COLOR_SETTINGS_H

#include <panel_color_settings_base.h>


class PANEL_COLOR_SETTINGS : public PANEL_COLOR_SETTINGS_BASE
{
public:
    PANEL_COLOR_SETTINGS( wxWindow* aParent );

    ~PANEL_COLOR_SETTINGS() = default;

protected:
    void OnBtnOpenThemeFolderClicked( wxCommandEvent& event ) override;
};


#endif
