/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

#ifndef HIDPI_GL_3D_CANVAS_H
#define HIDPI_GL_3D_CANVAS_H

#include <gal/gal.h>
#include <atomic>
#include <gal/3d/camera.h>
#include <gal/hidpi_gl_canvas.h>
#include <wx/image.h>
#include <wx/timer.h>

/**
 *  Provides basic 3D controls ( zoom, rotate, translate, ... )
 *
 */
class GAL_API HIDPI_GL_3D_CANVAS : public HIDPI_GL_CANVAS
{
public:
    // wxGLCanvas constructor

    HIDPI_GL_3D_CANVAS( const KIGFX::VC_SETTINGS& aVcSettings, CAMERA& aCamera, wxWindow* parent,
                        const wxGLAttributes& aGLAttribs, wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize, long style = 0,
                        const wxString&  name = wxGLCanvasName,
                        const wxPalette& palette = wxNullPalette );

    bool m_mouse_is_moving; // Mouse activity is in progress
    bool m_mouse_was_moved;
    bool m_camera_is_moving; // Camera animation is ongoing

    CAMERA&            m_camera;
    static const float m_delta_move_step_factor; // Step factor to used with cursor on
                                                 // relation to the current zoom

    /**
     * Get the canvas camera.
     */
    CAMERA* GetCamera() { return &m_camera; }

    void OnMouseMoveCamera( wxMouseEvent& event );
    void OnMouseWheelCamera( wxMouseEvent& event, bool aPan );
};


#endif // HIDPI_GL_3D_CANVAS_H
