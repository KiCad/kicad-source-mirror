/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras ar wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file libarch.cpp
 * @brief Module for generation of component archive files.
 */

#include <fctsys.h>
#include <confirm.h>
#include <wxstruct.h>
#include <wildcards_and_files_ext.h>

#include <class_sch_screen.h>
#include <schframe.h>
#include <symbol_lib_table.h>
#include <class_library.h>
#include <sch_component.h>
#include <sch_sheet.h>


bool SCH_EDIT_FRAME::CreateArchiveLibraryCacheFile( bool aUseCurrentSheetFilename )
{
    wxFileName fn;

    if( aUseCurrentSheetFilename )
        fn = GetScreen()->GetFileName();
    else
        fn = g_RootSheet->GetScreen()->GetFileName();

    fn.SetName( fn.GetName() + "-cache" );
    fn.SetExt( SchematicLibraryFileExtension );

    bool success = CreateArchiveLibrary( fn.GetFullPath() );

    // Update the schematic symbol library links.
    // because the lib cache has changed
    SCH_SCREENS schematic;
    schematic.UpdateSymbolLinks();

    return success;
}


bool SCH_EDIT_FRAME::CreateArchiveLibrary( const wxString& aFileName )
{
    wxString          msg;
    SCH_SCREENS       screens;

    // Create a new empty library to archive components:
    std::unique_ptr<PART_LIB> archLib( new PART_LIB( LIBRARY_TYPE_EESCHEMA, aFileName ) );

    // Save symbols to file only when the library will be fully filled
    archLib->EnableBuffering();

    /* Examine all screens (not hierarchical sheets) used in the schematic and build a
     * library of unique symbols found in all screens.  Complex hierarchies are not a
     * problem because we just want to know the library symbols used in the schematic
     * not their reference.
     */
    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
        {
            if( item->Type() != SCH_COMPONENT_T )
                continue;

            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            if( archLib->FindAlias( component->GetLibId().GetLibItemName() ) )
                continue;

            LIB_PART* part = GetLibPart( component->GetLibId() );

            if( !part )
            {
                try
                {
                    part = Prj().SchLibs()->GetCacheLibrary()->FindPart(
                        component->GetLibId().GetLibItemName() );
                }
                catch( ... /* IO_ERROR ioe */ )
                {
                    msg.Printf( _( "Failed to add symbol %s to library file '%s'" ),
                                component->GetLibId().GetLibItemName().wx_str(), aFileName );
                    DisplayError( this, msg );
                    return false;
                }

                if( part )
                {
                    // AddPart() does first clone the part before adding.
                    archLib->AddPart( part );
                }
            }
        }
    }

    try
    {
        archLib->Save( false );
    }
    catch( ... /* IO_ERROR ioe */ )
    {
        msg.Printf( _( "Failed to save symbol library file '%s'" ), aFileName );
        DisplayError( this, msg );
        return false;
    }

    return true;
}
