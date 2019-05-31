/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include <common.h>
#include <macros.h>
#include <pl_editor_screen.h>
#include <base_units.h>
#include <pl_editor_id.h>


#define MM_GRID( x )      wxRealPoint( x * IU_PER_MM, x * IU_PER_MM )
#define ZOOM_FACTOR( x )  ( x * IU_PER_MM / 1000 )


/**
    Default zoom values.
    Roughly a 1.5 progression.
*/
static const double pl_editorZoomList[] =
{
    ZOOM_FACTOR( 10.0 ),        // Zoom in
    ZOOM_FACTOR( 15.0 ),
    ZOOM_FACTOR( 22.0 ),
    ZOOM_FACTOR( 35.0 ),
    ZOOM_FACTOR( 50.0 ),
    ZOOM_FACTOR( 80.0 ),
    ZOOM_FACTOR( 120.0 ),
    ZOOM_FACTOR( 160.0 ),
    ZOOM_FACTOR( 230.0 ),
    ZOOM_FACTOR( 290.0 ),
    ZOOM_FACTOR( 380.0 ),
    ZOOM_FACTOR( 500.0 ),
    ZOOM_FACTOR( 750.0 ),
    ZOOM_FACTOR( 1000.0 ),
    ZOOM_FACTOR( 1500.0 ),
    ZOOM_FACTOR( 2000.0 ),
    ZOOM_FACTOR( 3000.0 ),
    ZOOM_FACTOR( 4500.0 ),      // Zoom out
};


// Default grid sizes for page layout editor screens.
static GRID_TYPE pl_editorGridList[] =
{
    // predefined grid list in mm
    { ID_POPUP_GRID_LEVEL_1MM,      MM_GRID( 1.0 )     },
    { ID_POPUP_GRID_LEVEL_0_5MM,    MM_GRID( 0.5 )     },
    { ID_POPUP_GRID_LEVEL_0_25MM,   MM_GRID( 0.25 )    },
    { ID_POPUP_GRID_LEVEL_0_2MM,    MM_GRID( 0.2 )     },
    { ID_POPUP_GRID_LEVEL_0_1MM,    MM_GRID( 0.1 )     },
};


PL_EDITOR_SCREEN::PL_EDITOR_SCREEN( const wxSize& aPageSizeIU ) :
    BASE_SCREEN( SCREEN_T )
{
    for( double zoom : pl_editorZoomList )
        m_ZoomList.push_back( zoom );

    for( GRID_TYPE grid : pl_editorGridList )
        AddGrid( grid );

    // pl_editor uses the same frame position as schematic and board editors
    m_Center = false;

    // Set the working grid size to a reasonable value
    SetGrid( MM_GRID( 1.0 ) );

    InitDataPoints( aPageSizeIU );
    m_NumberOfScreens = 2;
}


PL_EDITOR_SCREEN::~PL_EDITOR_SCREEN()
{
    ClearUndoRedoList();
}


void PL_EDITOR_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    unsigned icnt = aList.m_CommandsList.size();

    if( aItemCount > 0 )
        icnt = aItemCount;

    for( unsigned ii = 0; ii < icnt; ii++ )
    {
        if( aList.m_CommandsList.size() == 0 )
            break;

        PICKED_ITEMS_LIST* curr_cmd = aList.m_CommandsList[0];
        aList.m_CommandsList.erase( aList.m_CommandsList.begin() );

        curr_cmd->ClearListAndDeleteItems();
        delete curr_cmd;    // Delete command
    }
}
