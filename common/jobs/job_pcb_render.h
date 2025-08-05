/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 Alex Shvartzkop <dudesuchamazing@gmail.com>
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JOB_PCB_RENDER_H
#define JOB_PCB_RENDER_H

#include <kicommon.h>
#include "job.h"
#include <optional>
#include <math/vector3.h>

// Defined in wingdi.h
#ifdef TRANSPARENT
#undef TRANSPARENT
#endif

#ifdef OPAQUE
#undef OPAQUE
#endif

class KICOMMON_API JOB_PCB_RENDER : public JOB
{
public:
    enum class FORMAT
    {
        PNG,
        JPEG
    };

    static std::map<JOB_PCB_RENDER::FORMAT, wxString>& GetFormatNameMap();

public:
    JOB_PCB_RENDER();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    wxString m_filename;

    // Do not rename enum values as they are used for CLI args
    enum class QUALITY
    {
        BASIC,
        HIGH,
        USER,
        JOB_SETTINGS
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
    std::string m_appearancePreset;
    bool        m_useBoardStackupColors = true;
    SIDE        m_side = SIDE::TOP;
    double      m_zoom = 1.0;
    bool        m_perspective = false;
    VECTOR3D    m_rotation;
    VECTOR3D    m_pan;
    VECTOR3D    m_pivot;
    bool        m_proceduralTextures = false;
    bool        m_floor = false;
    bool        m_antiAlias = true;
    bool        m_postProcess = false;
    VECTOR3D    m_lightTopIntensity;
    VECTOR3D    m_lightBottomIntensity;
    VECTOR3D    m_lightCameraIntensity;
    VECTOR3D    m_lightSideIntensity = VECTOR3D( 0.5, 0.5, 0.5 );
    int         m_lightSideElevation = 60;
};

#endif