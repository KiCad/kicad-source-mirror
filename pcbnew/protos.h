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


/****************/
/* CONTROLE.CPP */
/****************/
void RemoteCommand( const char* cmdline );
bool Project( wxPoint* res, wxPoint on_grid, const TRACK* track );


#endif  /* #define PROTO_H */
