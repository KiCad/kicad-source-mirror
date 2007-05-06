/********************/
/* Fichier colors.h */
/********************/

#ifndef _COLORS_H
#define _COLORS_H

#ifndef COMMON_GLOBL
#define COMMON_GLOBL extern
#endif

/* Definitions des Numeros des Couleurs ( palette de 32) */
#define NBCOLOR 32
#define MASKCOLOR 31	// masque pour bits significatifs

/* bit indicateur d'affichage (vu / non vu) des items : (defini dans les valeurs des couleurs*/
#define ITEM_NOT_SHOW 0x40000
/* Definition du bit de surbrillance */
#define HIGHT_LIGHT_FLAG 0x80000

enum EDA_Colors
{
 BLACK = 0,
 BLUE,
 GREEN,
 CYAN,
 RED,
 MAGENTA,
 BROWN,
 LIGHTGRAY,
 DARKGRAY,
 LIGHTBLUE,
 LIGHTGREEN,
 LIGHTCYAN,
 LIGHTRED,
 LIGHTMAGENTA,
 YELLOW,
 WHITE,
 DARKDARKGRAY,
 DARKBLUE,
 DARKGREEN,
 DARKCYAN,
 DARKRED,
 DARKMAGENTA,
 DARKBROWN,
 LIGHTYELLOW,
LASTCOLOR
};

class StructColors
{
public:
	unsigned char m_Blue, m_Green, m_Red, m_Numcolor;
	wxChar* m_Name;
	int m_LightColor;
};

extern StructColors ColorRefs[NBCOLOR];
#ifdef MAIN
StructColors ColorRefs[NBCOLOR]	=
{
	 { 0,  0,	0,  BLACK, wxT("BLACK"), DARKDARKGRAY},
	 { 192,  0,  0, BLUE, wxT("BLUE"), LIGHTBLUE},
	 { 0, 160,  0,  GREEN, wxT("GREEN"), LIGHTGREEN },
	 { 160, 160, 0,  CYAN, wxT("CYAN"), LIGHTCYAN },
	 { 0,  0, 160,  RED, wxT("RED"), LIGHTRED },
	 { 160,  0, 160,  MAGENTA, wxT("MAGENTA"), LIGHTMAGENTA },
	 { 0, 128, 128,  BROWN, wxT("BROWN"), YELLOW },
	 { 192, 192, 192,  LIGHTGRAY, wxT("GRAY"), WHITE },
	 { 128,  128,  128,  DARKGRAY, wxT("DARKGRAY"), LIGHTGRAY },
	 { 255,	  0, 0,  LIGHTBLUE, wxT("LIGHTBLUE"),  LIGHTBLUE },
	 { 0, 255, 0, LIGHTGREEN, wxT("LIGHTGREEN"), LIGHTGREEN },
	 { 255, 255, 0, LIGHTCYAN, wxT("LIGHTCYAN"), LIGHTCYAN },
	 { 0,  0, 255, LIGHTRED, wxT("LIGHTRED"), LIGHTRED },
	 { 255,  0, 255, LIGHTMAGENTA, wxT("LIGHTMAGENTA"), LIGHTMAGENTA },
	 { 0, 255, 255, YELLOW, wxT("YELLOW"), YELLOW },
	 { 255, 255, 255, WHITE, wxT("WHITE"), WHITE },
	 {  64,  64, 64,  DARKDARKGRAY, wxT("DARKDARKGRAY"),  DARKGRAY },
	 {  64,	  0,  0,  DARKBLUE, wxT("DARKBLUE"), BLUE },
	 {	  0,  64,  0,  DARKGREEN, wxT("DARKGREEN"),  GREEN },
	 {  64,  64,  0,  DARKCYAN, wxT("DARKCYAN"),  CYAN },
	 {	  0,   0, 80,  DARKRED, wxT("DARKRED"),  RED },
	 {  64,   0, 64,  DARKMAGENTA, wxT("DARKMAGENTA"), MAGENTA },
	 {	  0,  64, 64,  DARKBROWN, wxT("DARKBROWN"),  BROWN },
 	 {	128, 255, 255,   LIGHTYELLOW, wxT("LIGHTYELLOW"),   LIGHTYELLOW }
   };
#endif /* ifdef MAIN */


COMMON_GLOBL wxPen * DrawPen;
COMMON_GLOBL wxBrush * DrawBrush;

#endif /* ifndef _COLORS_H */
