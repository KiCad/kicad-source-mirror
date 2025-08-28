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
#include "command_pcb_export_ps.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_ps.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>

#define ARG_MODE_SINGLE "--mode-single"
#define ARG_MODE_MULTI "--mode-multi"
#define ARG_TRACK_WIDTH_CORRECTION "--track-width-correction"
#define ARG_X_SCALE_FACTOR "--x-scale-factor"
#define ARG_Y_SCALE_FACTOR "--y-scale-factor"
#define ARG_FORCE_A4 "--force-a4"

CLI::PCB_EXPORT_PS_COMMAND::PCB_EXPORT_PS_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "ps", false, true )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate Postscript from a list of layers" ) ) );

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

    m_argParser.add_argument( ARG_NEGATIVE_SHORT, ARG_NEGATIVE )
            .help( UTF8STDSTR( _( ARG_NEGATIVE_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
            .flag();

    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to PCB Editor settings)" ) ) )
            .metavar( "THEME_NAME" );

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

    m_argParser.add_argument( "-C", ARG_TRACK_WIDTH_CORRECTION )
            .help( UTF8STDSTR( _( "Track width correction [mm]. Used to compensate errors in "
                                  "track widths, pad and via sizes." ) ) )
            .scan<'g', double>()
            .default_value( 0.0 )
            .metavar( "TRACK_COR" );

    m_argParser.add_argument( "-X", ARG_X_SCALE_FACTOR )
            .help( UTF8STDSTR( _( "X scale adjust for exact scale." ) ) )
            .scan<'g', double>()
            .default_value( 1.0 )
            .metavar( "X_SCALE" );

    m_argParser.add_argument( "-Y", ARG_Y_SCALE_FACTOR )
            .help( UTF8STDSTR( _( "Y scale adjust for exact scale." ) ) )
            .scan<'g', double>()
            .default_value( 1.0 )
            .metavar( "Y_SCALE" );

    m_argParser.add_argument( "-A", ARG_FORCE_A4 )
            .help( UTF8STDSTR( _( "Force A4 paper size." ) ) )
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


int CLI::PCB_EXPORT_PS_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_PS> psJob( new JOB_EXPORT_PCB_PS() );

    psJob->m_filename = m_argInput;
    psJob->SetConfiguredOutputPath( m_argOutput );
    psJob->m_drawingSheet = m_argDrawingSheet;
    psJob->SetVarOverrides( m_argDefineVars );

    if( !wxFile::Exists( psJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    psJob->m_plotFootprintValues = !m_argParser.get<bool>( ARG_EXCLUDE_VALUE );
    psJob->m_plotRefDes = !m_argParser.get<bool>( ARG_EXCLUDE_REFDES );

    psJob->m_plotDrawingSheet = m_argParser.get<bool>( ARG_INCLUDE_BORDER_TITLE );
    psJob->m_subtractSolderMaskFromSilk = m_argParser.get<bool>( ARG_SUBTRACT_SOLDERMASK );
    psJob->m_mirror = m_argParser.get<bool>( ARG_MIRROR );
    psJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    psJob->m_colorTheme = From_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );
    psJob->m_negative = m_argParser.get<bool>( ARG_NEGATIVE );
    psJob->m_scale = m_argParser.get<double>( ARG_SCALE );
    psJob->m_checkZonesBeforePlot = m_argParser.get<bool>( ARG_CHECK_ZONES );

    psJob->m_sketchPadsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_PADS_ON_FAB_LAYERS );
    psJob->m_hideDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS );
    psJob->m_sketchDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS );
    psJob->m_crossoutDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS );

    int drillShape = m_argParser.get<int>( ARG_DRILL_SHAPE_OPTION );
    psJob->m_drillShapeOption = static_cast<DRILL_MARKS>( drillShape );

    psJob->m_argLayers = From_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );
    psJob->m_argCommonLayers = From_UTF8( m_argParser.get<std::string>( ARG_COMMON_LAYERS ).c_str() );

    if( m_argParser.get<bool>( ARG_MODE_MULTI ) )
        psJob->m_genMode = JOB_EXPORT_PCB_PS::GEN_MODE::MULTI;
    else if( m_argParser.get<bool>( ARG_MODE_SINGLE ) )
        psJob->m_genMode = JOB_EXPORT_PCB_PS::GEN_MODE::SINGLE;

    psJob->m_trackWidthCorrection = m_argParser.get<double>( ARG_TRACK_WIDTH_CORRECTION );
    psJob->m_XScaleAdjust = m_argParser.get<double>( ARG_X_SCALE_FACTOR );
    psJob->m_YScaleAdjust = m_argParser.get<double>( ARG_Y_SCALE_FACTOR );
    psJob->m_forceA4 = m_argParser.get<bool>( ARG_FORCE_A4 );

    return aKiway.ProcessJob( KIWAY::FACE_PCB, psJob.get() );
}
