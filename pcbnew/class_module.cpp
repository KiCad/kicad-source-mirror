	/****************************************************/
	/* class_module.cpp : fonctions de la classe MODULE */
	/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "wxstruct.h"
#include "common.h"
#include "plot_common.h"
#include "pcbnew.h"
#include "trigo.h"

#ifdef PCBNEW
#include "autorout.h"
#include "drag.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "3d_struct.h"
#include "protos.h"

#define MAX_WIDTH 10000		// Epaisseur (en 1/10000 ") max raisonnable des traits, textes...

/*********************************************************************************/
void MODULE::DrawAncre(WinEDA_DrawPanel * panel, wxDC * DC, const wxPoint & offset,
								int dim_ancre, int draw_mode)
/*********************************************************************************/
/* trace de l'ancre (croix verticale)
(doit etre fait apres les pads,
car le trace du trou efface tout donc peut etre l'ancre */
{
int zoom = panel->GetZoom();
int anchor_size = dim_ancre * zoom;

	GRSetDrawMode(DC, draw_mode);

	if((g_AnchorColor & ITEM_NOT_SHOW) == 0 )
		{
		GRLine(&panel->m_ClipBox, DC,
				m_Pos.x - offset.x - anchor_size, m_Pos.y - offset.y,
				m_Pos.x -offset.x + anchor_size,m_Pos.y - offset.y,
				g_AnchorColor);
		GRLine(&panel->m_ClipBox, DC,
				m_Pos.x - offset.x, m_Pos.y - offset.y - anchor_size ,
				m_Pos.x - offset.x, m_Pos.y - offset.y + anchor_size ,
				g_AnchorColor);
		}
}


	/*************************************************/
	/* Class MODULE : description d'un composant pcb */
	/*************************************************/

/* Constructeur de la classe MODULE */
MODULE::MODULE(BOARD * parent):	EDA_BaseStruct( parent, TYPEMODULE)
{
	m_Pads = NULL;
	m_Drawings = NULL;
	m_3D_Drawings = NULL;
	m_Attributs = MOD_DEFAULT;
	m_Layer = CMP_N;
	m_Orient = 0;
	m_ModuleStatus = 0;
	flag = 0;
	m_CntRot90 = m_CntRot180 = 0;
	m_Surface = 0;
	m_Link = 0;
	m_LastEdit_Time = time(NULL);
	m_Reference = new TEXTE_MODULE(this, TEXT_is_REFERENCE);
	m_Reference->Pback = this;
	m_Value = new TEXTE_MODULE(this, TEXT_is_VALUE);
	m_Value->Pback = this;
	m_3D_Drawings = new Struct3D_Master(this);
}

	/* Destructeur */
MODULE::~MODULE(void)
{
D_PAD * Pad;
EDA_BaseStruct * Struct, * NextStruct;

	delete m_Reference;
	delete m_Value;
	for ( Struct = m_3D_Drawings;  Struct != NULL;  Struct = NextStruct)
	{
		NextStruct = Struct->Pnext;
		delete Struct;
	}

	/* effacement des pads */
	for( Pad = m_Pads; Pad != NULL; Pad = (D_PAD*)NextStruct )
		{
		NextStruct = Pad->Pnext;
		delete Pad;
		}

	/* effacement des elements de trace */
	for ( Struct = m_Drawings; Struct != NULL; Struct = NextStruct )
		{
		NextStruct = Struct->Pnext;
		switch( (Struct->m_StructType) )
			{
			case TYPEEDGEMODULE:
				delete (EDGE_MODULE*)Struct;
				break;

			case TYPETEXTEMODULE:
				delete (TEXTE_MODULE*)Struct;
				break;

			default:
				DisplayError(NULL, wxT("Warn: ItemType not handled in delete MODULE"));
				NextStruct = NULL;
				break;
			}
		}
}

