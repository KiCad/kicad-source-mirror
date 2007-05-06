		/****************************************************/
		/* Gestion des composants specifiques aux microndes */
		/*  Generation d'une self							*/
		/****************************************************/

		/* Fichier GEN_SELF.H */


/* Fonctions locales */

static void Exit_Self(WinEDA_DrawPanel * Panel, wxDC *DC);
static EDGE_MODULE * gen_arc(EDGE_MODULE * PtSegm, int cX, int cY, int angle);
static void ShowCadreSelf(WinEDA_DrawPanel * panel, wxDC * DC, bool erase);


/* structures locales */
class SELFPCB		// Definition d'une self constituee par une piste
{
public:
	int forme;				// Serpentin, spirale ..
	int orient;				// 0..3600
	int valeur;				// Valeur de la self
	wxPoint m_Start;
	wxPoint m_End;			// Coord du point de depart et d'arrivee
	wxSize m_Size;
	D_PAD * pt_pad_start, *pt_pad_end;	// Pointeurs sur les pads d'extremite
	int lng;				// Longueur de la piste constituant la self
	int m_Width;			// m_Size.xur de la piste
	int nbrin;				// Parametres de calcul: nombre de brins
	int lbrin;				// longueur du brin
	int rayon;				// Rayon des raccords entre brins
	int delta;				// distance aux pads
};

/* Variables locales */
static SELFPCB Mself;
static int Self_On;
static int Bl_X0, Bl_Y0 , Bl_Xf, Bl_Yf;	// Coord du cadre insrcivant la self


/*************************************************************************/
static void ShowCadreSelf(WinEDA_DrawPanel * panel, wxDC * DC, bool erase)
/*************************************************************************/
/* Routine d'affichage a l'ecran du cadre de la self */
{
int deltaX, deltaY;

	/* Calcul de l'orientation et de la taille de la fenetre:
		- orient = vert ou Horiz ( dimension max)
			- Size.x = Size.y / 2
	*/

	GRSetDrawMode(DC, GR_XOR);
	if( erase)/* effacement du cadre */
		{
		GRRect( & panel->m_ClipBox, DC, Bl_X0, Bl_Y0, Bl_Xf, Bl_Yf, YELLOW);
		}

	deltaX = (panel->GetScreen()->m_Curseur.x - Mself.m_Start.x) / 4;
	deltaY = (panel->GetScreen()->m_Curseur.y - Mself.m_Start.y) / 4;

	Mself.orient = 900;
	if( abs(deltaX) > abs(deltaY) ) Mself.orient = 0;

	if(Mself.orient == 0)
		{
		Bl_X0 = Mself.m_Start.x;
		Bl_Y0 = Mself.m_Start.y - deltaX;
		Bl_Xf = panel->GetScreen()->m_Curseur.x;
		Bl_Yf = Mself.m_Start.y + deltaX;
		}
	else
		{
		Bl_X0 = Mself.m_Start.x - deltaY;
		Bl_Y0 = Mself.m_Start.y;
		Bl_Xf = Mself.m_Start.x + deltaY;
		Bl_Yf = panel->GetScreen()->m_Curseur.y;
		}
	GRRect( & panel->m_ClipBox, DC, Bl_X0, Bl_Y0, Bl_Xf, Bl_Yf,YELLOW);
}


/*************************************************/
void Exit_Self(WinEDA_DrawPanel * Panel, wxDC *DC)
/*************************************************/
/* Routine de fermeture de l'application : ferme les commandes en cours */
{
	if(Self_On)
	{
		Self_On = 0;
		Panel->ManageCurseur(Panel, DC, 0); /* efface cadre */
		Panel->ManageCurseur = NULL;
		Panel->ForceCloseManageCurseur = NULL;
	}
}


/*******************************************/
void WinEDA_PcbFrame::Begin_Self( wxDC *DC)
/*******************************************/
/*
Routine d'initialisation d'un trace de self
*/
{

	if ( Self_On )
	{
		Genere_Self(DC);
		return;
	}
	
	Mself.m_Start = GetScreen()->m_Curseur;

	Self_On = 1;

	/* Mise a jour de l'origine des coord relatives */
	GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
	Affiche_Status_Box();

	Bl_X0 = Mself.m_Start.x; Bl_Y0 = Mself.m_Start.y;
	Bl_Xf = Bl_X0; Bl_Yf = Bl_Y0;

	DrawPanel->ManageCurseur = ShowCadreSelf;
	DrawPanel->ForceCloseManageCurseur = Exit_Self;
	DrawPanel->ManageCurseur(DrawPanel, DC, 0); /* Affiche cadre */
}

