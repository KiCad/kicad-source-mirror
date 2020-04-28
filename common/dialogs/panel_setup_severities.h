/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef KICAD_PANEL_SETUP_SEVERITIES_H
#define KICAD_PANEL_SETUP_SEVERITIES_H

#include <map>
#include <wx/panel.h>


class PAGED_DIALOG;
class EDA_DRAW_FRAME;
class wxRadioBox;


class PANEL_SETUP_SEVERITIES : public wxPanel
{
private:
    std::map<int, int>& m_severities;
    int                 m_firstErrorCode;
    int                 m_lastErrorCode;
    int                 m_pinMapSpecialCase;

    std::map<int, wxRadioButton*[4]> m_buttonMap;   // map from DRC error code to button group

public:
    PANEL_SETUP_SEVERITIES( PAGED_DIALOG* aParent, RC_ITEM& aDummyItem,
                            std::map<int, int>& aSeverities, int aFirstError, int aLastError,
                            int aPinMapSpecialCase = -1 );

    void ImportSettingsFrom( std::map<int, int>& aSettings );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
};

#endif //KICAD_PANEL_SETUP_SEVERITIES_H
