/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "command_pcb_render.h"
#include <cli/exit_codes.h>
#include "jobs/job_pcb_render.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <string_utils.h>
#include <wx/crt.h>
#include <magic_enum.hpp>

#include <macros.h>
#include <wx/tokenzr.h>
#include "../../3d-viewer/3d_viewer/eda_3d_viewer_settings.h"
#include <math/vector3.h>

#define ARG_BACKGROUND "--background"
#define ARG_QUALITY "--quality"

#define ARG_WIDTH "--width"
#define ARG_WIDTH_SHORT "-w"

#define ARG_HEIGHT "--height"
#define ARG_HEIGHT_SHORT "-h"

#define ARG_SIDE "--side"
#define ARG_PRESET "--preset"
#define ARG_PAN "--pan"
#define ARG_PIVOT "--pivot"
#define ARG_ROTATE "--rotate"
#define ARG_ZOOM "--zoom"
#define ARG_PERSPECTIVE "--perspective"
#define ARG_FLOOR "--floor"


template <typename T>
static wxString enumString()
{
    wxString str;
    auto     names = magic_enum::enum_names<T>();

    for( size_t i = 0; i < names.size(); i++ )
    {
        std::string name = { names[i].begin(), names[i].end() };

        if( i > 0 )
            str << ", ";

        std::transform( name.begin(), name.end(), name.begin(),
                        []( unsigned char c )
                        {
                            return std::tolower( c );
                        } );

        str << name;
    }

    return str;
}


template <typename T>
static std::vector<std::string> enumChoices()
{
    std::vector<std::string> out;

    for( auto& strView : magic_enum::enum_names<T>() )
    {
        std::string name = { strView.begin(), strView.end() };

        std::transform( name.begin(), name.end(), name.begin(),
                        []( unsigned char c )
                        {
                            return std::tolower( c );
                        } );

        out.emplace_back( name );
    }

    return out;
}


template <typename T>
static std::optional<T> strToEnum( std::string& aInput )
{
    return magic_enum::enum_cast<T>( aInput, magic_enum::case_insensitive );
}


template <typename T>
static bool getToEnum( const std::string& aInput, T& aOutput )
{
    // If not specified, leave at default
    if( aInput.empty() )
        return true;

    if( auto opt = magic_enum::enum_cast<T>( aInput, magic_enum::case_insensitive ) )
    {
        aOutput = *opt;
        return true;
    }

    return false;
}


static bool getToVector3( const std::string& aInput, VECTOR3D& aOutput )
{
    // If not specified, leave at default
    if( aInput.empty() )
        return true;

    // Remove potential quotes
    wxString wxStr = From_UTF8( aInput );

    if( wxStr[0] == '\'' )
        wxStr = wxStr.AfterFirst( '\'' );

    if( wxStr[wxStr.length() - 1] == '\'' )
        wxStr = wxStr.BeforeLast( '\'' );

    wxArrayString arr = wxSplit( wxStr, ',', 0 );

    if( arr.size() != 3 )
        return false;

    VECTOR3D vec;
    bool     success = true;
    success &= arr[0].Trim().ToCDouble( &vec.x );
    success &= arr[1].Trim().ToCDouble( &vec.y );
    success &= arr[2].Trim().ToCDouble( &vec.z );

    if( !success )
        return false;

    aOutput = vec;
    return true;
}


