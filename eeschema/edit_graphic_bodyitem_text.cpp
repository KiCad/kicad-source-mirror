	/**********************************************/
	/* EESchema - symbtext.cpp for Library Editor */
	/**********************************************/

/* Menu et routines de creation, modification, suppression de textes
	du type symbole
	(textes autres que Fields)
*/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "dialog_bodygraphictext_properties_base.h"

#include "protos.h"


class Dialog_BodyGraphicText_Properties : public Dialog_BodyGraphicText_Properties_base
{
private:
	WinEDA_LibeditFrame * m_Parent;
	LibDrawText * m_GraphicText;

public:
	Dialog_BodyGraphicText_Properties( WinEDA_LibeditFrame* aParent, LibDrawText * aGraphicText);
	~Dialog_BodyGraphicText_Properties() {};

private:
	void OnInitDialog( wxInitDialogEvent& event );
	void OnOkClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );
};


Dialog_BodyGraphicText_Properties::Dialog_BodyGraphicText_Properties(  WinEDA_LibeditFrame* aParent, LibDrawText * aGraphicText) :
	Dialog_BodyGraphicText_Properties_base(aParent)
{
	m_Parent = aParent;
	m_GraphicText = aGraphicText;
}


/********************************************************************************/
void Dialog_BodyGraphicText_Properties::OnInitDialog( wxInitDialogEvent& event )
/********************************************************************************/
{
wxString msg;

	SetFocus();

	if ( m_GraphicText )
	{
		msg = ReturnStringFromValue(g_UnitMetric, m_GraphicText->m_Size.x, m_Parent->m_InternalUnits);
		m_TextSize->SetValue(msg);
		m_TextValue->SetValue(m_GraphicText->m_Text);
		if ( m_GraphicText->m_Unit == 0 ) m_CommonUnit->SetValue(TRUE);
		if ( m_GraphicText->m_Convert == 0 ) m_CommonConvert->SetValue(TRUE);
		if ( m_GraphicText->m_Orient == TEXT_ORIENT_VERT ) m_Orient->SetValue(TRUE);
		int shape = 0;
		if ( m_GraphicText->m_Italic)
			shape = 1;
		if ( m_GraphicText->m_Width > 1)
			shape |= 2;

		m_TextShapeOpt->SetSelection(shape);
	}
	else
	{
		msg = ReturnStringFromValue(g_UnitMetric, g_LastTextSize, m_Parent->m_InternalUnits);
		m_TextSize->SetValue(msg);
		if ( ! g_FlDrawSpecificUnit ) m_CommonUnit->SetValue(TRUE);
		if ( ! g_FlDrawSpecificConvert ) m_CommonConvert->SetValue(TRUE);
		if ( g_LastTextOrient == TEXT_ORIENT_VERT ) m_Orient->SetValue(TRUE);
	}

	msg = m_TextSizeText->GetLabel() + ReturnUnitSymbol();
	m_TextSizeText->SetLabel(msg);
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
}


void Dialog_BodyGraphicText_Properties::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}


/***************************************************************************/
void Dialog_BodyGraphicText_Properties::OnOkClick( wxCommandEvent& event )
/***************************************************************************/
/* Met a jour les differents parametres pour le composant en cours d'edition
*/
{
wxString Line;

	Line = m_TextValue->GetValue();
	g_LastTextOrient = m_Orient->GetValue() ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ;
	wxString msg = m_TextSize->GetValue();
	g_LastTextSize = ReturnValueFromString(g_UnitMetric, msg, m_Parent->m_InternalUnits);
	g_FlDrawSpecificConvert = m_CommonConvert->GetValue() ? FALSE : TRUE;
	g_FlDrawSpecificUnit = m_CommonUnit->GetValue() ? FALSE : TRUE;

	if ( m_GraphicText )
	{
		if ( ! Line.IsEmpty() ) m_GraphicText->m_Text = Line;
		else m_GraphicText->m_Text = wxT("[null]");
		m_GraphicText->m_Size.x = m_GraphicText->m_Size.y = g_LastTextSize;
		m_GraphicText->m_Orient = g_LastTextOrient;
		if( g_FlDrawSpecificUnit ) m_GraphicText->m_Unit = CurrentUnit;
		else m_GraphicText->m_Unit = 0;
		if( g_FlDrawSpecificConvert ) m_GraphicText->m_Convert = CurrentConvert;
		else m_GraphicText->m_Convert = 0;

		if ( (m_TextShapeOpt->GetSelection() & 1 ) != 0 )
			m_GraphicText->m_Italic = true;
		else
			m_GraphicText->m_Italic = false;

		if ( (m_TextShapeOpt->GetSelection() & 2 ) != 0 )
			m_GraphicText->m_Width = m_GraphicText->m_Size.x / 4;
		else
			m_GraphicText->m_Width = 0;

	}
	Close();

	if ( CurrentDrawItem )
		CurrentDrawItem->DisplayInfo( m_Parent );
	Close();
}



void WinEDA_LibeditFrame::EditSymbolText(wxDC * DC, LibEDA_BaseStruct * DrawItem)
{
    int DrawMode = g_XorMode;

    if ( ( DrawItem == NULL )
         || ( DrawItem->Type() != COMPONENT_GRAPHIC_TEXT_DRAW_TYPE ) )
        return;

	/* Effacement ancien texte */
	if( DC)
		DrawLibraryDrawStruct(DrawPanel, DC, CurrentLibEntry, wxPoint(0, 0),
                              DrawItem, DrawMode);


	Dialog_BodyGraphicText_Properties * frame =
			new Dialog_BodyGraphicText_Properties(this,  (LibDrawText *) DrawItem);
	frame->ShowModal(); frame->Destroy();

	GetScreen()->SetModify();

	/* Affichage nouveau texte */
	if( DC )
	{
		if ( (DrawItem->m_Flags & IS_MOVED) == 0 )
			DrawMode = GR_DEFAULT_DRAWMODE;
		DrawLibraryDrawStruct(DrawPanel, DC, CurrentLibEntry, wxPoint(0, 0),
				DrawItem, DrawMode);
	}
}


/****************************************************/
void WinEDA_LibeditFrame::RotateSymbolText(wxDC * DC)
/****************************************************/
/*
	90 deg Graphic text Rotation .
*/
{
LibDrawText * DrawItem = (LibDrawText *) CurrentDrawItem;

	if(DrawItem == NULL) return;

	/* Erase drawing (can be within a move command) */
	if ( DrawPanel->ManageCurseur == NULL)
		DrawLibraryDrawStruct(DrawPanel, DC, CurrentLibEntry, wxPoint(0, 0),
				DrawItem, g_XorMode);
	else DrawPanel->ManageCurseur(DrawPanel, DC, FALSE);

	if( DrawItem->m_Orient == TEXT_ORIENT_HORIZ)
			DrawItem->m_Orient = TEXT_ORIENT_VERT;
	else DrawItem->m_Orient = TEXT_ORIENT_HORIZ;

	GetScreen()->SetModify();

	/* Redraw item with new orient */
	if ( DrawPanel->ManageCurseur == NULL)
		DrawLibraryDrawStruct(DrawPanel, DC, CurrentLibEntry, wxPoint(0, 0),
				DrawItem, GR_DEFAULT_DRAWMODE);
	else DrawPanel->ManageCurseur(DrawPanel, DC, FALSE);

}


