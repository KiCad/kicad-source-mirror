/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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
 * @file class_gbr_screen.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <class_gbr_screen.h>
#include <base_units.h>
#include <gerbview_id.h>

#define DMIL_GRID( x ) wxRealPoint( x * IU_PER_DECIMILS,\
                                    x * IU_PER_DECIMILS )
#define MM_GRID( x )   wxRealPoint( x * IU_PER_MM,\
                                    x * IU_PER_MM )


/**
    Default GerbView zoom values.
    Roughly a 1.5 progression.
*/
static const double gbrZoomList[] =
{
    ZOOM_FACTOR( 0.5 ),
    ZOOM_FACTOR( 0.75 ),
    ZOOM_FACTOR( 1.0 ),
    ZOOM_FACTOR( 1.5 ),
    ZOOM_FACTOR( 2.0 ),
    ZOOM_FACTOR( 3.0 ),
    ZOOM_FACTOR( 4.5 ),
    ZOOM_FACTOR( 7.0 ),
    ZOOM_FACTOR( 10.0 ),
    ZOOM_FACTOR( 15.0 ),
    ZOOM_FACTOR( 22.0 ),
    ZOOM_FACTOR( 35.0 ),
    ZOOM_FACTOR( 50.0 ),
    ZOOM_FACTOR( 80.0 ),
    ZOOM_FACTOR( 110.0 ),
    ZOOM_FACTOR( 150.0 ),
    ZOOM_FACTOR( 200.0 ),
    ZOOM_FACTOR( 350.0 ),
    ZOOM_FACTOR( 500.0 ),
    ZOOM_FACTOR( 1000.0 ),
    ZOOM_FACTOR( 2000.0 )
};


// Default grid sizes for PCB editor screens.
static GRID_TYPE gbrGridList[] =
{
    // predefined grid list in 0.0001 inches
    { ID_POPUP_GRID_LEVEL_1000,     DMIL_GRID( 1000 )  },
    { ID_POPUP_GRID_LEVEL_500,      DMIL_GRID( 500 )   },
    { ID_POPUP_GRID_LEVEL_250,      DMIL_GRID( 250 )   },
    { ID_POPUP_GRID_LEVEL_200,      DMIL_GRID( 200 )   },
    { ID_POPUP_GRID_LEVEL_100,      DMIL_GRID( 100 )   },
    { ID_POPUP_GRID_LEVEL_50,       DMIL_GRID( 50 )    },
    { ID_POPUP_GRID_LEVEL_25,       DMIL_GRID( 25 )    },
    { ID_POPUP_GRID_LEVEL_20,       DMIL_GRID( 20 )    },
    { ID_POPUP_GRID_LEVEL_10,       DMIL_GRID( 10 )    },
    { ID_POPUP_GRID_LEVEL_5,        DMIL_GRID( 5 )     },
    { ID_POPUP_GRID_LEVEL_2,        DMIL_GRID( 2 )     },
    { ID_POPUP_GRID_LEVEL_1,        DMIL_GRID( 1 )     },

    // predefined grid list in mm
    { ID_POPUP_GRID_LEVEL_5MM,      MM_GRID( 5.0 )     },
    { ID_POPUP_GRID_LEVEL_2_5MM,    MM_GRID( 2.5 )     },
    { ID_POPUP_GRID_LEVEL_1MM,      MM_GRID( 1.0 )     },
    { ID_POPUP_GRID_LEVEL_0_5MM,    MM_GRID( 0.5 )     },
    { ID_POPUP_GRID_LEVEL_0_25MM,   MM_GRID( 0.25 )    },
    { ID_POPUP_GRID_LEVEL_0_2MM,    MM_GRID( 0.2 )     },
    { ID_POPUP_GRID_LEVEL_0_1MM,    MM_GRID( 0.1 )     },
    { ID_POPUP_GRID_LEVEL_0_0_5MM,  MM_GRID( 0.05 )    },
    { ID_POPUP_GRID_LEVEL_0_0_25MM, MM_GRID( 0.025 )   },
    { ID_POPUP_GRID_LEVEL_0_0_1MM,  MM_GRID( 0.01 )    }
};


GBR_SCREEN::GBR_SCREEN( const wxSize& aPageSizeIU ) :
    BASE_SCREEN( SCREEN_T )
{
    for( unsigned i = 0; i < DIM( gbrZoomList );  ++i )
        m_ZoomList.push_back( gbrZoomList[i] );

    for( unsigned i = 0; i < DIM( gbrGridList );  ++i )
        AddGrid( gbrGridList[i] );

    // Set the working grid size to a reasonable value (in 1/10000 inch)
    SetGrid( DMIL_GRID( 500 ) );

    m_Active_Layer       = B_Cu;      // default active layer = bottom layer

    SetZoom( ZOOM_FACTOR( 350 ) );            // a default value for zoom

    InitDataPoints( aPageSizeIU );
}


GBR_SCREEN::~GBR_SCREEN()
{
    ClearUndoRedoList();
}


// virtual function
int GBR_SCREEN::MilsToIuScalar()
{
    return (int)IU_PER_MILS;
}


/* Virtual function needed by classes derived from BASE_SCREEN
 * this is a virtual pure function in BASE_SCREEN
 * do nothing in GerbView
 * could be removed later
 */
void GBR_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER&, int )
{
}
