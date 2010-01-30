/****************************************************************/
/* member functions of classes used in pcbnew (see pcbstruct.h) */
/* except for tracks (see class_track.cpp)                      */
/****************************************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "class_board_design_settings.h"

#include "trigo.h"
#include "pcbnew_id.h"


/* Default pcbnew zoom values.
 * Limited to 19 values to keep a decent size to menus
 * 15 it better but does not allow a sufficient number of values
 * roughtly a 1.5 progression.
 * The last 2 values is  handy when somebody uses a library import of a module
 * (or foreign data) which has a bad coordinate
 * Also useful in Gerbview for this reason.
 * Zoom 5 and 10 can create artefacts when drawing (integer overflow in low level graphic functions )
 */
static const int PcbZoomList[] =
{
    5,    10,   15,   22,    30, 45, 70, 100, 150, 220, 350, 500, 800, 1200,
    2000, 3500, 5000, 10000, 20000
};

#define PCB_ZOOM_LIST_CNT ( sizeof( PcbZoomList ) / sizeof( int ) )
#define MM_TO_PCB_UNITS 10000.0 / 25.4000508001016


/* Default grid sizes for PCB editor screens. */
static GRID_TYPE PcbGridList[] =
{
    // predefined grid list in 0.0001 inches
    { ID_POPUP_GRID_LEVEL_1000,     wxRealPoint( 1000,                    1000 )                      },
    { ID_POPUP_GRID_LEVEL_500,      wxRealPoint( 500,                     500 )                       },
    { ID_POPUP_GRID_LEVEL_250,      wxRealPoint( 250,                     250 )                       },
    { ID_POPUP_GRID_LEVEL_200,      wxRealPoint( 200,                     200 )                       },
    { ID_POPUP_GRID_LEVEL_100,      wxRealPoint( 100,                     100 )                       },
    { ID_POPUP_GRID_LEVEL_50,       wxRealPoint( 50,                      50 )                        },
    { ID_POPUP_GRID_LEVEL_25,       wxRealPoint( 25,                      25 )                        },
    { ID_POPUP_GRID_LEVEL_20,       wxRealPoint( 20,                      20 )                        },
    { ID_POPUP_GRID_LEVEL_10,       wxRealPoint( 10,                      10 )                        },
    { ID_POPUP_GRID_LEVEL_5,        wxRealPoint( 5,                       5 )                         },
    { ID_POPUP_GRID_LEVEL_2,        wxRealPoint( 2,                       2 )                         },
    { ID_POPUP_GRID_LEVEL_1,        wxRealPoint( 1,                       1 )                         },

    // predefined grid list in mm
    { ID_POPUP_GRID_LEVEL_5MM,      wxRealPoint( MM_TO_PCB_UNITS * 5.0,   MM_TO_PCB_UNITS * 5.0 )     },
    { ID_POPUP_GRID_LEVEL_2_5MM,    wxRealPoint( MM_TO_PCB_UNITS * 2.5,   MM_TO_PCB_UNITS * 2.5 )     },
    { ID_POPUP_GRID_LEVEL_1MM,      wxRealPoint( MM_TO_PCB_UNITS,         MM_TO_PCB_UNITS )           },
    { ID_POPUP_GRID_LEVEL_0_5MM,    wxRealPoint( MM_TO_PCB_UNITS * 0.5,   MM_TO_PCB_UNITS * 0.5 )     },
    { ID_POPUP_GRID_LEVEL_0_25MM,   wxRealPoint( MM_TO_PCB_UNITS * 0.25,  MM_TO_PCB_UNITS * 0.25 )    },
    { ID_POPUP_GRID_LEVEL_0_2MM,    wxRealPoint( MM_TO_PCB_UNITS * 0.2,   MM_TO_PCB_UNITS * 0.2 )     },
    { ID_POPUP_GRID_LEVEL_0_1MM,    wxRealPoint( MM_TO_PCB_UNITS * 0.1,   MM_TO_PCB_UNITS * 0.1 )     },
    { ID_POPUP_GRID_LEVEL_0_0_5MM,  wxRealPoint( MM_TO_PCB_UNITS * 0.05,  MM_TO_PCB_UNITS * 0.05 )     },
    { ID_POPUP_GRID_LEVEL_0_0_25MM, wxRealPoint( MM_TO_PCB_UNITS * 0.025, MM_TO_PCB_UNITS * 0.025 )     },
    { ID_POPUP_GRID_LEVEL_0_0_1MM,  wxRealPoint( MM_TO_PCB_UNITS * 0.01,  MM_TO_PCB_UNITS * 0.01 )     }
};

