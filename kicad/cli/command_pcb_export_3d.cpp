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

#include "command_pcb_export_3d.h"
#include <cli/exit_codes.h>
#include <base_units.h>
#include <kiface_base.h>
#include <regex>
#include <string_utils.h>
#include <locale_io.h>
#include <wx/crt.h>

#include <macros.h>

#define ARG_DRILL_ORIGIN "--drill-origin"
#define ARG_GRID_ORIGIN "--grid-origin"
#define ARG_NO_UNSPECIFIED "--no-unspecified"
#define ARG_NO_DNP "--no-dnp"
#define ARG_SUBST_MODELS "--subst-models"
#define ARG_FORCE "--force"
#define ARG_MIN_DISTANCE "--min-distance"
#define ARG_USER_ORIGIN "--user-origin"
#define ARG_BOARD_ONLY "--board-only"
#define ARG_CUT_VIAS_IN_BODY "--cut-vias-in-body"
#define ARG_NO_BOARD_BODY "--no-board-body"
#define ARG_NO_COMPONENTS "--no-components"
#define ARG_INCLUDE_TRACKS "--include-tracks"
#define ARG_INCLUDE_PADS "--include-pads"
#define ARG_INCLUDE_ZONES "--include-zones"
#define ARG_INCLUDE_INNER_COPPER "--include-inner-copper"
#define ARG_INCLUDE_SILKSCREEN "--include-silkscreen"
#define ARG_INCLUDE_SOLDERMASK "--include-soldermask"
#define ARG_FUSE_SHAPES "--fuse-shapes"
#define ARG_FILL_ALL_VIAS "--fill-all-vias"
#define ARG_NO_OPTIMIZE_STEP "--no-optimize-step"
#define ARG_NO_EXTRA_PAD_THICKNESS "--no-extra-pad-thickness"
#define ARG_NET_FILTER "--net-filter"
#define ARG_FORMAT "--format"
#define ARG_VRML_UNITS "--units"
#define ARG_VRML_MODELS_DIR "--models-dir"
#define ARG_VRML_MODELS_RELATIVE "--models-relative"
#define ARG_COMPONENT_FILTER "--component-filter"

#define REGEX_QUANTITY "([\\s]*[+-]?[\\d]*[.]?[\\d]*)"
#define REGEX_DELIMITER "(?:[\\s]*x)"
#define REGEX_UNIT "([m]{2}|(?:in))"

