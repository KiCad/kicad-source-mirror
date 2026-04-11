/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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

#pragma once

#include <math/vector2d.h>
#include <wx/window.h>
#include <wx/panel.h>
#include <wx/notebook.h>

class PCB_BASE_FRAME;
class ZONE;


class ZONE_PREVIEW_NOTEBOOK : public wxNotebook
{
public:
    ZONE_PREVIEW_NOTEBOOK( wxWindow* aParent, PCB_BASE_FRAME* aPcbFrame );
    ~ZONE_PREVIEW_NOTEBOOK() override = default;

    void OnZoneSelectionChanged( ZONE* new_zone );

    void OnPageChanged( wxNotebookEvent& aEvent );

    void FitCanvasToScreen();

private:
    PCB_BASE_FRAME* m_pcbFrame;
    bool            m_hasSavedZoom;
    double          m_savedScale;
    VECTOR2D        m_savedCenter;
};
