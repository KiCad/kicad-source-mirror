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
#include <string_utf8_map.h>


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
                                         const STRING_UTF8_MAP* aProperties, PROJECT* aProject,
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
            BOX2I bbbox = m_board->GetBoardEdgesBoundingBox();

            int w = atoi( page_width.c_str() );
            int h = atoi( page_height.c_str() );

            int desired_x = ( w - bbbox.GetWidth() ) / 2;
            int desired_y = ( h - bbbox.GetHeight() ) / 2;

            m_board->Move( VECTOR2I( desired_x - bbbox.GetX(), desired_y - bbbox.GetY() ) );
        }
    }

    // Need to set legacy loading so that netclassess and design rules are loaded correctly
    m_board->m_LegacyNetclassesLoaded = true;
    m_board->m_LegacyDesignSettingsLoaded = true;

    m_loaded_footprints = tempPCB.GetLoadedLibraryFootpints();

    return m_board;
}


void CADSTAR_PCB_ARCHIVE_PLUGIN::FootprintEnumerate( wxArrayString&         aFootprintNames,
                                                     const wxString&        aLibraryPath,
                                                     bool                   aBestEfforts,
                                                     const STRING_UTF8_MAP* aProperties )
{
    ensureLoadedLibrary( aLibraryPath );

    if( !m_cache.count( aLibraryPath ) )
        return; // not found

    for( const auto& [name, fp] : m_cache.at( aLibraryPath ) )
        aFootprintNames.Add( name );
}


const FOOTPRINT*
CADSTAR_PCB_ARCHIVE_PLUGIN::GetEnumeratedFootprint( const wxString&        aLibraryPath,
                                                    const wxString&        aFootprintName,
                                                    const STRING_UTF8_MAP* aProperties )
{
    ensureLoadedLibrary( aLibraryPath );

    if( !m_cache.count( aLibraryPath ) )
        return nullptr;

    if( !m_cache.at( aLibraryPath ).count( aFootprintName ) )
        return nullptr;

    return m_cache.at( aLibraryPath ).at( aFootprintName ).get();
}


bool CADSTAR_PCB_ARCHIVE_PLUGIN::FootprintExists( const wxString&        aLibraryPath,
                                                  const wxString&        aFootprintName,
                                                  const STRING_UTF8_MAP* aProperties )
{
    ensureLoadedLibrary( aLibraryPath );

    if( !m_cache.count( aLibraryPath ) )
        return false;

    if( !m_cache.at( aLibraryPath ).count( aFootprintName ) )
        return false;

    return true;
}


FOOTPRINT* CADSTAR_PCB_ARCHIVE_PLUGIN::FootprintLoad( const wxString&        aLibraryPath,
                                                      const wxString&        aFootprintName,
                                                      bool                   aKeepUUID,
                                                      const STRING_UTF8_MAP* aProperties )
{
    ensureLoadedLibrary( aLibraryPath );

    if( !m_cache.count( aLibraryPath ) )
        return nullptr;

    if( !m_cache.at( aLibraryPath ).count( aFootprintName ) )
        return nullptr;

    return static_cast<FOOTPRINT*>( m_cache.at( aLibraryPath ).at( aFootprintName )->Duplicate() );
}


long long CADSTAR_PCB_ARCHIVE_PLUGIN::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    wxFileName fn( aLibraryPath );

    if( fn.IsFileReadable() )
        return fn.GetModificationTime().GetValue().GetValue();
    else
        return wxDateTime( 0.0 ).GetValue().GetValue();
}


void CADSTAR_PCB_ARCHIVE_PLUGIN::ensureLoadedLibrary( const wxString& aLibraryPath )
{
    if( m_cache.count( aLibraryPath ) )
    {
        wxCHECK( m_timestamps.count( aLibraryPath ), /*void*/ );

        if( m_timestamps.at( aLibraryPath ) == GetLibraryTimestamp( aLibraryPath ) )
            return;
    }

    CADSTAR_PCB_ARCHIVE_LOADER csLoader( aLibraryPath, m_layer_mapping_handler,
                                         false /*don't log stackup warnings*/, nullptr );

    NAME_TO_FOOTPRINT_MAP                   footprintMap;
    std::vector<std::unique_ptr<FOOTPRINT>> footprints = csLoader.LoadLibrary();

    for( std::unique_ptr<FOOTPRINT>& fp : footprints )
    {
        footprintMap.insert( { fp->GetFPID().GetLibItemName(), std::move( fp ) } );
    }

    m_cache.insert( { aLibraryPath, std::move( footprintMap ) } );
    m_timestamps[aLibraryPath] = GetLibraryTimestamp( aLibraryPath );
}