CLI::PCB_EXPORT_3D_COMMAND::PCB_EXPORT_3D_COMMAND( const std::string&        aName,
                                                   const std::string&        aDescription,
                                                   JOB_EXPORT_PCB_3D::FORMAT aFormat ) :
        COMMAND( aName ),
        m_format( aFormat )
{
    m_argParser.add_description( aDescription );
    addCommonArgs( true, true, false, false );
    addDefineArg();

    if( m_format == JOB_EXPORT_PCB_3D::FORMAT::UNKNOWN )
    {
        m_argParser.add_argument( ARG_FORMAT )
                .default_value( std::string( "step" ) )
                .help( UTF8STDSTR( _( "Output file format, options: step, stepz, brep, xao, glb "
                                      "(binary glTF), ply, stl" ) ) );
    }

    m_argParser.add_argument( ARG_FORCE, "-f" )
            .help( UTF8STDSTR( _( "Overwrite output file" ) ) )
            .flag();

    m_argParser.add_argument( ARG_NO_UNSPECIFIED )
            .help( UTF8STDSTR( _( "Exclude 3D models for components with 'Unspecified' footprint "
                                  "type" ) ) )
            .flag();

    m_argParser.add_argument( ARG_NO_DNP )
            .help( UTF8STDSTR( _( "Exclude 3D models for components with 'Do not populate' "
                                  "attribute" ) ) )
            .flag();

    if( m_format != JOB_EXPORT_PCB_3D::FORMAT::UNKNOWN
        && m_format != JOB_EXPORT_PCB_3D::FORMAT::VRML )
    {
        m_argParser.add_argument( ARG_GRID_ORIGIN )
                .help( UTF8STDSTR( _( "Use Grid Origin for output origin" ) ) )
                .flag();

        m_argParser.add_argument( ARG_DRILL_ORIGIN )
                .help( UTF8STDSTR( _( "Use Drill Origin for output origin" ) ) )
                .flag();

        m_argParser.add_argument( "--subst-models" )
                .help( UTF8STDSTR( _( "Substitute STEP or IGS models with the same name in place "
                                      "of VRML models" ) ) )
                .flag();

        m_argParser.add_argument( ARG_BOARD_ONLY )
                .help( UTF8STDSTR( _( "Only generate a board with no components" ) ) )
                .flag();

        m_argParser.add_argument( ARG_CUT_VIAS_IN_BODY )
                .help( UTF8STDSTR( _( "Cut via holes in board body even if conductor layers are "
                                      "not exported." ) ) )
                .flag();

        m_argParser.add_argument( ARG_NO_BOARD_BODY )
                .help( UTF8STDSTR( _( "Exclude board body" ) ) )
                .flag();

        m_argParser.add_argument( ARG_NO_COMPONENTS )
                .help( UTF8STDSTR( _( "Exclude 3D models for components" ) ) )
                .flag();

        m_argParser.add_argument( ARG_COMPONENT_FILTER )
                .help( UTF8STDSTR( _( "Only include component 3D models matching this list of "
                                      "reference designators (comma-separated, wildcards supported)"
                                      ) ) )
                .default_value( std::string() );

        m_argParser.add_argument( ARG_INCLUDE_TRACKS )
                .help( UTF8STDSTR( _( "Export tracks and vias" ) ) )
                .flag();

        m_argParser.add_argument( ARG_INCLUDE_PADS )
                .help( UTF8STDSTR( _( "Export pads" ) ) )
                .flag();

        m_argParser.add_argument( ARG_INCLUDE_ZONES )
                .help( UTF8STDSTR( _( "Export zones" ) ) )
                .flag();

        m_argParser.add_argument( ARG_INCLUDE_INNER_COPPER )
                .help( UTF8STDSTR( _( "Export elements on inner copper layers" ) ) )
                .flag();

        m_argParser.add_argument( ARG_INCLUDE_SILKSCREEN )
                .help( UTF8STDSTR( _( "Export silkscreen graphics as a set of flat faces" ) ) )
                .flag();

        m_argParser.add_argument( ARG_INCLUDE_SOLDERMASK )
                .help( UTF8STDSTR( _( "Export soldermask layers as a set of flat faces" ) ) )
                .flag();

        m_argParser.add_argument( ARG_FUSE_SHAPES )
                .help( UTF8STDSTR( _( "Fuse overlapping geometry together" ) ) )
                .flag();

        m_argParser.add_argument( ARG_FILL_ALL_VIAS )
                .help( UTF8STDSTR( _( "Don't cut via holes in conductor layers." ) ) )
                .flag();

        m_argParser.add_argument( ARG_NO_EXTRA_PAD_THICKNESS )
                .help( UTF8STDSTR( _( "Disable extra pad thickness (pads will have normal thickness)" ) ) )
                .flag();

        m_argParser.add_argument( ARG_MIN_DISTANCE )
                .default_value( std::string( "0.01mm" ) )
                .help( UTF8STDSTR( _( "Minimum distance between points to treat them as separate "
                                      "ones" ) ) )
                .metavar( "MIN_DIST" );

        m_argParser.add_argument( ARG_NET_FILTER )
                .default_value( std::string() )
                .help( UTF8STDSTR( _( "Only include copper items belonging to nets matching this "
                                      "wildcard" ) ) );
    }

    if( m_format == JOB_EXPORT_PCB_3D::FORMAT::STEP || m_format == JOB_EXPORT_PCB_3D::FORMAT::STEPZ )
    {
        m_argParser.add_argument( ARG_NO_OPTIMIZE_STEP )
                .help( UTF8STDSTR( _( "Do not optimize STEP file (enables writing parametric "
                                      "curves)" ) ) )
                .flag();
    }

    m_argParser.add_argument( ARG_USER_ORIGIN )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "User-specified output origin ex. 1x1in, 1x1inch, 25.4x25.4mm "
                                  "(default unit mm)" ) ) );

    if( m_format == JOB_EXPORT_PCB_3D::FORMAT::VRML )
    {
        m_argParser.add_argument( ARG_VRML_UNITS )
                .default_value( std::string( "in" ) )
                .help( UTF8STDSTR( _( "Output units; valid options: mm, m, in, tenths" ) ) );

        m_argParser.add_argument( ARG_VRML_MODELS_DIR )
                .default_value( std::string( "" ) )
                .help( UTF8STDSTR( _( "Name of folder to create and store 3d models in, if not "
                                      "specified or empty, the models will be embedded in main "
                                      "exported VRML file" ) ) );

        m_argParser.add_argument( ARG_VRML_MODELS_RELATIVE )
                .help( UTF8STDSTR( _( "Used with --models-dir to output relative paths in the "
                                      "resulting file" ) ) )
                .flag();
    }
}

