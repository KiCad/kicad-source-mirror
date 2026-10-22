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

#include <target3001/pcb_io_target3001.h>
#include <target3001/target3001_pcb_builder.h>

#include <board.h>
#include <footprint.h>
#include <io/target3001/target3001_parser.h>
#include <progress_reporter.h>


PCB_IO_TARGET3001::PCB_IO_TARGET3001() : PCB_IO( wxS( "TARGET 3001!" ) )
{
}


bool PCB_IO_TARGET3001::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    return TARGET3001::LooksLikeProject( aFileName );
}


void PCB_IO_TARGET3001::loadBoard( const wxString& aFileName, BOARD& aBoard, bool aIsNewLoad,
                                   const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    // The import sets up the board's stack and origin, which would disturb a board it is appended to
    if( !aIsNewLoad )
        THROW_IO_ERROR( _( "TARGET 3001! boards cannot be appended to an existing board." ) );

    m_props = aProperties;
    m_board = &aBoard;
    m_importedDesignRules.clear();
    m_libraryFootprints.clear();

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_CANCELLED();
    }

    TARGET3001::PROJECT project = TARGET3001::ParseFile( aFileName );

    TARGET3001_PCB_BUILDER builder( m_board, project, m_reporter );
    builder.Build();

    m_importedDesignRules = builder.DesignRules();

    // One unrotated, front-side copy of each package seeds the project footprint library
    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        wxString name = fp->GetFPID().GetUniStringLibItemName();

        if( m_libraryFootprints.count( name ) )
            continue;

        std::unique_ptr<FOOTPRINT> def( static_cast<FOOTPRINT*>( fp->Duplicate( false ) ) );
        def->SetParent( nullptr );

        if( def->IsFlipped() )
            def->Flip( def->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );

        def->SetOrientation( ANGLE_0 );
        def->SetPosition( VECTOR2I( 0, 0 ) );
        m_libraryFootprints.emplace( name, std::move( def ) );
    }
}


std::vector<FOOTPRINT*> PCB_IO_TARGET3001::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> result;

    for( const auto& [name, fp] : m_libraryFootprints )
        result.push_back( static_cast<FOOTPRINT*>( fp->Clone() ) );

    return result;
}
