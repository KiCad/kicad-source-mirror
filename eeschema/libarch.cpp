/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#include "fctsys.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxstruct.h"
#include "sch_item_struct.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "netlist.h"
#include "protos.h"
#include "class_library.h"
#include "sch_component.h"

#include <wx/wfstream.h>


bool SCH_EDIT_FRAME::CreateArchiveLibrary( const wxString& aFileName )
{
    wxString msg;
    LIB_COMPONENT* libComponent;
    CMP_LIBRARY* libCache;
    SCH_SCREENS screens;

    libCache = new CMP_LIBRARY( LIBRARY_TYPE_EESCHEMA, aFileName );
    libCache->SetCache();

    /* examine all screens (not sheets) used and build the list of components
     * found in lib complex hierarchies are not a problem because we just want
     * to know used components in libraries
     */
    for( SCH_SCREEN* screen = screens.GetFirst(); screen != NULL; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
        {
            if( item->Type() != SCH_COMPONENT_T )
                continue;

            SCH_COMPONENT* component = (SCH_COMPONENT*) item;
            // If not already saved in the new cache, put it:

            if( libCache->FindEntry( component->GetLibName()) == NULL )
            {
                libComponent = CMP_LIBRARY::FindLibraryComponent( component->GetLibName() );

                if( libComponent )    // if NULL : component not found, cannot be stored
                    libCache->AddComponent( libComponent );
            }
        }
    }

    wxFFileOutputStream os( aFileName, wxT( "wt" ) );

    if( !os.IsOk() )
    {
        msg = wxT( "Failed to create component library file " ) + aFileName;
        DisplayError( this, msg );
        return false;
    }

    STREAM_OUTPUTFORMATTER formatter( os );

    if( !libCache->Save( formatter ) )
    {
        msg.Printf( _( "An error occurred attempting to save component \
library <%s>." ), GetChars( aFileName ) );
        DisplayError( this, msg );
        return false;
    }

    return true;
}