#define PCB_GRID_LIST_CNT ( sizeof( PcbGridList ) / sizeof( GRID_TYPE ) )


/*******************************************************************/
/* Class PCB_SCREEN: class to handle parametres to display a board */
/********************************************************************/
PCB_SCREEN::PCB_SCREEN() : BASE_SCREEN( TYPE_SCREEN )
{
    size_t i;

    for( i = 0; i < PCB_ZOOM_LIST_CNT; i++ )
        m_ZoomList.Add( PcbZoomList[i] );

    for( i = 0; i < PCB_GRID_LIST_CNT; i++ )
        AddGrid( PcbGridList[i] );

    SetGrid( wxRealPoint( 500, 500 ) );        /* Set the working grid size to a reasonnable value (in 1/10000 inch) */
    Init();
}


/***************************/
PCB_SCREEN::~PCB_SCREEN()
/***************************/
{
    ClearUndoRedoList();
}


/*************************/
void PCB_SCREEN::Init()
/*************************/
{
    InitDatas();
    m_Active_Layer       = LAYER_N_BACK;      /* default active layer = bottom layer */
    m_Route_Layer_TOP    = LAYER_N_FRONT;                /* default layers pair for vias (bottom to top) */
    m_Route_Layer_BOTTOM = LAYER_N_BACK;
    m_Zoom = 150;                               /* a default value for zoom */
}


int PCB_SCREEN::GetInternalUnits( void )
{
    return PCB_INTERNAL_UNIT;
}


/* Return true if a microvia can be put on board
 * A microvia ia a small via restricted to 2 near neighbour layers
 * because its is hole is made by laser which can penetrate only one layer
 * It is mainly used to connect BGA to the first inner layer
 * And it is allowed from an external layer to the first inner layer
 */
bool PCB_SCREEN::IsMicroViaAcceptable( void )
{
    int copperlayercnt = g_DesignSettings.GetCopperLayerCount( );

    if( !g_DesignSettings.m_MicroViasAllowed )
        return false;   // Obvious..
    if( copperlayercnt < 4 )
        return false;   // Only on multilayer boards..
    if( ( m_Active_Layer == LAYER_N_BACK )
       || ( m_Active_Layer == LAYER_N_FRONT )
       || ( m_Active_Layer == g_DesignSettings.GetCopperLayerCount( ) - 2 )
       || ( m_Active_Layer == LAYER_N_2 ) )
        return true;

    return false;
}


/*************************/
/* class DISPLAY_OPTIONS */
/*************************/

/*
 *  Handle display options like enable/disable some optional drawings:
 */

DISPLAY_OPTIONS::DISPLAY_OPTIONS()
{
    DisplayPadFill          = FILLED;
    DisplayViaFill          = FILLED;
    DisplayPadNum           = true;
    DisplayPadIsol          = true;

    DisplayModEdge          = true;
    DisplayModText          = true;
    DisplayPcbTrackFill     = true;  /* false = sketch , true = filled */
    ShowTrackClearanceMode  = SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS;
    m_DisplayViaMode        = VIA_HOLE_NOT_SHOW;

    DisplayPolarCood        = false; /* false = display absolute coordinates,
                                      * true = display polar cordinates */
    DisplayZonesMode        = 0;     /* 0 = Show filled areas outlines in zones,
                                      * 1 = do not show filled areas outlines
                                      * 2 = show outlines of filled areas */
    DisplayNetNamesMode     = 3;     /* 0 do not show netnames,
                                      * 1 show netnames on pads
                                      * 2 show netnames on tracks
                                      * 3 show netnames on tracks and pads */
    DisplayDrawItems        = true;
    ContrastModeDisplay     = false;
}
