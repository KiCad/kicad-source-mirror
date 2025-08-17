/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Chetan Subhash Shinde<chetanshinde2001@gmail.com>
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_io/ltspice/ltspice_schematic.h>
#include <sch_io/ltspice/sch_io_ltspice.h>
#include <sch_io/ltspice/sch_io_ltspice_parser.h>

#include <project_sch.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <kiplatform/environment.h>

/**
 * @brief schematic PLUGIN for LTspice (*.asc) and (.asy) format.
 */

int SCH_IO_LTSPICE::GetModifyHash() const
{
    return 0;
}


SCH_SHEET* SCH_IO_LTSPICE::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                              SCH_SHEET*             aAppendToMe,
                                              const std::map<std::string, UTF8>* aProperties )
{
    wxASSERT( !aFileName || aSchematic );

    SCH_SHEET* rootSheet = nullptr;

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, "Can't append to a schematic with no root!" );
        rootSheet = &aSchematic->Root();
    }
    else
    {
        rootSheet = new SCH_SHEET( aSchematic );
        rootSheet->SetFileName( aFileName );
        aSchematic->SetRoot( rootSheet );
    }

    if( !rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );

        screen->SetFileName( aFileName );
        rootSheet->SetScreen( screen );

        // Virtual root sheet UUID must be the same as the schematic file UUID.
        const_cast<KIID&>( rootSheet->m_Uuid ) = screen->GetUuid();
    }

    // Windows path: C:\Users\USERNAME\AppData\Local\LTspice\lib
    wxFileName ltspiceDataDir( KIPLATFORM::ENV::GetUserLocalDataPath(), wxEmptyString );
    ltspiceDataDir.AppendDir( wxS( "LTspice" ) );
    ltspiceDataDir.AppendDir( wxS( "lib" ) );

    if( !ltspiceDataDir.DirExists() )
    {
        // Mac path
        ltspiceDataDir = wxFileName( KIPLATFORM::ENV::GetUserDataPath(), wxEmptyString );
        ltspiceDataDir.RemoveLastDir();        // "kicad"
        ltspiceDataDir.AppendDir( wxS( "LTspice" ) );
        ltspiceDataDir.AppendDir( wxS( "lib" ) );
    }

    if( !ltspiceDataDir.DirExists() )
    {
        // See if user has older version of LTspice installed
        // (e.g. C:\Users\USERNAME\Documents\LTspiceXVII\lib
        wxString foundFile = wxFindFirstFile( KIPLATFORM::ENV::GetDocumentsPath() +
                                              wxFileName::GetPathSeparator() + "LTspice*", wxDIR );

        while( !foundFile.empty() )
        {
            ltspiceDataDir = wxFileName(foundFile, wxEmptyString);
            ltspiceDataDir.AppendDir( wxS( "lib" ) );

            if( ltspiceDataDir.DirExists() )
                break;

            foundFile = wxFindNextFile();
        }
    }

    if( !ltspiceDataDir.DirExists() )
    {
        wxFileName fn( aFileName );
        fn.SetFullName( wxEmptyString );
        fn.AppendDir( "lib" );

        wxString localLibPath = fn.GetFullPath();

        if( wxDirExists( localLibPath ) )
        {
            ltspiceDataDir = localLibPath;
        }
        else
        {
            m_reporter->Report( wxString::Format( _( "Unable to find LTspice symbols.\nInstall "
                                                     "LTspice or put its library files into %s" ),
                                                  localLibPath ),
                                RPT_SEVERITY_WARNING );
        }
    }

    try
    {
        LTSPICE_SCHEMATIC ascFile( aFileName, ltspiceDataDir, m_reporter, m_progressReporter );
        ascFile.Load( aSchematic, rootSheet, aFileName, m_reporter );
    }
    catch( IO_ERROR& e )
    {
        m_reporter->Report( e.What(), RPT_SEVERITY_ERROR );
    }

    aSchematic->CurrentSheet().UpdateAllScreenReferences();

    // fixing all junctions at the end
    aSchematic->FixupJunctionsAfterImport();

    return rootSheet;
}
