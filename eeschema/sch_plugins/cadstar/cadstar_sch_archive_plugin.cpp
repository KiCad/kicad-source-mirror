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

#include <sch_plugins/cadstar/cadstar_sch_archive_loader.h>
#include <sch_plugins/cadstar/cadstar_sch_archive_plugin.h>

#include <properties.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <wildcards_and_files_ext.h>


const wxString CADSTAR_SCH_ARCHIVE_PLUGIN::GetName() const
{
    return wxT( "CADSTAR Schematic Archive" );
}


const wxString CADSTAR_SCH_ARCHIVE_PLUGIN::GetFileExtension() const
{
    return wxT( "csa" );
}


const wxString CADSTAR_SCH_ARCHIVE_PLUGIN::GetLibraryFileExtension() const
{
    return wxT( "lib" );
}


int CADSTAR_SCH_ARCHIVE_PLUGIN::GetModifyHash() const
{
    return 0;
}


SCH_SHEET* CADSTAR_SCH_ARCHIVE_PLUGIN::Load( const wxString& aFileName, SCHEMATIC* aSchematic,
        SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    wxASSERT( !aFileName || aSchematic != NULL );

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
    }


    if( !rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );
        screen->SetFileName( aFileName );
        rootSheet->SetScreen( screen );
    }

    SYMBOL_LIB_TABLE* libTable = aSchematic->Prj().SchSymbolLibTable();

    wxCHECK_MSG( libTable, NULL, "Could not load symbol lib table." );

    // Lets come up with a nice library name
    wxString libName = aSchematic->Prj().GetProjectName();

    if( libName.IsEmpty() )
    {
        wxFileName fn( rootSheet->GetFileName() );
        libName = fn.GetName();
    }

    if( libName.IsEmpty() )
        libName = "noname";

    libName = LIB_ID::FixIllegalChars( libName, true );

    wxFileName libFileName(
            aSchematic->Prj().GetProjectPath(), libName, KiCadSymbolLibFileExtension );

    SCH_PLUGIN::SCH_PLUGIN_RELEASER sch_plugin;
    sch_plugin.set( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    if( !libTable->HasLibrary( libName ) )
    {
        // Create a new empty symbol library.
        sch_plugin->CreateSymbolLib( libFileName.GetFullPath() );
        wxString libTableUri = "${KIPRJMOD}/" + libFileName.GetFullName();

        // Add the new library to the project symbol library table.
        libTable->InsertRow(
                new SYMBOL_LIB_TABLE_ROW( libName, libTableUri, wxString( "KiCad" ) ) );

        // Save project symbol library table.
        wxFileName fn(
                aSchematic->Prj().GetProjectPath(), SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        // So output formatter goes out of scope and closes the file before reloading.
        {
            FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
            libTable->Format( &formatter, 0 );
        }

        // Relaod the symbol library table.
        aSchematic->Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, NULL );
        aSchematic->Prj().SchSymbolLibTable();
    }

    CADSTAR_SCH_ARCHIVE_LOADER csaFile( aFileName );
    csaFile.Load( aSchematic, rootSheet, &sch_plugin, libFileName );

    sch_plugin->SaveLibrary( libFileName.GetFullPath() );

    return rootSheet;
}


bool CADSTAR_SCH_ARCHIVE_PLUGIN::CheckHeader( const wxString& aFileName )
{
    // TODO: write a parser for the cpa header. For now assume it is valid
    // and throw exceptions when parsing
    return true;
}
