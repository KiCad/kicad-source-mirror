/**********************************************************************/
/* fonctions membres des classes utilisees dans pcbnew (voir pcbstruct.h */
/*    sauf routines relatives aux pistes (voir class_track.cpp)       */
/**********************************************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"

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
    m_Active_Layer       = COPPER_LAYER_N;      /* default active layer = bottom layer */
    m_Route_Layer_TOP    = CMP_N;               /* default layers pair for vias (bottom to top) */
    m_Route_Layer_BOTTOM = COPPER_LAYER_N;
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
    int copperlayercnt = g_DesignSettings.m_CopperLayerCount;

    if( !g_DesignSettings.m_MicroViasAllowed )
        return false;   // Obvious..
    if( copperlayercnt < 4 )
        return false;   // Only on multilayer boards..
    if( ( m_Active_Layer == COPPER_LAYER_N )
       || ( m_Active_Layer == LAYER_CMP_N )
       || ( m_Active_Layer == g_DesignSettings.m_CopperLayerCount - 2 )
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
    DisplayPadNoConn        = true;
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
    Show_Modules_Cmp        = true;
    Show_Modules_Cu         = true;

    DisplayDrawItems        = true;
    ContrastModeDisplay     = false;
}


/*****************************************************/
EDA_BoardDesignSettings::EDA_BoardDesignSettings()
/*****************************************************/

// Default values for designing boards
{
    int ii;

    static const int default_layer_color[32] =
    {
        GREEN,     LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, RED,
        LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY,
        MAGENTA,   CYAN,
        LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY
    };

    m_CopperLayerCount      = 2;                    // Default design is a double sided board
    m_ViaDrillCustomValue   = 250;                  // via drill for vias which must have a defined drill value
    m_CurrentViaSize        = 450;                  // Current via size
    m_CurrentViaType        = VIA_THROUGH;          // via type (VIA_BLIND_BURIED, VIA_THROUGH VIA_MICROVIA)
    m_CurrentTrackWidth     = 170;                  // current track width
    m_UseConnectedTrackWidth = false;               // if true, when creating a new track starting on an existing track, use this track width
    m_CurrentMicroViaSize   = 150;                  // Current micro via size
    m_MicroViasAllowed      = false;                // true to allow micro vias
    m_DrawSegmentWidth      = 100;                  // current graphic line width (not EDGE layer)
    m_EdgeSegmentWidth      = 100;                  // current graphic line width (EDGE layer only)
    m_PcbTextWidth          = 100;                  // current Pcb (not module) Text width
    m_PcbTextSize           = wxSize( 500, 500 );   // current Pcb (not module) Text size
    m_TrackMinWidth         = 80;                   // track min value for width ((min copper size value
    m_ViasMinSize           = 350;                  // vias (not micro vias) min diameter
    m_ViasMinDrill          = 200;                  // vias (not micro vias) min drill diameter
    m_MicroViasMinSize      = 200;                  // micro vias (not vias) min diameter
    m_MicroViasMinDrill     = 80;                   // micro vias (not vias) min drill diameter
    m_MaskMargin            = 150;                  // Solder mask margin
    /* Color options for screen display of the Printed Board: */


//@@IMB: Not used    m_PcbGridColor = DARKGRAY;                      // Grid color

    m_EnabledLayers     = ALL_LAYERS;               // All layers enabled at first.
    m_VisibleLayers     = 0xffffffff;               // IMB: All layers visible at first. TODO: Use a macro for the initial value.
    m_VisibleElements   = 0x00000fff;               // IMB: All elements visible at first. TODO: Use a macro for the initial value.

    for( ii = 0; ii < 32; ii++ )
        m_LayerColor[ii] = default_layer_color[ii];

    // Layer colors (tracks and graphic items)
    m_ViaColor[VIA_NOT_DEFINED]     = DARKGRAY;
    m_ViaColor[VIA_MICROVIA]        = CYAN;
    m_ViaColor[VIA_BLIND_BURIED]    = BROWN;
    m_ViaColor[VIA_THROUGH]         = WHITE;

//@@IMB: Not used    m_ModuleTextCMPColor = LIGHTGRAY;       // Text module color for modules on the COMPONENT layer
//@@IMB: Not used    m_ModuleTextCUColor  = MAGENTA;         // Text module color for modules on the COPPER layer
//@@IMB: Not used    m_ModuleTextNOVColor = DARKGRAY;        // Text module color for "invisible" texts (must be BLACK if really not displayed)
//@@IMB: Not used    m_AnchorColor   = BLUE;                 // Anchor color for modules and texts
//@@IMB: Not used    m_PadCUColor    = GREEN;                // Pad color for the COMPONENT side of the pad
//@@IMB: Not used    m_PadCMPColor   = RED;                  // Pad color for the COPPER side of the pad

    m_RatsnestColor = WHITE;                // Ratsnest color
}


// see pcbstruct.h
int EDA_BoardDesignSettings::GetVisibleLayers() const
{
    return m_VisibleLayers;
}

void EDA_BoardDesignSettings::SetVisibleLayers( int aMask )
{
    m_VisibleLayers = aMask & m_EnabledLayers & ALL_LAYERS;
}

void EDA_BoardDesignSettings::SetLayerVisibility( int aLayerIndex, bool aNewState )
{
    // Altough Pcbnew uses only 29, Gerbview uses all 32 layers
    if( aLayerIndex < 0 || aLayerIndex >= 32 )
        return;
    if( aNewState && IsLayerEnabled( aLayerIndex ))
        m_VisibleLayers |= 1 << aLayerIndex;
    else
        m_VisibleLayers &= ~( 1 << aLayerIndex );
}

void EDA_BoardDesignSettings::SetElementVisibility( int aElementCategory, bool aNewState )
{
    if( aElementCategory < 0 || aElementCategory > PAD_CMP_VISIBLE )
        return;
    if( aNewState )
        m_VisibleElements |= 1 << aElementCategory;
    else
        m_VisibleElements &= ~( 1 << aElementCategory );
}
