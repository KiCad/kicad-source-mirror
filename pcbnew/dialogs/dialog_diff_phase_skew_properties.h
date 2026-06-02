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

#ifndef DIALOG_DIFF_PHASE_SKEW_PROPERTIES_H
#define DIALOG_DIFF_PHASE_SKEW_PROPERTIES_H


#include "dialog_diff_phase_skew_properties_base.h"
#include <pcbnew_settings.h>

class PCB_BASE_EDIT_FRAME;


class DIALOG_DIFF_PHASE_SKEW_PROPERTIES : public DIALOG_DIFF_PHASE_SKEW_PROPERTIES_BASE
{
public:
    DIALOG_DIFF_PHASE_SKEW_PROPERTIES( PCB_BASE_EDIT_FRAME*                       aParent,
                                       PCBNEW_SETTINGS::DIFF_PHASE_SKEW_SETTINGS* aSettings );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    PCBNEW_SETTINGS::DIFF_PHASE_SKEW_SETTINGS* m_settings;
};

#endif // DIALOG_TUNING_PATTERN_PROPERTIES_H