/*********************************/
void MODULE::Copy(MODULE * Module)
/*********************************/
{
D_PAD * pad,* lastpad;

	m_Pos = Module->m_Pos;
	m_Layer = Module->m_Layer;
	m_LibRef = Module->m_LibRef;
	m_Attributs = Module->m_Attributs;
	m_Orient = Module->m_Orient;
	m_BoundaryBox = Module->m_BoundaryBox;
	m_PadNum = Module->m_PadNum;
	m_CntRot90 = Module->m_CntRot90;
	m_CntRot180 = Module->m_CntRot180;
	m_LastEdit_Time = Module->m_LastEdit_Time;
	m_TimeStamp = GetTimeStamp();

	/* Copy des structures auxiliaires: Reference et value */
	m_Reference->Copy(Module->m_Reference);
	m_Value->Copy(Module->m_Value);

	/* Copie des structures auxiliaires: Pads */
	lastpad = NULL; pad = Module->m_Pads;
	for( ;pad != NULL ; pad = (D_PAD*)pad->Pnext)
		{
		D_PAD * newpad = new D_PAD (this);
		newpad->Copy(pad);

		if(m_Pads == NULL)
			{
			newpad->Pback = this;
			m_Pads = (D_PAD*)newpad;
			}
		else
			{
			newpad->Pback = lastpad;
			lastpad->Pnext = newpad;
			}
		lastpad = newpad;
		}

	/* Copy des structures auxiliaires: Drawings */
	EDA_BaseStruct* OldStruct = (EDA_BaseStruct*) Module->m_Drawings;
	EDA_BaseStruct* NewStruct, *LastStruct = NULL;
	for( ;OldStruct ; OldStruct = OldStruct->Pnext)
		{
		NewStruct = NULL;
		switch(OldStruct->m_StructType)
			{
			case TYPETEXTEMODULE:
				NewStruct = new TEXTE_MODULE( this );
				((TEXTE_MODULE*)NewStruct)->Copy((TEXTE_MODULE*)OldStruct);
				break;

			case TYPEEDGEMODULE:
				NewStruct = new EDGE_MODULE( this );
				((EDGE_MODULE*)NewStruct)->Copy((EDGE_MODULE*)OldStruct);
				break;
			default:
				DisplayError(NULL, wxT("Internal Err: CopyModule: type indefini"));
				break;
			}
		if( NewStruct == NULL) break;
		if(m_Drawings == NULL)
			{
			NewStruct->Pback = this;
			m_Drawings = NewStruct;
			}
		else
			{
			NewStruct->Pback = LastStruct;
			LastStruct->Pnext = NewStruct;
			}
		LastStruct = NewStruct;
		}

	/* Copy des elements complementaires Drawings 3D */
	m_3D_Drawings->Copy(Module->m_3D_Drawings);
Struct3D_Master * Struct3D, *NewStruct3D, *CurrStruct3D;
	Struct3D = (Struct3D_Master *) Module->m_3D_Drawings->Pnext;
	CurrStruct3D = m_3D_Drawings;
	for ( ; Struct3D != NULL; Struct3D = (Struct3D_Master*) Struct3D->Pnext)
	{
		NewStruct3D = new Struct3D_Master(this);
		NewStruct3D->Copy(Struct3D);
		CurrStruct3D->Pnext = NewStruct3D;
		NewStruct3D->Pback = CurrStruct3D;
		CurrStruct3D = NewStruct3D;
	}

	/* Copie des elements complementaires */
	m_Doc = Module->m_Doc;
	m_KeyWord = Module->m_KeyWord;

}

/* supprime du chainage la structure Struct
  les structures arrieres et avant sont chainees directement
 */
void MODULE::UnLink( void )
{
	/* Modification du chainage arriere */
	if( Pback )
		{
		if( Pback->m_StructType != TYPEPCB)
			{
			Pback->Pnext = Pnext;
			}

		else /* Le chainage arriere pointe sur la structure "Pere" */
			{
			if ( GetState(DELETED) )	// A REVOIR car Pback = NULL si place en undelete
				{
				if( g_UnDeleteStack ) g_UnDeleteStack[g_UnDeleteStackPtr-1] = Pnext;
				}
			else ((BOARD*)Pback)->m_Modules = (MODULE *) Pnext;
			}
		}

	/* Modification du chainage avant */
	if( Pnext) Pnext->Pback = Pback;

	Pnext = Pback = NULL;
}


/**********************************************************/
void MODULE::Draw(WinEDA_DrawPanel * panel, wxDC * DC,
					const wxPoint & offset, int draw_mode)
/**********************************************************/
/* Dessin d'une empreinte sur l'ecran actif:
	Entree :
		Module: pointeur sur le module
		ox, oy = offset de trace
		draw_mode = mode de trace ( GR_OR, GR_XOR, GR_AND)
	Utilise par ailleur:
		Description des parametres de l'empreinte calcules par caract() ;
*/
{
D_PAD * pt_pad ;
EDA_BaseStruct * PtStruct;
TEXTE_MODULE * PtTexte;

	/* trace des pastilles */
	pt_pad = m_Pads;
	for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
		{
		if ( pt_pad->m_Flags & IS_MOVED ) continue;
			pt_pad->Draw(panel, DC, offset,draw_mode);
		}

	/* Impression de l'ancre du module */
	DrawAncre(panel, DC, offset, DIM_ANCRE_MODULE, draw_mode) ;

	/* impression des graphismes */
	if ( ! (m_Reference->m_Flags & IS_MOVED) )
		m_Reference->Draw(panel, DC, offset, draw_mode);
	if ( ! (m_Value->m_Flags & IS_MOVED) )
		m_Value->Draw(panel, DC, offset, draw_mode);

	PtStruct = m_Drawings;
	for( ;PtStruct != NULL; PtStruct = PtStruct->Pnext )
		{
		if ( PtStruct->m_Flags & IS_MOVED ) continue;

		switch( PtStruct->m_StructType )
			{
			case TYPETEXTEMODULE:
				PtTexte =  (TEXTE_MODULE *) PtStruct;
				PtTexte->Draw(panel, DC, offset, draw_mode);
				break;

			case TYPEEDGEMODULE:
				((EDGE_MODULE *) PtStruct)->Draw(panel, DC, offset, draw_mode);
				break;

			default: break;
			}
		}
}


