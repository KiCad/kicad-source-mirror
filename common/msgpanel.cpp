	/******************************************************************/
	/* msgpanel.cpp - fonctions des classes du type WinEDA_MsgPanel */
	/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"

#include "wxstruct.h"
#include "gr_basic.h"
#include "macros.h"
#include "common.h"

/* table des evenements captes par un WinEDA_MsgPanel */
BEGIN_EVENT_TABLE(WinEDA_MsgPanel, wxPanel)
	EVT_PAINT(WinEDA_MsgPanel::OnPaint)
END_EVENT_TABLE()



	/***********************************************************/
	/* Fonctions de base de WinEDA_MsgPanel: l'ecran de messages */
	/***********************************************************/

WinEDA_MsgPanel::WinEDA_MsgPanel(WinEDA_DrawFrame *parent, int id,
						const wxPoint& pos, const wxSize& size):
		wxPanel(parent, id, pos, size )
{
	m_Parent = parent;
	SetFont( *g_MsgFont );
}



WinEDA_MsgPanel::~WinEDA_MsgPanel(void)
{
}



/*************************************************/
void WinEDA_MsgPanel::OnPaint(wxPaintEvent & event)
/*************************************************/
{
wxPaintDC dc(this);

	EraseMsgBox(&dc); event.Skip();
}




/*****************************************************************************/
void WinEDA_MsgPanel::Affiche_1_Parametre(int pos_X,const wxString & texte_H,
			const wxString & texte_L,int color)
/*****************************************************************************/
/*
 Routine d'affichage d'un parametre.
	pos_X = cadrage horizontal
		si pos_X < 0 : la position horizontale est la derniere
			valeur demandee >= 0
	texte_H = texte a afficher en ligne superieure.
		si "", par d'affichage sur cette ligne
	texte_L = texte a afficher en ligne inferieure.
		si "", par d'affichage sur cette ligne
	color = couleur d'affichage
*/
{
static int old_pos_X;
wxPoint pos;
wxSize FontSizeInPixels, DrawSize;
wxClientDC dc(this);

	DrawSize = GetClientSize();

	dc.SetBackground(* wxBLACK_BRUSH );
	dc.SetBackgroundMode(wxSOLID);
//	dc.SetBackgroundMode(wxTRANSPARENT);
	dc.SetTextBackground(GetBackgroundColour());
	dc.SetFont(*g_MsgFont);
	dc.GetTextExtent( wxT("W"), &FontSizeInPixels.x, &FontSizeInPixels.y );

	if ( color >= 0 )
	{
		color &= MASKCOLOR;
		dc.SetTextForeground(wxColour(
			ColorRefs[color].m_Red,	ColorRefs[color].m_Green,
			ColorRefs[color].m_Blue) );
	}

	if ( pos_X >= 0 )
	{
		old_pos_X = pos.x = pos_X * (FontSizeInPixels.x + 2);
	}
	else pos.x = old_pos_X;


	if( !texte_H.IsEmpty())
	{
		pos.y = (DrawSize.y/2) - FontSizeInPixels.y;;
		dc.DrawText(texte_H.GetData(), pos.x, pos.y);
	}
	if( !texte_L.IsEmpty())
	{
		pos.y = DrawSize.y - FontSizeInPixels.y;
		dc.DrawText(texte_L.GetData(), pos.x, pos.y);
	}
}

/****************************************/
void WinEDA_MsgPanel::EraseMsgBox(void)
/****************************************/
/* Effacement de la fenetre d'affichage des messages de bas d'ecran
*/
{
wxClientDC dc(this);
	EraseMsgBox(&dc);
}

/*******************************************/
void WinEDA_MsgPanel::EraseMsgBox(wxDC * DC)
/*******************************************/
{
wxSize size;
wxColor color;
wxPen pen;
wxBrush brush;

	size = GetClientSize();
	color = GetBackgroundColour();
	pen.SetColour(color);
	brush.SetColour(color);
	brush.SetStyle(wxSOLID);
	DC->SetPen(pen);
	DC->SetBrush(brush);

	DC->DrawRectangle(0,0,size.x,size.y);
	DC->SetBrush(wxNullBrush);
	DC->SetPen(wxNullPen);
}


