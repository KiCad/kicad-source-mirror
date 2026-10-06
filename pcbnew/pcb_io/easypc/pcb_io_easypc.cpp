/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <easypc/pcb_io_easypc.h>
#include <easypc/easypc_pcb_builder.h>
#include <easypc/easypc_pcb_items.h>

#include <io/easypc/easypc_classes_library.h>
#include <io/easypc/easypc_document.h>

#include <footprint.h>
#include <ki_exception.h>
#include <lib_id.h>
#include <progress_reporter.h>
#include <reporter.h>
#include <string_utils.h>

#include <wx/filename.h>


PCB_IO_EASYPC::PCB_IO_EASYPC() :
        PCB_IO( wxS( "Easy-PC / DesignSpark PCB" ) )
{
}


PCB_IO_EASYPC::~PCB_IO_EASYPC() = default;


static bool isKind( const wxString& aFileName, EASYPC::FILE_KIND aKind )
{
    EASYPC::FILE_INFO info = EASYPC::Probe( aFileName );
    return info.FileKind == aKind;
}


bool PCB_IO_EASYPC::CanReadBoard( const wxString& aFileName ) const
{
    return PCB_IO::CanReadBoard( aFileName ) && isKind( aFileName, EASYPC::FILE_KIND::PCB_DESIGN );
}


bool PCB_IO_EASYPC::CanReadLibrary( const wxString& aFileName ) const
{
    return PCB_IO::CanReadLibrary( aFileName ) && isKind( aFileName, EASYPC::FILE_KIND::PCB_SYMBOL_LIBRARY );
}


long long PCB_IO_EASYPC::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return EASYPC::LIBRARY_FILE::Timestamp( aLibraryPath );
}


PCB_IO_EASYPC::LIBRARY& PCB_IO_EASYPC::library( const wxString& aLibraryPath )
{
    const long long stamp = EASYPC::LIBRARY_FILE::Timestamp( aLibraryPath );
    auto            it = m_libraries.find( aLibraryPath );

    if( it != m_libraries.end() && it->second.Timestamp == stamp )
        return it->second;

    std::unique_ptr<EASYPC::LIBRARY_FILE> file = std::make_unique<EASYPC::LIBRARY_FILE>( aLibraryPath );

    // A component library only names its footprints, which live in a .psl
    if( file->Kind() != EASYPC::FILE_KIND::PCB_SYMBOL_LIBRARY )
        THROW_IO_ERROR( wxString::Format( _( "'%s' is not an Easy-PC PCB symbol library" ), aLibraryPath ) );

    for( const wxString& warning : file->Warnings() )
    {
        if( m_reporter )
            m_reporter->Report( warning, RPT_SEVERITY_WARNING );
    }

    // A changed file drops the footprints converted from the old one
    return m_libraries[aLibraryPath] = LIBRARY{ stamp, std::move( file ), {} };
}


void PCB_IO_EASYPC::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath, bool aBestEfforts,
                                        const std::map<std::string, UTF8>* aProperties )
{
    try
    {
        // KiCad names carry the characters a LIB_ID forbids escaped, and lookups take them back the same way
        for( const wxString& name : library( aLibraryPath ).File->ItemNames() )
            aFootprintNames.Add( EscapeString( name, CTX_LIBID ) );
    }
    catch( const IO_ERROR& )
    {
        if( !aBestEfforts )
            throw;
    }
}


bool PCB_IO_EASYPC::FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName,
                                     const std::map<std::string, UTF8>* aProperties )
{
    return library( aLibraryPath ).File->HasItem( UnescapeString( aFootprintName ) );
}


const FOOTPRINT* PCB_IO_EASYPC::GetEnumeratedFootprint( const wxString& aLibraryPath, const wxString& aFootprintName,
                                                        const std::map<std::string, UTF8>* aProperties )
{
    LIBRARY& lib = library( aLibraryPath );

    if( auto it = lib.Footprints.find( aFootprintName ); it != lib.Footprints.end() )
        return it->second.get();

    const wxString itemName = UnescapeString( aFootprintName );

    if( !lib.File->HasItem( itemName ) )
        return nullptr;

    std::unique_ptr<EASYPC::LIBRARY_ITEM> item = lib.File->LoadItem( itemName );
    const EASYPC::SOURCE_SYMBOL*          symbol = nullptr;

    if( !item->Streams.empty() )
        symbol = dynamic_cast<const EASYPC::SOURCE_SYMBOL*>( item->Streams.front().Item );

    if( !symbol )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Easy-PC library item '%s' in '%s' is not a symbol" ), itemName, aLibraryPath ) );
    }

    // A library item carries the layers its items use, introduced in place; their load order stands for the stack
    std::vector<const EASYPC::SOURCE_LAYER*> layers;

    for( const std::unique_ptr<EASYPC::OBJECT>& obj : item->Objects )
    {
        if( const EASYPC::SOURCE_LAYER* layer = dynamic_cast<const EASYPC::SOURCE_LAYER*>( obj.get() ) )
            layers.push_back( layer );
    }

    LIB_ID id( wxFileName( aLibraryPath ).GetName(), EscapeString( itemName, CTX_LIBID ) );

    std::unique_ptr<FOOTPRINT> footprint =
            EASYPC_PCB::ConvertFootprint( *symbol, EASYPC_PCB::LAYER_MAPPER( layers ), id );
    return ( lib.Footprints[aFootprintName] = std::move( footprint ) ).get();
}


std::unique_ptr<FOOTPRINT> PCB_IO_EASYPC::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                                                         bool                               aKeepUUID,
                                                         const std::map<std::string, UTF8>* aProperties )
{
    const FOOTPRINT* cached = GetEnumeratedFootprint( aLibraryPath, aFootprintName );

    if( !cached )
        return nullptr;

    if( aKeepUUID )
        return std::make_unique<FOOTPRINT>( *cached );

    return std::unique_ptr<FOOTPRINT>( static_cast<FOOTPRINT*>( cached->Duplicate( IGNORE_PARENT_GROUP ) ) );
}


void PCB_IO_EASYPC::loadBoard( const wxString& aFileName, BOARD& aBoard, bool aIsNewLoad,
                               const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    m_props = aProperties;
    m_board = &aBoard;

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_CANCELLED();
    }

    std::unique_ptr<EASYPC::DESIGN_DOCUMENT> doc = EASYPC::LoadDesign( aFileName );
    m_importedDesignRules = EASYPC_PCB::BuildBoard( *doc, aBoard, m_reporter, m_layer_mapping_handler );
}
