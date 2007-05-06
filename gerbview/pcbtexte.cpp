		/****************************************/
		/* traitement des textes sur empreintes */
		/****************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"

#include "protos.h"

#define L_MIN_DESSIN 1 /* seuil de largeur des segments pour trace autre que filaire */

/* Fonctions externe: */

/* Routines Locales */
static void Move_Texte_Pcb(WinEDA_DrawPanel * panel, wxDC * DC, int flag) ;
static void Exit_Texte_Pcb(WinEDA_DrawFrame * frame, wxDC *DC) ;

/* variables exportees */
int Angle_Rot_Fin = 50;  /* Valeur par defaut de l'angle de rotation fine */

/* Variables locales : */
static int LocalOrient;
static int LocalDimH;
static int LocalDimV;
static int LocalWidth;

static wxPoint old_pos;	// position originelle du texte selecte


	/**************************************/
	/* void Exit_Texte_Pcb(COMMAND * Cmd) */
	/**************************************/

/*
 Routine de sortie du menu edit texte Pcb
Si un texte est selectionne, ses coord initiales sont regenerees
*/
void Exit_Texte_Pcb(WinEDA_DrawFrame * frame, wxDC * DC)
{
TEXTE_PCB * TextePcb;

	TextePcb = (TEXTE_PCB *) frame->CurrentScreen->m_CurrentPcbItem;

	if ( TextePcb )
		{
		Trace_1_texte_pcb(frame->DrawPanel, DC, TextePcb, 0, 0, GR_XOR) ;
		TextePcb->m_Pos = old_pos;
		Trace_1_texte_pcb(frame->DrawPanel, DC, TextePcb, 0, 0, GR_OR) ;
		}

	frame->CurrentScreen->ManageCurseur = NULL;
	frame->CurrentScreen->ForceCloseManageCurseur = NULL;
	frame->CurrentScreen->m_CurrentPcbItem = NULL;
}

	/******************************************/
	/* void Place_Texte_Pcb(MTOOL_EDA * Menu) */
	/******************************************/

/* Routine de placement du texte en cours de deplacement
*/
void WinEDA_PcbFrame::Place_Texte_Pcb(TEXTE_PCB * TextePcb, wxDC * DC)
{

	if( TextePcb == NULL ) return;

	Trace_1_texte_pcb(DrawPanel, DC, TextePcb,0, 0, GR_XOR) ;
	TextePcb->m_Layer = CurrentScreen->m_Active_Layer;
	Trace_1_texte_pcb(DrawPanel, DC, TextePcb,0, 0, GR_OR) ;
	CurrentScreen->ManageCurseur = NULL;
	CurrentScreen->ForceCloseManageCurseur = NULL;
	CurrentScreen->m_CurrentPcbItem = NULL;
	CurrentScreen->SetModify();
}

	/*****************************************/
	/* void StartMoveTextePcb(COMMAND * Cmd) */
	/*****************************************/

/* Routine de preparation du deplacement d'un texte
*/

void WinEDA_PcbFrame::StartMoveTextePcb(TEXTE_PCB * TextePcb, wxDC * DC)
{
	if( TextePcb == NULL ) return;

	old_pos = TextePcb->m_Pos;
	Affiche_Infos_PCB_Texte(this, TextePcb);
	CurrentScreen->ManageCurseur = Move_Texte_Pcb;
	CurrentScreen->ForceCloseManageCurseur = Exit_Texte_Pcb;
	CurrentScreen->m_CurrentPcbItem = TextePcb;
}

	/*****************************/
	/* void Move_Texte_Pcb(void) */
	/*****************************/

/* Routine deplacant le texte PCB suivant le curseur de la souris */

static void Move_Texte_Pcb(WinEDA_DrawPanel * panel, wxDC *DC, int flag )
{
TEXTE_PCB * TextePcb = (TEXTE_PCB *)
		panel->m_Parent->CurrentScreen->m_CurrentPcbItem;

	if (TextePcb == NULL ) return ;

	/* effacement du texte : */
	
	if ( flag == CURSEUR_MOVED)
		Trace_1_texte_pcb(panel, DC, TextePcb,0, 0, GR_XOR) ;

	TextePcb->m_Pos = panel->m_Parent->CurrentScreen->m_Curseur;
	TextePcb->m_Layer = panel->m_Parent->CurrentScreen->m_Active_Layer;

	/* Redessin du Texte */
	Trace_1_texte_pcb(panel, DC, TextePcb,0, 0, GR_XOR) ;
}

 
	/********************************/
	/* void Delete_Texte_Pcb(COMMAND * Cmd) */
	/********************************/

void WinEDA_PcbFrame::Delete_Texte_Pcb(TEXTE_PCB * TextePcb, wxDC * DC)
{

	if( TextePcb == NULL ) return;

	Trace_1_texte_pcb(DrawPanel, DC, TextePcb,0, 0, GR_XOR);

	/* Suppression du texte en Memoire*/
	DeleteStructure(TextePcb);
	CurrentScreen->ManageCurseur = NULL;
	CurrentScreen->ForceCloseManageCurseur = NULL;
	CurrentScreen->m_CurrentPcbItem = NULL;
}


	/****************************/
	/* void Add_Texte_Pcb(COMMAND * Cmd) */
	/****************************/

