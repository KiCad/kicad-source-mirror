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

#include "pcb_io_sprint_layout.h"
#include "sprint_layout_parser.h"

#include <board.h>
#include <footprint.h>
#include <pcb_group.h>
#include <zone.h>
#include <font/fontconfig.h>
#include <gestfich.h>
#include <reporter.h>

#include <ranges>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/dir.h>


PCB_IO_SPRINT_LAYOUT::PCB_IO_SPRINT_LAYOUT() :
        PCB_IO( wxS( "Sprint Layout" ) )
{
}


void PCB_IO_SPRINT_LAYOUT::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                                               bool aBestEfforts, const std::map<std::string, UTF8>* aProperties )
{
    wxFileName libFn( aLibraryPath );

    if( libFn.FileExists() && libFn.GetExt().Upper() == wxS( "LMK" ) )
    {
        aFootprintNames.Add( libFn.GetName() );
        return;
    }

    if( wxDir::Exists( aLibraryPath ) )
    {
        wxArrayString files;
        CollectFilesLoopSafe( aLibraryPath, files, wxEmptyString, wxDIR_FILES | wxDIR_DIRS );

        for( const wxString& filePath : files )
        {
            wxFileName file( filePath );

            if( file.GetExt().Upper() != wxS( "LMK" ) )
                continue;

            file.MakeRelativeTo( aLibraryPath );
            aFootprintNames.Add( file.GetFullPath().BeforeLast( '.' ) );
        }
    }
}


std::unique_ptr<FOOTPRINT> PCB_IO_SPRINT_LAYOUT::FootprintLoad( const wxString& aLibraryPath,
                                                                const wxString& aFootprintName, bool aKeepUUID,
                                                                const std::map<std::string, UTF8>* aProperties )
{
    wxFileName libFn( aLibraryPath );
    wxFileName lmkPath;

    if( libFn.FileExists() && libFn.GetExt().Upper() == wxS( "LMK" ) )
    {
        lmkPath = libFn;
    }
    else
    {
        if( !wxDir::Exists( aLibraryPath ) )
            return nullptr;

        lmkPath = wxFileName( aLibraryPath + wxFileName::GetPathSeparator() + aFootprintName + wxS( ".LMK" ) );

        if( !lmkPath.FileExists() )
        {
            lmkPath.SetExt( "lmk" );

            if( !lmkPath.FileExists() )
                return nullptr;
        }
    }

    SPRINT_LAYOUT_PARSER parser;

    if( !parser.ParseMacroFile( lmkPath.GetFullPath() ) )
        return nullptr;

    return parser.CreateFootprint();
}


PCB_IO_SPRINT_LAYOUT::~PCB_IO_SPRINT_LAYOUT()
{
    for( std::unique_ptr<FOOTPRINT>& fp : m_loadedFootprints | std::views::values )
        fp->SetParent( nullptr );
}


bool PCB_IO_SPRINT_LAYOUT::CanReadBoard( const wxString& aFileName ) const
{
    const wxFileName fn( aFileName );

    if( !fn.FileExists() )
        return false;

    wxString ext = fn.GetExt().Lower();

    if( ext != wxS( "lay6" ) && ext != wxS( "lay" ) )
        return false;

    // Check magic bytes: version (<=6), 0x33, 0xAA, 0xFF
    wxFFileInputStream stream( aFileName );

    if( !stream.IsOk() || stream.GetLength() < 8 )
        return false;

    uint8_t header[4];
    stream.Read( header, 4 );

    if( stream.LastRead() != 4 )
        return false;

    if( header[0] > 6 || header[1] != 0x33 || header[2] != 0xAA || header[3] != 0xFF )
        return false;

    return true;
}


