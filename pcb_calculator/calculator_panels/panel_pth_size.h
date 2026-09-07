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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "panel_pth_size_base.h"

#include <pth_hole_size.h>


class PCB_CALCULATOR_SETTINGS;


class PANEL_PTH_SIZE : public PANEL_PTH_SIZE_BASE
{
public:
    PANEL_PTH_SIZE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL,
                    const wxString& name = wxEmptyString );
    ~PANEL_PTH_SIZE();

    // Methods from CALCULATOR_PANEL that must be overridden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;

    // Event handlers from PANEL_PTH_SIZE_BASE
    void OnControlsChanged( wxCommandEvent& aEvent ) override;
    void OnValueChanged( wxCommandEvent& aEvent ) override;
    void OnPreviewSettingCb( wxCommandEvent& aEvent ) override;
    void OnCopyReportText( wxCommandEvent& aEvent ) override;

private:
    void populateStandardChoice();
    void recalculate();
    void updateLeadShapeUI();
    void updateStandard();
    void updateSummaryTable();
    void refreshPreview();

    const PTH_HOLE_SIZE_STANDARD* m_holeSizeStandard = nullptr; // Points into the standard registry

    PTH_LEAD_SHAPE m_leadShape = PTH_LEAD_SHAPE::ROUND;
    int            m_level = 1; // KiCad libraries use IPC level B by default

    // The last valid calculation, kept so the preview and report can be redrawn without
    // recalculating (e.g. when only a preview overlay setting changes).
    PTH_LEAD_DEF         m_lastLead;
    PTH_HOLE_SIZE_RESULT m_lastRange;
    int                  m_lastHoleIU = 0; // Recommended hole
    int                  m_lastPadIU = 0;  // Pad outer diameter
};
