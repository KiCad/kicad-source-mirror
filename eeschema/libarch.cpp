/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras ar wanadoo.fr
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <class_sch_screen.h>
#include <wxstruct.h>
#include <wxEeschemaStruct.h>

#include <class_library.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <wildcards_and_files_ext.h>


bool SCH_EDIT_FRAME::CreateArchiveLibraryCacheFile( bool aUseCurrentSheetFilename )
{
    wxFileName fn;

    if( aUseCurrentSheetFilename )
        fn = GetScreen()->GetFileName();
    else
        fn = g_RootSheet->GetScreen()->GetFileName();

    fn.SetName( fn.GetName() + wxT( "-cache" ) );
    fn.SetExt( SchematicLibraryFileExtension );

    return CreateArchiveLibrary( fn.GetFullPath() );
}


bool SCH_EDIT_FRAME::CreateArchiveLibrary( const wxString& aFileName )
{
    SCH_SCREENS     screens;
    PART_LIBS*      libs = Prj().SchLibs();

    std::auto_ptr<PART_LIB> libCache( new PART_LIB( LIBRARY_TYPE_EESCHEMA, aFileName ) );

    libCache->SetCache();

    /* examine all screens (not sheets) used and build the list of components
     * found in lib.
     * Complex hierarchies are not a problem because we just want
     * to know used components in libraries
     */
    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
        {
            if( item->Type() != SCH_COMPONENT_T )
                continue;

            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            // If not already saved in the new cache, put it:
            if( !libCache->FindEntry( component->GetPartName() ) )
            {
                if( LIB_PART* part = libs->FindLibPart( component->GetPartName() ) )
                {
                    // AddPart() does first clone the part before adding.
                    libCache->AddPart( part );
                }
            }
        }
    }

    try
    {
        FILE_OUTPUTFORMATTER    formatter( aFileName );

        if( !libCache->Save( formatter ) )
        {
            wxString msg = wxString::Format( _(
                "An error occurred attempting to save component library '%s'." ),
                GetChars( aFileName )
                );
            DisplayError( this, msg );
            return false;
        }
    }
    catch( ... /* IO_ERROR ioe */ )
    {
        wxString msg = wxString::Format( _(
            "Failed to create component library file '%s'" ),
            GetChars( aFileName )
            );
        DisplayError( this, msg );
        return false;
    }

    return true;
}