void PCB_IO_SPRINT_LAYOUT::loadBoard( const wxString& aFileName, BOARD& aBoard, bool aIsNewLoad,
                                      const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

    m_props = aProperties;
    m_loadedFootprints.clear();

    SPRINT_LAYOUT_PARSER parser;

    if( !parser.ParseBoard( aFileName ) )
        THROW_IO_ERRORF( _( "Failed to parse Sprint Layout file '%s'" ), aFileName );

    const auto& fileData = parser.GetFileData();
    size_t      boardIndex = 0;

    if( m_props && m_props->contains( "pcb_id" ) )
    {
        unsigned long idx = std::stoul( m_props->at( "pcb_id" ) );
        boardIndex = static_cast<size_t>( idx );
    }
    else if( fileData.boards.size() > 1 && m_choose_project_handler )
    {
        std::vector<IMPORT_PROJECT_DESC> options;

        for( size_t i = 0; i < fileData.boards.size(); i++ )
        {
            IMPORT_PROJECT_DESC desc;
            wxString            name = wxString::FromUTF8( fileData.boards[i].name );

            if( name.empty() )
                name = wxString::Format( wxS( "Board %zu" ), i + 1 );

            desc.PCBName = name;
            desc.PCBId = wxString::Format( wxS( "%zu" ), i );
            options.push_back( desc );
        }

        std::vector<IMPORT_PROJECT_DESC> chosen = m_choose_project_handler( options );

        if( chosen.empty() )
            THROW_IO_CANCELLED();

        unsigned long idx = std::stoul( chosen[0].PCBId.ToStdString() );
        boardIndex = static_cast<size_t>( idx );
    }

    if( aIsNewLoad )
    {
        if( !parser.CreateBoard( aBoard, m_loadedFootprints, boardIndex ) )
            THROW_IO_ERRORF( _( "Failed to create board from Sprint Layout file '%s'" ), aFileName );
    }
    else
    {
        // Parse into a scratch board, then copy the design's items into the caller's
        // board.  The destination keeps its own setup when merging.
        std::unique_ptr<BOARD>         scratchBoard = std::make_unique<BOARD>();
        std::map<EDA_ITEM*, EDA_ITEM*> ptrMap;

        if( !parser.CreateBoard( *scratchBoard, m_loadedFootprints, boardIndex ) )
            THROW_IO_ERRORF( _( "Failed to create board from Sprint Layout file '%s'" ), aFileName );

        for( NETINFO_ITEM* netInfo : scratchBoard->GetNetInfo() )
        {
            NETINFO_ITEM* newNetInfo = static_cast<NETINFO_ITEM*>( netInfo->Clone() );
            ptrMap[netInfo] = newNetInfo;
            aBoard.Add( newNetInfo );
        }

        for( FOOTPRINT* fp : scratchBoard->Footprints() )
        {
            FOOTPRINT* newFP = static_cast<FOOTPRINT*>( fp->Clone() );
            ptrMap[ fp ] = newFP;
            aBoard.Add( newFP );
        }

        for( BOARD_ITEM* drawing : scratchBoard->Drawings() )
        {
            BOARD_ITEM* newDrawing = static_cast<BOARD_ITEM*>( drawing->Clone() );
            ptrMap[drawing] = newDrawing;
            aBoard.Add( newDrawing );
        }

        for( ZONE* zone : scratchBoard->Zones() )
        {
            ZONE* newZone = static_cast<ZONE*>( zone->Clone() );
            ptrMap[zone] = newZone;
            aBoard.Add( newZone );
        }

        for( PCB_GROUP* group : scratchBoard->Groups() )
        {
            PCB_GROUP* newGroup = static_cast<PCB_GROUP*>( group->Clone() );
            ptrMap[group] = newGroup;
            aBoard.Add( newGroup );
        }

        // Rebuild groups
        for( PCB_GROUP* group : scratchBoard->Groups() )
        {
            PCB_GROUP* newGroup = static_cast<PCB_GROUP*>( ptrMap[ group ] );

            newGroup->GetItems().clear();

            for( EDA_ITEM* member : group->GetItems() )
            {
                if( ptrMap.count( member ) )
                    newGroup->AddItem( ptrMap[ member ] );
            }
        }

        // Rebuild zone nets
        for( ZONE* zone : scratchBoard->Zones() )
        {
            ZONE* newZone = static_cast<ZONE*>( ptrMap[zone] );

            if( newZone && newZone->GetNet() )
                newZone->SetNet( static_cast<NETINFO_ITEM*>( ptrMap[ newZone->GetNet() ] ) );
        }
    }
}


std::vector<FOOTPRINT*> PCB_IO_SPRINT_LAYOUT::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> result;

    for( auto& [name, footprint] : m_loadedFootprints )
        result.push_back( static_cast<FOOTPRINT*>( footprint->Clone() ) );

    return result;
}
