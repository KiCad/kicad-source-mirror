/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file highlight_connection.cpp
 * @brief This file contains basic functions related to the command to
 * highlight a connection (wires and labels) in a schematic
 * (that can be a simple or a complex hierarchy)
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <schframe.h>
#include <erc.h>

#include <class_netlist_object.h>


bool SCH_EDIT_FRAME::HighlightConnectionAtPosition( wxPoint aPosition )
{
    m_SelectedNetName = "";
    bool buildNetlistOk = false;

    // find which connected item is selected
    EDA_ITEMS nodeList;
    wxPoint   gridPosition = GetGridPosition( aPosition );

    if( GetScreen()->GetNode( gridPosition,nodeList ) )
    {
        if( TestDuplicateSheetNames( false ) > 0 )
            wxMessageBox( _( "Error: duplicate sub-sheet names found in current sheet. Fix it" ) );
        else
        {
            // Build netlist info to get the proper netnames of connected items
            std::unique_ptr<NETLIST_OBJECT_LIST> objectsConnectedList( BuildNetListBase() );
            buildNetlistOk = true;

            for( auto obj : *objectsConnectedList )
            {
                if( obj->m_SheetPath == *m_CurrentSheet && obj->m_Comp == nodeList[0] )
                {
                    m_SelectedNetName = obj->GetNetName( true );
                    break;
                }
            }
        }
    }

    SetStatusText( "selected net: " + m_SelectedNetName );
    SetCurrentSheetHighlightFlags();
    m_canvas->Refresh();

    return buildNetlistOk;
}


bool SCH_EDIT_FRAME::SetCurrentSheetHighlightFlags()
{
    SCH_SCREEN* screen = m_CurrentSheet->LastScreen();

    // Disable highlight flag on all items in the current screen
    for( SCH_ITEM* ptr = screen->GetDrawItems(); ptr; ptr = ptr->Next() )
    {
        ptr->SetState( BRIGHTENED, false );

        if( ptr->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN& pin : static_cast<SCH_SHEET*>( ptr )->GetPins() )
                pin.SetState( BRIGHTENED, false );
        }
    }

    if( m_SelectedNetName == "" )
        return true;

    if( TestDuplicateSheetNames( false ) > 0 )
        return false;

    // Build netlist info to get the proper netnames
    std::unique_ptr<NETLIST_OBJECT_LIST> objectsConnectedList( BuildNetListBase( false ) );

    // highlight the items belonging to this net
    for( auto obj1 : *objectsConnectedList )
    {
        if( obj1->m_SheetPath == *m_CurrentSheet &&
            obj1->GetNetName( true ) == m_SelectedNetName && obj1->m_Comp )
        {
            obj1->m_Comp->SetState( BRIGHTENED, true );

            //if a bus is associated with this net highlight it as well
            if( obj1->m_BusNetCode )
            {
                for( auto obj2 : *objectsConnectedList )
                {
                    if( obj2 && obj2->m_Comp && obj2->m_SheetPath == *m_CurrentSheet &&
                        obj1->m_BusNetCode == obj2->m_BusNetCode )
                        obj2->m_Comp->SetState( BRIGHTENED, true );
                }
            }
        }
    }

    return true;
}
