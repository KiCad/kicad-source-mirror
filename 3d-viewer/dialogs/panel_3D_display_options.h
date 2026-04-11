/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PANEL_3D_DISPLAY_OPTIONS_H
#define PANEL_3D_DISPLAY_OPTIONS_H

#include "panel_3D_display_options_base.h"
#include <3d_viewer/eda_3d_viewer_frame.h>


class PANEL_3D_DISPLAY_OPTIONS : public PANEL_3D_DISPLAY_OPTIONS_BASE
{
public:
    explicit PANEL_3D_DISPLAY_OPTIONS( wxWindow* aParent );

    void OnCheckEnableAnimation( wxCommandEvent& WXUNUSED( event ) ) override;

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void ResetPanel() override;

private:
    void loadViewSettings( EDA_3D_VIEWER_SETTINGS* aCfg );
};


#endif  // PANEL_3D_DISPLAY_OPTIONS_H