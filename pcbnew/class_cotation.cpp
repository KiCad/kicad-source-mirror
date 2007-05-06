/************************************/
/* fonctions de la classe COTATION */
/************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"



COTATION::COTATION(EDA_BaseStruct * StructFather):
		EDA_BaseStruct( StructFather, TYPECOTATION)
{
	m_Layer = DRAW_LAYER;
	m_Width = 50;
	m_Value = 0;
	m_Shape = 0;
	m_Unit = INCHES;
	m_Text = new TEXTE_PCB(this);
}

/* Effacement memoire de la structure */
COTATION::~COTATION(void)
{
	delete m_Text;
}

/* supprime du chainage la structure Struct
  les structures arrieres et avant sont chainees directement
 */
void COTATION::UnLink( void )
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
			((BOARD*)Pback)->m_Drawings = Pnext;
			}
		}

	/* Modification du chainage avant */
	if( Pnext) Pnext->Pback = Pback;

	Pnext = Pback = NULL;
}

/* Changement du texte de la cotation */
void COTATION:: SetText(const wxString & NewText)
{
	m_Text->m_Text = NewText;
}


/*************************************/
void COTATION::Copy(COTATION * source)
/*************************************/
{
	m_Value = source->m_Value;
	m_Layer = source->m_Layer;
	m_Width = source->m_Width;
	m_Pos = source->m_Pos;
	m_Shape = source->m_Shape;
	m_Unit = source->m_Unit;
	m_TimeStamp = GetTimeStamp();
	m_Text->Copy(source->m_Text);

	Barre_ox = source->Barre_ox; Barre_oy = source->Barre_oy;
	Barre_fx = source->Barre_fx; Barre_fy = source->Barre_fy;
	TraitG_ox = source->TraitG_ox; TraitG_oy = source->TraitG_oy;
	TraitG_fx = source->TraitG_fx; TraitG_fy = source->TraitG_fy;
	TraitD_ox = source->TraitD_ox; TraitD_oy = source->TraitD_oy;
	TraitD_fx = source->TraitD_fx; TraitD_fy = source->TraitD_fy;
	FlecheD1_ox = source->FlecheD1_ox; FlecheD1_oy = source->FlecheD1_oy;
	FlecheD1_fx = source->FlecheD1_fx; FlecheD1_fy = source->FlecheD1_fy;
	FlecheD2_ox = source->FlecheD2_ox; FlecheD2_oy = source->FlecheD2_oy;
	FlecheD2_fx = source->FlecheD2_fx; FlecheD2_fy = source->FlecheD2_fy;
	FlecheG1_ox = source->FlecheG1_ox; FlecheG1_oy = source->FlecheG1_oy;
	FlecheG1_fx = source->FlecheG1_fx; FlecheG1_fy = source->FlecheG1_fy;
	FlecheG2_ox = source->FlecheG2_ox; FlecheG2_oy = source->FlecheG2_oy;
	FlecheG2_fx = source->FlecheG2_fx; FlecheG2_fy = source->FlecheG2_fy;
}


/***************************************************************/
bool COTATION::ReadCotationDescr(FILE * File, int * LineNum)
/***************************************************************/
{
char  Line[2048], Text[2048];

	while(  GetLine(File, Line, LineNum ) != NULL )
		{
		if(strnicmp(Line,"$EndCOTATION",4) == 0) return TRUE;

		if(Line[0] == 'V')
		{
			sscanf( Line+2," %d",&m_Value);
			continue;
		}

		if(Line[0] == 'G')
		{
			sscanf( Line+2," %d %d %lX",&m_Shape, &m_Layer, &m_TimeStamp);

			/* Mise a jour des param .layer des sous structures */
			if ( m_Layer < FIRST_NO_COPPER_LAYER )
				m_Layer = FIRST_NO_COPPER_LAYER;
			if ( m_Layer > LAST_NO_COPPER_LAYER )
				m_Layer = LAST_NO_COPPER_LAYER;

			m_Text->m_Layer = m_Layer;
			continue;
		}

		if(Line[0] == 'T')
		{
			ReadDelimitedText(Text, Line+2, sizeof(Text) );
			m_Text->m_Text = CONV_FROM_UTF8(Text);
			continue;
		}

		if(Line[0] == 'P')
			{
			sscanf( Line+2," %d %d %d %d %d %d %d",
				&m_Text->m_Pos.x, &m_Text->m_Pos.y,
				&m_Text->m_Size.x, &m_Text->m_Size.y,
				&m_Text->m_Width,&m_Text->m_Orient,
				&m_Text->m_Miroir);

			m_Pos = m_Text->m_Pos;
			continue;
			}

		if( Line[0] == 'S')
			{
			switch( Line[1] )
				{
				int Dummy;
				case 'b' :
					sscanf( Line+2," %d %d %d %d %d %d",
							&Dummy,
							&Barre_ox, &Barre_oy,
							&Barre_fx, &Barre_fy,
							&m_Width );
					break;

				case 'd':
					sscanf( Line+2," %d %d %d %d %d %d",
							&Dummy,
							&TraitD_ox, &TraitD_oy,
							&TraitD_fx, &TraitD_fy,
							&Dummy );
					break;

				case 'g':
					sscanf( Line+2," %d %d %d %d %d %d",
							&Dummy,
							&TraitG_ox, &TraitG_oy,
							&TraitG_fx, &TraitG_fy,
							&Dummy );
					break;

				case '1':
					sscanf( Line+2," %d %d %d %d %d %d",
						&Dummy,
						&FlecheD1_ox, &FlecheD1_oy,
						&FlecheD1_fx, &FlecheD1_fy,
						&Dummy );
					break;

				case '2':
					sscanf( Line+2," %d %d %d %d %d %d",
						&Dummy,
						&FlecheD2_ox, &FlecheD2_oy,
						&FlecheD2_fx, &FlecheD2_fy,
						&Dummy );
					break;

				case '3':
					sscanf( Line+2," %d %d %d %d %d %d\n",
						&Dummy,
						&FlecheG1_ox, &FlecheG1_oy,
						&FlecheG1_fx, &FlecheG1_fy,
						&Dummy);
					break;

				case '4':
					sscanf( Line+2," %d %d %d %d %d %d",
						&Dummy,
						&FlecheG2_ox, &FlecheG2_oy,
						&FlecheG2_fx, &FlecheG2_fy,
						&Dummy );
					break;
				}

			continue;
			}
		}
	 return FALSE;
}


