/***********/
/* protos.h */
/***********/

#ifndef PROTO_H
#define PROTO_H


#include <vector>


class COMMAND;

/**
 * Function SwapData
 * Used in undo / redo command:
 *  swap data between Item and a copy
 *  swapped data is data modified by edition, so NOT ALL values are swapped
 * @param aItem = the item
 * @param aImage = a copy of the item
 */
void SwapData( BOARD_ITEM* aItem, BOARD_ITEM* aImage );


/*******************/
/* PAD_CONNECT.CPP */
/*******************/

class D_PAD;

/**
 * Function CreateSortedPadListByXCoord
 * first empties then fills the vector with all pads and sorts them by
 * increasing x coordinate.  The vector only holds pointers to the pads and
 * those pointers are only references to pads which are owned by the BOARD
 * through other links.
 * @param aBoard Which board to gather pads from.
 * @param aVector Where to put the pad pointers.
 */
void CreateSortedPadListByXCoord( BOARD* aBoard, std::vector<D_PAD*>* aVector );


/***************/
/* TRPISTE.CPP */
/***************/

/* Routine trace of n consecutive segments in memory.
 * Useful for mounting a track record for being the segments that
 * Tracks are contiguous in memory
 * Parameters:
 * Pt_start_piste = starting address of the list of segments
 * Nbsegment = number of segments was traced
 * Mode_color = mode (GrXOR, Gror ..)
 * CAUTION:
 * The starting point of a track following MUST exist: may be
 * Then put a 0 before calling a routine if the track is the last draw
 */
void DrawTraces( EDA_DRAW_PANEL* panel,
                 wxDC*           DC,
                 TRACK*          aStartTrace,
                 int             nbsegment,
                 int             mode_color );

/*************/
/* MODULES.C */
/*************/
int ChangeSideNumLayer( int oldlayer );
void DrawModuleOutlines( EDA_DRAW_PANEL* panel, wxDC* DC, MODULE* module );
void MoveFootprint( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase );


/****************/
/* EDITRACK.C : */
/****************/

TRACK* LocateIntrusion( TRACK* listStart, TRACK* aTrack, int aLayer, const wxPoint& aRef );

void ShowNewTrackWhenMovingCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                   bool aErase );

/* Determine coordinate for a segment direction of 0, 90 or 45 degrees,
 * depending on it's position from the origin (ox, oy) and \a aPosiition..
 */
void CalculateSegmentEndPoint( const wxPoint& aPosition, int ox, int oy, int* fx, int* fy );


/* Routine to find the point "attachment" at the end of a trace.
 * This may be a PAD or another trace segment.
 * Returns:
 * - Pointer to the PAD or:
 * - Pointer to the segment or:
 * - NULL
 * Parameters:
 * - aPos - coordinate point test
 * ALayerMask of mask layers to be tested
 */
BOARD_ITEM* LocateLockPoint( BOARD* aPcb, wxPoint aPos, int aLayerMask );

/* Create an intermediate point on a segment
 * aSegm segment is broken into 2 segments connecting point pX, pY
 * After insertion:
 *   The new segment starts from  to new point, and ends to initial aSegm ending point
 *   the old segment aSegm ends to new point
 * Returns:
 *   NULL if no new point (ie if aRefPoint already corresponded at one end of aSegm
 * or
 *   Pointer to the segment created
 *   Returns the exact value of aRefPoint
 * If aSegm points to a via:
 *   Returns the exact value of aRefPoint and a pointer to the via,
 *   But does not create extra point
 */
TRACK* CreateLockPoint( BOARD*             aPcb,
                        wxPoint&           aRefPoint,
                        TRACK*             aSegm,
                        PICKED_ITEMS_LIST* aItemsListPicker );


/****************/
/* CONTROLE.CPP */
/****************/
void RemoteCommand( const char* cmdline );
bool Project( wxPoint* res, wxPoint on_grid, const TRACK* track );


#endif  /* #define PROTO_H */