/**************************************************************/
void MODULE::DrawEdgesOnly(WinEDA_DrawPanel * panel, wxDC * DC,
					const wxPoint & offset, int draw_mode)
/**************************************************************/
{
EDA_BaseStruct * PtStruct;

	/* impression des graphismes */
	PtStruct = m_Drawings;
	for( ;PtStruct != NULL; PtStruct = PtStruct->Pnext )
		{
		switch( PtStruct->m_StructType )
			{
			case TYPEEDGEMODULE:
				((EDGE_MODULE *) PtStruct)->Draw(panel, DC, offset, draw_mode);
				break;

			default: break;
			}
		}
}


/*************************************/
int MODULE::WriteDescr( FILE * File )
/*************************************/
/* Sauvegarde de la description d'un MODULE
*/
{
char StringStat[20];
TEXTE_MODULE * PtText;
EDGE_MODULE * PtEdge;
D_PAD * ptpad;
EDA_BaseStruct * PtStruct;
int ii, NbLigne = 0;
wxString msg;

	if( GetState(DELETED) ) return(NbLigne);

	/* Generation du fichier module: */
	fprintf( File,"$MODULE %s\n", CONV_TO_UTF8(m_LibRef));
	NbLigne++;

	/* Generation des coord et caracteristiques */
	memset(StringStat, 0, sizeof(StringStat) );
	if( m_ModuleStatus & MODULE_is_LOCKED)
		 StringStat[0] = 'F';
	else  StringStat[0] = '~';
	if( m_ModuleStatus & MODULE_is_PLACED)
		 StringStat[1] = 'P';
	else  StringStat[1] = '~';

	fprintf( File,"Po %d %d %d %d %8.8lX %8.8lX %s\n",
			m_Pos.x, m_Pos.y,
			m_Orient, m_Layer,m_LastEdit_Time,
			m_TimeStamp, StringStat);
	NbLigne++;

	fprintf(File,"Li %s\n", CONV_TO_UTF8(m_LibRef));
	NbLigne++;

	if ( ! m_Doc.IsEmpty())
		{
		fprintf(File,"Cd %s\n", CONV_TO_UTF8(m_Doc));
		NbLigne++;
		}

	if ( ! m_KeyWord.IsEmpty())
		{
		fprintf(File,"Kw %s\n", CONV_TO_UTF8(m_KeyWord));
		NbLigne++;
		}

	fprintf(File,"Sc %8.8lX\n", m_TimeStamp);
	NbLigne++;

	fprintf(File,"Op %X %X 0\n", m_CntRot90, m_CntRot180);
	NbLigne++;

	/* Attributs du module */
	if( m_Attributs != MOD_DEFAULT )
		{
		fprintf(File,"At ");
		if( m_Attributs & MOD_CMS ) fprintf(File,"SMD ");
		if( m_Attributs & MOD_VIRTUAL ) fprintf(File,"VIRTUAL ");
		fprintf(File,"\n");
		}

	/* Texte Reference du module */
	fprintf(File,"T%d %d %d %d %d %d %d %c %c %d \"%.16s\"\n",
				m_Reference->m_Type,
				m_Reference->m_Pos0.x, m_Reference->m_Pos0.y,
				m_Reference->m_Size.y,m_Reference->m_Size.x,
				m_Reference->m_Orient + m_Orient, m_Reference->m_Width,
				m_Reference->m_Miroir ? 'N' : 'M', m_Reference->m_NoShow ? 'I' : 'V',
				m_Reference->m_Layer,
				CONV_TO_UTF8(m_Reference->m_Text) );
	NbLigne++;

	/* Texte Value du module */
	fprintf(File,"T%d %d %d %d %d %d %d %c %c %d \"%.16s\"\n",
				m_Value->m_Type,
				m_Value->m_Pos0.x, m_Value->m_Pos0.y,
				m_Value->m_Size.y,m_Value->m_Size.x,
				m_Value->m_Orient + m_Orient, m_Value->m_Width,
				m_Value->m_Miroir ? 'N' : 'M', m_Value->m_NoShow ? 'I' : 'V',
				m_Value->m_Layer,
				CONV_TO_UTF8(m_Value->m_Text) );
	NbLigne++;

	/* Generation des elements Drawing modules */
	PtStruct = m_Drawings;
	for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext)
		{
		switch(PtStruct->m_StructType )
			{
			case TYPETEXTEMODULE:
				PtText = ((TEXTE_MODULE *) PtStruct);
				fprintf(File,"T%d %d %d %d %d %d %d %c %c %d \"%s\"\n",
						PtText->m_Type,
						PtText->m_Pos0.x, PtText->m_Pos0.y,
						PtText->m_Size.y,PtText->m_Size.x,
						PtText->m_Orient + m_Orient, PtText->m_Width,
						PtText->m_Miroir ? 'N' : 'M',
						PtText->m_NoShow ? 'I' : 'V',
						PtText->m_Layer, CONV_TO_UTF8(PtText->m_Text) );
				NbLigne++;
				break;

			case TYPEEDGEMODULE:
				PtEdge = (EDGE_MODULE*) PtStruct;
				PtEdge->WriteDescr( File );
				break;

			default:
				msg.Printf( wxT("Type (%d) Draw Module inconnu"),
										PtStruct->m_StructType);
				DisplayError(NULL, msg);
				break;
			}	/* Fin switch gestion des Items draw */
		}

	/* Generation de la liste des pads */
	ptpad = m_Pads;
	for( ; ptpad != NULL; ptpad = (D_PAD*)ptpad->Pnext)
		{
		ii = ptpad->WriteDescr(File);
		NbLigne += ii;
		}

	/* Generation des informations de tracé 3D */
	Write_3D_Descr( File );

	/* Fin de description: */
	fprintf( File,"$EndMODULE  %s\n", CONV_TO_UTF8(m_LibRef));
	NbLigne++;
	return(NbLigne);
}

