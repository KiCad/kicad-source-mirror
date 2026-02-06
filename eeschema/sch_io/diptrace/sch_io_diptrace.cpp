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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <sch_io/diptrace/sch_io_diptrace.h>
#include <sch_io/diptrace/diptrace_sch_parser.h>

#include <io/io_utils.h>

#include <wx/ffile.h>
#include <wx/filename.h>

#include <reporter.h>
#include <progress_reporter.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <schematic.h>
#include <wildcards_and_files_ext.h>


/// DipTrace schematic modern magic header: byte(7) + "DTSCHEM"
static const std::vector<uint8_t> DIPTRACE_SCH_HEADER_V7 = { 0x07, 'D', 'T', 'S', 'C', 'H',
                                                              'E',  'M' };

/// DipTrace schematic legacy magic header: byte(11) + "DTSCHEMx.yy"
static const std::vector<uint8_t> DIPTRACE_SCH_HEADER_V11 = { 0x0B, 'D', 'T', 'S', 'C', 'H',
                                                               'E',  'M' };


bool SCH_IO_DIPTRACE::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadSchematicFile( aFileName ) )
        return false;

    return IO_UTILS::fileHasBinaryHeader( aFileName, DIPTRACE_SCH_HEADER_V7 )
           || IO_UTILS::fileHasBinaryHeader( aFileName, DIPTRACE_SCH_HEADER_V11 );
}


SCH_SHEET* SCH_IO_DIPTRACE::LoadSchematicFile( const wxString& aFileName,
                                                SCHEMATIC* aSchematic,
                                                SCH_SHEET* aAppendToMe,
                                                const std::map<std::string, UTF8>* aProperties )
{
    wxASSERT( !aFileName.IsEmpty() && aSchematic != nullptr );

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "Open canceled by user." ) );
    }

    SCH_SHEET* rootSheet = nullptr;

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr,
                     wxT( "Can't append to a schematic with no root!" ) );
        rootSheet = &aSchematic->Root();
    }
    else
    {
        rootSheet = new SCH_SHEET( aSchematic );
        const_cast<KIID&>( rootSheet->m_Uuid ) = niluuid;

        wxFileName newFilename( aFileName );
        newFilename.SetExt( FILEEXT::KiCadSchematicFileExtension );
        rootSheet->SetFileName( newFilename.GetFullPath() );

        aSchematic->SetTopLevelSheets( { rootSheet } );
    }

    if( !rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );

        wxFileName newFilename( aFileName );
        newFilename.SetExt( FILEEXT::KiCadSchematicFileExtension );
        screen->SetFileName( newFilename.GetFullPath() );

        rootSheet->SetScreen( screen );
        const_cast<KIID&>( rootSheet->m_Uuid ) = niluuid;
    }

    DIPTRACE::SCH_PARSER parser( aFileName, aSchematic, rootSheet,
                                 m_progressReporter, m_reporter );
    parser.Parse();

    return rootSheet;
}
