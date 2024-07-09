/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_ZONE_PROPERTIES_H
#define PANEL_ZONE_PROPERTIES_H


#include "widgets/unit_binder.h"
#include "zone_manager/panel_zone_properties_base.h"
#include "zone_management_base.h"

#include "zone_manager/zone_selection_change_notifier.h"
#include <memory>
#include <zone_settings.h>

wxDECLARE_EVENT( EVT_ZONE_NAME_UPDATE, wxCommandEvent );


class PCB_BASE_FRAME;
class ZONES_CONTAINER;
class PANEL_ZONE_PROPERTIES : public PANEL_ZONE_PROPERTIES_BASE,
                              public ZONE_SELECTION_CHANGE_NOTIFIER,
                              public ZONE_MANAGEMENT_BASE
{
public:
    PANEL_ZONE_PROPERTIES( wxWindow* aParent, PCB_BASE_FRAME* aPcb,
                           ZONES_CONTAINER& aZoneContainer );


    void ActivateSelectedZone( ZONE* new_zone ) override;

    void OnUserConfirmChange() override;

    std::shared_ptr<ZONE_SETTINGS> GetZoneSettings() const { return m_settings; }

    bool TransferZoneSettingsFromWindow();

private:
    static constexpr int INVALID_NET_CODE{ 0 };

    static constexpr int DEFAULT_SORT_CONFIG{ -1 };
    static constexpr int NO_PERSISTENT_SORT_MODE{ 0 };
    static constexpr int HIDE_ANONYMOUS_NETS{ 1 << 0 };
    static constexpr int SORT_BY_PAD_COUNT{ 1 << 1 };

    bool TransferZoneSettingsToWindow();

    /**
     * @param aUseExportableSetupOnly is true to use exportable parameters only (used to
     *                                export this setup to other zones).
     * @return bool - false if incorrect options, true if ok.
     */
    bool AcceptOptions( bool aUseExportableSetupOnly = false );

    void OnStyleSelection( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& ) override;
    void OnRemoveIslandsSelection( wxCommandEvent& event ) override;
    void OnZoneNameChanged( wxCommandEvent& event ) override;


private:
    ZONES_CONTAINER& m_ZoneContainer;
    PCB_BASE_FRAME*  m_PCB_Frame;

    std::shared_ptr<ZONE_SETTINGS> m_settings;

    int         m_cornerSmoothingType;
    UNIT_BINDER m_outlineHatchPitch;

    UNIT_BINDER m_cornerRadius;
    UNIT_BINDER m_clearance;
    UNIT_BINDER m_minThickness;
    UNIT_BINDER m_antipadClearance;
    UNIT_BINDER m_spokeWidth;

    UNIT_BINDER m_gridStyleRotation;
    UNIT_BINDER m_gridStyleThickness;
    UNIT_BINDER m_gridStyleGap;
    UNIT_BINDER m_islandThreshold;
    bool        m_isTeardrop;
};


#endif