/***************************************/
int MODULE::Write_3D_Descr( FILE * File )
/***************************************/
/* Sauvegarde de la description 3D du MODULE
*/
{
char buf[512];
Struct3D_Master * Struct3D = m_3D_Drawings;

	for ( ; Struct3D != NULL; Struct3D = (Struct3D_Master *) Struct3D->Pnext)
	{
		if ( ! Struct3D->m_Shape3DName.IsEmpty() )
		{
		fprintf( File,"$SHAPE3D\n");

		fprintf( File,"Na \"%s\"\n", CONV_TO_UTF8(Struct3D->m_Shape3DName));

		sprintf( buf,"Sc %lf %lf %lf\n",
				Struct3D->m_MatScale.x,
				Struct3D->m_MatScale.y,
				Struct3D->m_MatScale.z);
		fprintf( File, to_point(buf) );

		sprintf( buf,"Of %lf %lf %lf\n",
				Struct3D->m_MatPosition.x,
				Struct3D->m_MatPosition.y,
				Struct3D->m_MatPosition.z);
		fprintf( File, to_point(buf) );

		sprintf( buf,"Ro %lf %lf %lf\n",
				Struct3D->m_MatRotation.x,
				Struct3D->m_MatRotation.y,
				Struct3D->m_MatRotation.z);
		fprintf( File, to_point(buf) );

		fprintf( File,"$EndSHAPE3D\n");
		}
	}

	return 0;
}

/****************************************************/
int MODULE::Read_3D_Descr( FILE * File, int * LineNum)
/****************************************************/
/* Lecture de la description d'un MODULE (format Ascii)
	la 1ere ligne de descr ($MODULE) est supposee etre deja lue
	retourne 0 si OK
*/
{
char Line[1024];
char * text = Line + 3;
Struct3D_Master * Struct3D = m_3D_Drawings;

	if ( ! Struct3D->m_Shape3DName.IsEmpty() )
	{
	Struct3D_Master * NewStruct3D;
		while ( Struct3D->Pnext )
			Struct3D = (Struct3D_Master *)Struct3D->Pnext;
		Struct3D->Pnext = NewStruct3D = new Struct3D_Master(this);
		NewStruct3D->Pback = Struct3D;
		Struct3D = NewStruct3D;
	}

	while( GetLine(File, Line, LineNum , sizeof(Line) -1) != NULL )
		{
		switch( Line[0] )
		{
			case '$':	// Fin de description
				if( Line[1] == 'E' ) return 0;
				return 1;

			case 'N':	// Shape File Name
				{
				char buf[512];
				ReadDelimitedText( buf, text, 512);
				Struct3D->m_Shape3DName = CONV_FROM_UTF8(buf);
				break;
				}

			case 'S':	// Scale
				sscanf( from_point(text),"%lf %lf %lf\n",
					&Struct3D->m_MatScale.x,
					&Struct3D->m_MatScale.y,
					&Struct3D->m_MatScale.z);
			break;

			case 'O':	// Offset
				sscanf( from_point(text),"%lf %lf %lf\n",
					&Struct3D->m_MatPosition.x,
					&Struct3D->m_MatPosition.y,
					&Struct3D->m_MatPosition.z);
				break;

			case 'R':	// Rotation
				sscanf( from_point(text),"%lf %lf %lf\n",
					&Struct3D->m_MatRotation.x,
					&Struct3D->m_MatRotation.y,
					&Struct3D->m_MatRotation.z);
				break;

			default:
				break;
		}
	}
	return 1;
}

