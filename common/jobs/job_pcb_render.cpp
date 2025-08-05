/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job_registry.h>
#include <jobs/job_pcb_render.h>
#include <i18n_utility.h>


namespace nlohmann
{
template <>
struct adl_serializer<VECTOR3D>
{
    static void from_json( const json& j, VECTOR3D& s )
    {
        if( !j.is_array() || j.size() != 3 )
            throw std::invalid_argument( "JSON array size should be 3 for VECTOR3D" );

        s.x = j[0];
        s.y = j[1];
        s.z = j[2];
    }

    static void to_json( json& j, const VECTOR3D& s ) { j = json::array( { s.x, s.y, s.z } ); }
};
} // namespace nlohmann


std::map<JOB_PCB_RENDER::FORMAT, wxString> outputFormatNameMap = {
    { JOB_PCB_RENDER::FORMAT::JPEG, wxT( "JPEG" ) },
    { JOB_PCB_RENDER::FORMAT::PNG, wxT( "PNG" ) }
};


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
                                    { JOB_PCB_RENDER::BG_STYLE::DEFAULT, "default" },
                                    { JOB_PCB_RENDER::BG_STYLE::OPAQUE, "opaque" },
                                    { JOB_PCB_RENDER::BG_STYLE::TRANSPARENT, "transparent" }
                            } )

JOB_PCB_RENDER::JOB_PCB_RENDER() :
        JOB( "render", false ), m_filename()
{
    m_params.emplace_back( new JOB_PARAM<FORMAT>( "format", &m_format, m_format ) );
    m_params.emplace_back( new JOB_PARAM<std::string>( "preset", &m_appearancePreset, m_appearancePreset ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "use_board_stackup_colors", &m_useBoardStackupColors, m_useBoardStackupColors ) );
    m_params.emplace_back( new JOB_PARAM<QUALITY>( "quality", &m_quality, m_quality ) );
    m_params.emplace_back( new JOB_PARAM<BG_STYLE>( "bg_style", &m_bgStyle, m_bgStyle ) );
    m_params.emplace_back( new JOB_PARAM<SIDE>( "side", &m_side, m_side ) );

    m_params.emplace_back( new JOB_PARAM<double>( "zoom", &m_zoom, m_zoom ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "perspective", &m_perspective, m_perspective ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "floor", &m_floor, m_floor ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "anti_alias", &m_antiAlias, m_antiAlias ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "post_process", &m_postProcess, m_postProcess ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "procedural_textures", &m_proceduralTextures, m_proceduralTextures ) );

    m_params.emplace_back( new JOB_PARAM<int>( "width", &m_width, m_width ) );
    m_params.emplace_back( new JOB_PARAM<int>( "height", &m_height, m_height ) );

    m_params.emplace_back( new JOB_PARAM<double>( "pivot_x", &m_pivot.x, m_pivot.x ) );
    m_params.emplace_back( new JOB_PARAM<double>( "pivot_y", &m_pivot.y, m_pivot.y ) );
    m_params.emplace_back( new JOB_PARAM<double>( "pivot_z", &m_pivot.z, m_pivot.z ) );

    m_params.emplace_back( new JOB_PARAM<double>( "pan_x", &m_pan.x, m_pan.x ) );
    m_params.emplace_back( new JOB_PARAM<double>( "pan_y", &m_pan.y, m_pan.y ) );
    m_params.emplace_back( new JOB_PARAM<double>( "pan_z", &m_pan.z, m_pan.z ) );

    m_params.emplace_back( new JOB_PARAM<double>( "rotation_x", &m_rotation.x, m_rotation.x ) );
    m_params.emplace_back( new JOB_PARAM<double>( "rotation_y", &m_rotation.y, m_rotation.y ) );
    m_params.emplace_back( new JOB_PARAM<double>( "rotation_z", &m_rotation.z, m_rotation.z ) );

    m_params.emplace_back( new JOB_PARAM<VECTOR3D>( "light_top_intensity", &m_lightTopIntensity, m_lightTopIntensity ) );
    m_params.emplace_back( new JOB_PARAM<VECTOR3D>( "light_bottom_intensity", &m_lightBottomIntensity, m_lightBottomIntensity ) );
    m_params.emplace_back( new JOB_PARAM<VECTOR3D>( "light_side_intensity", &m_lightSideIntensity, m_lightSideIntensity ) );
    m_params.emplace_back( new JOB_PARAM<VECTOR3D>( "light_camera_intensity", &m_lightCameraIntensity, m_lightCameraIntensity ) );

    m_params.emplace_back( new JOB_PARAM<int>( "light_side_elevation", &m_lightSideElevation, m_lightSideElevation ) );
}


std::map<JOB_PCB_RENDER::FORMAT, wxString>& JOB_PCB_RENDER::GetFormatNameMap()
{
    return outputFormatNameMap;
}


wxString JOB_PCB_RENDER::GetDefaultDescription() const
{
    return wxString::Format( _( "Render %s" ), GetFormatNameMap()[m_format] );
}


wxString JOB_PCB_RENDER::GetSettingsDialogTitle() const
{
    return _( "Render PCB Job Settings" );
}


REGISTER_JOB( pcb_render, _HKI( "PCB: Render" ), KIWAY::FACE_PCB, JOB_PCB_RENDER );
