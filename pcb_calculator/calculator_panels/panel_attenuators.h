/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_ATTENUATORS_H
#define PANEL_ATTENUATORS_H

#include "panel_attenuators_base.h"

#include <vector>

class ATTENUATOR;
class PCB_CALCULATOR_SETTINGS;

class PANEL_ATTENUATORS : public PANEL_ATTENUATORS_BASE
{
public:
    PANEL_ATTENUATORS( wxWindow* parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_ATTENUATORS();

    static wxString GetWindowName() { return wxT( "PANEL_ATTENUATORS" ); }

    wxRadioBox* GetAttenuatorsSelector() { return m_AttenuatorsSelection; }

    // Methods from CALCULATOR_PANEL that must be overriden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;

    void UpdateUI();    // Update bitmaps

    void OnAttenuatorSelection( wxCommandEvent& event ) override;
    void SetAttenuator( unsigned aIdx );
    void OnCalculateAttenuator( wxCommandEvent& event ) override;
    void TransfPanelDataToAttenuator();
    void TransfAttenuatorDataToPanel();
    void TransfAttenuatorResultsToPanel();

public:
    ATTENUATOR*              m_CurrAttenuator;
    std::vector<ATTENUATOR*> m_AttenuatorList;

};

#endif
