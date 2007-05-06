	/***************************************************************/
	/* Edition des Modules: Structures et variables de gestion des */
	/*		fonctions de "DRAG" des segments de piste				*/
	/***************************************************************/

class DRAG_SEGM
{
public:
	
	DRAG_SEGM * Pnext;	/* Pointeur de chainage */
	TRACK * m_Segm;		/* pointeur sur le segment a "dragger */
	D_PAD * m_Pad_Start;  /* pointeur sur le Pad origine si origine segment sur pad */
	D_PAD * m_Pad_End;	/* pointeur sur le Pad fin si fin segment sur pad */
	int m_Flag;			/* indicateur divers */

private:
	wxPoint m_StartInitialValue;
	wxPoint m_EndInitialValue;	/* For abort: initial m_Start and m_End values for m_Segm */
	

public:

	DRAG_SEGM(TRACK * segm);
	~DRAG_SEGM();

	void SetInitialValues(void);

};

	/* Variables */

eda_global DRAG_SEGM * g_DragSegmentList;	/* pointe le debut de la liste
										des structures DRAG_SEGM */

/* routines specifiques */
void Dessine_Segments_Dragges(WinEDA_DrawPanel * panel, wxDC * DC);
void Build_Drag_Liste(WinEDA_DrawPanel * panel, wxDC * DC, MODULE * Module);
void Build_1_Pad_SegmentsToDrag(WinEDA_DrawPanel * panel, wxDC * DC, D_PAD * PtPad );
void Collect_TrackSegmentsToDrag(WinEDA_DrawPanel * panel, wxDC * DC,
	wxPoint & point, int MasqueLayer, int net_code);
void EraseDragListe(void);
