/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "panel_zone_conversion_base.h"
#include <widgets/unit_binder.h>

class EDA_DRAW_FRAME;
struct CONVERT_SETTINGS;


class PANEL_ZONE_CONVERSION : public PANEL_ZONE_CONVERSION_BASE
{
public:
    PANEL_ZONE_CONVERSION( wxWindow* aParent, EDA_DRAW_FRAME* aFrame, CONVERT_SETTINGS& aSettings );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void OnUpdateUI( wxUpdateUIEvent& aEvent ) override;

    CONVERT_SETTINGS& m_convertSettings;
    UNIT_BINDER       m_gap;
};
