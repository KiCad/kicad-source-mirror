	/*************************************/
	/* ar-proto.h: Fichier entete pour l'autorouteur */
	/*************************************/

/**************/
/* AUTOROUT.H */
/**************/

MODULE ** GenListeModules( BOARD * Pcb, int * NbModules );

/***************/
/* GRAPHPCB.CPP */
/***************/
/* Rem: op_logique done le type d'ecriture ( WRITE, OR , XOR , AND ) */

void Place_1_Pad_Board(BOARD * Pcb, D_PAD * pt_pad, int type, int marge, int op_logique);
	/* Initialise a la valeur type, les cellules du Board comprises dans la
	surface du pad pointe par pt_pad, avec la marge reservee pour l'isolement */

void TraceSegmentPcb(BOARD * Pcb, TRACK * pt_segm, int type, int marge, int op_logique);
	/* trace un Segment de piste sur le BOARD */

void TraceLignePcb( int x0, int y0, int x1, int y1, int layer, int type, int op_logique );

void TraceFilledRectangle(BOARD * Pcb, int ux0, int uy0, int ux1, int uy1 ,int side,
				int color, int op_logique);
	/* Met a la valeur color l'ensemble des cellules du board inscrites dans
		 le rectangle de coord ux0,uy0 ( angle haut a droite )
		 a ux1,uy1 ( angle bas a gauche ) (coord PCB)
		 Le rectangle est horizontal ( ou vertical )
		masque_layer = masque des couches;
		op_logique = WRITE_CELL, WRITE_OR_CELL, WRITE_XOR_CELL, WRITE_AND_CELL
	*/

/* fonction surchargee: */
void TraceFilledRectangle(BOARD * Pcb, int ux0, int uy0, int ux1, int uy1, int angle,
				int masque_layer, int color, int op_logique);
				/* Identique a precedemment, mais le rectangle est
					incline de l'angle angle */


void TraceArc(int ux0,int uy0,int ux1,int uy1, int ArcAngle, int lg,int layer,
					int color, int op_logique);
/* Remplit toutes les cellules du BOARD contenues dans l'arc de "longueur" angle
	de demi-largeur lg, centre ux,y0 commencant en ux,y1 a la valeur color .
	coord en unites PCB (0.1 mil) relatives a l'origine pt_pcb->Pcb_oX,Y du board.
*/

/* SOLVE.CPP */

/* QUEUE.CPP */
void FreeQueue(void);	/* Libere la memoire de la queue de recherche */
void InitQueue( void );
void GetQueue( int *, int *, int *, int *, int * );
int  SetQueue( int, int, int, int, int, int, int );
void ReSetQueue( int, int, int, int, int, int, int );


/* WORK.CPP */
void InitWork( void );
void ReInitWork( void );
int SetWork( int, int, int , int, int, CHEVELU *, int );
void GetWork( int *, int *, int *, int *, int *, CHEVELU ** );
void SortWork( void ); /* order the work items; shortest first */

/* DIST.CPP */
int GetApxDist( int, int, int, int );
int CalcDist( int, int, int ,int );

/* BOARD.CPP */
bool ComputeMatriceSize(WinEDA_BasePcbFrame * frame, int pas_route);
int Build_Work(BOARD * Pcb, CHEVELU* pt_chevelus);
void PlaceCells(BOARD * Pcb, int net_code, int flag = 0);

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