CLI::PCB_RENDER_COMMAND::PCB_RENDER_COMMAND() : COMMAND( "render" )
{
    addCommonArgs( true, true, false, false );
    addDefineArg();

    m_argParser.add_description(
            UTF8STDSTR( _( "Renders the PCB in 3D view to PNG or JPEG image" ) ) );

    m_argParser.add_argument( ARG_WIDTH, ARG_WIDTH_SHORT )
            .default_value( 1600 )
            .scan<'i', int>()
            .metavar( "WIDTH" )
            .help( UTF8STDSTR( _( "Image width" ) ) );

    m_argParser.add_argument( ARG_HEIGHT, ARG_HEIGHT_SHORT )
            .default_value( 900 )
            .scan<'i', int>()
            .metavar( "HEIGHT" )
            .help( UTF8STDSTR( _( "Image height" ) ) );

    m_argParser.add_argument( ARG_SIDE )
            .default_value( std::string( "top" ) )
            .add_choices( enumChoices<JOB_PCB_RENDER::SIDE>() )
            .metavar( "SIDE" )
            .help( UTF8STDSTR( wxString::Format( _( "Render from side. Options: %s" ),
                                                 enumString<JOB_PCB_RENDER::SIDE>() ) ) );

    m_argParser.add_argument( ARG_BACKGROUND )
            .default_value( std::string( "" ) )
            .help( UTF8STDSTR( _( "Image background. Options: transparent, opaque. Default: "
                                  "transparent for PNG, opaque for JPEG" ) ) )
            .metavar( "BG" );

    m_argParser.add_argument( ARG_QUALITY )
            .default_value( std::string( "basic" ) )
            .add_choices( enumChoices<JOB_PCB_RENDER::QUALITY>() )
            .metavar( "QUALITY" )
            .help( UTF8STDSTR( wxString::Format( _( "Render quality. Options: %s" ),
                                                 enumString<JOB_PCB_RENDER::QUALITY>() ) ) );

    m_argParser.add_argument( ARG_PRESET )
            .default_value( std::string( wxString( FOLLOW_PLOT_SETTINGS ) ) )
            .metavar( "PRESET" )
            .help( UTF8STDSTR( wxString::Format( _( "Color preset. Options: %s, %s, %s, ..." ),
                                                 FOLLOW_PCB, FOLLOW_PLOT_SETTINGS,
                                                 LEGACY_PRESET_FLAG ) ) );

    m_argParser.add_argument( ARG_FLOOR )
            .flag()
            .help( UTF8STDSTR( _( "Enables floor, shadows and post-processing, even if disabled in "
                                  "quality preset" ) ) );

    m_argParser.add_argument( ARG_PERSPECTIVE )
            .flag()
            .help( UTF8STDSTR( _( "Use perspective projection instead of orthogonal" ) ) );

    m_argParser.add_argument( ARG_ZOOM )
            .default_value( 1.0 )
            .scan<'g', double>()
            .metavar( "ZOOM" )
            .help( UTF8STDSTR( _( "Camera zoom" ) ) );

    m_argParser.add_argument( ARG_PAN )
            .default_value( std::string( "" ) )
            .metavar( "VECTOR" )
            .help( UTF8STDSTR( _( "Pan camera, format 'X,Y,Z' e.g.: '3,0,0'" ) ) );

    m_argParser.add_argument( ARG_PIVOT )
            .default_value( std::string( "" ) )
            .metavar( "PIVOT" )
            .help( UTF8STDSTR( _( "Set pivot point relative to the board center in centimeters, format 'X,Y,Z' "
                                  "e.g.: '-10,2,0'" ) ) );

    m_argParser.add_argument( ARG_ROTATE )
            .default_value( std::string( "" ) )
            .metavar( "ANGLES" )
            .help( UTF8STDSTR(
                    _( "Rotate board, format 'X,Y,Z' e.g.: '-45,0,45' for isometric view" ) ) );
}


int CLI::PCB_RENDER_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_PCB_RENDER> renderJob( new JOB_PCB_RENDER() );

    renderJob->SetOutputPath( m_argOutput );
    renderJob->m_filename = m_argInput;
    renderJob->SetVarOverrides( m_argDefineVars );

    renderJob->m_colorPreset = m_argParser.get<std::string>( ARG_PRESET );
    renderJob->m_width = m_argParser.get<int>( ARG_WIDTH );
    renderJob->m_height = m_argParser.get<int>( ARG_HEIGHT );
    renderJob->m_zoom = m_argParser.get<double>( ARG_ZOOM );
    renderJob->m_perspective = m_argParser.get<bool>( ARG_PERSPECTIVE );
    renderJob->m_floor = m_argParser.get<bool>( ARG_FLOOR );

    getToEnum( m_argParser.get<std::string>( ARG_QUALITY ), renderJob->m_quality );
    getToEnum( m_argParser.get<std::string>( ARG_SIDE ), renderJob->m_side );

    if( !getToEnum( m_argParser.get<std::string>( ARG_BACKGROUND ), renderJob->m_bgStyle ) )
    {
        wxFprintf( stderr, _( "Invalid background\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    if( !getToVector3( m_argParser.get<std::string>( ARG_ROTATE ), renderJob->m_rotation ) )
    {
        wxFprintf( stderr, _( "Invalid rotation format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    if( !getToVector3( m_argParser.get<std::string>( ARG_PAN ), renderJob->m_pan ) )
    {
        wxFprintf( stderr, _( "Invalid pan format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    if( !getToVector3( m_argParser.get<std::string>( ARG_PIVOT ), renderJob->m_pivot ) )
    {
        wxFprintf( stderr, _( "Invalid pivot format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    if( m_argOutput.Lower().EndsWith( wxS( ".png" ) ) )
    {
        renderJob->m_format = JOB_PCB_RENDER::FORMAT::PNG;
    }
    else if( m_argOutput.Lower().EndsWith( wxS( ".jpg" ) )
             || m_argOutput.Lower().EndsWith( wxS( ".jpeg" ) ) )
    {
        renderJob->m_format = JOB_PCB_RENDER::FORMAT::JPEG;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid image format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, renderJob.get() );

    return exitCode;
}
