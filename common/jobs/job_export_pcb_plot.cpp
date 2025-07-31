/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <jobs/job_export_pcb_plot.h>

NLOHMANN_JSON_SERIALIZE_ENUM( DRILL_MARKS,
                              {
                                { DRILL_MARKS::NO_DRILL_SHAPE, "none" },
                                { DRILL_MARKS::SMALL_DRILL_SHAPE, "small" },
                                { DRILL_MARKS::FULL_DRILL_SHAPE, "full" }
                              } )

JOB_EXPORT_PCB_PLOT::JOB_EXPORT_PCB_PLOT( PLOT_FORMAT aFormat, const std::string& aType,
                                          bool aOutputIsDirectory ) :
        JOB( aType, aOutputIsDirectory ),
        m_plotFormat( aFormat ),
        m_filename(),
        m_colorTheme(),
        m_drawingSheet(),
        m_mirror( false ),
        m_blackAndWhite( false ),
        m_negative( false ),
        m_scale( 1.0 ),
        m_sketchPadsOnFabLayers( false ),
        m_hideDNPFPsOnFabLayers( false ),
        m_sketchDNPFPsOnFabLayers( true ),
        m_crossoutDNPFPsOnFabLayers( true ),
        m_plotFootprintValues( true ),
        m_plotRefDes( true ),
        m_plotDrawingSheet( true ),
        m_subtractSolderMaskFromSilk( false ),
        m_plotPadNumbers( false ),
        m_plotLayerSequence(),
        m_plotOnAllLayersSequence(),
        m_drillShapeOption( DRILL_MARKS::FULL_DRILL_SHAPE ),
        m_useDrillOrigin( false ),
        m_checkZonesBeforePlot( false )
{
    m_params.emplace_back( new JOB_PARAM_LSEQ( "layers",
                                               &m_plotLayerSequence, m_plotLayerSequence ) );

    m_params.emplace_back( new JOB_PARAM_LSEQ( "layers_to_include_on_all_layers",
                                                &m_plotOnAllLayersSequence,
                                               m_plotOnAllLayersSequence ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "mirror",
                                                &m_mirror, m_mirror ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "black_and_white",
                                                &m_blackAndWhite, m_blackAndWhite ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "negative",
                                                &m_negative, m_negative ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "plot_footprint_values",
                                                &m_plotFootprintValues, m_plotFootprintValues ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_ref_des",
                                                &m_plotRefDes, m_plotRefDes ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "hide_dnp_footprints_on_fab_layers",
                                                &m_hideDNPFPsOnFabLayers,
                                                m_hideDNPFPsOnFabLayers ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "sketch_dnp_footprints_on_fab_layers",
                                                &m_sketchDNPFPsOnFabLayers,
                                                m_sketchDNPFPsOnFabLayers ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "crossout_dnp_footprints_on_fab_layers",
                                                &m_crossoutDNPFPsOnFabLayers,
                                                m_crossoutDNPFPsOnFabLayers ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "sketch_pads_on_fab_layers",
                                                &m_sketchPadsOnFabLayers,
                                                m_sketchPadsOnFabLayers ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_pad_numbers",
                                                &m_plotPadNumbers, m_plotPadNumbers ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "plot_drawing_sheet",
                                                &m_plotDrawingSheet, m_plotDrawingSheet ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "subtract_solder_mask_from_silk",
                                                &m_subtractSolderMaskFromSilk,
                                                m_subtractSolderMaskFromSilk ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "use_drill_origin",
                                                &m_useDrillOrigin, m_useDrillOrigin ) );

    m_params.emplace_back( new JOB_PARAM<DRILL_MARKS>( "drill_shape",
                                                       &m_drillShapeOption, m_drillShapeOption ) );

    m_params.emplace_back( new JOB_PARAM<wxString>( "drawing_sheet",
                                                    &m_drawingSheet, m_drawingSheet ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "check_zones", &m_checkZonesBeforePlot, m_checkZonesBeforePlot ) );
}