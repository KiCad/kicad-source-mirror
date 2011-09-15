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


/****************/
/* LOCATE.CPP : */
/****************/

/* Locates the center through the point x, y, on layer data
 * by masquelayer.
 * Search is done to address start_adr has end_adr (not included)
 */
TRACK* GetTrace( TRACK* start_adr, TRACK* end_adr, const wxPoint& ref_pos, int masquelayer );

/* Search for segment connected to the segment edge by
 * Ptr_piste:
 * If int = START, the point of beginning of the segment is used
 * If int = END, the end point of the segment is used
 * The search is done only on the ends of segments
 * The search is limited to the area [... pt_base] pt_lim.
 */
TRACK* GetConnectedTrace( TRACK* aTrace, TRACK* pt_base, TRACK* pt_lim, int extr );

/*
 * 1 - Locate segment of track leading from the mouse.
 * 2 - Locate segment of track point by point.
 * Ref_pX, ref_pY.
 *
 * If layer <0 the layer is not tested.
 *
 * The search begins to address start_adresse
 */
TRACK* GetTrace( BOARD* aPcb, TRACK* start_adresse, const wxPoint& ref_pos, int layer );

/* Locate a footprint by its bounding rectangle. */
MODULE* Locate_Prefered_Module( BOARD* aPcb, const wxPoint& aPosition, int aActiveLayer,
                                bool aVisibleOnly, bool aIgnoreLocked = false );

/* Locate a trace segment at the current cursor position.
 * The search begins to address start_adresse.
 */
TRACK* GetTrace( TRACK* start_adresse, int typeloc );

DRAWSEGMENT* Locate_Segment_Pcb( BOARD* Pcb, int LayerSearch, int typeloc );

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


/***************/
/* TRACK.CPP : */
/***************/

/**
 * Function MarkTrace
 * marks a chain of track segments, connected to aTrackList.
 * Each segment is marked by setting the BUSY bit into m_Flags.  Electrical
 * continuity is detected by walking each segment, and finally the segments
 * are rearranged into a contiguous chain within the given list.
 *
 * @param aPcb = the board to analyze
 * @param aStartSegm - The first interesting segment within a list of track
 *                     segment of aPcb
 * @param aSegmCount = a pointer to an integer where to return the number of
 *                     interesting segments
 * @param aTrackLen = a pointer to an integer where to return the lenght of the
 *                    track
 * @param aLengthDie = a pointer to an integer where to return the extra lengths inside
 *                     integrated circuits from the pads connected to this track to the
 *                     die (if any)
 * @param aReorder = true for reorder the interesting segments (useful for
 *                   track edition/deletion) in this case the flag BUSY is
 *                   set (the user is responsible of flag clearing). False
 *                   for no reorder : useful when we want just calculate the
 *                   track length in this case, flags are reset
 * @return TRACK* the first in the chain of interesting segments.
 */
TRACK* MarkTrace( BOARD* aPcb,
                  TRACK* aStartSegm,
                  int*   aSegmCount,
                  int*   aTrackLen,
                  int*   aLengthDie,
                  bool   aReorder );

/* Calculate end  coordinate of a trace.
 * Returns 1 if OK, 0 if trace looped back on itself.
 * The coord are returned StartTrack-> ox, oy
 * And EndTrack-> fx, fy if OK
 * The segments are drawn consecutively.
 */
int ReturnEndsTrack( TRACK* RefTrack, int NbSegm, TRACK** StartTrack, TRACK** EndTrack );


/***************/
/***************/

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
