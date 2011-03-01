/**************/
/* ar-proto.h */
/**************/

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
bool ComputeMatriceSize( PCB_BASE_FRAME * frame, int pas_route );
int Build_Work( BOARD * Pcb );
void PlaceCells( BOARD * Pcb, int net_code, int flag = 0 );

BoardCell GetCell( int, int, int );
void SetCell( int, int, int, BoardCell );
void OrCell( int, int, int, BoardCell );
void XorCell( int, int, int, BoardCell );
void AndCell( int, int, int, BoardCell );
void AddCell( int, int, int, BoardCell );
DistCell GetDist( int, int, int );
void SetDist( int, int, int, DistCell );
int GetDir( int, int, int );
void SetDir( int, int, int, int );
