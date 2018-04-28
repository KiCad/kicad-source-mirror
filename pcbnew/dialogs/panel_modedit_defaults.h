/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_PANEL_MODEDIT_DEFAULTS_H
#define KICAD_PANEL_MODEDIT_DEFAULTS_H

#include <panel_modedit_defaults_base.h>
#include <board_design_settings.h>
#include <widgets/unit_binder.h>

class FOOTPRINT_EDIT_FRAME;


class PANEL_MODEDIT_DEFAULTS : public PANEL_MODEDIT_DEFAULTS_BASE
{
    BOARD_DESIGN_SETTINGS   m_brdSettings;
    FOOTPRINT_EDIT_FRAME*   m_frame;
    PAGED_DIALOG*           m_Parent;

public:
    PANEL_MODEDIT_DEFAULTS( FOOTPRINT_EDIT_FRAME* aFrame, PAGED_DIALOG* aParent );
    ~PANEL_MODEDIT_DEFAULTS() override;

private:
    int getGridValue( int aRow, int aCol );

    bool validateData();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
};




#endif //KICAD_PANEL_MODEDIT_DEFAULTS_H

