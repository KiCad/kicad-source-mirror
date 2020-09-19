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

    mProperties = aProperties;
    mSchematic  = aSchematic;

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, "Can't append to a schematic with no root!" );
        mRootSheet = &aSchematic->Root();
    }
    else
    {
        mRootSheet = new SCH_SHEET( aSchematic );
        mRootSheet->SetFileName( aFileName );
    }

    if( !mRootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( mSchematic );
        screen->SetFileName( aFileName );
        mRootSheet->SetScreen( screen );
    }


    CADSTAR_SCH_ARCHIVE_LOADER csaFile( aFileName );
    csaFile.Load( mSchematic, mRootSheet );

    return mRootSheet;
}


bool CADSTAR_SCH_ARCHIVE_PLUGIN::CheckHeader( const wxString& aFileName )
{
    // TODO: write a parser for the cpa header. For now assume it is valid
    // and throw exceptions when parsing
    return true;
}