/**********************************************/
MODULE * WinEDA_PcbFrame::Genere_Self( wxDC *DC)
/**********************************************/
/* Genere une self en forme de serpentin
	- longueur Mself.lng
	- Extremites Mself.m_Start et Mself.m_End
	- Contrainte: m_Start.x = m_End.x ( self verticale )
			ou	  m_Start.y = m_End.y ( self horizontale )

	On doit determiner:
		Mself.nbrin = nombre de segments perpendiculaires a la direction
				( le serpention aura nbrin + 1 demicercles + 2 1/4 de cercle)
		Mself.lbrin = longueur d'un brin
		Mself.rayon = rayon des parties arrondies du serpentin
		Mself.delta = segments raccord entre extremites et le serpention lui meme

	Les equations sont
		Mself.m_Size.x = 2*Mself.rayon + Mself.lbrin
		Mself.m_Size.y  = 2*Mself.delta + 2*Mself.nbrin*Mself.rayon
		Mself.lng	  = 2*Mself.delta				// Raccords au serpentin
					 + (Mself.nbrin-2) * Mself.lbrin //longueur des brins sauf 1er et dernier
					 + (Mself.nbrin+1) * ( PI * Mself.rayon) // longueur des arrondis
					 + Mself.lbrin/2 - Melf.rayon*2) // longueur du 1er et dernier brin

	Les contraintes sont:
		nbrin >= 2
		Mself.rayon < Mself.m_Size.x
		Mself.m_Size.y = Mself.rayon*4 + 2*Mself.raccord
		Mself.lbrin > Mself.rayon *2

	Le calcul est conduit de la facon suivante:
	Initialement:
		nbrin = 2
		rayon = 4 * m_Size.x (valeur fixe arbitraire)
	puis:
		on augmente le nombre de brins jusqu'a la longueur desiree
		( le rayon est diminue si necessaire )

*/
{
EDGE_MODULE * PtSegm, * LastSegm, *FirstSegm, * newedge;
MODULE * Module;
D_PAD * PtPad;
int ii, ll, lextbrin;
float fcoeff;
bool abort = FALSE;
wxString msg;
	
	DrawPanel->ManageCurseur(DrawPanel, DC, FALSE);	/* efface cadre */
	DrawPanel->ManageCurseur = NULL;
	DrawPanel->ForceCloseManageCurseur = NULL;

	if(Self_On == 0)
	{
		DisplayError(this, wxT("Starting point not init..")); return NULL;
	}

	Self_On = 0;

	Mself.m_End = m_CurrentScreen->m_Curseur;

	/* Agencement des parametres pour simplifier le calcul : */
	/* le point de depart doit avoir la coord depart < celle du point de fin */

	if(Mself.orient == 0)	 // Self horizontale
	{
		Mself.m_End.y = Mself.m_Start.y;
		if(Mself.m_Start.x > Mself.m_End.x) EXCHG(Mself.m_Start.x,Mself.m_End.x);
		Mself.m_Size.y = Mself.m_End.x - Mself.m_Start.x;
		Mself.lng = Mself.m_Size.y;
	}

	else					// Self verticale
	{
		Mself.m_End.x = Mself.m_Start.x;
		if(Mself.m_Start.y > Mself.m_End.y) EXCHG(Mself.m_Start.y,Mself.m_End.y);
		Mself.m_Size.y = Mself.m_End.y - Mself.m_Start.y;
		Mself.lng = Mself.m_Size.y;
	}

	/* Entree de la vraie longueur desiree */
	if( ! g_UnitMetric )
	{
		fcoeff = 10000.0 ;
		msg.Printf( wxT("%1.4f"), Mself.lng /fcoeff);
		abort = Get_Message(_("Length(inch):"),msg, this);
	}
	else
	{
		fcoeff = 10000.0/25.4 ;
		msg.Printf( wxT("%2.3f"), Mself.lng /fcoeff);
		abort = Get_Message( _("Length(mm):"),msg, this);
	}
	if ( abort ) return NULL;

	double fval;
	if ( ! msg.ToDouble(&fval) )
	{
		DisplayError(this, _("Incorrect number, abort"));
		return NULL;
	}
	Mself.lng = (int) round( fval * fcoeff );

	/* Controle des valeurs ( ii = valeur minimale de la longueur */
	if( Mself.lng < Mself.m_Size.y )
	{
		DisplayError(this, _("Requested length < minimum length"));
		return NULL;
	}

	/* Generation du composant: calcul des elements de la self */
	Mself.m_Width = g_DesignSettings.m_CurrentTrackWidth;
	Mself.m_Size.x = Mself.m_Size.y / 2 ;
	// Choix d'une Valeur de depart raisonnable pour le rayon des arcs de cercle
	Mself.rayon = min(Mself.m_Width * 5, Mself.m_Size.x/4);
	/* Calcul des parametres */

	for ( Mself.nbrin = 2 ; ; Mself.nbrin++)
	{
		Mself.delta = (Mself.m_Size.y - ( Mself.rayon * 2 * Mself.nbrin ) ) / 2 ;
		if(Mself.delta < Mself.m_Size.y / 10) // C.a.d. si m_Size.yeur self > m_Size.yeur specifiee
		{	// Reduction du rayon des arrondis
			Mself.delta = Mself.m_Size.y / 10;
			Mself.rayon = (Mself.m_Size.y - 2*Mself.delta) / ( 2 * Mself.nbrin) ;
			if(Mself.rayon < Mself.m_Width)
			{ // Rayon vraiment trop petit...
				Affiche_Message(_("Unable to create line: Requested length is too big"));
				return NULL;
			}
		}
		Mself.lbrin = Mself.m_Size.x - (Mself.rayon * 2);
		lextbrin = (Mself.lbrin/2) - Mself.rayon;
		ll = 2 * lextbrin ;			// Longueur du 1er et dernier brin
		ll += 2 * Mself.delta ;		// Longueur des raccord au serpentin
		ll += Mself.nbrin * (Mself.lbrin - 2);	// longueur des autres brins
		ll += ((Mself.nbrin+1) * 314 * Mself.rayon) /100 ;

		msg.Printf( _("Segm count = %d, Lenght = "), Mself.nbrin);
		wxString stlen;
		valeur_param(ll, stlen); msg += stlen;
		Affiche_Message(msg);
		if ( ll >= Mself.lng) break;
	}

	/* Generation du composant : le calcul est fait self Verticale */
	if( Create_1_Module(DC, wxEmptyString) == NULL ) return NULL;

	Module = m_Pcb->m_Modules;
	Module->m_LibRef = wxT("MuSelf");
	Module->m_Attributs = MOD_VIRTUAL | MOD_CMS;
	Module->m_Flags = 0;

	Module->Draw(DrawPanel, DC, wxPoint(0,0), GR_XOR);

	/* Generation des elements speciaux: drawsegments */
	LastSegm = (EDGE_MODULE*) Module->m_Drawings;
	if( LastSegm ) while( LastSegm->Pnext) LastSegm = (EDGE_MODULE*)LastSegm->Pnext;

	FirstSegm = PtSegm = new EDGE_MODULE(Module);
	if (LastSegm )
		{
		LastSegm->Pnext = PtSegm;
		PtSegm->Pback = LastSegm;
		}
	else
		{
		Module->m_Drawings = PtSegm; PtSegm->Pback = Module;
		}
	PtSegm->m_Start = Mself.m_Start;
	PtSegm->m_End.x = Mself.m_Start.x;
	PtSegm->m_End.y = PtSegm->m_Start.y + Mself.delta;
	PtSegm->m_Width = Mself.m_Width;
	PtSegm->m_Layer = Module->m_Layer;
	PtSegm->m_Shape = S_SEGMENT;

	newedge = new EDGE_MODULE(Module);
	newedge->Copy(PtSegm);
	newedge->AddToChain(PtSegm);
	PtSegm = newedge;
	PtSegm->m_Start = PtSegm->m_End;
	PtSegm = gen_arc(PtSegm,PtSegm->m_End.x - Mself.rayon, PtSegm->m_End.y, -900);

	if(lextbrin)
	{
		newedge = new EDGE_MODULE(Module);
		newedge->Copy(PtSegm);
		newedge->AddToChain(PtSegm);
		PtSegm = newedge;
		PtSegm->m_Start = PtSegm->m_End;
		PtSegm->m_End.x -= lextbrin;
	}

	/* Trace du serpentin */
	for (ii = 1 ; ii < Mself.nbrin; ii++)
		{
		int arc_angle;
		newedge = new EDGE_MODULE(Module);
		newedge->Copy(PtSegm);
		newedge->AddToChain(PtSegm);
		PtSegm = newedge;
		PtSegm->m_Start = PtSegm->m_End;
		if( ii & 1)		/* brin d'ordre impair : cercles de sens > 0 */
			arc_angle = 1800;
		else arc_angle = -1800;

		PtSegm = gen_arc(PtSegm, PtSegm->m_End.x,
								PtSegm->m_End.y + Mself.rayon, arc_angle);

		if( ii < Mself.nbrin-1)
			{
			newedge = new EDGE_MODULE(Module);
			newedge->Copy(PtSegm);
			newedge->AddToChain(PtSegm);
			PtSegm = newedge;
			PtSegm->m_Start = PtSegm->m_End;
			if( ii & 1) PtSegm->m_End.x += Mself.lbrin;
			else PtSegm->m_End.x -= Mself.lbrin;
			}
		}

	/* Trace du point final */

	if( ii & 1)		/* brin final de sens > 0 */
	{
		if(lextbrin)
		{
			newedge = new EDGE_MODULE(Module);
			newedge->Copy(PtSegm);
			newedge->AddToChain(PtSegm);
			PtSegm = newedge;
			PtSegm->m_Start = PtSegm->m_End;
			PtSegm->m_End.x -= lextbrin;
		}

		newedge = new EDGE_MODULE(Module);
		newedge->Copy(PtSegm);
		newedge->AddToChain(PtSegm);
		PtSegm = newedge;
		PtSegm->m_Start.x = PtSegm->m_End.x; PtSegm->m_Start.y = PtSegm->m_End.y;
		PtSegm = gen_arc(PtSegm, PtSegm->m_End.x, PtSegm->m_End.y + Mself.rayon, 900);
	}
	else
	{
		if(lextbrin)
		{
			newedge = new EDGE_MODULE(Module);
			newedge->Copy(PtSegm);
			newedge->AddToChain(PtSegm);
			PtSegm = newedge;
			PtSegm->m_Start = PtSegm->m_End;
			PtSegm->m_End.x += lextbrin;
		}
		newedge = new EDGE_MODULE(Module);
		newedge->Copy(PtSegm);
		newedge->AddToChain(PtSegm);
		PtSegm = newedge;
		PtSegm->m_Start = PtSegm->m_End;
		PtSegm = gen_arc(PtSegm, PtSegm->m_End.x, PtSegm->m_End.y + Mself.rayon, -900);
	}

	newedge = new EDGE_MODULE(Module);
	newedge->Copy(PtSegm);
	newedge->AddToChain(PtSegm);
	PtSegm = newedge;
	PtSegm->m_Start = PtSegm->m_End;
	PtSegm->m_End = Mself.m_End;
	PtSegm->Pnext = NULL;

	/* Rotation de la self si le trace doit etre horizontal : */
	LastSegm = PtSegm;
	if ( Mself.orient == 0)
	{
		for( PtSegm = FirstSegm; PtSegm != NULL; PtSegm = (EDGE_MODULE*) PtSegm->Pnext )
		{
			RotatePoint(&PtSegm->m_Start.x, &PtSegm->m_Start.y,
							FirstSegm->m_Start.x, FirstSegm->m_Start.y, 900 );
			if( PtSegm != LastSegm )
				RotatePoint(&PtSegm->m_End.x, &PtSegm->m_End.y,
							FirstSegm->m_Start.x, FirstSegm->m_Start.y, 900 );
		}
	}

	/* Modif  position ancre  */
	Module->m_Pos.x = LastSegm->m_End.x; Module->m_Pos.y = LastSegm->m_End.y;

	/* Placement des 2 pads sur extremite */
	PtPad = new D_PAD(Module);

	Module->m_Pads = PtPad; PtPad->Pback = Module;
	PtPad->SetPadName( wxT("1") );
	PtPad->m_Pos.x = LastSegm->m_End.x; PtPad->m_Pos.y = LastSegm->m_End.y;
	PtPad->m_Pos0.x = PtPad->m_Pos.x - Module->m_Pos.x;
	PtPad->m_Pos0.y = PtPad->m_Pos.y - Module->m_Pos.y;
	PtPad->m_Size.x = PtPad->m_Size.y = LastSegm->m_Width;
	PtPad->m_Masque_Layer = g_TabOneLayerMask[LastSegm->m_Layer];
	PtPad->m_Attribut = SMD;
	PtPad->m_PadShape = CIRCLE;
	PtPad->m_Rayon = PtPad->m_Size.x / 2;

	D_PAD * newpad = new D_PAD(Module);
	newpad->Copy(PtPad);
	newpad->AddToChain(PtPad);
	PtPad = newpad;
	PtPad->SetPadName( wxT("2") );
	PtPad->m_Pos.x = FirstSegm->m_Start.x; PtPad->m_Pos.y = FirstSegm->m_Start.y;
	PtPad->m_Pos0.x = PtPad->m_Pos.x - Module->m_Pos.x;
	PtPad->m_Pos0.y = PtPad->m_Pos.y - Module->m_Pos.y;

	/* Modif des positions textes */
	Module->Display_Infos(this);
	Module->m_Value->m_Pos.x = Module->m_Reference->m_Pos.x = ( FirstSegm->m_Start.x + LastSegm->m_End.x ) /2 ;
	Module->m_Value->m_Pos.y = Module->m_Reference->m_Pos.y = ( FirstSegm->m_Start.y + LastSegm->m_End.y ) /2 ;

	Module->m_Reference->m_Pos.y -= Module->m_Reference->m_Size.y;
	Module->m_Value->m_Pos.y += Module->m_Value->m_Size.y;

	Module->m_Reference->m_Pos0.x = Module->m_Reference->m_Pos.x - Module->m_Pos.x;
	Module->m_Reference->m_Pos0.y = Module->m_Reference->m_Pos.y - Module->m_Pos.y;
	Module->m_Value->m_Pos0.x = Module->m_Value->m_Pos.x - Module->m_Pos.x;
	Module->m_Value->m_Pos0.y = Module->m_Value->m_Pos.y - Module->m_Pos.y;

	/* Init des Coord locales des segments */
	for( PtSegm = FirstSegm; PtSegm != NULL; PtSegm = (EDGE_MODULE*) PtSegm->Pnext )
	{
		PtSegm->m_Start0.x = PtSegm->m_Start.x - Module->m_Pos.x;
		PtSegm->m_Start0.y = PtSegm->m_Start.y - Module->m_Pos.y;
		PtSegm->m_End0.x = PtSegm->m_End.x - Module->m_Pos.x;
		PtSegm->m_End0.y = PtSegm->m_End.y - Module->m_Pos.y;
	}

	Module->Set_Rectangle_Encadrement();

	Module->Draw(DrawPanel, DC, wxPoint(0,0), GR_OR);

	return Module;
}


