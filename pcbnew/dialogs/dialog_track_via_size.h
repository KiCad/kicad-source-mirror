/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014  CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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
 * Custom track width and via size dialog.
 */

#ifndef __dialog_track_via_size__
#define __dialog_track_via_size__

#include <widgets/unit_binder.h>

#include "dialog_track_via_size_base.h"

class BOARD_DESIGN_SETTINGS;

/** Implementing DIALOG_TRACK_VIA_SIZE_BASE */
class DIALOG_TRACK_VIA_SIZE : public DIALOG_TRACK_VIA_SIZE_BASE
{
public:
    /** Constructor */
    DIALOG_TRACK_VIA_SIZE( EDA_DRAW_FRAME* aParent, BOARD_DESIGN_SETTINGS& aSettings );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

protected:
    UNIT_BINDER m_trackWidth;
    UNIT_BINDER m_viaDiameter;
    UNIT_BINDER m_viaDrill;

    // Routings settings that are modified by the dialog.
    BOARD_DESIGN_SETTINGS& m_settings;
};

#endif // __dialog_track_via_size__
