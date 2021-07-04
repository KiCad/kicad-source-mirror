/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_3D_COLORS_H
#define PANEL_3D_COLORS_H

#include "panel_3D_colors_base.h"
#include <3d_viewer/eda_3d_viewer.h>

class PANEL_3D_COLORS : public PANEL_3D_COLORS_BASE
{
public:
    explicit PANEL_3D_COLORS( EDA_3D_VIEWER_FRAME* aFrame, wxWindow* aParent );

    /// Automatically called when clicking on the OK button
    bool TransferDataFromWindow() override;

    /// Automatically called after creating the dialog
    bool TransferDataToWindow() override;

    void OnLoadColorsFromBoardStackup( wxCommandEvent& event ) override;

private:
    EDA_3D_VIEWER_FRAME* m_frame;
    BOARD_ADAPTER&       m_boardAdapter;

    CUSTOM_COLORS_LIST   m_silkscreenColors;
    CUSTOM_COLORS_LIST   m_maskColors;
    CUSTOM_COLORS_LIST   m_pasteColors;
    CUSTOM_COLORS_LIST   m_finishColors;
    CUSTOM_COLORS_LIST   m_boardColors;
};


#endif  // PANEL_3D_COLORS_H