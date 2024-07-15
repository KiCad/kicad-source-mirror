/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <jobs/job_export_pcb_gerber.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>


JOB_EXPORT_PCB_GERBER::JOB_EXPORT_PCB_GERBER( const std::string& aType, bool aIsCli ) :
    JOB_EXPORT_PCB_PLOT( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::GERBER, aType, false, aIsCli ),
    m_subtractSolderMaskFromSilk( false ),
    m_includeNetlistAttributes( true ),
    m_useX2Format( true ),
    m_disableApertureMacros( false ),
    m_useAuxOrigin( false ),
    m_useProtelFileExtension( true ),
    m_precision( 5 )
{
    m_plotDrawingSheet = false;

    m_params.emplace_back( new JOB_PARAM<wxString>( "drawing_sheet",
                                                    &m_drawingSheet,
                                                    m_drawingSheet ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_footprint_values",
                                                &m_plotFootprintValues,
                                                m_plotFootprintValues ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "plot_ref_des", &m_plotRefDes, m_plotRefDes ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "plot_drawing_sheet",
                                                &m_plotDrawingSheet,
                                                m_plotDrawingSheet ) );


    m_params.emplace_back( new JOB_PARAM<bool>( "subtract_solder_mask_from_silk",
                                                &m_subtractSolderMaskFromSilk,
                                                m_subtractSolderMaskFromSilk ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "include_netlist_attributes",
                                                &m_includeNetlistAttributes,
                                                m_includeNetlistAttributes ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "use_x2_format", &m_useX2Format, m_useX2Format ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "disable_aperture_macros", &m_disableApertureMacros,
                                                m_disableApertureMacros ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "use_aux_origin",
                                                &m_useAuxOrigin,
                                                m_useAuxOrigin ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "use_protel_file_extension",
                                                &m_useProtelFileExtension,
                                                m_useProtelFileExtension ) );
    m_params.emplace_back( new JOB_PARAM<int>( "precision", &m_precision, m_precision ) );
    m_params.emplace_back( new JOB_PARAM<LSEQ>( "layers", &m_printMaskLayer, m_printMaskLayer ) );
}


JOB_EXPORT_PCB_GERBER::JOB_EXPORT_PCB_GERBER( bool aIsCli ) :
    JOB_EXPORT_PCB_GERBER( "gerber", aIsCli )
{
}


wxString JOB_EXPORT_PCB_GERBER::GetDescription()
{
    return wxString::Format( _( "Single gerber export" ) );
}