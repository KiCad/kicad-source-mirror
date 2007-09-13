	/**************************************************/
	/* Module de tst "ERC" ( Electrical Rules Check ) */
	/**************************************************/

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"

#include "protos.h"


#include "../bitmaps/ercgreen.xpm"
#include "../bitmaps/ercwarn.xpm"
#include "../bitmaps/ercerr.xpm"

#include "dialog_erc.h"

/* On teste
	1 - conflits entre pins connectees ( ex: 2 sorties connectees )
	2 - les imperatifs minimaux ( 1 entree doit etre connectee a une sortie )
*/


/* fonctions exportees */

/* fonctions importees */

/* fonctions locales */
static bool WriteDiagnosticERC(const wxString & FullFileName);
static void Diagnose(WinEDA_DrawPanel * panel, wxDC * DC,
				ObjetNetListStruct * NetItemRef,
				ObjetNetListStruct * NetItemTst, int MinConnexion, int Diag);
static void TestOthersItems(WinEDA_DrawPanel * panel,
								wxDC * DC, ObjetNetListStruct *  NetItemRef,
								ObjetNetListStruct *  NetStart,
								int * NetNbItems, int * MinConnexion );
static void TestLabel(WinEDA_DrawPanel * panel, wxDC * DC,
							ObjetNetListStruct * NetItemRef,
							ObjetNetListStruct * StartNet);

/* Variable locales */
int WriteFichierERC = FALSE;

/* Tableau des types de conflit :
	PIN_INPUT, PIN_OUTPUT, PIN_BIDI, PIN_TRISTATE, PIN_PASSIVE,
	PIN_UNSPECIFIED, PIN_POWER_IN, PIN_POWER_OUT, PIN_OPENCOLLECTOR,
	PIN_OPENEMITTER, PIN_NC
*/
#define OK 0
#define WAR 1	// utilisé aussi dans eeredraw
#define ERR 2
#define UNC 3

static wxChar * CommentERC_H[] =
{
	wxT("Input Pin...."),
	wxT("Output Pin..."),
	wxT("BiDi Pin....."),
	wxT("3 State Pin.."),
	wxT("Passive Pin.."),
	wxT("Unspec Pin..."),
	wxT("Power IN Pin."),
	wxT("PowerOUT Pin."),
	wxT("Open Coll...."),
	wxT("Open Emit...."),
	wxT("No Conn......"),
	NULL
};
static wxChar * CommentERC_V[] =
{
	wxT("Input Pin"),
	wxT("Output Pin"),
	wxT("BiDi Pin"),
	wxT("3 State Pin"),
	wxT("Passive Pin"),
	wxT("Unspec Pin"),
	wxT("Power IN Pin"),
	wxT("PowerOUT Pin"),
	wxT("Open Coll"),
	wxT("Open Emit"),
	wxT("No Conn"),
	NULL
};


/* Look up table which gives the diag for a pair of connected pins
	Can be modified by ERC options.
	at start up: must be loaded by DefaultDiagErc
*/
static int DiagErc[PIN_NMAX][PIN_NMAX];
bool DiagErcTableInit;	// go to TRUE after DiagErc init

/* Default Look up table which gives the diag for a pair of connected pins
	Same as DiagErc, but cannot be modified
	Used to init or reset DiagErc
*/
static int DefaultDiagErc[PIN_NMAX][PIN_NMAX] =
{ /*       I,   O,   Bi,  3S, Pas, UnS,PwrI,PwrO,  OC,  OE,  NC */
/* I */ { OK,   OK,  OK,  OK,  OK, WAR,  OK,  OK,  OK,  OK, WAR },
/* O */ { OK,  ERR,  OK, WAR,  OK, WAR,  OK, ERR, ERR, ERR, WAR },
/* Bi*/ { OK,   OK,  OK,  OK,  OK, WAR,  OK, WAR,  OK, WAR, WAR },
/* 3S*/ { OK,  WAR,  OK,  OK,  OK, WAR, WAR, ERR, WAR, WAR, WAR },
/*Pas*/ { OK,   OK,  OK,  OK,  OK, WAR,  OK,  OK,  OK,  OK, WAR },
/*UnS */{ WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR },
/*PwrI*/{ OK,   OK,  OK, WAR,  OK, WAR,  OK,  OK,  OK,  OK, ERR },
/*PwrO*/{ OK,  ERR, WAR, ERR,  OK, WAR,  OK, ERR, ERR, ERR, WAR },
/* OC */{ OK,  ERR,  OK, WAR,  OK, WAR,  OK, ERR,  OK,  OK, WAR },
/* OE */{ OK,  ERR, WAR, WAR,  OK, WAR,  OK, ERR,  OK,  OK, WAR },
/* NC */{ WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR }
} ;


