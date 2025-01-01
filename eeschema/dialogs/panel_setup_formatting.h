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

#ifndef KICAD_PANEL_SETUP_FORMATTING_H
#define KICAD_PANEL_SETUP_FORMATTING_H

#include <widgets/unit_binder.h>
#include "panel_setup_formatting_base.h"

class SCH_EDIT_FRAME;
class SCHEMATIC_SETTINGS;
class GAL_OPTIONS_PANEL;


class PANEL_SETUP_FORMATTING : public PANEL_SETUP_FORMATTING_BASE
{
public:
    PANEL_SETUP_FORMATTING( wxWindow* aWindow, SCH_EDIT_FRAME* aFrame  );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ImportSettingsFrom( SCHEMATIC_SETTINGS& aSettings );

protected:
    void onCheckBoxIref( wxCommandEvent& event ) override;

private:
    SCH_EDIT_FRAME*    m_frame;

    UNIT_BINDER        m_textSize;
    UNIT_BINDER        m_lineWidth;

    UNIT_BINDER        m_pinSymbolSize;
    UNIT_BINDER        m_connectionGridSize;
};


#endif //KICAD_PANEL_SETUP_FORMATTING_H