/**************************************************/
bool COTATION::WriteCotationDescr(FILE * File)
/**************************************************/
{
	if( GetState(DELETED) ) return FALSE;

	fprintf( File, "$COTATION\n");

	fprintf( File,"Ge %d %d %lX\n",m_Shape,
			m_Layer, m_TimeStamp);

	fprintf( File,"Va %d\n",m_Value);

	if( ! m_Text->m_Text.IsEmpty() )
		fprintf( File,"Te \"%s\"\n",CONV_TO_UTF8(m_Text->m_Text));
	else fprintf( File,"Te \"?\"\n");

	fprintf( File,"Po %d %d %d %d %d %d %d\n",
			m_Text->m_Pos.x, m_Text->m_Pos.y,
			m_Text->m_Size.x, m_Text->m_Size.y,
			m_Text->m_Width,m_Text->m_Orient,
			m_Text->m_Miroir);

	fprintf( File,"Sb %d %d %d %d %d %d\n",S_SEGMENT,
			Barre_ox, Barre_oy,
			Barre_fx, Barre_fy, m_Width );

	fprintf( File,"Sd %d %d %d %d %d %d\n",S_SEGMENT,
			TraitD_ox, TraitD_oy,
			TraitD_fx, TraitD_fy, m_Width );

	fprintf( File,"Sg %d %d %d %d %d %d\n",S_SEGMENT,
			TraitG_ox, TraitG_oy,
			TraitG_fx, TraitG_fy, m_Width );

	fprintf( File,"S1 %d %d %d %d %d %d\n",S_SEGMENT,
			FlecheD1_ox, FlecheD1_oy,
			FlecheD1_fx, FlecheD1_fy, m_Width );

	fprintf( File,"S2 %d %d %d %d %d %d\n",S_SEGMENT,
			FlecheD2_ox, FlecheD2_oy,
			FlecheD2_fx, FlecheD2_fy, m_Width );


	fprintf( File,"S3 %d %d %d %d %d %d\n",S_SEGMENT,
			FlecheG1_ox, FlecheG1_oy,
			FlecheG1_fx, FlecheG1_fy, m_Width );

	fprintf( File,"S4 %d %d %d %d %d %d\n",S_SEGMENT,
			FlecheG2_ox, FlecheG2_oy,
			FlecheG2_fx, FlecheG2_fy, m_Width );

	fprintf( File, "$EndCOTATION\n");

	return(1);
}




/************************************************************************/
void COTATION::Draw(WinEDA_DrawPanel * panel , wxDC * DC,
			const wxPoint & offset, int mode_color)
