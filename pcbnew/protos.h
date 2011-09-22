/***********/
/* protos.h */
/***********/

#ifndef PROTO_H
#define PROTO_H


#include <vector>


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

/**
 * Function DrawTraces
 * Draws n consecutive track segments in list.
 * Useful to show a track when it is a chain of segments
 * (fir instance when creating a new track)
 * @param aTrackList = First segment
 * @param nbsegment = number of segments in list
 * @param Mode_color = mode (GRXOR, GROR ..)
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
