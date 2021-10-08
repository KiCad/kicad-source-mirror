/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file cadstar_pcb_archive_plugin.cpp
 * @brief Pcbnew PLUGIN for CADSTAR PCB Archive (*.cpa) format: an ASCII format
 *        based on S-expressions.
 */

#include <cadstar_pcb_archive_loader.h>
#include <cadstar_pcb_archive_plugin.h>
#include <board.h>
#include <footprint.h>
#include <properties.h>


std::map<wxString, PCB_LAYER_ID> CADSTAR_PCB_ARCHIVE_PLUGIN::DefaultLayerMappingCallback(
        const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector )
{
    std::map<wxString, PCB_LAYER_ID> retval;

    // Just return a the auto-mapped layers
    for( INPUT_LAYER_DESC layerDesc : aInputLayerDescriptionVector )
    {
        retval.insert( { layerDesc.Name, layerDesc.AutoMapLayer } );
    }

    return retval;
}


void CADSTAR_PCB_ARCHIVE_PLUGIN::RegisterLayerMappingCallback(
        LAYER_MAPPING_HANDLER aLayerMappingHandler )
{
    LAYER_REMAPPABLE_PLUGIN::RegisterLayerMappingCallback( aLayerMappingHandler );
    m_show_layer_mapping_warnings = false; // only show warnings with default callback
}


CADSTAR_PCB_ARCHIVE_PLUGIN::CADSTAR_PCB_ARCHIVE_PLUGIN()
{
    m_board                       = nullptr;
    m_props                       = nullptr;
    m_show_layer_mapping_warnings = true;
    LAYER_REMAPPABLE_PLUGIN::RegisterLayerMappingCallback(
        CADSTAR_PCB_ARCHIVE_PLUGIN::DefaultLayerMappingCallback );
}


CADSTAR_PCB_ARCHIVE_PLUGIN::~CADSTAR_PCB_ARCHIVE_PLUGIN()
{
}


void CADSTAR_PCB_ARCHIVE_PLUGIN::clearLoadedFootprints()
{
    for( FOOTPRINT* fp : m_loaded_footprints )
    {
        delete fp;
    }

    m_loaded_footprints.clear();
}


const wxString CADSTAR_PCB_ARCHIVE_PLUGIN::PluginName() const
{
    return wxT( "CADSTAR PCB Archive" );
}


const wxString CADSTAR_PCB_ARCHIVE_PLUGIN::GetFileExtension() const
{
    return wxT( "cpa" );
}


std::vector<FOOTPRINT*> CADSTAR_PCB_ARCHIVE_PLUGIN::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> retval;

    for( FOOTPRINT* fp : m_loaded_footprints )
    {
        retval.push_back( static_cast<FOOTPRINT*>( fp->Clone() ) );
    }

    return retval;
}


BOARD* CADSTAR_PCB_ARCHIVE_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,
                                         const PROPERTIES* aProperties, PROJECT* aProject,
                                         PROGRESS_REPORTER* aProgressReporter )
{
    m_props = aProperties;
    m_board = aAppendToMe ? aAppendToMe : new BOARD();
    clearLoadedFootprints();

    CADSTAR_PCB_ARCHIVE_LOADER tempPCB( aFileName, m_layer_mapping_handler,
                                        m_show_layer_mapping_warnings, aProgressReporter );
    tempPCB.Load( m_board, aProject );

    //center the board:
    if( aProperties )
    {
        UTF8 page_width;
        UTF8 page_height;

        if( aProperties->Value( "page_width", &page_width )
                && aProperties->Value( "page_height", &page_height ) )
        {
            EDA_RECT bbbox = m_board->GetBoardEdgesBoundingBox();

            int w = atoi( page_width.c_str() );
            int h = atoi( page_height.c_str() );

            int desired_x = ( w - bbbox.GetWidth() ) / 2;
            int desired_y = ( h - bbbox.GetHeight() ) / 2;

            m_board->Move( wxPoint( desired_x - bbbox.GetX(), desired_y - bbbox.GetY() ) );
        }
    }

    // Need to set legacy loading so that netclassess and design rules are loaded correctly
    m_board->m_LegacyNetclassesLoaded = true;
    m_board->m_LegacyDesignSettingsLoaded = true;

    m_loaded_footprints = tempPCB.GetLoadedLibraryFootpints();

    return m_board;
}
