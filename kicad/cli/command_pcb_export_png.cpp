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

#include "command_pcb_export_png.h"
#include <cli/exit_codes.h>
#include "command_pcb_export_base.h"
#include "jobs/job_export_pcb_png.h"
#include <kiface_base.h>
#include <plotters/plotter_png.h>
#include <string_utils.h>
#include <wx/crt.h>
#include <wx/file.h>


#define ARG_DPI "--dpi"
#define ARG_NO_ANTIALIAS "--no-antialias"

CLI::PCB_EXPORT_PNG_COMMAND::PCB_EXPORT_PNG_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "png", IO_TYPE::FILE, IO_TYPE::DIRECTORY )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate PNG outputs of a given layer list" ) ) );

    addLayerArg();
    addCommonLayersArg();
    addDrawingSheetArg();
    addDefineArg();

    m_argParser.add_argument( ARG_SUBTRACT_SOLDERMASK )
            .help( UTF8STDSTR( _( "Subtract soldermask from silkscreen" ) ) )
            .flag();

    m_argParser.add_argument( "-m", ARG_MIRROR )
            .help( UTF8STDSTR( _( "Mirror the board (useful for trying to show bottom layers)" ) ) )
            .flag();

    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to PCB editor settings)" ) ) )
            .metavar( "THEME_NAME" );

    m_argParser.add_argument( ARG_NEGATIVE_SHORT, ARG_NEGATIVE )
            .help( UTF8STDSTR( _( ARG_NEGATIVE_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--sp", ARG_SKETCH_PADS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_SKETCH_PADS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--spn", ARG_SKETCH_PAD_NUMBERS )
            .help( UTF8STDSTR( _( ARG_SKETCH_PAD_NUMBERS_DESC ) ) )
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

    m_argParser.add_argument( "--ibt", ARG_INCLUDE_BORDER_TITLE )
            .help( UTF8STDSTR( _( "Include the border and title block" ) ) )
            .flag();

    m_argParser.add_argument( ARG_DRILL_SHAPE_OPTION )
            .help( UTF8STDSTR( _( ARG_DRILL_SHAPE_OPTION_DESC ) ) )
            .scan<'i', int>()
            .default_value( 2 )
            .metavar( "SHAPE_OPTION" );

    m_argParser.add_argument( ARG_SCALE )
            .help( UTF8STDSTR( _( ARG_SCALE_DESC ) ) )
            .scan<'g', double>()
            .default_value( 1.0 )
            .metavar( "SCALE" );

    m_argParser.add_argument( ARG_DPI )
            .help( UTF8STDSTR( wxString::Format( _( "Resolution in dots per inch (default %d)" ),
                                                 DEFAULT_PNG_DPI ) ) )
            .scan<'i', int>()
            .default_value( DEFAULT_PNG_DPI )
            .metavar( "DPI" );

    m_argParser.add_argument( ARG_NO_ANTIALIAS )
            .help( UTF8STDSTR( _( "Disable anti-aliasing" ) ) )
            .flag();

    m_argParser.add_argument( ARG_CHECK_ZONES )
            .help( UTF8STDSTR( _( ARG_CHECK_ZONES_DESC ) ) )
            .flag();

    addVariantsArg();
}


int CLI::PCB_EXPORT_PNG_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_PNG> pngJob( new JOB_EXPORT_PCB_PNG() );

    pngJob->m_mirror = m_argParser.get<bool>( ARG_MIRROR );
    pngJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    pngJob->m_negative = m_argParser.get<bool>( ARG_NEGATIVE );
    pngJob->m_sketchPadsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_PADS_ON_FAB_LAYERS );
    pngJob->m_plotPadNumbers = pngJob->m_sketchPadsOnFabLayers && m_argParser.get<bool>( ARG_SKETCH_PAD_NUMBERS );

    pngJob->m_hideDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS );
    pngJob->m_sketchDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS );
    pngJob->m_crossoutDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS );
    int drillShape = m_argParser.get<int>( ARG_DRILL_SHAPE_OPTION );
    pngJob->m_drillShapeOption = static_cast<DRILL_MARKS>( drillShape );
    pngJob->m_drawingSheet = m_argDrawingSheet;
    pngJob->m_subtractSolderMaskFromSilk = m_argParser.get<bool>( ARG_SUBTRACT_SOLDERMASK );
    pngJob->m_scale = m_argParser.get<double>( ARG_SCALE );
    pngJob->m_checkZonesBeforePlot = m_argParser.get<bool>( ARG_CHECK_ZONES );
    int dpi = m_argParser.get<int>( ARG_DPI );

    if( dpi < MIN_PNG_DPI || dpi > MAX_PNG_DPI )
    {
        wxFprintf( stderr, _( "DPI must be between %d and %d\n" ), MIN_PNG_DPI, MAX_PNG_DPI );
        return EXIT_CODES::ERR_ARGS;
    }

    pngJob->m_dpi = dpi;
    pngJob->m_antialias = !m_argParser.get<bool>( ARG_NO_ANTIALIAS );

    pngJob->m_filename = m_argInput;
    pngJob->SetConfiguredOutputPath( m_argOutput );
    pngJob->m_colorTheme = From_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );

    if( m_argParser.is_used( ARG_INCLUDE_BORDER_TITLE ) )
        pngJob->m_plotDrawingSheet = true;

    pngJob->SetVarOverrides( m_argDefineVars );

    if( !m_argVariantNames.empty() )
        pngJob->m_variant = m_argVariantNames.front();

    pngJob->m_argLayers = From_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );
    pngJob->m_argCommonLayers = From_UTF8( m_argParser.get<std::string>( ARG_COMMON_LAYERS ).c_str() );

    if( !wxFile::Exists( pngJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    return aKiway.ProcessJob( KIWAY::FACE_PCB, pngJob.get() );
}
