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

#ifndef DIALOG_TUNING_PATTERN_PROPERTIES_H
#define DIALOG_TUNING_PATTERN_PROPERTIES_H

#include "dialog_tuning_pattern_properties_base.h"

#include <widgets/unit_binder.h>

#include <router/pns_router.h>

namespace PNS {

class MEANDER_SETTINGS;

}

class PCB_BASE_EDIT_FRAME;
class DRC_CONSTRAINT;


class DIALOG_TUNING_PATTERN_PROPERTIES : public DIALOG_TUNING_PATTERN_PROPERTIES_BASE
{
public:
    DIALOG_TUNING_PATTERN_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent,
                                      PNS::MEANDER_SETTINGS& aSettings,
                                      PNS::ROUTER_MODE aMeanderType,
                                      const DRC_CONSTRAINT& aConstraint );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onOverrideCustomRules( wxCommandEvent& event ) override;
    void onRadioBtnTargetLengthClick( wxCommandEvent& event ) override;
    void onRadioBtnTargetDelayClick( wxCommandEvent& event ) override;

private:
    const DRC_CONSTRAINT&  m_constraint;

    UNIT_BINDER            m_targetLength;
    UNIT_BINDER            m_targetDelay;
    UNIT_BINDER            m_minA;
    UNIT_BINDER            m_maxA;
    UNIT_BINDER            m_spacing;
    UNIT_BINDER            m_r;

    PNS::MEANDER_SETTINGS& m_settings;
    PNS::ROUTER_MODE       m_mode;
};

#endif // DIALOG_TUNING_PATTERN_PROPERTIES_H
