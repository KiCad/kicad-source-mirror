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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#ifndef PANEL_SETUP_TUNING_PATTERNS_H
#define PANEL_SETUP_TUNING_PATTERNS_H

#include <panel_setup_tuning_patterns_base.h>
#include <widgets/unit_binder.h>


namespace PNS {

class MEANDER_SETTINGS;

}

class BOARD;


class PANEL_SETUP_TUNING_PATTERNS : public PANEL_SETUP_TUNING_PATTERNS_BASE
{
public:
    PANEL_SETUP_TUNING_PATTERNS( wxWindow* aParent, EDA_DRAW_FRAME* aFrame,
                          PNS::MEANDER_SETTINGS& aTrackSettings,
                          PNS::MEANDER_SETTINGS& aDiffPairSettings,
                          PNS::MEANDER_SETTINGS& aSkewSettings );

    void ImportSettingsFrom( BOARD* aBoard );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    UNIT_BINDER            m_track_minA;
    UNIT_BINDER            m_track_maxA;
    UNIT_BINDER            m_track_spacing;
    UNIT_BINDER            m_track_r;

    UNIT_BINDER            m_dp_minA;
    UNIT_BINDER            m_dp_maxA;
    UNIT_BINDER            m_dp_spacing;
    UNIT_BINDER            m_dp_r;

    UNIT_BINDER            m_skew_minA;
    UNIT_BINDER            m_skew_maxA;
    UNIT_BINDER            m_skew_spacing;
    UNIT_BINDER            m_skew_r;

    PNS::MEANDER_SETTINGS& m_trackSettings;
    PNS::MEANDER_SETTINGS& m_dpSettings;
    PNS::MEANDER_SETTINGS& m_skewSettings;
};

#endif // PANEL_SETUP_TUNING_PATTERNS_H