/************************************************************************/
/* impression de 1 cotation : serie de n segments + 1 texte
*/
{
int ox, oy, typeaff, width, gcolor;
int zoom = panel->GetScreen()->GetZoom();

	ox = offset.x;
	oy = offset.y;

	m_Text->Draw(panel, DC, offset, mode_color );

	gcolor = g_DesignSettings.m_LayerColor[m_Layer];
	if( (gcolor & ITEM_NOT_SHOW) != 0 ) return ;

	GRSetDrawMode(DC, mode_color);
	typeaff = DisplayOpt.DisplayDrawItems;
	width = m_Width;
	if( width/zoom < 2 ) typeaff = FILAIRE;

	switch( typeaff )
	{
		case FILAIRE:
			GRLine(&panel->m_ClipBox, DC,
					Barre_ox - ox, Barre_oy - oy,
					Barre_fx - ox, Barre_fy- oy, gcolor);
			GRLine(&panel->m_ClipBox, DC,
					TraitG_ox - ox, TraitG_oy - oy,
					TraitG_fx - ox, TraitG_fy- oy, gcolor);
			GRLine(&panel->m_ClipBox, DC,
					TraitD_ox - ox, TraitD_oy - oy,
					TraitD_fx - ox, TraitD_fy- oy, gcolor);
			GRLine(&panel->m_ClipBox, DC,
					FlecheD1_ox - ox, FlecheD1_oy - oy,
					FlecheD1_fx - ox, FlecheD1_fy- oy, gcolor);
			GRLine(&panel->m_ClipBox, DC,
					FlecheD2_ox - ox, FlecheD2_oy - oy,
					FlecheD2_fx - ox, FlecheD2_fy- oy, gcolor);
			GRLine(&panel->m_ClipBox, DC,
					FlecheG1_ox - ox, FlecheG1_oy - oy,
					FlecheG1_fx - ox, FlecheG1_fy- oy, gcolor);
			GRLine(&panel->m_ClipBox, DC,
					FlecheG2_ox - ox, FlecheG2_oy - oy,
					FlecheG2_fx - ox, FlecheG2_fy- oy, gcolor);
			break;

		case FILLED:
			GRFillCSegm(&panel->m_ClipBox, DC,
					Barre_ox - ox, Barre_oy - oy,
					Barre_fx - ox, Barre_fy- oy,
					width, gcolor);
			GRFillCSegm(&panel->m_ClipBox, DC,
					TraitG_ox - ox, TraitG_oy - oy,
					TraitG_fx - ox, TraitG_fy- oy,
					width, gcolor);
			GRFillCSegm(&panel->m_ClipBox, DC,
					TraitD_ox - ox, TraitD_oy - oy,
					TraitD_fx - ox, TraitD_fy- oy,
					width, gcolor);
			GRFillCSegm(&panel->m_ClipBox, DC,
					FlecheD1_ox - ox, FlecheD1_oy - oy,
					FlecheD1_fx - ox, FlecheD1_fy- oy,
					width, gcolor);
			GRFillCSegm(&panel->m_ClipBox, DC,
					FlecheD2_ox - ox, FlecheD2_oy - oy,
					FlecheD2_fx - ox, FlecheD2_fy- oy,
					width, gcolor);
			GRFillCSegm(&panel->m_ClipBox, DC,
					FlecheG1_ox - ox, FlecheG1_oy - oy,
					FlecheG1_fx - ox, FlecheG1_fy- oy,
					width, gcolor);
			GRFillCSegm(&panel->m_ClipBox, DC,
					FlecheG2_ox - ox, FlecheG2_oy - oy,
					FlecheG2_fx - ox, FlecheG2_fy- oy,
					width, gcolor);
			break;

		case SKETCH:
			GRCSegm(&panel->m_ClipBox, DC,
					Barre_ox - ox, Barre_oy - oy,
					Barre_fx - ox, Barre_fy- oy,
					width, gcolor);
			GRCSegm(&panel->m_ClipBox, DC,
					TraitG_ox - ox, TraitG_oy - oy,
					TraitG_fx - ox, TraitG_fy- oy,
					width, gcolor);
			GRCSegm(&panel->m_ClipBox, DC,
					TraitD_ox - ox, TraitD_oy - oy,
					TraitD_fx - ox, TraitD_fy- oy,
					width, gcolor);
			GRCSegm(&panel->m_ClipBox, DC,
					FlecheD1_ox - ox, FlecheD1_oy - oy,
					FlecheD1_fx - ox, FlecheD1_fy- oy,
					width, gcolor);
			GRCSegm(&panel->m_ClipBox, DC,
					FlecheD2_ox - ox, FlecheD2_oy - oy,
					FlecheD2_fx - ox, FlecheD2_fy- oy,
					width, gcolor);
			GRCSegm(&panel->m_ClipBox, DC,
					FlecheG1_ox - ox, FlecheG1_oy - oy,
					FlecheG1_fx - ox, FlecheG1_fy- oy,
					width, gcolor);
			GRCSegm(&panel->m_ClipBox, DC,
					FlecheG2_ox - ox, FlecheG2_oy - oy,
					FlecheG2_fx - ox, FlecheG2_fy - oy,
					width, gcolor);
			break;
	}
}


