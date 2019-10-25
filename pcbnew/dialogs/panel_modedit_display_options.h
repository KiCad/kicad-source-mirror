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


#ifndef KICAD_PANEL_MODEDIT_DISPLAY_OPTIONS_H
#define KICAD_PANEL_MODEDIT_DISPLAY_OPTIONS_H

#include <wx/panel.h>

class GAL_OPTIONS_PANEL;
class FOOTPRINT_EDIT_FRAME;
class PAGED_DIALOG;

class PANEL_MODEDIT_DISPLAY_OPTIONS : public wxPanel
{
public:
    PANEL_MODEDIT_DISPLAY_OPTIONS( FOOTPRINT_EDIT_FRAME* aFrame, PAGED_DIALOG* aParent );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    FOOTPRINT_EDIT_FRAME* m_frame;
    GAL_OPTIONS_PANEL*    m_galOptsPanel;
};

#endif //KICAD_PANEL_MODEDIT_DISPLAY_OPTIONS_H
