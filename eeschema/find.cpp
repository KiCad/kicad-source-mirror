/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <kicad_string.h>
#include <sch_edit_frame.h>
#include <base_units.h>
#include <trace_helpers.h>
#include <sch_view.h>
#include <general.h>
#include <class_library.h>
#include <lib_pin.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>


SCH_ITEM* SCH_EDIT_FRAME::FindComponentAndItem( const wxString& aReference,
                                                bool            aSearchHierarchy,
                                                SCH_SEARCH_T    aSearchType,
                                                const wxString& aSearchText )
{
    SCH_SHEET_PATH* sheet = NULL;
    SCH_SHEET_PATH* sheetWithComponentFound = NULL;
    SCH_ITEM*       item = NULL;
    SCH_COMPONENT*  Component = NULL;
    wxPoint         pos;
    bool            notFound = true;
    LIB_PIN*        pin = nullptr;
    SCH_SHEET_LIST  sheetList( g_RootSheet );
    EDA_ITEM*       foundItem = nullptr;

    if( !aSearchHierarchy )
        sheetList.push_back( *g_CurrentSheet );
    else
        sheetList.BuildSheetList( g_RootSheet );

    for( SCH_SHEET_PATHS_ITER it = sheetList.begin(); it != sheetList.end(); ++it )
    {
        sheet = &(*it);
        item = (*it).LastDrawList();

        for( ; ( item != NULL ) && ( notFound == true ); item = item->Next() )
        {
            if( item->Type() != SCH_COMPONENT_T )
                continue;

            SCH_COMPONENT* pSch = (SCH_COMPONENT*) item;

            if( aReference.CmpNoCase( pSch->GetRef( sheet ) ) == 0 )
            {
                Component = pSch;
                sheetWithComponentFound = sheet;

                switch( aSearchType )
                {
                default:
                case FIND_COMPONENT_ONLY:    // Find component only
                    notFound = false;
                    pos = pSch->GetPosition();
                    foundItem = Component;
                    break;

                case FIND_PIN:               // find a pin
                    pos = pSch->GetPosition();  // temporary: will be changed if the pin is found.
                    pin = pSch->GetPin( aSearchText );

                    if( pin == NULL )
                        break;

                    notFound = false;
                    pos += pin->GetPosition();
                    foundItem = Component;
                    break;

                case FIND_REFERENCE:         // find reference
                    notFound = false;
                    pos = pSch->GetField( REFERENCE )->GetPosition();
                    foundItem = pSch->GetField( REFERENCE );
                    break;

                case FIND_VALUE:             // find value
                    pos = pSch->GetPosition();

                    if( aSearchText.CmpNoCase( pSch->GetField( VALUE )->GetShownText() ) != 0 )
                        break;

                    notFound = false;
                    pos = pSch->GetField( VALUE )->GetPosition();
                    foundItem = pSch->GetField( VALUE );

                    break;
                }
            }
        }

        if( notFound == false )
            break;
    }

    if( Component )
    {
        sheet = sheetWithComponentFound;

        if( *sheet != *g_CurrentSheet )
        {
            sheet->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *g_CurrentSheet = *sheet;
            DisplayCurrentSheet();
        }

        wxPoint delta;
        pos  -= Component->GetPosition();
        delta = Component->GetTransform().TransformCoordinate( pos );
        pos   = delta + Component->GetPosition();

        SetCrossHairPosition( pos );
        CenterScreen( pos, false );
    }

    /* Print diag */
    wxString msg_item;
    wxString msg;

    switch( aSearchType )
    {
    default:
    case FIND_COMPONENT_ONLY: msg_item = _( "component" );                         break;
    case FIND_PIN:            msg_item.Printf( _( "pin %s" ), aSearchText );       break;
    case FIND_REFERENCE:      msg_item.Printf( _( "reference %s" ), aSearchText ); break;
    case FIND_VALUE:          msg_item.Printf( _( "value %s" ), aSearchText );     break;
    case FIND_FIELD:          msg_item.Printf( _( "field %s" ), aSearchText );     break;
    }

    if( Component )
    {
        if( !notFound )
            msg.Printf( _( "%s %s found" ), aReference, msg_item );
        else
            msg.Printf( _( "%s found but %s not found" ), aReference, msg_item );
    }
    else
        msg.Printf( _( "Component %s not found" ), aReference );

    SetStatusText( msg );

    // highlight selection if foundItem is not null, or clear any highlighted selection
    GetCanvas()->GetView()->HighlightItem( foundItem, pin );
    GetCanvas()->Refresh();

    return item;
}

