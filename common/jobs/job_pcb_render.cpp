/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright (C) 2023-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <jobs/job_registry.h>
#include <jobs/job_pcb_render.h>
#include <i18n_utility.h>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_PCB_RENDER::FORMAT,
                              {
                                    { JOB_PCB_RENDER::FORMAT::JPEG, "jpeg" },
                                    { JOB_PCB_RENDER::FORMAT::PNG, "png" }
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_PCB_RENDER::QUALITY,
                              {
                                    { JOB_PCB_RENDER::QUALITY::BASIC, "basic" },
                                    { JOB_PCB_RENDER::QUALITY::HIGH, "high" },
                                    { JOB_PCB_RENDER::QUALITY::USER, "user" }
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_PCB_RENDER::SIDE,
                              {
                                    { JOB_PCB_RENDER::SIDE::BACK, "back" },
                                    { JOB_PCB_RENDER::SIDE::BOTTOM, "bottom" },
                                    { JOB_PCB_RENDER::SIDE::FRONT, "front" },
                                    { JOB_PCB_RENDER::SIDE::LEFT, "left" },
                                    { JOB_PCB_RENDER::SIDE::RIGHT, "right" },
                                    { JOB_PCB_RENDER::SIDE::TOP, "top" }
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_PCB_RENDER::BG_STYLE,
                            {
                                    { JOB_PCB_RENDER::BG_STYLE::BG_DEFAULT, "default" },
                                    { JOB_PCB_RENDER::BG_STYLE::BG_OPAQUE, "opaque" },
                                    { JOB_PCB_RENDER::BG_STYLE::BG_TRANSPARENT, "transparent" }
                            } )

JOB_PCB_RENDER::JOB_PCB_RENDER() :
        JOB( "render", false ), m_filename()
{
    m_params.emplace_back( new JOB_PARAM<FORMAT>( "format", &m_format, m_format ) );
    m_params.emplace_back( new JOB_PARAM<QUALITY>( "quality", &m_quality, m_quality ) );
    m_params.emplace_back( new JOB_PARAM<BG_STYLE>( "bg_style", &m_bgStyle, m_bgStyle ) );
    m_params.emplace_back( new JOB_PARAM<SIDE>( "side", &m_side, m_side ) );

    m_params.emplace_back( new JOB_PARAM<double>( "zoom", &m_zoom, m_zoom ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "perspective", &m_perspective, m_perspective ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "floor", &m_floor, m_floor ) );

    m_params.emplace_back( new JOB_PARAM<int>( "width", &m_width, m_width ) );
    m_params.emplace_back( new JOB_PARAM<int>( "height", &m_height, m_height ) );

}


wxString JOB_PCB_RENDER::GetDefaultDescription() const
{
    return _( "Render PCB" );
}


wxString JOB_PCB_RENDER::GetOptionsDialogTitle() const
{
    return wxString::Format( _( "PCB Render Job Options" ) );
}



REGISTER_JOB( pcb_render, _HKI( "PCB: Render" ), KIWAY::FACE_PCB, JOB_PCB_RENDER );