/**************************************************/
int MODULE::ReadDescr( FILE * File, int * LineNum)
/**************************************************/
/* Lecture de la description d'un MODULE (format Ascii)
	la 1ere ligne de descr ($MODULE) est supposee etre deja lue
	retourne 0 si OK
*/
{
D_PAD * LastPad = NULL, * ptpad;
EDA_BaseStruct * LastModStruct = NULL;
EDGE_MODULE * DrawSegm;
TEXTE_MODULE * DrawText;
char Line[256], BufLine[256], BufCar1[128], BufCar2[128], *PtLine;
int itmp1, itmp2;

	while( GetLine(File, Line, LineNum , sizeof(Line) -1) != NULL )
		{
		if( Line[0] == '$' )
			{
			if( Line[1] == 'E' ) break;
			if( Line[1] == 'P' )
				{
				ptpad = new D_PAD(this);
				ptpad->ReadDescr(File, LineNum );
				RotatePoint( &ptpad->m_Pos.x, &ptpad->m_Pos.y, m_Orient );
				ptpad->m_Pos.x += m_Pos.x;
				ptpad->m_Pos.y += m_Pos.y;

				if(LastPad == NULL )
					{
					ptpad->Pback = (EDA_BaseStruct *) this;
					m_Pads = ptpad;
					}
				else
					{
					ptpad->Pback = (EDA_BaseStruct *) LastPad;
					LastPad->Pnext = (EDA_BaseStruct *)ptpad;
					}
				LastPad = ptpad;
				continue;
				}
			if( Line[1] == 'S' ) Read_3D_Descr(File, LineNum );
			}


		if ( strlen(Line) < 4 ) continue;
		PtLine = Line + 3;  /* Pointe 1er code utile de la ligne */
		switch( Line[0] )
			{
			case 'P':
			memset(BufCar1, 0, sizeof(BufCar1) );
				sscanf( PtLine,"%d %d %d %d %lX %lX %s",
						&m_Pos.x, &m_Pos.y,
						&m_Orient, &m_Layer ,
						&m_LastEdit_Time, &m_TimeStamp,BufCar1);

				m_ModuleStatus = 0;
				if(BufCar1[0] == 'F') m_ModuleStatus |= MODULE_is_LOCKED;
				if(BufCar1[1] == 'P') m_ModuleStatus |= MODULE_is_PLACED;
				break;

			case 'L':	/* Li = Lecture du nom librairie du module */
				*BufLine = 0;
				sscanf(PtLine," %s",BufLine);
				m_LibRef = CONV_FROM_UTF8(BufLine);
				break;

			case 'S':
				sscanf(PtLine," %lX", &m_TimeStamp);
				break;

			case 'O':	/* (Op)tions de placement auto */
				itmp1 = itmp2 = 0;
				sscanf(PtLine," %X %X", &itmp1, &itmp2);

				m_CntRot180 = itmp2 & 0x0F;
				if( m_CntRot180 > 10 ) m_CntRot180 = 10;

				m_CntRot90 = itmp1 & 0x0F;
				if( m_CntRot90 > 10 ) m_CntRot90 = 0;
				itmp1 = (itmp1 >> 4) & 0x0F;
				if( itmp1 > 10 ) itmp1 = 0;
				m_CntRot90 |= itmp1 << 4;
				break;

			case 'A':	/* At = (At)tributs du module */
				if ( strstr(PtLine, "SMD") ) m_Attributs |= MOD_CMS;
				if ( strstr(PtLine, "VIRTUAL") ) m_Attributs |= MOD_VIRTUAL;
				break;

			case 'T': /* lecture des textes modules */
				sscanf(Line+1,"%d", &itmp1);
				if ( itmp1 == TEXT_is_REFERENCE )
					DrawText = m_Reference;
				else if ( itmp1 == TEXT_is_VALUE )
					DrawText = m_Value;
				else	/* text is a drawing */
					{DrawText = new TEXTE_MODULE(this);
					if(LastModStruct == NULL )
						{
						DrawText->Pback = this;
						m_Drawings = (EDA_BaseStruct*) DrawText;
						}
					else
						{
						DrawText->Pback = LastModStruct;
						LastModStruct->Pnext = DrawText;
						}
					LastModStruct = (EDA_BaseStruct*) DrawText;
					}

				sscanf(Line+1,"%d %d %d %d %d %d %d %s %s %d",
					&itmp1,
					&DrawText->m_Pos0.x, &DrawText->m_Pos0.y,
					&DrawText->m_Size.y, &DrawText->m_Size.x,
					&DrawText->m_Orient, &DrawText->m_Width,
					BufCar1, BufCar2, &DrawText->m_Layer);

				DrawText->m_Type = itmp1;
				DrawText->m_Orient -= m_Orient;	// m_Orient texte relative au module
				if( BufCar1[0] == 'M') DrawText->m_Miroir = 0;
				else DrawText->m_Miroir = 1;
				if( BufCar2[0]  == 'I') DrawText->m_NoShow = 1;
				else  DrawText->m_NoShow = 0;

				if(m_Layer == CUIVRE_N) DrawText->m_Layer = SILKSCREEN_N_CU;
				if(m_Layer == CMP_N) DrawText->m_Layer = SILKSCREEN_N_CMP;

				/* calcul de la position vraie */
				DrawText->SetDrawCoord();
				/* Lecture de la chaine "text" */
				ReadDelimitedText(BufLine, Line	, sizeof(BufLine) );
				DrawText->m_Text = CONV_FROM_UTF8(BufLine);
				// Controle d'epaisseur raisonnable:
				if( DrawText->m_Width <= 1 ) DrawText->m_Width = 1;
				if( DrawText->m_Width > MAX_WIDTH ) DrawText->m_Width = MAX_WIDTH;
				break;

			case 'D': /* lecture du contour */
				DrawSegm = new EDGE_MODULE(this);

				if(LastModStruct == NULL )
					{
					DrawSegm->Pback = this;
					m_Drawings = DrawSegm;
					}
				else
					{
					DrawSegm->Pback = LastModStruct;
					LastModStruct->Pnext = DrawSegm;
					}

				LastModStruct = DrawSegm;
				DrawSegm->ReadDescr( Line, File, LineNum);
				DrawSegm->SetDrawCoord();
				break;

			case 'C': /* Lecture de la doc */
				m_Doc = CONV_FROM_UTF8(StrPurge(PtLine));
				break;

			case 'K': /* Lecture de la liste des mots cle */
				m_KeyWord = CONV_FROM_UTF8(StrPurge(PtLine));
				break;

			default:
				break;
			}
		}
	/* Recalcul de l'encadrement */
	Set_Rectangle_Encadrement();
	return(0);
}

