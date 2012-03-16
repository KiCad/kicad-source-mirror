/**
 * @file pcbcommon.h
 */

#ifndef PCBCOMMON_H_
#define PCBCOMMON_H_


#include <dlist.h>
#include <layers_id_colors_and_visibility.h>  // LAYER_COUNT and NB_COPPER_LAYERS definitions.


#define MIN_DRAW_WIDTH      1               ///< Minimum trace drawing width.


class D_PAD;
class TRACK;
class BOARD;
class DISPLAY_OPTIONS;


/**
 * Function GetLayerMask
 * @return a one bit layer mask from a layer number
 * @param aLayerNumber = the layer number to convert (0 .. LAYER_COUNT-1)
 */
int GetLayerMask( int aLayerNumber );

/// Look up Table for conversion copper layer count -> general copper layer mask:
extern int g_TabAllCopperLayerMask[NB_COPPER_LAYERS];

extern DISPLAY_OPTIONS DisplayOpt;

extern int g_CurrentVersionPCB;

extern int g_RotationAngle;

/// List of segments of the trace currently being drawn.
extern DLIST<TRACK> g_CurrentTrackList;

#define g_CurrentTrackSegment g_CurrentTrackList.GetLast()    ///< most recently created segment

#define g_FirstTrackSegment   g_CurrentTrackList.GetFirst()   ///< first segment created


#endif  // PCBCOMMON_H_
