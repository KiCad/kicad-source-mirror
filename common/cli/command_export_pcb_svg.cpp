/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "command_export_pcb_svg.h"
#include "exit_codes.h"
#include "jobs/job_export_pcb_svg.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <regex>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_PAGE_SIZE "--page-size-mode"
#define ARG_MIRROR "--mirror"
#define ARG_BLACKANDWHITE "--black-and-white"
#define ARG_OUTPUT "--output"
#define ARG_THEME "--theme"
#define ARG_LAYERS "--layers"
#define ARG_INPUT "input"


CLI::EXPORT_PCB_SVG_COMMAND::EXPORT_PCB_SVG_COMMAND() : COMMAND( "svg" )
{
    m_argParser.add_argument( "-o", ARG_OUTPUT )
            .default_value( std::string() )
            .help( "output file name" );

    m_argParser.add_argument( "-l", ARG_LAYERS )
            .default_value( std::string() )
            .help( "comma separated list of untranslated layer names to include such as F.Cu,B.Cu" );

    m_argParser.add_argument( "-m", ARG_MIRROR )
            .help( "Mirror the board (useful for trying to show bottom layers)" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( "Color theme to use (will default to pcbnew settings)" );

    m_argParser.add_argument( ARG_BLACKANDWHITE )
            .help( "Black and white only" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_PAGE_SIZE )
            .help( "Set page sizing mode (0 = page with frame and title block, 1 = current page size, 2 = board area only)" )
            .default_value( 0 );

    m_argParser.add_argument( ARG_INPUT ).help( "input file" );
}


int CLI::EXPORT_PCB_SVG_COMMAND::Perform( KIWAY& aKiway ) const
{
    std::map<std::string, LSET> layerMasks;
    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        std::string untranslated = TO_UTF8( wxString( LSET::Name( PCB_LAYER_ID( layer ) ) ) );

        //m_layerIndices[untranslated] = PCB_LAYER_ID( layer );
        layerMasks[untranslated] = LSET( PCB_LAYER_ID( layer ) );
    }
    layerMasks["*.Cu"] = LSET::AllCuMask();
    layerMasks["*In.Cu"] = LSET::InternalCuMask();
    layerMasks["F&B.Cu"] = LSET( 2, F_Cu, B_Cu );
    layerMasks["*.Adhes"] = LSET( 2, B_Adhes, F_Adhes );
    layerMasks["*.Paste"] = LSET( 2, B_Paste, F_Paste );
    layerMasks["*.Mask"] = LSET( 2, B_Mask, F_Mask );
    layerMasks["*.SilkS"] = LSET( 2, B_SilkS, F_SilkS );
    layerMasks["*.Fab"] = LSET( 2, B_Fab, F_Fab );
    layerMasks["*.CrtYd"] = LSET( 2, B_CrtYd, F_CrtYd );

    JOB_EXPORT_PCB_SVG* svgJob = new JOB_EXPORT_PCB_SVG( true );

    svgJob->m_mirror = m_argParser.get<bool>( ARG_MIRROR );
    svgJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    svgJob->m_pageSizeMode = m_argParser.get<int>( ARG_PAGE_SIZE );

    svgJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    svgJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );
    svgJob->m_colorTheme = FROM_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );

    wxString layers = FROM_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );

    LSET              layerMask = LSET::AllCuMask();

    if( !layers.IsEmpty() )
    {
        layerMask.reset();
        wxStringTokenizer layerTokens( layers, "," );
        while( layerTokens.HasMoreTokens() )
        {
            std::string token = TO_UTF8(layerTokens.GetNextToken());
            if( layerMasks.count( token ) )
            {
                layerMask |= layerMasks[token];
            }
        }
    }

    svgJob->m_printMaskLayer = layerMask;

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, svgJob );

    return exitCode;
}