/****************************************************/
void MODULE::SetPosition(const wxPoint & newpos)
/****************************************************/
// replace le module en position newpos
{
int	deltaX = newpos.x - m_Pos.x;
int	deltaY = newpos.y - m_Pos.y;

	/* deplacement de l'ancre */
	m_Pos.x += deltaX ; m_Pos.y += deltaY ;

	/* deplacement de la reference */
	m_Reference->m_Pos.x += deltaX ; m_Reference->m_Pos.y += deltaY;

	/* deplacement de la Valeur */
	m_Value->m_Pos.x += deltaX ; m_Value->m_Pos.y += deltaY;

	/* deplacement des pastilles */
	D_PAD * pad = m_Pads;
	for ( ; pad != NULL; pad = (D_PAD*) pad->Pnext )
		{
		pad->m_Pos.x += deltaX ; pad->m_Pos.y += deltaY ;
		}

	/* deplacement des dessins de l'empreinte : */
	EDA_BaseStruct * PtStruct = m_Drawings;
	for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext)
		{
		switch( PtStruct->m_StructType)
			{
			case TYPEEDGEMODULE:
				{
				EDGE_MODULE * pt_edgmod = (EDGE_MODULE*) PtStruct;
				pt_edgmod->SetDrawCoord();
				break;
				}

			case TYPETEXTEMODULE:
				{
				TEXTE_MODULE * pt_texte = (TEXTE_MODULE*) PtStruct;
				pt_texte->m_Pos.x += deltaX ; pt_texte->m_Pos.y += deltaY;
				break;
				}

			default: DisplayError(NULL, wxT("Type Draw Indefini")); break;
			}
		}

	Set_Rectangle_Encadrement();
}



/*********************************************/
void MODULE::SetOrientation(int newangle)
/*********************************************/
/* Tourne de newangle (en 0.1 degres) le module
*/
{
int px ,py;

	newangle -= m_Orient;		// = delta de rotation

	m_Orient += newangle;
	NORMALIZE_ANGLE_POS(m_Orient);

	/* deplacement et rotation des pastilles */
	D_PAD * pad = m_Pads;
	for (; pad != NULL; pad = (D_PAD*) pad->Pnext )
		{
		px = pad->m_Pos0.x;
		py = pad->m_Pos0.y;

		pad->m_Orient += newangle ;	/* change m_Orientation */
		NORMALIZE_ANGLE_POS(pad->m_Orient);

		RotatePoint( &px, &py, (int) m_Orient );
		pad->m_Pos.x = m_Pos.x + px;
		pad->m_Pos.y = m_Pos.y + py;
		}

	/* mise a jour de la reference et de la valeur*/
	m_Reference->SetDrawCoord();
	m_Value->SetDrawCoord();

	/* deplacement des contours et textes de l'empreinte : */
	EDA_BaseStruct * PtStruct = m_Drawings;
	for(; PtStruct != NULL; PtStruct = PtStruct->Pnext )
		{
		if( PtStruct->m_StructType == TYPEEDGEMODULE )
			{
			EDGE_MODULE* pt_edgmod = (EDGE_MODULE*) PtStruct ;
			pt_edgmod->SetDrawCoord();
			}
		if( PtStruct->m_StructType == TYPETEXTEMODULE )
			{
			/* deplacement des inscriptions : */
			TEXTE_MODULE * pt_texte = (TEXTE_MODULE*) PtStruct;
			pt_texte->SetDrawCoord();
			}
		}

	/* Recalcul du rectangle d'encadrement */
	Set_Rectangle_Encadrement();
}

