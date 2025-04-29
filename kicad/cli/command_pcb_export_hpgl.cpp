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

#include <base_units.h>
#include "command_pcb_export_hpgl.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_hpgl.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <locale_io.h>

#define ARG_MODE_SINGLE "--mode-single"
#define ARG_MODE_MULTI "--mode-multi"
#define ARG_DEFAULT_PEN_SIZE "--default-pen-size"
#define ARG_PEN_NUMBER "--pen-number"
#define ARG_PEN_SPEED "--pen-speed"

CLI::PCB_EXPORT_HPGL_COMMAND::PCB_EXPORT_HPGL_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "hpgl", false, true )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate HPGL from a list of layers" ) ) );

    addLayerArg();
    addCommonLayersArg();
    addDrawingSheetArg();
    addDefineArg();

    m_argParser.add_argument( "-m", ARG_MIRROR )
            .help( UTF8STDSTR( _( "Mirror the board (useful for trying to show bottom layers)" ) ) )
            .flag();

    m_argParser.add_argument( "--erd", ARG_EXCLUDE_REFDES )
            .help( UTF8STDSTR( _( "Exclude the reference designator text" ) ) )
            .flag();

    m_argParser.add_argument( "--ev", ARG_EXCLUDE_VALUE )
            .help( UTF8STDSTR( _( "Exclude the value text" ) ) )
            .flag();

    m_argParser.add_argument( "--ibt", ARG_INCLUDE_BORDER_TITLE )
            .help( UTF8STDSTR( _( "Include the border and title block" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SUBTRACT_SOLDERMASK )
            .help( UTF8STDSTR( _( "Subtract soldermask from silkscreen" ) ) )
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

    m_argParser.add_argument( ARG_SCALE )
            .help( UTF8STDSTR( _( ARG_SCALE_DESC ) ) )
            .scan<'g', double>()
            .default_value( 1.0 )
            .metavar( "SCALE" );

    m_argParser.add_argument( "-P", ARG_DEFAULT_PEN_SIZE )
            .help( UTF8STDSTR( _( "Size for the default pen [mm]" ) ) )
            .scan<'g', double>()
            .default_value( 0.381 )
            .metavar( "DEF_SIZE" );

    m_argParser.add_argument( "-N", ARG_PEN_NUMBER )
            .help( UTF8STDSTR( _( "Pen number selection (1 to 9)" ) ) )
            .scan<'d', int>()
            .default_value( 1 )
            .metavar( "PEN_NUM" );

    m_argParser.add_argument( "-S", ARG_PEN_SPEED )
            .help( UTF8STDSTR( _( "Pen speed [cm/s] (1 to 99 cm/s)" ) ) )
            .scan<'d', int>()
            .default_value( 40 )
            .metavar( "PEN_SPD" );
}


int CLI::PCB_EXPORT_HPGL_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_HPGL> hpglJob( new JOB_EXPORT_PCB_HPGL() );

    hpglJob->m_filename = m_argInput;
    hpglJob->SetConfiguredOutputPath( m_argOutput );
    hpglJob->m_drawingSheet = m_argDrawingSheet;
    hpglJob->SetVarOverrides( m_argDefineVars );

    if( !wxFile::Exists( hpglJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    hpglJob->m_plotFootprintValues = !m_argParser.get<bool>( ARG_EXCLUDE_VALUE );
    hpglJob->m_plotRefDes = !m_argParser.get<bool>( ARG_EXCLUDE_REFDES );

    hpglJob->m_plotDrawingSheet = m_argParser.get<bool>( ARG_INCLUDE_BORDER_TITLE );
    hpglJob->m_subtractSolderMaskFromSilk = m_argParser.get<bool>( ARG_SUBTRACT_SOLDERMASK );
    hpglJob->m_mirror = m_argParser.get<bool>( ARG_MIRROR );
    // We don't have colors here, we just use a pen, and we don't even know its color
    hpglJob->m_blackAndWhite = true;
    hpglJob->m_negative = false;

    hpglJob->m_sketchPadsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_PADS_ON_FAB_LAYERS );
    hpglJob->m_hideDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS );
    hpglJob->m_sketchDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS );
    hpglJob->m_crossoutDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS );

    int drillShape = m_argParser.get<int>( ARG_DRILL_SHAPE_OPTION );
    hpglJob->m_drillShapeOption = static_cast<DRILL_MARKS>( drillShape );

    hpglJob->m_argLayers = From_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );
    hpglJob->m_argCommonLayers = From_UTF8( m_argParser.get<std::string>( ARG_COMMON_LAYERS ).c_str() );

    if( m_argParser.get<bool>( ARG_MODE_MULTI ) )
        hpglJob->m_genMode = JOB_EXPORT_PCB_HPGL::GEN_MODE::MULTI;
    else if( m_argParser.get<bool>( ARG_MODE_SINGLE ) )
        hpglJob->m_genMode = JOB_EXPORT_PCB_HPGL::GEN_MODE::SINGLE;

    hpglJob->m_scale = m_argParser.get<double>( ARG_SCALE );
    hpglJob->m_defaultPenSize = m_argParser.get<double>( ARG_DEFAULT_PEN_SIZE );
    hpglJob->m_penNumber = m_argParser.get<int>( ARG_PEN_NUMBER );
    hpglJob->m_penSpeed = m_argParser.get<int>( ARG_PEN_SPEED );

    LOCALE_IO dummy;    // Switch to "C" locale
    return aKiway.ProcessJob( KIWAY::FACE_PCB, hpglJob.get() );
}
