/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#include "command_pcb_export_dxf.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_dxf.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <macros.h>

#define ARG_USE_CONTOURS "--use-contours"
#define ARG_OUTPUT_UNITS "--output-units"
#define ARG_USE_DRILL_ORIGIN "--use-drill-origin"
#define ARG_MODE_SINGLE "--mode-single"
#define ARG_MODE_MULTI "--mode-multi"

CLI::PCB_EXPORT_DXF_COMMAND::PCB_EXPORT_DXF_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "dxf", false, true )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate a DXF from a list of layers" ) ) );

    addLayerArg();
    addCommonLayersArg();
    addDrawingSheetArg();
    addDefineArg();

    m_argParser.add_argument( "--erd", ARG_EXCLUDE_REFDES )
            .help( UTF8STDSTR( _( "Exclude the reference designator text" ) ) )
            .flag();

    m_argParser.add_argument( "--ev", ARG_EXCLUDE_VALUE )
            .help( UTF8STDSTR( _( "Exclude the value text" ) ) )
            .flag();

    m_argParser.add_argument( "--sp", ARG_SKETCH_PADS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_SKETCH_PADS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--hdnp", ARG_HIDE_DNP_FPS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--sdnp", ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--cdnp", ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_SUBTRACT_SOLDERMASK )
            .help( UTF8STDSTR( _( "Subtract soldermask from silkscreen" ) ) )
            .flag();

    m_argParser.add_argument( "--uc", ARG_USE_CONTOURS )
            .help( UTF8STDSTR( _( "Plot graphic items using their contours" ) ) )
            .flag();

    m_argParser.add_argument( "--udo", ARG_USE_DRILL_ORIGIN )
            .help( UTF8STDSTR( _( "Plot using the drill/place file origin" ) ) )
            .flag();

    m_argParser.add_argument( "--ibt", ARG_INCLUDE_BORDER_TITLE )
            .help( UTF8STDSTR( _( "Include the border and title block" ) ) )
            .flag();

    m_argParser.add_argument( "--ou", ARG_OUTPUT_UNITS )
            .default_value( std::string( "in" ) )
            .help( UTF8STDSTR( _( "Output units, valid options: mm, in" ) ) )
            .metavar( "UNITS" );

    m_argParser.add_argument( ARG_DRILL_SHAPE_OPTION )
            .help( UTF8STDSTR( _( ARG_DRILL_SHAPE_OPTION_DESC ) ) )
            .scan<'i', int>()
            .default_value( 2 );

    m_argParser.add_argument( ARG_MODE_SINGLE )
            .help( UTF8STDSTR(
                    _( "Generates a single file with the output arg path acting as the complete "
                       "directory and filename path. COMMON_LAYER_LIST does not function in this "
                       "mode. Instead LAYER_LIST controls all layers plotted." ) ) )
            .flag();

    m_argParser.add_argument( ARG_MODE_MULTI )
            .help( UTF8STDSTR( _( "Generates one or more files with behavior similar to the KiCad "
                                  "GUI plotting. The given output path specifies a directory in "
                                  "which files may be output." ) ) )
            .flag();

    m_argParser.add_argument( DEPRECATED_ARG_PLOT_INVISIBLE_TEXT )
            .help( UTF8STDSTR( _( DEPRECATED_ARG_PLOT_INVISIBLE_TEXT_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_SCALE )
            .help( UTF8STDSTR( _( ARG_SCALE_DESC ) ) )
            .scan<'g', double>()
            .default_value( 1.0 )
            .metavar( "SCALE" );

    m_argParser.add_argument( ARG_CHECK_ZONES )
            .help( UTF8STDSTR( _( ARG_CHECK_ZONES_DESC ) ) )
            .flag();
}


int CLI::PCB_EXPORT_DXF_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_DXF> dxfJob( new JOB_EXPORT_PCB_DXF() );

    dxfJob->m_filename = m_argInput;
    dxfJob->SetConfiguredOutputPath( m_argOutput );
    dxfJob->m_drawingSheet = m_argDrawingSheet;
    dxfJob->m_sketchPadsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_PADS_ON_FAB_LAYERS );
    dxfJob->m_hideDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS );
    dxfJob->m_sketchDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS );
    dxfJob->m_crossoutDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS );
    dxfJob->SetVarOverrides( m_argDefineVars );

    if( !wxFile::Exists( dxfJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    dxfJob->m_plotFootprintValues = m_argParser.get<bool>( ARG_EXCLUDE_VALUE );
    dxfJob->m_plotRefDes = !m_argParser.get<bool>( ARG_EXCLUDE_REFDES );
    dxfJob->m_plotGraphicItemsUsingContours = m_argParser.get<bool>( ARG_USE_CONTOURS );
    dxfJob->m_polygonMode = m_argParser.get<bool>( ARG_USE_CONTOURS );
    dxfJob->m_useDrillOrigin = m_argParser.get<bool>( ARG_USE_DRILL_ORIGIN );
    dxfJob->m_plotDrawingSheet = m_argParser.get<bool>( ARG_INCLUDE_BORDER_TITLE );
    dxfJob->m_subtractSolderMaskFromSilk = m_argParser.get<bool>( ARG_SUBTRACT_SOLDERMASK );
    dxfJob->m_scale = m_argParser.get<double>( ARG_SCALE );
    dxfJob->m_checkZonesBeforePlot = m_argParser.get<bool>( ARG_CHECK_ZONES );

    if( m_argParser.get<bool>( DEPRECATED_ARG_PLOT_INVISIBLE_TEXT ) )
        wxFprintf( stdout, DEPRECATED_ARG_PLOT_INVISIBLE_TEXT_WARNING );

    int drillShape = m_argParser.get<int>( ARG_DRILL_SHAPE_OPTION );
    dxfJob->m_drillShapeOption = static_cast<DRILL_MARKS>( drillShape );

    wxString units = From_UTF8( m_argParser.get<std::string>( ARG_OUTPUT_UNITS ).c_str() );

    if( units == wxS( "mm" ) )
    {
        dxfJob->m_dxfUnits = JOB_EXPORT_PCB_DXF::DXF_UNITS::MM;
    }
    else if( units == wxS( "in" ) )
    {
        dxfJob->m_dxfUnits = JOB_EXPORT_PCB_DXF::DXF_UNITS::INCH;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid units specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    dxfJob->m_argLayers = From_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );
    dxfJob->m_argCommonLayers = From_UTF8( m_argParser.get<std::string>( ARG_COMMON_LAYERS ).c_str() );

    if( m_argParser.get<bool>( ARG_MODE_MULTI ) )
        dxfJob->m_genMode = JOB_EXPORT_PCB_DXF::GEN_MODE::MULTI;
    else if( m_argParser.get<bool>( ARG_MODE_SINGLE ) )
        dxfJob->m_genMode = JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE;
    else
    {
        wxFprintf( stdout, wxT( "\033[33;1m%s\033[0m\n" ),
                   _( "This command has deprecated behavior as of KiCad 9.0, the default behavior "
                      "of this command will change in a future release." ) );

        wxFprintf( stdout, wxT( "\033[33;1m%s\033[0m\n" ),
                   _( "The new behavior will match --mode-multi" ) );
    }

    return aKiway.ProcessJob( KIWAY::FACE_PCB, dxfJob.get() );
}