/* Minimal connection table */
#define DRV 3		/* Net driven by a signal (a pin output for instance) */
#define NET_NC 2	/* Net "connected" to a "NoConnect symbol" */
#define NOD 1		/* Net not driven ( Such as 2 or more connected inputs )*/
#define NOC 0		/* Pin isolee, non connectee */

/* Look up table which gives the minimal drive for a pair of connected pins on a net
	Initial state of a net is NOC (No Connection)
	Can be updated to NET_NC, or NOD (Not Driven) or DRV (DRIven)

	Can be updated to NET_NC only if the previous state is NOC

	Nets are OK when their final state is NET_NC or DRV
	Nets with the state NOD have no source signal
*/
static int MinimalReq[PIN_NMAX][PIN_NMAX] =
		{ /* In, Out,  Bi,  3S, Pas, UnS,PwrI,PwrO,  OC,  OE,  NC */
/* In*/ {	NOD, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/*Out*/ {	DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, NOC },
/* Bi*/ {	DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/* 3S*/ {	DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/*Pas*/ {	DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/*UnS*/ {	DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/*PwrI*/{	NOD, DRV, NOD, NOD, NOD, NOD, NOD, DRV, NOD, NOD, NOC },
/*PwrO*/{	DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, NOC },
/* OC*/ {	DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/* OE*/ {	DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/* NC*/ {	NOC, NOC, NOC, NOC, NOC, NOC, NOC, NOC, NOC, NOC, NOC }
} ;




/*************************************************************/
void InstallErcFrame(WinEDA_SchematicFrame *parent, wxPoint & pos)
/*************************************************************/
/* Install function  for the ERC dialog frame
*/
{
	WinEDA_ErcFrame * frame = new WinEDA_ErcFrame(parent);
	frame->ShowModal(); frame->Destroy();
}


/*********************************************/
void WinEDA_ErcFrame::ReBuildMatrixPanel()
/*********************************************/
/* Build or rebuild the panel showing the ERC matrix
*/
{
int ii, jj, event_id, text_height;
wxPoint pos, BoxMatrixPosition;
#define BITMAP_SIZE 19
int bitmap_size = BITMAP_SIZE;
wxStaticText * text;
int x, y;
wxSize BoxMatrixMinSize;

	if ( ! DiagErcTableInit )
	{
		memcpy(DiagErc, DefaultDiagErc, sizeof (DefaultDiagErc));
		DiagErcTableInit = TRUE;
	}

	// Get the current text size :
	text = new wxStaticText( m_PanelERCOptions,-1,wxT("W"), pos);	// this is a dummy text
	text_height = text->GetRect().GetHeight();
	bitmap_size = MAX(bitmap_size, text_height);
	delete text;
	// compute the Y pos interval:
	BoxMatrixMinSize.y = (bitmap_size*(PIN_NMAX+1)) + 5;
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
	pos = m_MatrixSizer->GetPosition();
	// Size computation is not made in constructor, in some wxWidgets version,
	// and m_BoxSizerForERC_Opt position is always 0,0. and we can't use it
	pos.x = MAX( pos.x, 5);
	pos.y = MAX( pos.y, m_ResetOptButton->GetRect().GetHeight() + 30);

	BoxMatrixPosition = pos;
	
	pos.y += text_height;
	
	if ( m_Initialized == FALSE )
	{
		for ( ii = 0; ii < PIN_NMAX; ii++ )
		{
			y = pos.y + (ii * bitmap_size);
			text = new wxStaticText( m_PanelERCOptions,-1,CommentERC_H[ii], wxPoint(5,y));
			x = text->GetRect().GetRight();
			pos.x = MAX(pos.x, x);
		}
		pos.x += 5;
	}
	else pos = m_ButtonList[0][0]->GetPosition();

	for ( ii = 0; ii < PIN_NMAX; ii++ )
	{
		y = pos.y + (ii * bitmap_size);
		for ( jj = 0; jj <= ii; jj++ )
		{
			int diag = DiagErc[ii][jj];
			x = pos.x + (jj * bitmap_size);
			if( (ii == jj) && ! m_Initialized )
			{
				wxPoint txtpos;
				txtpos.x = x + 4; txtpos.y = y - bitmap_size;
				text = new wxStaticText( m_PanelERCOptions,-1,CommentERC_V[ii], txtpos);
				BoxMatrixMinSize.x = MAX( BoxMatrixMinSize.x, text->GetRect().GetRight());
			}
			event_id = ID_MATRIX_0 + ii + (jj * PIN_NMAX);
			delete m_ButtonList[ii][jj];
			switch ( diag )
			{
				case OK:
					m_ButtonList[ii][jj] = new wxBitmapButton(m_PanelERCOptions,
						event_id,
						wxBitmap(green_xpm),
						wxPoint(x,y) );
					break;

				case WAR:
					m_ButtonList[ii][jj] = new wxBitmapButton(m_PanelERCOptions,
						event_id,
						wxBitmap(warning_xpm),
						wxPoint(x,y) );
					break;

				case ERR:
					m_ButtonList[ii][jj] = new wxBitmapButton(m_PanelERCOptions,
						event_id,
						wxBitmap(error_xpm),
						wxPoint(x,y) );
					break;
			}
		}
	}

	if ( !m_Initialized )
	{
		BoxMatrixMinSize.x += 5;
		m_MatrixSizer->SetMinSize(BoxMatrixMinSize);
		BoxMatrixMinSize.y += BoxMatrixPosition.y;
		m_PanelMatrixSizer->SetMinSize(BoxMatrixMinSize);
	}
	m_Initialized = TRUE;
}


/**************************************************/
void WinEDA_ErcFrame::TestErc(wxCommandEvent& event)
/**************************************************/
{
ObjetNetListStruct * NetItemRef, * OldItem, * StartNet, * Lim;
int NetNbItems, MinConn;

	if ( ! DiagErcTableInit )
	{
		memcpy(DiagErc, DefaultDiagErc, sizeof (DefaultDiagErc));
		DiagErcTableInit = TRUE;
	}

	WriteFichierERC = m_WriteResultOpt->GetValue();

	ReAnnotatePowerSymbolsOnly();
	if( CheckAnnotate(m_Parent, 0) )
	{
		DisplayError(this, _("Annotation Required!") );
		return;
	}

	/* Effacement des anciens marqueurs DRC */
	DelERCMarkers(event);

wxClientDC dc(m_Parent->DrawPanel);

	m_Parent->DrawPanel->PrepareGraphicContext(&dc);

	g_EESchemaVar.NbErrorErc = 0;
	g_EESchemaVar.NbWarningErc = 0;

	/* Cleanup the entire hierarchy */
	EDA_ScreenList ScreenList(NULL);
	for ( SCH_SCREEN * Screen = ScreenList.GetFirst(); Screen != NULL; Screen = ScreenList.GetNext() )
	{
		bool ModifyWires;
		ModifyWires = Screen->SchematicCleanUp(NULL);
		/* if wire list has changed, delete Udo Redo list to avoid
		pointers on deleted data problems */
		if ( ModifyWires )
			Screen->ClearUndoRedoList();
	}

	m_Parent->BuildNetListBase();

	/* Analyse de la table des connexions : */
	Lim = g_TabObjNet + g_NbrObjNet;

	/* Reset du flag m_FlagOfConnection, utilise par la suite */
	for (NetItemRef = g_TabObjNet; NetItemRef < Lim; NetItemRef ++ )
		NetItemRef->m_FlagOfConnection = (IsConnectType) 0;

	NetNbItems = 0; MinConn = NOC;
	StartNet = OldItem = NetItemRef = g_TabObjNet;
	for ( ; NetItemRef < Lim; NetItemRef ++ )
	{
		/* Tst changement de net */
		if( OldItem->m_NetCode != NetItemRef->m_NetCode)
		{
			MinConn = NOC; NetNbItems = 0; StartNet = NetItemRef;
		}

		switch ( NetItemRef->m_Type )
		{
			case NET_SEGMENT:
			case NET_BUS:
			case NET_JONCTION:
			case NET_LABEL:
			case NET_BUSLABELMEMBER:
			case NET_PINLABEL:
				break;

			case NET_GLOBLABEL:
			case NET_GLOBBUSLABELMEMBER:
			case NET_SHEETLABEL:
			case NET_SHEETBUSLABELMEMBER:
				TestLabel(m_Parent->DrawPanel, &dc, NetItemRef, StartNet);
				break;

			case NET_NOCONNECT:
				MinConn = NET_NC;
				if( NetNbItems != 0 )
					  Diagnose(m_Parent->DrawPanel, &dc, NetItemRef, NULL, MinConn, UNC);
				break;

			case NET_PIN:
				TestOthersItems(m_Parent->DrawPanel, &dc,
						NetItemRef, StartNet, &NetNbItems , &MinConn);
				break;
		}
		OldItem = NetItemRef;
	}

	FreeTabNetList(g_TabObjNet, g_NbrObjNet );

	wxString num;
	num.Printf(wxT("%d"), g_EESchemaVar.NbErrorErc);
	m_TotalErrCount->SetLabel(num);

	num.Printf(wxT("%d"), g_EESchemaVar.NbErrorErc-g_EESchemaVar.NbWarningErc);
	m_LastErrCount->SetLabel(num);

	num.Printf(wxT("%d"), g_EESchemaVar.NbWarningErc);
	m_LastWarningCount->SetLabel(num);

	/* Generation ouverture fichier diag */
	if( WriteFichierERC == TRUE )
	{
		wxString ErcFullFileName;
		ErcFullFileName = ScreenSch->m_FileName;
		ChangeFileNameExt(ErcFullFileName, wxT(".erc"));
		ErcFullFileName = EDA_FileSelector(_("ERC file:"),
					wxEmptyString,					/* Chemin par defaut */
					ErcFullFileName,	/* nom fichier par defaut */
					wxT(".erc"),				/* extension par defaut */
					wxT("*.erc"),			/* Masque d'affichage */
					this,
					wxFD_SAVE,
					TRUE
					);
		if ( ErcFullFileName.IsEmpty()) return;

		if ( WriteDiagnosticERC(ErcFullFileName) )
		{
			Close(TRUE);
			wxString editorname = GetEditorName();
			AddDelimiterString(ErcFullFileName);
			ExecuteFile(this, editorname, ErcFullFileName);
		}
	}
}


/***********************************************************/
void WinEDA_ErcFrame::DelERCMarkers(wxCommandEvent& event)
/***********************************************************/
/* Delete the old ERC markers, over the whole hierarchy
*/
{
EDA_BaseStruct * DrawStruct;
DrawMarkerStruct * Marker;
wxClientDC dc(m_Parent->DrawPanel);

	m_Parent->DrawPanel->PrepareGraphicContext(&dc);

	// Delete markers for the current screen
	DrawStruct = m_Parent->GetScreen()->EEDrawList;
	for ( ; DrawStruct != NULL; DrawStruct = DrawStruct->Pnext )
	{
		if(DrawStruct->Type() != DRAW_MARKER_STRUCT_TYPE ) continue;
		/* Marqueur trouve */
		Marker = (DrawMarkerStruct * ) DrawStruct;
		if( Marker->m_Type == MARQ_ERC )
			RedrawOneStruct(m_Parent->DrawPanel, &dc, Marker, g_XorMode);
	}
	/* Suppression en memoire des marqueurs ERC */
	DeleteAllMarkers(MARQ_ERC);
}


/**************************************************************/
void WinEDA_ErcFrame::ResetDefaultERCDiag(wxCommandEvent& event)
/**************************************************************/
/* Remet aux valeurs par defaut la matrice de diagnostic
*/
{
	memcpy(DiagErc,DefaultDiagErc, sizeof(DiagErc) );
	ReBuildMatrixPanel();
}

/************************************************************/
void WinEDA_ErcFrame::ChangeErrorLevel(wxCommandEvent& event)
/************************************************************/
/* Change the error level for the pressed button, on the matrix table
*/
{
int id, level, ii, x, y;
wxBitmapButton * Butt;
char ** new_bitmap_xpm = NULL;
wxPoint pos;

	id = event.GetId();
	ii = id - ID_MATRIX_0;
	Butt = (wxBitmapButton*) event.GetEventObject();
	pos = Butt->GetPosition();

	x = ii /PIN_NMAX; y = ii % PIN_NMAX;

	level = DiagErc[y][x];
	switch (level )
	{
		case OK:
			level = WAR;
			new_bitmap_xpm = warning_xpm;
			break;

		case WAR :
			level = ERR;
			new_bitmap_xpm = error_xpm;
			break;

		case ERR:
			level = OK;
			new_bitmap_xpm = green_xpm;
			break;

	}

	if ( new_bitmap_xpm )
	{
		delete Butt;
		Butt = new wxBitmapButton(m_PanelERCOptions, id,
							wxBitmap(new_bitmap_xpm), pos);
		m_ButtonList[y][x] = Butt;
		DiagErc[y][x] = DiagErc[x][y] = level;
	}
}


/********************************************************/
static void Diagnose(WinEDA_DrawPanel * panel, wxDC * DC,
                    ObjetNetListStruct * NetItemRef,
					ObjetNetListStruct * NetItemTst,
                    int MinConn, int Diag)
/********************************************************/
/* Routine de creation du marqueur ERC correspondant au conflit electrique
	entre NetItemRef et NetItemTst
	si MinConn < 0: Traitement des erreurs sur labels
*/
{
DrawMarkerStruct * Marker = NULL;
wxString DiagLevel;
SCH_SCREEN * screen;
int ii, jj;

	if( Diag == OK ) return;

	/* Creation du nouveau marqueur type Erreur ERC */
	Marker = new DrawMarkerStruct(NetItemRef->m_Start, wxEmptyString);
	Marker->m_Type = MARQ_ERC;
	Marker->m_MarkFlags = WAR;
	screen = NetItemRef->m_Screen;
	Marker->Pnext = screen->EEDrawList;
	screen->EEDrawList = Marker;
	g_EESchemaVar.NbErrorErc++;
	g_EESchemaVar.NbWarningErc++;

	if( MinConn < 0 )	// Traitement des erreurs sur labels
		{
		if( (NetItemRef->m_Type == NET_GLOBLABEL) ||
			(NetItemRef->m_Type == NET_GLOBBUSLABELMEMBER) )
			{
			Marker->m_Comment.Printf( _("Warning GLabel %s not connected to SheetLabel"),
					NetItemRef->m_Label->GetData());
			}
		else Marker->m_Comment.Printf( _("Warning SheetLabel %s not connected to GLabel"),
					NetItemRef->m_Label->GetData());

		if( screen == panel->GetScreen() ) RedrawOneStruct(panel, DC, Marker, GR_COPY);
		return;
		}

	ii = NetItemRef->m_ElectricalType;

	if( NetItemTst == NULL )
		{
		if( MinConn == NOC )	/* 1 seul element dans le net */
			{
			Marker->m_Comment.Printf( _("Warning Pin %s Unconnected"), MsgPinElectricType[ii]);
			if( screen == panel->GetScreen() )
				RedrawOneStruct(panel, DC, Marker, GR_COPY);
			return;
			}

		if( MinConn == NOD )	/* pas de pilotage du net */
			{
			Marker->m_Comment.Printf(
				_("Warning Pin %s not driven (Net %d)"),
				MsgPinElectricType[ii], NetItemRef->m_NetCode);
			if( screen == panel->GetScreen() )
				RedrawOneStruct(panel, DC, Marker, GR_COPY);
			return;
			}

		if( Diag == UNC )
			{
			Marker->m_Comment.Printf(
                _("Warning More than 1 Pin connected to UnConnect symbol") );
			if( screen == panel->GetScreen() )
				RedrawOneStruct(panel, DC, Marker, GR_COPY);
			return;
			}

		}

	if( NetItemTst )		 /* Erreur entre 2 pins */
	{
		jj = NetItemTst->m_ElectricalType;
		DiagLevel = _("Warning");
		if(Diag == ERR)
		{
			DiagLevel = _("Error");
			Marker->m_MarkFlags = ERR;
			g_EESchemaVar.NbWarningErc--;
		}


		Marker->m_Comment.Printf( _("%s: Pin %s connected to Pin %s (net %d)"), DiagLevel.GetData(),
					 MsgPinElectricType[ii],
					 MsgPinElectricType[jj], NetItemRef->m_NetCode);

		if( screen == panel->GetScreen() )
			RedrawOneStruct(panel, DC, Marker, GR_COPY);
	}
}


/********************************************************************/
static void TestOthersItems(WinEDA_DrawPanel * panel, wxDC * DC,
								ObjetNetListStruct *  NetItemRef,
								ObjetNetListStruct *  netstart,
								int * NetNbItems, int * MinConnexion)
/********************************************************************/
/* Routine testant les conflits electriques entre
	NetItemRef
	et les autres items du meme net
*/
{
ObjetNetListStruct * NetItemTst, * Lim;
int ref_elect_type, jj, erc = OK, local_minconn;

	/* Analyse de la table des connexions : */
	Lim = g_TabObjNet + g_NbrObjNet; // pointe la fin de la liste
	ref_elect_type = NetItemRef->m_ElectricalType;

	NetItemTst = netstart;
	local_minconn = NOC;

	/* Examen de la liste des Pins connectees a NetItemRef */
	for ( ; ; NetItemTst ++ )
		{
		if ( NetItemRef == NetItemTst ) continue;

		/* Est - on toujours dans le meme net ? */
		if( (NetItemTst >= Lim) ||  	// fin de liste (donc fin de net)
			(NetItemRef->m_NetCode != NetItemTst->m_NetCode) )  // fin de net
			{	/* Fin de netcode trouve: Tst connexion minimum */
			if( (*MinConnexion < NET_NC ) &&
				 (local_minconn < NET_NC ) ) /* pin non connectée ou non pilotee */
				{
				Diagnose(panel, DC, NetItemRef, NULL, local_minconn, WAR);
				* MinConnexion = DRV;	// inhibition autres messages de ce type pour ce net
				}
			return;
			}

		switch ( NetItemTst->m_Type )
			{
			case NET_SEGMENT:
			case NET_BUS:
			case NET_JONCTION:
			case NET_LABEL:
			case NET_GLOBLABEL:
			case NET_BUSLABELMEMBER:
			case NET_GLOBBUSLABELMEMBER:
			case NET_SHEETBUSLABELMEMBER:
			case NET_SHEETLABEL:
			case NET_PINLABEL:
				break;

			case NET_NOCONNECT:
				local_minconn = MAX( NET_NC, local_minconn);
				break;

			case NET_PIN:
				jj = NetItemTst->m_ElectricalType;
				local_minconn = MAX( MinimalReq[ref_elect_type][jj], local_minconn );

				if ( NetItemTst <= NetItemRef ) break;
				*NetNbItems += 1;
				if( erc == OK )		// 1 marqueur par pin maxi
					{
					erc = DiagErc[ref_elect_type][jj];
					if (erc != OK )
						{
					   if( NetItemTst->m_FlagOfConnection == 0 )
							{
							Diagnose(panel, DC, NetItemRef, NetItemTst, 0, erc);
							NetItemTst->m_FlagOfConnection = (IsConnectType) 1;
							}
						}
					}
				break;
			}
		}
}


/********************************************************/
static bool WriteDiagnosticERC(const wxString & FullFileName)
/*********************************************************/
/* Create the Diagnostic file (<xxx>.erc file)
*/
{
EDA_BaseStruct * DrawStruct;
DrawMarkerStruct * Marker;
char Line[256];
static FILE * OutErc;
DrawSheetStruct * Sheet;
wxString msg;

	if( (OutErc = wxFopen( FullFileName, wxT("wt"))) == NULL ) return FALSE;

	DateAndTime(Line);
	msg = _("ERC control");
	fprintf( OutErc, "%s (%s)\n", CONV_TO_UTF8(msg), Line);

	EDA_ScreenList ScreenList(NULL);
	for ( SCH_SCREEN * Screen = ScreenList.GetFirst(); Screen != NULL; Screen = ScreenList.GetNext() )
	{
		Sheet = (DrawSheetStruct*) Screen;
		msg.Printf( _("\n***** Sheet %d (%s)\n"),
							Sheet->m_SheetNumber,
							Screen == ScreenSch ? _("Root") : Sheet->m_SheetName.GetData());
		fprintf( OutErc, "%s", CONV_TO_UTF8(msg));

		DrawStruct = Screen->EEDrawList;
		for ( ; DrawStruct != NULL; DrawStruct = DrawStruct->Pnext)
		{
			if(DrawStruct->Type() != DRAW_MARKER_STRUCT_TYPE )
				continue;

			/* Marqueur trouve */
			Marker = (DrawMarkerStruct * ) DrawStruct;
			if( Marker->m_Type != MARQ_ERC ) continue;
			/* Write diag marqueur */
			msg.Printf( _("ERC: %s (X= %2.3f inches, Y= %2.3f inches\n"),
								 Marker->GetComment().GetData(),
								 (float)Marker->m_Pos.x / 1000,
								 (float)Marker->m_Pos.y / 1000);
			fprintf( OutErc, "%s", CONV_TO_UTF8(msg));
		}
	}
	msg.Printf( _("\n >> Errors ERC: %d\n"), g_EESchemaVar.NbErrorErc);
	fprintf( OutErc, "%s", CONV_TO_UTF8(msg));
	fclose ( OutErc );

	return TRUE;
}


/***********************************************************************/
void TestLabel(WinEDA_DrawPanel * panel, wxDC * DC,
		ObjetNetListStruct * NetItemRef, ObjetNetListStruct * StartNet)
/***********************************************************************/
/* Routine controlant qu'un sheetLabel est bien connecte a un Glabel de la
sous-feuille correspondante
*/
{
ObjetNetListStruct * NetItemTst, * Lim;
int erc = 1;

	/* Analyse de la table des connexions : */
	Lim = g_TabObjNet + g_NbrObjNet;

	NetItemTst = StartNet;

	/* Examen de la liste des Labels connectees a NetItemRef */
	for ( ; ; NetItemTst ++ )
		{
		if( NetItemTst == NetItemRef ) continue;

		/* Est - on toujours dans le meme net ? */
		if( ( NetItemTst ==  Lim ) ||
			( NetItemRef->m_NetCode != NetItemTst->m_NetCode ) )
			{	/* Fin de netcode trouve */
			if( erc )
				{  /* GLabel ou SheetLabel orphelin */
				Diagnose(panel, DC, NetItemRef, NULL, -1, WAR);
				}
			return;
			}

		if( (NetItemRef->m_Type == NET_GLOBLABEL) ||
			(NetItemRef->m_Type == NET_GLOBBUSLABELMEMBER) )
			{
			switch ( NetItemTst->m_Type )
				{
				case NET_SEGMENT:
				case NET_BUS:
				case NET_JONCTION:
				case NET_LABEL:
				case NET_GLOBLABEL:
				case NET_BUSLABELMEMBER:
				case NET_GLOBBUSLABELMEMBER:
				case NET_PINLABEL:
				case NET_NOCONNECT:
				case NET_PIN:
					break;

				case NET_SHEETBUSLABELMEMBER:
				case NET_SHEETLABEL:
					/* Tst si le GLabel est bien dans la bonne sousfeuille */
					if( NetItemRef->m_SheetNumber == NetItemTst->m_NumInclude )
						{
						erc = 0;
						}
					break;
				}
			}

		else
			{
			switch ( NetItemTst->m_Type )
				{
				case NET_SEGMENT:
				case NET_BUS:
				case NET_JONCTION:
				case NET_LABEL:
				case NET_BUSLABELMEMBER:
				case NET_SHEETBUSLABELMEMBER:
				case NET_SHEETLABEL:
				case NET_PINLABEL:
				case NET_NOCONNECT:
				case NET_PIN:
					break;

				case NET_GLOBLABEL:
				case NET_GLOBBUSLABELMEMBER:
					/* Tst si le GLabel est bien dans la bonne sous-feuille */
					if( NetItemTst->m_SheetNumber == NetItemRef->m_NumInclude )
						{
						erc = 0;
						}
					break;
				}
			}
		}
}


