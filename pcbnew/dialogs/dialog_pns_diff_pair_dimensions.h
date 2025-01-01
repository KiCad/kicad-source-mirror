/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014-2015  CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

/**
 * Push and Shove diff pair dimensions (gap) settings dialog.
 */

#ifndef __dialog_diff_pair_dimensions_settings__
#define __dialog_diff_pair_dimensions_settings__

#include <widgets/unit_binder.h>

#include "dialog_pns_diff_pair_dimensions_base.h"

namespace PNS {

class SIZES_SETTINGS;

}

class DIALOG_PNS_DIFF_PAIR_DIMENSIONS : public DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE
{
public:
    DIALOG_PNS_DIFF_PAIR_DIMENSIONS( EDA_DRAW_FRAME* aParent, PNS::SIZES_SETTINGS& aSizes );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

private:
    void updateCheckbox();

    virtual void OnViaTraceGapEqualCheck( wxCommandEvent& event ) override;

    UNIT_BINDER m_traceWidth;
    UNIT_BINDER m_traceGap;
    UNIT_BINDER m_viaGap;

    PNS::SIZES_SETTINGS& m_sizes;
};

#endif // __dialog_pns_settings__
