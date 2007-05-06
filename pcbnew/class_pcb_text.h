/************************************/
/* fonctions de la classe TEXTE_PCB */
/************************************/
#ifndef CLASS_PCB_TEXT_H
#define CLASS_PCB_TEXT_H

#include "base_struct.h"

class TEXTE_PCB: public EDA_BaseStruct, public EDA_TextStruct
{
public:
	TEXTE_PCB(EDA_BaseStruct * parent);
	TEXTE_PCB(TEXTE_PCB * textepcb);
	~TEXTE_PCB(void);

	/* supprime du chainage la structure Struct */
	void UnLink( void );

	/* duplicate structure */
	void Copy(TEXTE_PCB * source);

	void Draw(WinEDA_DrawPanel * panel, wxDC * DC,
					const wxPoint & offset, int DrawMode);

	// File Operations:
	int ReadTextePcbDescr(FILE * File, int * LineNum);
	int WriteTextePcbDescr(FILE * File);
};

#endif 	// #define CLASS_PCB_TEXT_H