int CLI::PCB_EXPORT_3D_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_3D> step( new JOB_EXPORT_PCB_3D() );
    EXPORTER_STEP_PARAMS& params = step->m_3dparams;

    if( m_format != JOB_EXPORT_PCB_3D::FORMAT::UNKNOWN
        && m_format != JOB_EXPORT_PCB_3D::FORMAT::VRML )
    {
        params.m_UseDrillOrigin = m_argParser.get<bool>( ARG_DRILL_ORIGIN );
        params.m_UseGridOrigin = m_argParser.get<bool>( ARG_GRID_ORIGIN );
        params.m_SubstModels = m_argParser.get<bool>( ARG_SUBST_MODELS );
        params.m_CutViasInBody = m_argParser.get<bool>( ARG_CUT_VIAS_IN_BODY );
        params.m_ExportBoardBody = !m_argParser.get<bool>( ARG_NO_BOARD_BODY );
        params.m_ExportComponents = !m_argParser.get<bool>( ARG_NO_COMPONENTS );
        params.m_ExportTracksVias = m_argParser.get<bool>( ARG_INCLUDE_TRACKS );
        params.m_ExportPads = m_argParser.get<bool>( ARG_INCLUDE_PADS );
        params.m_ExportZones = m_argParser.get<bool>( ARG_INCLUDE_ZONES );
        params.m_ExportInnerCopper = m_argParser.get<bool>( ARG_INCLUDE_INNER_COPPER );
        params.m_ExportSilkscreen = m_argParser.get<bool>( ARG_INCLUDE_SILKSCREEN );
        params.m_ExportSoldermask = m_argParser.get<bool>( ARG_INCLUDE_SOLDERMASK );
        params.m_FuseShapes = m_argParser.get<bool>( ARG_FUSE_SHAPES );
        params.m_FillAllVias = m_argParser.get<bool>( ARG_FILL_ALL_VIAS );
        params.m_ExtraPadThickness = !m_argParser.get<bool>( ARG_NO_EXTRA_PAD_THICKNESS );
        params.m_BoardOnly = m_argParser.get<bool>( ARG_BOARD_ONLY );
        params.m_NetFilter = From_UTF8( m_argParser.get<std::string>( ARG_NET_FILTER ).c_str() );
        params.m_ComponentFilter =
                From_UTF8( m_argParser.get<std::string>( ARG_COMPONENT_FILTER ).c_str() );

    }

    if( m_format == JOB_EXPORT_PCB_3D::FORMAT::STEP || m_format == JOB_EXPORT_PCB_3D::FORMAT::STEPZ )
    {
        params.m_OptimizeStep = !m_argParser.get<bool>( ARG_NO_OPTIMIZE_STEP );
    }

    params.m_IncludeUnspecified = !m_argParser.get<bool>( ARG_NO_UNSPECIFIED );
    params.m_IncludeDNP = !m_argParser.get<bool>( ARG_NO_DNP );
    params.m_Overwrite = m_argParser.get<bool>( ARG_FORCE );
    step->SetConfiguredOutputPath( m_argOutput );

    step->m_filename = m_argInput;
    step->m_format = m_format;
    step->SetVarOverrides( m_argDefineVars );

    if( step->m_format == JOB_EXPORT_PCB_3D::FORMAT::UNKNOWN )
    {
        wxString format = From_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );

        if( format == wxS( "step" ) )
            step->m_format = JOB_EXPORT_PCB_3D::FORMAT::STEP;
        else if( format == wxS( "stpz" ) )
            step->m_format = JOB_EXPORT_PCB_3D::FORMAT::STEPZ;
        else if( format == wxS( "brep" ) )
            step->m_format = JOB_EXPORT_PCB_3D::FORMAT::BREP;
        else if( format == wxS( "xao" ) )
            step->m_format = JOB_EXPORT_PCB_3D::FORMAT::XAO;
        else if( format == wxS( "glb" ) )
            step->m_format = JOB_EXPORT_PCB_3D::FORMAT::GLB;
        else if( format == wxS( "ply" ) )
            step->m_format = JOB_EXPORT_PCB_3D::FORMAT::PLY;
        else if( format == wxS( "stl" ) )
            step->m_format = JOB_EXPORT_PCB_3D::FORMAT::STL;
        else if( format == wxS( "u3d" ) )
            step->m_format = JOB_EXPORT_PCB_3D::FORMAT::U3D;
        else if( format == wxS( "pdf" ) )
            step->m_format = JOB_EXPORT_PCB_3D::FORMAT::PDF;
        else
        {
            wxFprintf( stderr, _( "Invalid format specified\n" ) );
            return EXIT_CODES::ERR_ARGS;
        }
    }

    if( step->m_format == JOB_EXPORT_PCB_3D::FORMAT::VRML )
    {
        wxString units = From_UTF8( m_argParser.get<std::string>( ARG_VRML_UNITS ).c_str() );

        if( units == wxS( "in" ) )
            step->m_vrmlUnits = JOB_EXPORT_PCB_3D::VRML_UNITS::INCH;
        else if( units == wxS( "mm" ) )
            step->m_vrmlUnits = JOB_EXPORT_PCB_3D::VRML_UNITS::MM;
        else if( units == wxS( "m" ) )
            step->m_vrmlUnits = JOB_EXPORT_PCB_3D::VRML_UNITS::METERS;
        else if( units == wxS( "tenths" ) )
            step->m_vrmlUnits = JOB_EXPORT_PCB_3D::VRML_UNITS::TENTHS;
        else
        {
            wxFprintf( stderr, _( "Invalid units specified\n" ) );
            return EXIT_CODES::ERR_ARGS;
        }

        step->m_vrmlModelDir = From_UTF8( m_argParser.get<std::string>( ARG_VRML_MODELS_DIR ).c_str() );

        step->m_vrmlRelativePaths = m_argParser.get<bool>( ARG_VRML_MODELS_RELATIVE );
    }

    wxString userOrigin = From_UTF8( m_argParser.get<std::string>( ARG_USER_ORIGIN ).c_str() );

    LOCALE_IO dummy;    // Switch to "C" locale

    if( !userOrigin.IsEmpty() )
    {
        std::regex  re_pattern( REGEX_QUANTITY REGEX_DELIMITER REGEX_QUANTITY REGEX_UNIT,
                                std::regex_constants::icase );
        std::smatch sm;
        std::string str( userOrigin.ToUTF8() );
        std::regex_search( str, sm, re_pattern );
        step->m_3dparams.m_Origin.x = atof( sm.str( 1 ).c_str() );
        step->m_3dparams.m_Origin.y = atof( sm.str( 2 ).c_str() );

        // Default unit for m_xOrigin and m_yOrigin is mm.
        // Convert in to board units. If the value is given in inches, it will be converted later
        step->m_3dparams.m_Origin.x = pcbIUScale.mmToIU( step->m_3dparams.m_Origin.x );
        step->m_3dparams.m_Origin.y = pcbIUScale.mmToIU( step->m_3dparams.m_Origin.y );

        std::string tunit( sm[3] );

        if( tunit.size() > 0 ) // unit specified ( default = mm, but can be in, inch or mm )
        {
            if( ( !sm.str( 1 ).compare( " " ) || !sm.str( 2 ).compare( " " ) )
                || ( sm.size() != 4 ) )
            {
                std::cout << m_argParser;
                return CLI::EXIT_CODES::ERR_ARGS;
            }

            // only in, inch and mm are valid:
            if( !tunit.compare( "in" ) || !tunit.compare( "inch" ) )
            {
                step->m_3dparams.m_Origin *= 25.4;
            }
            else if( tunit.compare( "mm" ) )
            {
                std::cout << m_argParser;
                return CLI::EXIT_CODES::ERR_ARGS;
            }
        }

        step->m_hasUserOrigin = true;
    }

    if( m_format != JOB_EXPORT_PCB_3D::FORMAT::UNKNOWN
        && m_format != JOB_EXPORT_PCB_3D::FORMAT::VRML )
    {
        wxString minDistance =
                From_UTF8( m_argParser.get<std::string>( ARG_MIN_DISTANCE ).c_str() );

        if( !minDistance.IsEmpty() )
        {
            std::regex  re_pattern( REGEX_QUANTITY REGEX_UNIT, std::regex_constants::icase );
            std::smatch sm;
            std::string str( minDistance.ToUTF8() );
            std::regex_search( str, sm, re_pattern );
            step->m_3dparams.m_BoardOutlinesChainingEpsilon = atof( sm.str( 1 ).c_str() );

            std::string tunit( sm[2] );

            if( tunit.size() > 0 ) // No unit accepted ( default = mm )
            {
                if( !tunit.compare( "in" ) || !tunit.compare( "inch" ) )
                {
                    step->m_3dparams.m_BoardOutlinesChainingEpsilon *= 25.4;
                }
                else if( tunit.compare( "mm" ) )
                {
                    std::cout << m_argParser;
                    return CLI::EXIT_CODES::ERR_ARGS;
                }
            }
        }
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, step.get() );

    return exitCode;
}
