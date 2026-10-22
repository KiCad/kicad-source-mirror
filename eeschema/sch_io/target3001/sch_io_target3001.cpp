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

#include <sch_io/target3001/sch_io_target3001.h>
#include <sch_io/target3001/target3001_sch_builder.h>

#include <io/target3001/target3001_parser.h>

#include <wx/filename.h>

#include <progress_reporter.h>
#include <project.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <wildcards_and_files_ext.h>


bool SCH_IO_TARGET3001::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadSchematicFile( aFileName ) )
        return false;

    return TARGET3001::LooksLikeProject( aFileName );
}


SCH_SHEET* SCH_IO_TARGET3001::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                                 SCH_SHEET*                         aAppendToMe,
                                                 const std::map<std::string, UTF8>* aProperties )
{
    wxASSERT( !aFileName.IsEmpty() && aSchematic != nullptr );

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_CANCELLED();
    }

    TARGET3001::PROJECT project = TARGET3001::ParseFile( aFileName );

    wxFileName newFilename( aFileName );
    newFilename.SetExt( FILEEXT::KiCadSchematicFileExtension );

    SCH_SHEET* rootSheet = nullptr;

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, wxT( "Can't append to a schematic with no root!" ) );
        rootSheet = &aSchematic->Root();
    }
    else
    {
        // A real UUID lets SetTopLevelSheets() accept the content root as a top-level sheet
        rootSheet = new SCH_SHEET( aSchematic );
        rootSheet->SetFileName( newFilename.GetFullPath() );

        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );
        screen->SetFileName( newFilename.GetFullPath() );
        rootSheet->SetScreen( screen );

        aSchematic->SetTopLevelSheets( { rootSheet } );
    }

    if( !rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );
        screen->SetFileName( newFilename.GetFullPath() );
        rootSheet->SetScreen( screen );
    }

    TARGET3001_SCH_BUILDER builder( aSchematic, rootSheet, project, aFileName, m_reporter );
    builder.Build();

    aSchematic->RefreshHierarchy();

    // Foreign imports do not get per-sheet instance data from the native loader
    if( !aAppendToMe )
    {
        wxString projectName = aSchematic->Project().GetProjectName();

        if( projectName.IsEmpty() )
            projectName = wxFileName( aFileName ).GetName();

        SCH_SHEET_LIST sheets = aSchematic->BuildUnorderedSheetList();
        sheets.AddNewSymbolInstances( SCH_SHEET_PATH(), projectName );
        sheets.AddNewSheetInstances( SCH_SHEET_PATH(), 0 );

        if( sheets.AllSheetPageNumbersEmpty() )
            sheets.SetInitialPageNumbers();
    }

    return rootSheet;
}