TEXTE_PCB * WinEDA_PcbFrame::Create_Texte_Pcb( wxDC * DC)
{
char buf_new_texte[256];
TEXTE_PCB * TextePcb;

	buf_new_texte[0]= 0 ;
	Get_Message("Texte:",buf_new_texte, this);
	if(*buf_new_texte <=' ') return NULL;

	CurrentScreen->SetModify();
	/* placement du texte en memoire */
	TextePcb = new TEXTE_PCB(pt_pcb);

	/* Chainage de la nouvelle structure en debut de liste */
	TextePcb->Pnext = pt_pcb->m_Drawings;
	TextePcb->Pback = (EDA_BaseStruct * )pt_pcb;
	if( pt_pcb->m_Drawings) pt_pcb->m_Drawings->Pback = (EDA_BaseStruct*) TextePcb;
	pt_pcb->m_Drawings = (EDA_BaseStruct*) TextePcb;

	/* Mise a jour des caracteristiques */
	TextePcb->m_Layer = CurrentScreen->m_Active_Layer;
	TextePcb->m_Miroir = 1;
	if(CurrentScreen->m_Active_Layer == CUIVRE_N) TextePcb->m_Miroir = 0;

	TextePcb->m_Size.y = Texte_Pcb_DimV ;
	TextePcb->m_Size.x = Texte_Pcb_DimH ;
	TextePcb->m_Pos = CurrentScreen->m_Curseur;
	TextePcb->m_Width = Texte_Segment_Largeur;
	TextePcb->m_Layer = CurrentScreen->m_Active_Layer;

	/* Copie du texte */
	TextePcb->SetText(buf_new_texte);

	/* Dessin du Texte */
	Trace_1_texte_pcb(DrawPanel, DC, TextePcb,0, 0, GR_XOR) ;
	Affiche_Infos_PCB_Texte(this, TextePcb);

	StartMoveTextePcb(TextePcb, DC);

	return TextePcb;
}

	/*******************************/
	/* void Rotate_Texte_Pcb(COMMAND * Cmd) */
	/*******************************/

void WinEDA_PcbFrame::Rotate_Texte_Pcb(TEXTE_PCB * TextePcb, wxDC * DC)
{
int angle = 900;
int drawmode = GR_XOR;

	if( TextePcb == NULL ) return;

	/* effacement du texte : */
	Trace_1_texte_pcb(DrawPanel, DC, TextePcb,0, 0, GR_XOR) ;


	TextePcb->m_Orient += angle;
	if(TextePcb->m_Orient >= 3600) TextePcb->m_Orient -= 3600 ;
	if(TextePcb->m_Orient < 0) TextePcb->m_Orient += 3600 ;

	/* Redessin du Texte */
	Trace_1_texte_pcb(DrawPanel, DC, TextePcb, 0, 0, drawmode);
	Affiche_Infos_PCB_Texte(this, TextePcb);

	CurrentScreen->SetModify();
}

	/***************************************************************/
	/* void Trace_1_texte(char * ptr,int ox,int oy),int mode_color */
	/***************************************************************/

/*
 Trace de 1 texte :
	ptr = pointeur sur le debut de la description du texte
	ox, oy = Offsets de trace
	mode_color = GR_OR, GR_XOR..
*/

void Trace_1_texte_pcb(WinEDA_DrawPanel * panel, wxDC * DC,
						TEXTE_PCB * pt_texte,int ox,int oy, int DrawMode)
{
int zoom = panel->m_Parent->CurrentScreen->GetZoom();
int cX, cY;
int screen_epais, gcolor ;
int size_h , size_v , width ;
char *ptr;
int orient;

	if( pt_texte->GetText() == NULL ) return;

	/* lecture des caracteristiques du texte */
	size_h = pt_texte->m_Size.x; size_v = pt_texte->m_Size.y ;
	orient = pt_texte->m_Orient;
	width = pt_texte->m_Width;
	screen_epais = width / zoom;

	if( (DisplayOpt.DisplayDrawItems == FILAIRE) || ( screen_epais < L_MIN_DESSIN) )
		width = 0;

	/* calcul de la position du texte */
	cX = pt_texte->m_Pos.x - ox;
	cY = pt_texte->m_Pos.y - oy;

	if ( pt_texte->m_Miroir == 0 ) size_h = -size_h;

	/* choix de la couleur du texte : */
	gcolor = color_layer[pt_texte->m_Layer];

	ptr = pt_texte->GetText(); /* ptr pointe 1er caractere du texte */

	GRSetDrawMode(DC, DrawMode);
	/* trace ancre du texte */
	if((E_ancre_color&ITEM_NON_VISIBLE) == 0 )
		{
		GRLine(panel, DC, cX - (2*zoom), cY,
			cX + (2*zoom), cY, E_ancre_color);
		GRLine(panel, DC, cX, cY - (2*zoom) ,
			cX, cY + (2*zoom) , E_ancre_color);
		}

	/* trace du texte */
	Display_1_Texte(panel, DC, ptr, strlen(ptr), cX, cY,
					size_h, size_v, width, orient, gcolor);
}

	/*********************************************/
	/* void InstallEditCaractMenu(COMMAND * Cmd) */
	/*********************************************/

