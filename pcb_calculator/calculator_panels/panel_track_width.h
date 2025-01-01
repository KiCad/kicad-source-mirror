/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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

#ifndef PANEL_TRACK_WIDTH_H
#define PANEL_TRACK_WIDTH_H

#include "panel_track_width_base.h"

class PCB_CALCULATOR_SETTINGS;


class PANEL_TRACK_WIDTH : public PANEL_TRACK_WIDTH_BASE
{
public:
    PANEL_TRACK_WIDTH( wxWindow* parent, wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_TRACK_WIDTH();

    // Methods from CALCULATOR_PANEL that must be overridden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;

    /**
     * Update the calculations the user changes the general parameters.
     */
    void OnTWParametersChanged( wxCommandEvent& event ) override;

    /**
     * Update the calculations when the user changes the desired maximum current.
     */
    void OnTWCalculateFromCurrent( wxCommandEvent& event ) override;

    /**
     * Update the calculations when the user changes the desired external trace width.
     */
    void OnTWCalculateFromExtWidth( wxCommandEvent& event ) override;

    /**
     * Update the calculations when the user changes the desired internal trace width.
     */
    void OnTWCalculateFromIntWidth( wxCommandEvent& event ) override;

    /**
     * Update the calculations when the user clicks the reset button.
     */
    void OnTWResetButtonClick( wxCommandEvent& event ) override;

    /**
     * Calculate track width required based on given current and temperature rise.
     */
    double TWCalculateWidth( double aCurrent, double aThickness, double aDeltaT_C,
                             bool aUseInternalLayer );

    /**
     * Calculate maximum current based on given width and temperature rise.
     */
    double TWCalculateCurrent( double aWidth, double aThickness, double aDeltaT_C,
                               bool aUseInternalLayer );

    /**
     * Display the results of a calculation (including resulting values such
     * as the resistance and power loss).
     */
    void TWDisplayValues( double aCurrent, double aExtWidth, double aIntWidth,
                          double aExtThickness, double aIntThickness );

    /**
     * Update the fields to show whether the maximum current, external trace
     * width, or internal trace width is currently the controlling parameter.
     */
    void TWUpdateModeDisplay();

private:
    enum                         // Which dimension is controlling the track width / current
    {                            // calculations:
        TW_MASTER_CURRENT,       //   the maximum current,
        TW_MASTER_EXT_WIDTH,     //   the external trace width,
        TW_MASTER_INT_WIDTH      //   or the internal trace width?
    } m_TWMode;

    bool                          m_TWNested; // Used to stop events caused by setting the answers.
};

#endif