/**************************************************************************/
static EDGE_MODULE * gen_arc(EDGE_MODULE * PtSegm, int cX, int cY, int angle)
/**************************************************************************/

/* Genere un arc de EDGE_MODULE :
	de centre cX,cY
	d'angle "angle"
	de point de depart donne dans la structure pointee par PtSegm, qui doit
	entre a jour (type,net..)
	Retourne un pointeur sur la derniere structure EDGE_MODULE generee
*/
{
int ii, nb_seg;
float alpha, beta, fsin, fcos;
int x0, xr0, y0, yr0;
EDGE_MODULE * newedge;

	angle = -angle;
	y0 = PtSegm->m_Start.x - cX; x0 = PtSegm->m_Start.y - cY;

	nb_seg = (abs(angle)) / 225 ; if(nb_seg == 0) nb_seg = 1 ;
	alpha = ( (float)angle * 3.14159 / 1800 ) / nb_seg;

	for ( ii = 1 ; ii <= nb_seg ; ii++ )
		{
		if( ii > 1)
			{
			newedge = new EDGE_MODULE( (MODULE*) NULL);
			newedge->Copy(PtSegm);
			newedge->m_Parent = PtSegm->m_Parent;
			newedge->AddToChain(PtSegm);
			PtSegm = newedge;
			PtSegm->m_Start.x = PtSegm->m_End.x; PtSegm->m_Start.y = PtSegm->m_End.y;
			}

		beta = (alpha * ii);
		fcos =  cos(beta); fsin = sin(beta);

		xr0 = (int)(x0 * fcos + y0 * fsin);
		yr0	= (int)(y0 * fcos - x0 * fsin);
		PtSegm->m_End.x = cX + yr0; PtSegm->m_End.y = cY + xr0 ;
		}
	return( PtSegm );
}

