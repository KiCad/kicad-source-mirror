/**
 * @file pcbnew/protos.h
 */

#ifndef PROTO_H
#define PROTO_H


class wxDC;
class wxPoint;
class EDA_DRAW_PANEL;
class BOARD_ITEM;
class TRACK;
class MODULE;

/**
 * Function SwapData
 * Used in undo / redo command:
 *  swap data between Item and a copy
 *  swapped data is data modified by edition, so NOT ALL values are swapped
 * @param aItem = the item
 * @param aImage = a copy of the item
 */
void SwapData( BOARD_ITEM* aItem, BOARD_ITEM* aImage );



/***************/
/* TRPISTE.CPP */
/***************/

/**
 * Function DrawTraces
 * Draws n consecutive track segments in list.
 * Useful to show a track when it is a chain of segments
 * (for instance when creating a new track)
 *
 * @param panel A EDA_DRAW_ITEM pointer to the canvas.
 * @param DC A wxDC pointer of the device context used for drawing.
 * @param aStartTrace First segment
 * @param nbsegment Number of segments in list
 * @param mode_color Drawing mode (GRXOR, GROR ..)
 */
void DrawTraces( EDA_DRAW_PANEL* panel,
                 wxDC*           DC,
                 TRACK*          aStartTrace,
                 int             nbsegment,
                 int             mode_color );

/*************/
/* MODULES.C */
/*************/

/**
 * Function ChangeSideMaskLayer
 * calculates the mask layer when flipping a footprint.
 * BACK and FRONT copper layers , mask, paste, solder layers are swapped.
 */
int ChangeSideMaskLayer( int aMask );

void DrawModuleOutlines( EDA_DRAW_PANEL* panel, wxDC* DC, MODULE* module );


/****************/
/* EDITRACK.C : */
/****************/

TRACK* LocateIntrusion( TRACK* listStart, TRACK* aTrack, int aLayer, const wxPoint& aRef );

void ShowNewTrackWhenMovingCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                   bool aErase );

/**
 * Determine coordinate for a segment direction of 0, 90, or 45 degrees
 * depending on it's position from the origin (ox, oy) and \a aPosiition.
 */
void CalculateSegmentEndPoint( const wxPoint& aPosition, int ox, int oy, int* fx, int* fy );


/****************/
/* CONTROLE.CPP */
/****************/
void RemoteCommand( const char* cmdline );
bool Project( wxPoint* res, wxPoint on_grid, const TRACK* track );


#endif  /* #define PROTO_H */
