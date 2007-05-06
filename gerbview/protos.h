/* declarations prototype */

/****************/
/* undelete.cpp */
/****************/
EDA_BaseStruct * SaveItemEfface(EDA_BaseStruct * PtItem, int nbitems);


/***********/
/* cfg.cpp */
/***********/
bool Read_Config(void);


/* pcbplot.cpp */
void Plume(int state);

/****************/
/* lay2plot.cpp */

int GetLayerNumber(void);
void Print_PcbItems(BOARD * Pcb, wxDC *DC, int drawmode, int printmasklayer);

/*****************/
/* set_color.cpp */
/*****************/
void DisplayColorSetupFrame(WinEDA_DrawFrame * parent, const wxPoint & framepos);

/***************/
/* affiche.cpp */
/***************/
void valeur_param(int valeur,char * buf_texte);
void Affiche_Infos_PCB_Texte(WinEDA_BasePcbFrame * frame, TEXTE_PCB* pt_texte);
void Affiche_Infos_Piste(WinEDA_BasePcbFrame * frame, TRACK * pt_piste);

/* PLOT_RTN.CC */
void ComputePlotFileName( char * FullFileName, char * Ext );
void calcule_coord_plot(int * dx, int * dy ) ;
void calcule_dim_plot(int * dx, int * dy ) ;
void Trace_Un_TextePcb( TEXTE_PCB * pt_texte,int format_plot,int masque_layer);
		/* Trace 1 Texte type PCB , c.a.d autre que les textes sur modules,
		prepare les parametres de trace de Plot_1_texte */
void trace_1_arc(int format_plot,int cx,int cy,int start,int end,
					int rayon,int epaisseur);
void trace_1_cercle(int format_plot,int epaisseur,int cx, int cy, int rayon);
void Plot_1_texte( int format_plot,
						char * ptr,int t_nbcodes, int t_orient,
						int epaisseur, int ox,int oy,int size_h,int size_v);
			/* Routine de base de trace de 1 chaine de caracteres */

void Trace_Un_DrawSegment( DRAWSEGMENT* PtSegm, int format_plot,int masque_layer );

void Trace_Une_MirePcb( MIREPCB* PtMire, int format_plot,int masque_layer );

/* PLOTGERB.CC */
void trace_1_segment_GERBER(int pos_X0,int pos_Y0,int pos_X1,int pos_Y1,
							int hauteur);
void trace_1_cercle_GERBER( int cx, int cy, int rayon, int epaisseur);
void trace_1_contour_GERBER(int cX,int cY, int dimX,int dimY,
										int deltaX, int deltaY,
										int dim_trait, int orient);
	/* Trace 1 contour rectangulaire ou trapezoidal d'orientation quelconque
		 donne par son centre cX, cY, ses dimensions dimX et dimY,
		 ses variations deltaX et deltaY  et son orientation orient */

/* PLOTHPGL.CC */
void Init_Trace_HPGL(void);
void Fin_Trace_HPGL(void);
void trace_1_segment_HPGL(int pos_X0,int pos_Y0,int pos_X1,int pos_Y1,
							int hauteur);

void trace_1_pad_TRAPEZE_HPGL(int cX,int cY,
								int dimX,int dimY,int deltaX, int deltaY,
								int orient,int modetrace);

void trace_1_pastille_RONDE_HPGL(int pos_X,int pos_Y,int diametre,int modetrace) ;

void trace_1_pastille_OVALE_HPGL(int pos_X,int pos_Y,int dx,int dy,
										int orient,int modetrace) ;
void trace_1_pad_rectangulaire_HPGL(int cX,int cY,int dimX,int dimY,
										int orient,int modetrace) ;
void Move_Plume_HPGL( int x,int y,int plume);
			/*  deplace la plume levee (plume = 'U') ou baissee (plume = 'D')
		 	en position x,y */

void Plume_HPGL( int plume );
			/* leve (plume = 'U') ou baisse (plume = 'D') la plume */

/**************/
/* PRINTPS.CC */
/**************/
void trace_1_pastille_OVALE_POST(int pos_X,int pos_Y,
								int dx, int dy, int orient, int modetrace);
void trace_1_pastille_RONDE_POST(int pos_X,int pos_Y,int diametre,
								 int modetrace);
void trace_1_pad_rectangulaire_POST(int cX,int cY,
										int dimX,int dimY,int orient,
										int modetrace);
void trace_1_contour_POST(int cX,int cY, int dimX,int dimY,
										int deltaX, int deltaY,
										int dim_trait, int orient);
void trace_1_pad_TRAPEZE_POST(int cX,int cY,
							int dimX,int dimY,int deltaX, int deltaY,
							int orient,int modetrace);
void trace_1_segment_POST(int pos_X0,int pos_Y0,int pos_X1,int pos_Y1,
							int large);
void trace_1_Cercle_POST(int pos_X,int pos_Y,int diametre, int width);
void PlotArcPS(int x, int y, int StAngle, int EndAngle, int rayon, int width);


/***************/
/* trpiste.cpp */
/***************/
void Trace_Pistes(WinEDA_DrawPanel * panel, wxDC * DC, BOARD * Pcb, int drawmode);
void Trace_Segment(WinEDA_DrawPanel * panel, wxDC * DC, TRACK* pt_piste, int draw_mode);
void Trace_DrawSegmentPcb(WinEDA_DrawPanel * panel, wxDC * DC,
		DRAWSEGMENT * PtDrawSegment, int draw_mode);
void Trace_1_texte_pcb(WinEDA_DrawPanel * panel, wxDC * DC,
						TEXTE_PCB * pt_texte,int ox,int oy, int DrawMode);

void Affiche_DCodes_Pistes(WinEDA_DrawPanel * panel, wxDC * DC,
			BOARD * Pcb, int drawmode);

/**************/
/* struct.cpp */
/**************/
void DeleteStructure(EDA_BaseStruct * Struct);
void DeleteStructList( EDA_BaseStruct * Struct);

/*************/
/* dcode.cpp */
/*************/
class D_CODE;

D_CODE * ReturnToolDescr(int layer, int Dcode, int * index = NULL);

/**************/
/* rs274x.cpp */
/**************/
bool GetEndOfBlock( char * buff, char * &text, FILE *gerber_file);


