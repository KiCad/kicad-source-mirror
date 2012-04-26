/**
 * @file classpcb.cpp
 * @brief Member functions of classes used in Pcbnew (see pcbstruct.h)
 *        except for tracks (see class_track.cpp).
 */

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <trigo.h>
#include <class_pcb_screen.h>
#include <eda_text.h>                // FILLED
#include <base_units.h>

#include <pcbnew.h>
#include <class_board_design_settings.h>
#include <layers_id_colors_and_visibility.h>

#include <pcbnew_id.h>


#define ZOOM_FACTOR( x )       ( x * IU_PER_DECIMILS )
#define DMIL_GRID( x )         wxRealPoint( x * IU_PER_DECIMILS,\
                                            x * IU_PER_DECIMILS )
#define MM_GRID( x )           wxRealPoint( x * IU_PER_MM,\
                                            x * IU_PER_MM )


/**
    Default Pcbnew zoom values.
    Limited to 19 values to keep a decent size to menus.
    Roughly a 1.5 progression.
    The last 2 values are handy when somebody uses a library import of a module
    (or foreign data) which has a bad coordinate.
    Also useful in GerbView for this reason.
    Zoom 5 and 10 can create artefacts when drawing (integer overflow in low level graphic
    functions )
*/
static const double pcbZoomList[] =
{
#if defined( USE_PCBNEW_NANOMETRES )
    ZOOM_FACTOR( 0.1 ),
    ZOOM_FACTOR( 0.2 ),
    ZOOM_FACTOR( 0.3 ),
#endif

    ZOOM_FACTOR( 0.5 ),
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
    ZOOM_FACTOR( 120.0 ),
    ZOOM_FACTOR( 200.0 ),
    ZOOM_FACTOR( 300.0 ),


#if !defined( USE_PCBNEW_NANOMETRES )
    ZOOM_FACTOR( 500.0 ),
    ZOOM_FACTOR( 1000.0 ),
    ZOOM_FACTOR( 2000.0 )
#endif
};


// Default grid sizes for PCB editor screens.
static GRID_TYPE pcbGridList[] =
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


PCB_SCREEN::PCB_SCREEN( const wxSize& aPageSizeIU ) :
    BASE_SCREEN( SCREEN_T )
{
    wxSize displayz = wxGetDisplaySize();

    for( unsigned i = 0; i < DIM( pcbZoomList );  ++i )
        m_ZoomList.Add( pcbZoomList[i] );

    for( unsigned i = 0; i < DIM( pcbGridList );  ++i )
        AddGrid( pcbGridList[i] );

    // Set the working grid size to a reasonnable value (in 1/10000 inch)
    SetGrid( DMIL_GRID( 500 ) );

    m_Active_Layer       = LAYER_N_BACK;      // default active layer = bottom layer
    m_Route_Layer_TOP    = LAYER_N_FRONT;     // default layers pair for vias (bottom to top)
    m_Route_Layer_BOTTOM = LAYER_N_BACK;

    SetZoom( ZOOM_FACTOR( 120 ) );             // a default value for zoom

    InitDataPoints( aPageSizeIU );
}


PCB_SCREEN::~PCB_SCREEN()
{
    ClearUndoRedoList();
}


int PCB_SCREEN::MilsToIuScalar()
{
    return (int)IU_PER_MILS;
}


DISPLAY_OPTIONS::DISPLAY_OPTIONS()
{
    DisplayPadFill          = FILLED;
    DisplayViaFill          = FILLED;
    DisplayPadNum           = true;
    DisplayPadIsol          = true;

    DisplayModEdge          = true;
    DisplayModText          = true;
    DisplayPcbTrackFill     = true;  // false = sketch , true = filled
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
