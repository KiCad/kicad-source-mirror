/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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

/**
 * @brief Pcbnew PLUGIN for CADSTAR PCB Archive (*.cpa) format: an ASCII format
 *        based on S-expressions.
 */

#include <cadstar_pcb_archive_loader.h>
#include <font/fontconfig.h>
#include <pcb_io_cadstar_archive.h>
#include <board.h>
#include <footprint.h>
#include <io/io_utils.h>
#include <pcb_io/pcb_io.h>
#include <reporter.h>


std::map<wxString, PCB_LAYER_ID> PCB_IO_CADSTAR_ARCHIVE::DefaultLayerMappingCallback(
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


void PCB_IO_CADSTAR_ARCHIVE::RegisterCallback( LAYER_MAPPING_HANDLER aLayerMappingHandler )
{
    LAYER_MAPPABLE_PLUGIN::RegisterCallback( aLayerMappingHandler );
    m_show_layer_mapping_warnings = false; // only show warnings with default callback
}


PCB_IO_CADSTAR_ARCHIVE::PCB_IO_CADSTAR_ARCHIVE() : PCB_IO( wxS( "CADSTAR PCB Archive" ) )
{
    m_show_layer_mapping_warnings = true;
    LAYER_MAPPABLE_PLUGIN::RegisterCallback( PCB_IO_CADSTAR_ARCHIVE::DefaultLayerMappingCallback );
}


PCB_IO_CADSTAR_ARCHIVE::~PCB_IO_CADSTAR_ARCHIVE()
{
    clearLoadedFootprints();
}


void PCB_IO_CADSTAR_ARCHIVE::clearLoadedFootprints()
{
    for( FOOTPRINT* fp : m_loaded_footprints )
    {
        delete fp;
    }

    m_loaded_footprints.clear();
}


std::vector<FOOTPRINT*> PCB_IO_CADSTAR_ARCHIVE::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> retval;

    for( FOOTPRINT* fp : m_loaded_footprints )
    {
        retval.push_back( static_cast<FOOTPRINT*>( fp->Clone() ) );
    }

    return retval;
}


BOARD* PCB_IO_CADSTAR_ARCHIVE::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                              const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    m_props = aProperties;
    m_board = aAppendToMe ? aAppendToMe : new BOARD();
    clearLoadedFootprints();

    // Collect the font substitution warnings (RAII - automatically reset on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

    CADSTAR_PCB_ARCHIVE_LOADER tempPCB( aFileName, m_layer_mapping_handler,
                                        m_show_layer_mapping_warnings, m_progressReporter );
    tempPCB.Load( m_board, aProject );

    //center the board:
    if( aProperties )
    {
        UTF8 page_width;
        UTF8 page_height;

        if( auto it = aProperties->find( "page_width" ); it != aProperties->end() )
            page_width = it->second;

        if( auto it = aProperties->find( "page_height" ); it != aProperties->end() )
            page_height = it->second;

        if( !page_width.empty() && !page_height.empty() )
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


bool PCB_IO_CADSTAR_ARCHIVE::checkBoardHeader( const wxString& aFileName ) const
{
    return IO_UTILS::fileStartsWithPrefix( aFileName, wxT( "(CADSTARPCB" ), true );
}


bool PCB_IO_CADSTAR_ARCHIVE::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    return checkBoardHeader( aFileName );
}


bool PCB_IO_CADSTAR_ARCHIVE::CanReadLibrary( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadLibrary( aFileName ) )
        return false;

    return checkBoardHeader( aFileName );
}


bool PCB_IO_CADSTAR_ARCHIVE::CanReadFootprint( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadFootprint( aFileName ) )
        return false;

    return checkBoardHeader( aFileName );
}


void PCB_IO_CADSTAR_ARCHIVE::FootprintEnumerate( wxArrayString&         aFootprintNames,
                                                     const wxString&        aLibraryPath,
                                                     bool                   aBestEfforts,
                                                     const std::map<std::string, UTF8>* aProperties )
{
    ensureLoadedLibrary( aLibraryPath );

    if( !m_cache.count( aLibraryPath ) )
        return; // not found

    for( const auto& [name, fp] : m_cache.at( aLibraryPath ) )
        aFootprintNames.Add( name );
}


bool PCB_IO_CADSTAR_ARCHIVE::FootprintExists( const wxString&        aLibraryPath,
                                                  const wxString&        aFootprintName,
                                                  const std::map<std::string, UTF8>* aProperties )
{
    ensureLoadedLibrary( aLibraryPath );

    if( !m_cache.count( aLibraryPath ) )
        return false;

    if( !m_cache.at( aLibraryPath ).count( aFootprintName ) )
        return false;

    return true;
}


FOOTPRINT* PCB_IO_CADSTAR_ARCHIVE::FootprintLoad( const wxString&        aLibraryPath,
                                                      const wxString&        aFootprintName,
                                                      bool                   aKeepUUID,
                                                      const std::map<std::string, UTF8>* aProperties )
{
    ensureLoadedLibrary( aLibraryPath );

    if( !m_cache.count( aLibraryPath ) )
        return nullptr;

    if( !m_cache.at( aLibraryPath ).count( aFootprintName ) )
        return nullptr;

    if( !m_cache.at( aLibraryPath ).at( aFootprintName ) )
        return nullptr;

    return static_cast<FOOTPRINT*>( m_cache.at( aLibraryPath ).at( aFootprintName )->Duplicate( IGNORE_PARENT_GROUP ) );
}


long long PCB_IO_CADSTAR_ARCHIVE::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    wxFileName fn( aLibraryPath );

    if( fn.IsFileReadable() && fn.GetModificationTime().IsValid() )
        return fn.GetModificationTime().GetValue().GetValue();
    else
        return wxDateTime( 0.0 ).GetValue().GetValue();
}


void PCB_IO_CADSTAR_ARCHIVE::ensureLoadedLibrary( const wxString& aLibraryPath )
{
    // Suppress font substitution warnings (RAII - automatically restored on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( nullptr );

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
        footprintMap.insert( { fp->GetFPID().GetLibItemName().wx_str(), std::move( fp ) } );
    }

    m_cache.insert( { aLibraryPath, std::move( footprintMap ) } );
    m_timestamps[aLibraryPath] = GetLibraryTimestamp( aLibraryPath );
}
