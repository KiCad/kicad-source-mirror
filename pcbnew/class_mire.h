/****************************************************/
/* fonctions de la classe MIRE (targets for photos) */
/****************************************************/
#ifndef MIRE_H
#define MIRE_H

#include "base_struct.h"


class MIREPCB: public EDA_BaseStruct
	{
	public:
	int m_Layer;				// 0.. 32 ( NON bit a bit)
	int m_Width;
	wxPoint m_Pos;
	int m_Shape;					// bit 0 : 0 = forme +, 1 = forme X
	int m_Size;

	public:
	MIREPCB(EDA_BaseStruct * StructFather);
	~MIREPCB(void);

	bool WriteMirePcbDescr(FILE * File);
	bool ReadMirePcbDescr(FILE * File, int * LineNum);

	/* supprime du chainage la structure Struct */
	void UnLink( void );

	void Copy(MIREPCB * source);

	void Draw(WinEDA_DrawPanel * panel, wxDC * DC, const wxPoint & offset, int mode_color);

};



#endif 	// #define MIRE_H
