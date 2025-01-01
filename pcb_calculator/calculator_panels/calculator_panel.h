/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ian McInerney
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CALCULATOR_PANEL_H_
#define CALCULATOR_PANEL_H_

#include <wx/panel.h>

class PCB_CALCULATOR_SETTINGS;

class CALCULATOR_PANEL : public wxPanel
{
public:
    CALCULATOR_PANEL( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos, const wxSize& aSize,
                      long aStyle, const wxString& aName )
        : wxPanel( aParent, aId, aPos, aSize, aStyle, aName )
    {}

    virtual ~CALCULATOR_PANEL() {}

    /**
     * Load the settings into the panel
     *
     *@param aCfg is the settings structure to load from
     */
    virtual void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) = 0;

    /**
     * Save the settings from the panel
     *
     *@param aCfg is the settings structure to save to
     */
    virtual void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) = 0;

    /**
     * Update UI elements of the panel when the theme changes to ensure the images
     * and fonts/colors are appropriate for the new theme.
     */
    virtual void ThemeChanged() = 0;
};

#endif
