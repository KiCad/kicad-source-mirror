/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef PANEL_PCBNEW_DISPLAY_ORIGIN_H
#define PANEL_PCBNEW_DISPLAY_ORIGIN_H 1

#include <frame_type.h>
#include "panel_pcbnew_display_origin_base.h"


class PANEL_PCBNEW_DISPLAY_ORIGIN : public PANEL_PCBNEW_DISPLAY_ORIGIN_BASE
{
public:
    PANEL_PCBNEW_DISPLAY_ORIGIN( wxWindow* aParent, APP_SETTINGS_BASE* aCfg, FRAME_T aFrameType );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ResetPanel() override;

private:
    void loadSettings( APP_SETTINGS_BASE* aCfg );

private:
    APP_SETTINGS_BASE* m_cfg;
    FRAME_T            m_frameType;
};


#endif // PANEL_PCBNEW_DISPLAY_ORIGIN_H
