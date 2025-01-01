/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef _PANEL_SETUP_PINMAP_H_
#define _PANEL_SETUP_PINMAP_H_

#include <sch_pin.h>        // For PINTYPE_COUNT definition
#include <erc/erc_settings.h>
#include "panel_setup_pinmap_base.h"


class SCH_EDIT_FRAME;
class SCHEMATIC;
class BITMAP_BUTTON;
class wxColour;


class PANEL_SETUP_PINMAP : public PANEL_SETUP_PINMAP_BASE
{
public:
    PANEL_SETUP_PINMAP( wxWindow* aWindow, SCH_EDIT_FRAME* aParent );
    ~PANEL_SETUP_PINMAP();

    void ImportSettingsFrom( PIN_ERROR aPinMap[][ELECTRICAL_PINTYPES_TOTAL] );

    void ResetPanel() override;

    void OnMouseEnter( wxMouseEvent& aEvent );
    void OnMouseLeave( wxMouseEvent& aEvent );
private:
    void changeErrorLevel( wxCommandEvent& event );
    void reBuildMatrixPanel();
    void setDRCMatrixButtonState( wxWindow *aButton, PIN_ERROR aState );

    DECLARE_EVENT_TABLE()

    SCH_EDIT_FRAME*  m_parent;
    SCHEMATIC*       m_schematic;
    wxColour         m_btnBackground;
    wxWindow*        m_buttonList[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL];
    bool             m_initialized;
};


#endif    // _PANEL_SETUP_PINMAP_H_
