/************************************/
/* fonctions de la classe TEXTE_PCB */
/************************************/

#include "fctsys.h"
#include "wxstruct.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"


	/*******************/
	/* class TEXTE_PCB */
	/*******************/

TEXTE_PCB::TEXTE_PCB(EDA_BaseStruct * parent):
		EDA_BaseStruct(parent, TYPETEXTE), EDA_TextStruct()
{
}

/* Destructeur */
TEXTE_PCB:: ~TEXTE_PCB(void)
{
}

/* copie de stucture */
void TEXTE_PCB::Copy(TEXTE_PCB * source)
{
	m_Parent = source->m_Parent;
	Pback = Pnext = NULL;
	m_Miroir = source->m_Miroir;
	m_Size = source->m_Size;
	m_Orient = source->m_Orient;
	m_Pos = source->m_Pos;
	m_Layer = source->m_Layer;
	m_Width = source->m_Width;
	m_Attributs = source->m_Attributs;
	m_CharType = source->m_CharType;
	m_HJustify = source->m_HJustify;
	m_VJustify = source->m_VJustify;

	m_Text = source->m_Text;
}

void TEXTE_PCB::UnLink( void )
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

/****************************************************************/
int TEXTE_PCB::ReadTextePcbDescr(FILE * File, int * LineNum)
/****************************************************************/
{
char text[1024], Line[1024];
int dummy;

	while( GetLine(File, Line, LineNum ) != NULL )
	{
		if(strnicmp(Line,"$EndTEXTPCB",11) == 0) return 0;
		if( strncmp(Line,"Te", 2) == 0 )	/* Texte */
		{
			ReadDelimitedText(text, Line+2, sizeof(text) );
			m_Text = CONV_FROM_UTF8(text);
			continue;
		}
		if( strncmp(Line,"Po", 2) == 0 )
		{
			sscanf( Line+2," %d %d %d %d %d %d",
				&m_Pos.x, &m_Pos.y, &m_Size.x, &m_Size.y,
				&m_Width, &m_Orient);
			continue;
		}
		if( strncmp(Line,"De", 2) == 0 )
		{
			sscanf( Line+2," %d %d %lX %d\n",&m_Layer, &m_Miroir,
							&m_TimeStamp, &dummy);
			if ( m_Layer < LAYER_CUIVRE_N )
				m_Layer = LAYER_CUIVRE_N;
			if ( m_Layer > LAST_NO_COPPER_LAYER )
				m_Layer = LAST_NO_COPPER_LAYER;

			continue;
		}
	}
	return(1);
}


/**************************************************/
int TEXTE_PCB::WriteTextePcbDescr(FILE * File)
/**************************************************/
{
	if( GetState(DELETED) ) return(0);

	if(m_Text.IsEmpty() ) return(0);
	fprintf( File,"$TEXTPCB\n");
	fprintf( File,"Te \"%s\"\n",CONV_TO_UTF8(m_Text));
	fprintf( File,"Po %d %d %d %d %d %d\n",
			m_Pos.x, m_Pos.y, m_Size.x, m_Size.y, m_Width, m_Orient );
	fprintf( File,"De %d %d %lX %d\n", m_Layer, m_Miroir, m_TimeStamp, 0);
	fprintf( File,"$EndTEXTPCB\n");
	return(1);
}

/**********************************************************************/
void TEXTE_PCB::Draw(WinEDA_DrawPanel * panel, wxDC * DC,
					const wxPoint & offset, int DrawMode)
/**********************************************************************/
/*
	DrawMode = GR_OR, GR_XOR.., -1 si mode courant.
*/
{
int color = g_DesignSettings.m_LayerColor[m_Layer];
	if(color & ITEM_NOT_SHOW ) return ;

	EDA_TextStruct::Draw(panel, DC, offset, color,
				DrawMode, DisplayOpt.DisplayDrawItems,
				(g_AnchorColor & ITEM_NOT_SHOW) ? -1 : (g_AnchorColor & MASKCOLOR));

}