/************************************************/
void  MODULE::Set_Rectangle_Encadrement(void)
/************************************************/
/* Mise a jour du rectangle d'encadrement du module
	Entree : pointeur sur module
	Le rectangle d'encadrement est le rectangle comprenant les contours et les
	pads.
	Le rectangle est calcule:
		pour orient 0
		en coord relatives / position ancre
*/
{
EDGE_MODULE* pt_edge_mod;
D_PAD * pad;
int width;
int cx, cy, uxf, uyf, rayon;
int xmax, ymax;


	/* Init des pointeurs */
	pt_edge_mod = (EDGE_MODULE*) m_Drawings;

	/* Init des coord du cadre a une valeur limite non nulle */
	m_BoundaryBox.m_Pos.x = -500; xmax = 500;
	m_BoundaryBox.m_Pos.y = -500; ymax = 500;

	/* Contours: Recherche des coord min et max et mise a jour du cadre */
	for( ;pt_edge_mod != NULL; pt_edge_mod = (EDGE_MODULE*)pt_edge_mod->Pnext )
		{
		if( pt_edge_mod->m_StructType != TYPEEDGEMODULE) continue;
		width = pt_edge_mod->m_Width / 2;
		switch(pt_edge_mod->m_Shape)
			{
			case S_ARC:
			case S_CIRCLE:
				{
				cx = pt_edge_mod->m_Start0.x; cy = pt_edge_mod->m_Start0.y ; // centre
				uxf = pt_edge_mod->m_End0.x; uyf = pt_edge_mod->m_End0.y ;
				rayon = (int)hypot((double)(cx-uxf),(double)(cy - uyf) );
				rayon += width;
				m_BoundaryBox.m_Pos.x = min(m_BoundaryBox.m_Pos.x,cx - rayon);
				m_BoundaryBox.m_Pos.y = min(m_BoundaryBox.m_Pos.y,cy - rayon);
				xmax = max(xmax, cx + rayon);
				ymax = max(ymax, cy + rayon);
				break;
				}

			default:
				m_BoundaryBox.m_Pos.x = min(m_BoundaryBox.m_Pos.x,pt_edge_mod->m_Start0.x - width);
				m_BoundaryBox.m_Pos.x = min(m_BoundaryBox.m_Pos.x,pt_edge_mod->m_End0.x - width);
				m_BoundaryBox.m_Pos.y = min(m_BoundaryBox.m_Pos.y,pt_edge_mod->m_Start0.y - width);
				m_BoundaryBox.m_Pos.y = min(m_BoundaryBox.m_Pos.y,pt_edge_mod->m_End0.y - width);
				xmax = max(xmax, pt_edge_mod->m_Start0.x + width);
				xmax = max(xmax, pt_edge_mod->m_End0.x + width);
				ymax = max(ymax, pt_edge_mod->m_Start0.y + width);
				ymax = max(ymax, pt_edge_mod->m_End0.y + width);
				break;
			}
		}

	/* Pads:  Recherche des coord min et max et mise a jour du cadre */
	for( pad = m_Pads; pad != NULL; pad = (D_PAD*)pad->Pnext )
		{
		rayon = pad->m_Rayon;
		cx = pad->m_Pos0.x; cy = pad->m_Pos0.y;
		m_BoundaryBox.m_Pos.x = min(m_BoundaryBox.m_Pos.x,cx - rayon);
		m_BoundaryBox.m_Pos.y = min(m_BoundaryBox.m_Pos.y,cy - rayon);
		xmax = max(xmax, cx + rayon);
		ymax = max(ymax, cy + rayon);
		}

	m_BoundaryBox.SetWidth(xmax - m_BoundaryBox.m_Pos.x);
	m_BoundaryBox.SetHeight(ymax - m_BoundaryBox.m_Pos.y);
}




/****************************************/
void MODULE::SetRectangleExinscrit(void)
/****************************************/

