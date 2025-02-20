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

#include <jobs/job_export_pcb_pdf.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_PDF::DRILL_MARKS,
                              {
                                { JOB_EXPORT_PCB_PDF::DRILL_MARKS::NO_DRILL_SHAPE, "none" },
                                { JOB_EXPORT_PCB_PDF::DRILL_MARKS::SMALL_DRILL_SHAPE, "small" },
                                { JOB_EXPORT_PCB_PDF::DRILL_MARKS::FULL_DRILL_SHAPE, "full" }
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_PDF::GEN_MODE,
                              {
                                { JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE, "all-layers-one-file" },
                                { JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_SEPARATE_FILE, "all-layers-separate-files" },
                                { JOB_EXPORT_PCB_PDF::GEN_MODE::ONE_PAGE_PER_LAYER_ONE_FILE, "one-page-per-layer-one-file" }
                              } )

JOB_EXPORT_PCB_PDF::JOB_EXPORT_PCB_PDF() :
        JOB_EXPORT_PCB_PLOT( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::PDF, "pdf", false ),
        m_pdfFrontFPPropertyPopups( true ),
        m_pdfBackFPPropertyPopups( true ),
        m_pdfMetadata( true ), m_pdfSingle( false ), m_pdfGenMode( GEN_MODE::ALL_LAYERS_ONE_FILE )
{
    m_plotDrawingSheet = false;

    m_params.emplace_back( new JOB_PARAM<wxString>( "color_theme",
            &m_colorTheme, m_colorTheme ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "mirror",
            &m_mirror, m_mirror ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "black_and_white",
            &m_blackAndWhite, m_blackAndWhite ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "negative",
            &m_negative, m_negative ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_footprint_values",
            &m_plotFootprintValues, m_plotFootprintValues ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_pad_numbers",
            &m_plotPadNumbers, m_plotPadNumbers ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "sketch_pads_on_fab_layers",
            &m_sketchPadsOnFabLayers, m_sketchPadsOnFabLayers ) );
    m_params.emplace_back( new JOB_PARAM<DRILL_MARKS>( "drill_shape",
            &m_drillShapeOption, m_drillShapeOption ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "pdf_metadata",
            &m_pdfMetadata, m_pdfMetadata ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "single_document",
            &m_pdfSingle, m_pdfSingle ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "front_fp_property_popups",
            &m_pdfFrontFPPropertyPopups, m_pdfFrontFPPropertyPopups ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "back_fp_property_popups",
            &m_pdfBackFPPropertyPopups, m_pdfBackFPPropertyPopups ) );
    m_params.emplace_back( new JOB_PARAM<GEN_MODE>( "pdf_gen_mode",
            &m_pdfGenMode, m_pdfGenMode ) );
}


wxString JOB_EXPORT_PCB_PDF::GetDefaultDescription() const
{
    return wxString::Format( _( "Export PDF" ) );
}


wxString JOB_EXPORT_PCB_PDF::GetSettingsDialogTitle() const
{
    return wxString::Format( _( "Export PDF Job Settings" ) );
}


REGISTER_JOB( pcb_export_pdf, _HKI( "PCB: Export PDF" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_PDF );