/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "command_pcb_export_base.h"
#include <cli/exit_codes.h>
#include <kiface_base.h>
#include <bitset>
#include <layer_ids.h>
#include <lset.h>
#include <lseq.h>
#include <string_utils.h>

#include <macros.h>
#include <wx/tokenzr.h>
#include <wx/crt.h>

CLI::PCB_EXPORT_BASE_COMMAND::PCB_EXPORT_BASE_COMMAND( const std::string& aName,
                                                       bool               aInputIsDir,
                                                       bool               aOutputIsDir ) :
        COMMAND( aName )
{
    m_selectedLayersSet = false;
    m_requireLayers = false;
    m_hasLayerArg = false;

    addCommonArgs( true, true, aInputIsDir, aOutputIsDir );

    // Build list of layer names and their layer mask:
    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        std::string untranslated = TO_UTF8( wxString( LSET::Name( PCB_LAYER_ID( layer ) ) ) );

        //m_layerIndices[untranslated] = PCB_LAYER_ID( layer );

        // Add layer name used in pcb files
        m_layerMasks[untranslated] = LSET( { PCB_LAYER_ID( layer ) } );
        // Add layer name using GUI canonical layer name
        m_layerGuiMasks[ TO_UTF8(LayerName( layer ) ) ] = LSET( { PCB_LAYER_ID( layer ) } );
    }

    // Add list of grouped layer names used in pcb files
    m_layerMasks["*"] = LSET::AllLayersMask();
    m_layerMasks["*.Cu"] = LSET::AllCuMask();
    m_layerMasks["*In.Cu"] = LSET::InternalCuMask();
    m_layerMasks["F&B.Cu"] = LSET( { F_Cu, B_Cu } );
    m_layerMasks["*.Adhes"] = LSET( { B_Adhes, F_Adhes } );
    m_layerMasks["*.Paste"] = LSET( { B_Paste, F_Paste } );
    m_layerMasks["*.Mask"] = LSET( { B_Mask, F_Mask } );
    m_layerMasks["*.SilkS"] = LSET( { B_SilkS, F_SilkS } );
    m_layerMasks["*.Fab"] = LSET( { B_Fab, F_Fab } );
    m_layerMasks["*.CrtYd"] = LSET( { B_CrtYd, F_CrtYd } );

    // Add list of grouped layer names using GUI canonical layer names
    m_layerGuiMasks["*.Adhesive"] = LSET( { B_Adhes, F_Adhes } );
    m_layerGuiMasks["*.Silkscreen"] = LSET( { B_SilkS, F_SilkS } );
    m_layerGuiMasks["*.Courtyard"] = LSET( { B_CrtYd, F_CrtYd } );
}


LSEQ CLI::PCB_EXPORT_BASE_COMMAND::convertLayerStringList( wxString& aLayerString,
                                                           bool& aLayerArgSet ) const
{
    LSEQ layerMask;

    if( !aLayerString.IsEmpty() )
    {
        wxStringTokenizer layerTokens( aLayerString, "," );

        while( layerTokens.HasMoreTokens() )
        {
            std::string token = TO_UTF8( layerTokens.GetNextToken() );

            // Search for a layer name in canonical layer name used in .kicad_pcb files:
            if( m_layerMasks.count( token ) )
            {
                for( PCB_LAYER_ID layer : m_layerMasks.at( token ).Seq() )
                    layerMask.push_back( layer );

                aLayerArgSet = true;
            }
            // Search for a layer name in canonical layer name used in GUI (not translated):
            else if( m_layerGuiMasks.count( token ) )
            {
                for( PCB_LAYER_ID layer : m_layerGuiMasks.at( token ).Seq() )
                    layerMask.push_back( layer );

                aLayerArgSet = true;
            }
            else
            {
                wxFprintf( stderr, _( "Invalid layer name \"%s\"\n" ), token );
            }
        }
    }

    return layerMask;
}


void CLI::PCB_EXPORT_BASE_COMMAND::addLayerArg( bool aRequire )
{
    m_argParser.add_argument( "-l", ARG_LAYERS )
            .default_value( std::string() )
            .help( UTF8STDSTR(
                    _( "Comma separated list of untranslated layer names to include such as "
                       "F.Cu,B.Cu" ) ) )
            .metavar( "LAYER_LIST" );

    m_hasLayerArg = true;
    m_requireLayers = aRequire;
}


int CLI::PCB_EXPORT_BASE_COMMAND::doPerform( KIWAY& aKiway )
{
    if( m_hasLayerArg )
    {
        wxString layers = From_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );

        LSEQ layerMask = convertLayerStringList( layers, m_selectedLayersSet );

        if( m_requireLayers && layerMask.size() < 1 )
        {
            wxFprintf( stderr, _( "At least one layer must be specified\n" ) );
            return EXIT_CODES::ERR_ARGS;
        }

        m_selectedLayers = layerMask;
    }

    return EXIT_CODES::OK;
}
