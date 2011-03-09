/**************/
/* ar-proto.h */
/**************/


int Propagation( PCB_EDIT_FRAME* frame );

/* Initialize a value type, the cells included in the board surface of the
 * pad edge by pt_pad, with the margin reserved for isolation. */
void Place_1_Pad_Board( BOARD * Pcb, D_PAD * pt_pad, int type, int marge,
                        int op_logique );

/* Draws a segment of track on the board. */
void TraceSegmentPcb( BOARD * Pcb, TRACK * pt_segm, int type, int marge,
                      int op_logique );

void TraceLignePcb( int x0, int y0, int x1, int y1, int layer, int type,
                    int op_logique );

/* Uses the color value of all cells included in the board
 * coord of the rectangle ux0, uy0 (top right corner)
 * a ux1, uy1 (lower left corner) (coord PCB)
 * the rectangle is horizontal (or vertical)
 * masque_layer = mask layers;
 * op_logique = WRITE_CELL, WRITE_OR_CELL, WRITE_XOR_CELL, WRITE_AND_CELL
 */
void TraceFilledRectangle( BOARD * Pcb, int ux0, int uy0, int ux1, int uy1,
                           int side, int color, int op_logique);


/* Same as above, but the rectangle is inclined angle angle. */
void TraceFilledRectangle( BOARD * Pcb, int ux0, int uy0, int ux1, int uy1,
                           int angle, int masque_layer, int color,
                           int op_logique );

/* Fills all BOARD cells contained in the arc of "L" angle half-width lg
 * ux center, starting in ux y0, y1 is set to color.  Coordinates are in
 * PCB units (0.1 mil) relating to the origin pt_pcb-> Pcb_oX, Y's board.
 */
void TraceArc( int ux0,int uy0, int ux1, int uy1, int ArcAngle, int lg,
               int layer, int color, int op_logique);

/* QUEUE.CPP */
void FreeQueue();
void InitQueue();
void GetQueue( int *, int *, int *, int *, int * );
int  SetQueue( int, int, int, int, int, int, int );
void ReSetQueue( int, int, int, int, int, int, int );

/* WORK.CPP */
void InitWork();
void ReInitWork();
int SetWork( int, int, int , int, int, RATSNEST_ITEM *, int );
void GetWork( int *, int *, int *, int *, int *, RATSNEST_ITEM ** );
void SortWork(); /* order the work items; shortest first */

/* DIST.CPP */
int GetApxDist( int, int, int, int );
int CalcDist( int, int, int ,int );

/* BOARD.CPP */
int Build_Work( BOARD * Pcb );
void PlaceCells( BOARD * Pcb, int net_code, int flag = 0 );

MATRIX_CELL GetCell( int aRow, int aCol, int aSide);
void SetCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell);
void OrCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell);
void XorCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell);
void AndCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell);
void AddCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell);
DIST_CELL GetDist( int aRow, int aCol, int aSide );
void SetDist( int aRow, int aCol, int aSide, DIST_CELL );
int GetDir( int aRow, int aCol, int aSide );
void SetDir( int aRow, int aCol, int aSide, int aDir);
