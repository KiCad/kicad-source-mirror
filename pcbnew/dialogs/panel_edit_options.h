/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PANEL_EDIT_OPTIONS_H
#define PANEL_EDIT_OPTIONS_H

#include <widgets/unit_binder.h>
#include "panel_edit_options_base.h"


class FOOTPRINT_EDITOR_SETTINGS;
class PCBNEW_SETTINGS;
class EDA_BASE_FRAME;


class PANEL_EDIT_OPTIONS : public PANEL_EDIT_OPTIONS_BASE
{
public:
    PANEL_EDIT_OPTIONS( wxWindow* aParent, UNITS_PROVIDER* aUnitsProvider, wxWindow* aEventSource,
                        bool isFootprintEditor );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ResetPanel() override;

private:
    void loadFPSettings( FOOTPRINT_EDITOR_SETTINGS* aCfg );
    void loadPCBSettings( PCBNEW_SETTINGS* aCfg );

private:
    bool        m_isFootprintEditor;

    UNIT_BINDER m_rotationAngle;
};


#endif	// PANEL_EDIT_OPTIONS_H
