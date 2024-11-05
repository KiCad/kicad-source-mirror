/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JOB_PCB_RENDER_H
#define JOB_PCB_RENDER_H

#include <kicommon.h>
#include <wx/string.h>
#include "job.h"
#include <optional>
#include <math/vector3.h>

class KICOMMON_API JOB_PCB_RENDER : public JOB
{
public:
    JOB_PCB_RENDER();

    wxString m_filename;
    wxString m_outputFile;

    enum class FORMAT
    {
        PNG,
        JPEG
    };

    enum class QUALITY
    {
        BASIC,
        HIGH,
        USER
    };

    enum class BG_STYLE
    {
        DEFAULT,
        TRANSPARENT,
        OPAQUE
    };

    enum class SIDE
    {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT,
        FRONT,
        BACK
    };

    FORMAT      m_format = FORMAT::PNG;
    QUALITY     m_quality = QUALITY::BASIC;
    BG_STYLE    m_bgStyle = BG_STYLE::DEFAULT;
    int         m_width = 0;
    int         m_height = 0;
    std::string m_colorPreset;
    SIDE        m_side = SIDE::TOP;
    double      m_zoom = 1.0;
    bool        m_perspective = false;
    VECTOR3D    m_rotation;
    VECTOR3D    m_pan;
    VECTOR3D    m_pivot;
    bool        m_floor = false;
};

#endif