/*	Analogue a MODULE::Set_Rectangle_Encadrement() mais en coord reelles:
	Mise a jour du rectangle d'encadrement reel du module c.a.d en coord PCB
	Entree : pointeur sur module
	Le rectangle d'encadrement est le rectangle comprenant les contours et les
	pads.
	Met egalement a jour la surface (.m_Surface) du module.
*/
{
EDGE_MODULE* EdgeMod;
D_PAD * Pad;
int width;
int cx, cy, uxf, uyf, rayon;
int xmax, ymax;

	m_RealBoundaryBox.m_Pos.x = xmax = m_Pos.x;
	m_RealBoundaryBox.m_Pos.y = ymax = m_Pos.y;

	/* Contours: Recherche des coord min et max et mise a jour du cadre */
	EdgeMod = (EDGE_MODULE*)m_Drawings;
	for( ;EdgeMod != NULL; EdgeMod = (EDGE_MODULE*)EdgeMod->Pnext )
		{
		if( EdgeMod->m_StructType != TYPEEDGEMODULE) continue;
		width = EdgeMod->m_Width / 2;
		switch(EdgeMod->m_Shape)
			{
			case S_ARC:
			case S_CIRCLE:
				{
				cx = EdgeMod->m_Start.x; cy = EdgeMod->m_Start.y ; // centre
				uxf = EdgeMod->m_End.x; uyf = EdgeMod->m_End.y ;
				rayon = (int)hypot((double)(cx-uxf),(double)(cy - uyf) );
				rayon += width;
				m_RealBoundaryBox.m_Pos.x = min(m_RealBoundaryBox.m_Pos.x,cx - rayon);
				m_RealBoundaryBox.m_Pos.y = min(m_RealBoundaryBox.m_Pos.y,cy - rayon);
				xmax = max(xmax,cx + rayon);
				ymax = max(ymax,cy + rayon);
				break;
				}

			default:
				m_RealBoundaryBox.m_Pos.x = min(m_RealBoundaryBox.m_Pos.x,EdgeMod->m_Start.x - width);
				m_RealBoundaryBox.m_Pos.x = min(m_RealBoundaryBox.m_Pos.x,EdgeMod->m_End.x - width);
				m_RealBoundaryBox.m_Pos.y = min(m_RealBoundaryBox.m_Pos.y,EdgeMod->m_Start.y - width);
				m_RealBoundaryBox.m_Pos.y = min(m_RealBoundaryBox.m_Pos.y,EdgeMod->m_End.y - width);
				xmax = max(xmax,EdgeMod->m_Start.x + width);
				xmax = max(xmax,EdgeMod->m_End.x + width);
				ymax = max(ymax,EdgeMod->m_Start.y + width);
				ymax = max(ymax,EdgeMod->m_End.y + width);
				break;
			}
		}

	/* Pads:  Recherche des coord min et max et mise a jour du cadre */
	for( Pad = m_Pads; Pad != NULL; Pad = (D_PAD*)Pad->Pnext )
		{
		rayon = Pad->m_Rayon;
		cx = Pad->m_Pos.x; cy = Pad->m_Pos.y;
		m_RealBoundaryBox.m_Pos.x = min(m_RealBoundaryBox.m_Pos.x,cx - rayon);
		m_RealBoundaryBox.m_Pos.y = min(m_RealBoundaryBox.m_Pos.y,cy - rayon);
		xmax = max(xmax,cx + rayon);
		ymax = max(ymax,cy + rayon);
		}
	m_RealBoundaryBox.SetWidth(xmax - m_RealBoundaryBox.m_Pos.x);
	m_RealBoundaryBox.SetHeight(ymax - m_RealBoundaryBox.m_Pos.y);
	m_Surface = ABS((float) m_RealBoundaryBox.GetWidth() * m_RealBoundaryBox.GetHeight());
}


/*******************************************************/
void MODULE::Display_Infos(WinEDA_BasePcbFrame * frame)
/*******************************************************/
{
int nbpad;
char bufcar[512], Line[512];
int pos;
bool flag = FALSE;
wxString msg;
	
	frame->MsgPanel->EraseMsgBox() ;	/* Effacement de la zone message */
	if ( frame->m_Ident != PCB_FRAME ) flag = TRUE;
	pos = 1;
	Affiche_1_Parametre(frame, pos, m_Reference->m_Text, m_Value->m_Text, DARKCYAN);

	/* Affiche signature temporelle ou date de modif (en edition de modules) */
	pos += 14;
	if ( flag )	// Affichage date de modification (utile en Module Editor)
	{
		strcpy(Line, ctime(&m_LastEdit_Time));
		strtok(Line," \n\r");
		strcpy( bufcar, strtok(NULL," \n\r") ); strcat(bufcar," ");
		strcat( bufcar, strtok(NULL," \n\r") ); strcat(bufcar,", ");
		strtok(NULL," \n\r");
		strcat( bufcar, strtok(NULL," \n\r") );
		msg = CONV_FROM_UTF8(bufcar);
		Affiche_1_Parametre(frame, pos, _("Last Change"), msg, BROWN);
		pos += 4;
	}
	else
	{
		msg.Printf( wxT("%8.8lX"), m_TimeStamp);
		Affiche_1_Parametre(frame, pos, _("TimeStamp"), msg,BROWN);
	}

	pos += 6;
	Affiche_1_Parametre(frame, pos,_("Layer"),ReturnPcbLayerName(m_Layer),RED) ;

	pos += 6;
	EDA_BaseStruct * PtStruct = m_Pads;
	nbpad = 0;
	while( PtStruct ) { nbpad ++; PtStruct = PtStruct->Pnext; }
	msg.Printf( wxT("%d"),nbpad);
	Affiche_1_Parametre(frame, pos,_("Pads"), msg,BLUE);

	pos += 4;
	msg = wxT("..");
	if( m_ModuleStatus & MODULE_is_LOCKED ) msg[0] = 'F';
	if( m_ModuleStatus & MODULE_is_PLACED ) msg[1] = 'P';
	Affiche_1_Parametre(frame, pos,_("Stat"), msg, MAGENTA);

	pos += 4;
	msg.Printf( wxT("%.1f"),(float)m_Orient / 10 );
	Affiche_1_Parametre(frame, pos,_("Orient"), msg, BROWN);

	pos += 5;
	Affiche_1_Parametre(frame, pos,_("Module"), m_LibRef, BLUE);

	pos += 9;
	Affiche_1_Parametre(frame, pos, _("3D-Shape"),
			m_3D_Drawings->m_Shape3DName, RED);

	pos += 14;
	wxString doc = _("Doc:  ") +  m_Doc;
	wxString keyword = _("KeyW: ") + m_KeyWord;
	Affiche_1_Parametre(frame, pos, doc, keyword, BLACK);

}

