/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job_export_pcb_ps.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_PS::GEN_MODE,
                              {
                                      { JOB_EXPORT_PCB_PS::GEN_MODE::MULTI, "multi" },
                                      { JOB_EXPORT_PCB_PS::GEN_MODE::SINGLE, "single" },
                              } )


JOB_EXPORT_PCB_PS::JOB_EXPORT_PCB_PS() :
        JOB_EXPORT_PCB_PLOT( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::POST, "ps", false ),
        m_genMode( GEN_MODE::MULTI ), m_trackWidthCorrection( 0.0 ),
        m_XScaleAdjust( 1.0 ), m_YScaleAdjust( 1.0 ), m_forceA4( false ),
        m_useGlobalSettings( true )
{
    m_plotDrawingSheet = false;

    m_params.emplace_back( new JOB_PARAM<double>( "scale", &m_scale, m_scale ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "color_theme",
            &m_colorTheme, m_colorTheme ) );
    m_params.emplace_back( new JOB_PARAM<GEN_MODE>( "gen_mode", &m_genMode, m_genMode ) );
    m_params.emplace_back( new JOB_PARAM<double>( "track_width_correction",
            &m_trackWidthCorrection, m_trackWidthCorrection ) );
    m_params.emplace_back( new JOB_PARAM<double>( "x_scale_factor", &m_XScaleAdjust, m_XScaleAdjust) );
    m_params.emplace_back( new JOB_PARAM<double>( "y_scale_factor", &m_YScaleAdjust, m_YScaleAdjust) );
    m_params.emplace_back( new JOB_PARAM<bool>( "force_a4", &m_forceA4, m_forceA4) );
    m_params.emplace_back( new JOB_PARAM<bool>( "use_global_settings",
            &m_useGlobalSettings, m_useGlobalSettings) );
}


wxString JOB_EXPORT_PCB_PS::GetDefaultDescription() const
{
    return wxString::Format( _( "Export Postscript" ) );
}


wxString JOB_EXPORT_PCB_PS::GetSettingsDialogTitle() const
{
    return wxString::Format( _( "Export Postscript Job Settings" ) );
}


REGISTER_JOB( pcb_export_ps, _HKI( "PCB: Export Postscript" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_PS );
