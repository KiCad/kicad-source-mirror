/************************************/
/* fonctions de la classe COTATION */
/************************************/
#ifndef COTATION_H
#define COTATION_H

#include "base_struct.h"

class COTATION: public EDA_BaseStruct
{
	public:
	int m_Layer;				// 0.. 32 ( NON bit a bit)
	int m_Width;
	wxPoint m_Pos;
	int m_Shape;
	int m_Unit;		/* 0 = inches, 1 = mm */
	int m_Value;	/* valeur en unites PCB de la cote */

	TEXTE_PCB * m_Text;	/* pour affichage du texte */
	int Barre_ox, Barre_oy, Barre_fx, Barre_fy;
	int TraitG_ox, TraitG_oy, TraitG_fx, TraitG_fy;
	int TraitD_ox, TraitD_oy, TraitD_fx, TraitD_fy;
	int FlecheD1_ox, FlecheD1_oy, FlecheD1_fx, FlecheD1_fy;
	int FlecheD2_ox, FlecheD2_oy, FlecheD2_fx, FlecheD2_fy;
	int FlecheG1_ox, FlecheG1_oy, FlecheG1_fx, FlecheG1_fy;
	int FlecheG2_ox, FlecheG2_oy, FlecheG2_fx, FlecheG2_fy;

	public:
	COTATION(EDA_BaseStruct * StructFather);
	~COTATION(void);

	bool ReadCotationDescr(FILE * File, int * LineNum);
	bool WriteCotationDescr(FILE * File);

	/* supprime du chainage la structure Struct */
	void UnLink( void );

	/* Modification du texte de la cotation */
	void SetText(const wxString & NewText);

	void Copy(COTATION * source);

	void Draw(WinEDA_DrawPanel * panel, wxDC * DC, const wxPoint & offset, int mode_color);

};

#endif 	// #define COTATION_H