static void InstallEditCaractMenu(COMMAND * Cmd)
{
#if 0
TEXTE_PCB * TextePcb = PtTextePcbSelecte;
wxDC * DC = Cmd->DC;

	if( TextePcb == NULL )
		{
		TextePcb = (TEXTE_PCB*)pt_pcb->Drawings;
		while( (TextePcb = Locate_Texte_Pcb(TextePcb,CURSEUR_OFF_GRILLE)) != 0 )
			{
			if( TextePcb->Layer == CurrentScreen->Active_Layer ) break;
			// le Texte n'est pas sur la couche active
			TextePcb = (TEXTE_PCB*) TextePcb->Pnext;
			}
		}

	TextePcbEdite = TextePcb;

	if( TextePcb == NULL )
		{
		DisplayError("Texte non trouve sur cette couche",10);
		Cmd->Menu->State = WAIT; DisplayMenu(DC, Cmd->Menu, NULL, ON);
		return;
		}

	Affiche_Infos_PCB_Texte(Cmd, TextePcb);

	if(TextePcb->orient >= 3600) TextePcb->orient -= 3600 ;
	if(TextePcb->orient < 0) TextePcb->orient += 3600 ;

	/* Init des variables */
	strncpy(BufNameTxt.s, TextePcb->GetText(), sizeof(BufNameTxt.s) - 1);
	if( TextePcb->miroir ) Texte_Pcb_Miroir_Item.State = WAIT;
	else Texte_Pcb_Miroir_Item.State = RUN;

	* (int*) Param_OrientText.param = TextePcb->orient;
	* (int*) Param_DimHText.param = TextePcb->Size.x;
	* (int*) Param_DimVText.param = TextePcb->Size.y;
	* (int*) Param_EpaisText.param = TextePcb->width;

	InstallNewList(Cmd);
#endif
}

	/*****************************************/
	/* void ChangeCaractTexte(COMMAND * Cmd) */
	/*****************************************/

/* Change les caracteristiques du texte en cours d'edition
*/
void WinEDA_PcbFrame::EditTextePcb(TEXTE_PCB * TextePcb, wxDC * DC)
{
int drawmode = GR_OR;
char buf_new_texte[1024];

	if( TextePcb == NULL)  return;

	if( PtTextePcbSelecte ) drawmode = GR_XOR;

	buf_new_texte[0]= 0 ;
	Get_Message("Texte:",buf_new_texte, this);
	if(*buf_new_texte <=' ') return;

	/* effacement du texte : */
	Trace_1_texte_pcb(DrawPanel, DC, TextePcb, 0, 0, GR_XOR) ;

	if( strcmp( TextePcb->GetText(), buf_new_texte) && strlen(buf_new_texte) )
		{
		TextePcb->SetText(buf_new_texte);
		CurrentScreen->SetModify();
		}

	while(LocalOrient >= 3600) LocalOrient -= 3600 ;
	while(LocalOrient < 0) LocalOrient += 3600 ;

	if( CurrentScreen->m_Active_Layer != TextePcb->m_Layer )
		{
		TextePcb->m_Layer = CurrentScreen->m_Active_Layer;
		CurrentScreen->SetModify();
		}

	if( LocalOrient != TextePcb->m_Orient )
		{
		TextePcb->m_Orient = LocalOrient;
		CurrentScreen->SetModify();
		}

	if( LocalDimV != TextePcb->m_Size.y )
		{
		TextePcb->m_Size.y = LocalDimV;
		CurrentScreen->SetModify();
		}
 
	if( LocalDimH != TextePcb->m_Size.x )
		{
		TextePcb->m_Size.x = LocalDimH;
		CurrentScreen->SetModify();
		}

	if( LocalWidth != TextePcb->m_Width )
		{
		TextePcb->m_Width = LocalWidth;
		CurrentScreen->SetModify();
		}

/*	if( (Texte_Pcb_Miroir_Item.State == WAIT) && (TextePcb->miroir == 0) )
		{
		TextePcb->miroir = 1;
		CurrentScreen->SetModify();
		}

	if( (Texte_Pcb_Miroir_Item.State == RUN) && TextePcb->miroir )
		{
		TextePcb->miroir = 0;
		CurrentScreen->SetModify();
		}
*/

	/* Redessin du Texte */
	Trace_1_texte_pcb(DrawPanel, DC, TextePcb, 0, 0, drawmode);

	Affiche_Infos_PCB_Texte(this, TextePcb);
}

