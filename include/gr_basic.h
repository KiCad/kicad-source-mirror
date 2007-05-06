		/**************/
		/* gr_basic.h */
		/**************/

#ifndef GR_BASIC
#define GR_BASIC

#ifndef COMMON_GLOBL
#define COMMON_GLOBL extern
#endif

#include "colors.h"

/* Constantes utiles */

#define GR_COPY 0
#define GR_OR 0x1000000
#define GR_XOR 0x2000000
#define GR_AND 0x4000000
#define GR_NXOR 0x8000000

#define GR_SURBRILL 0x80000000

#define GR_M_LEFT_DOWN		0x10000000
#define GR_M_RIGHT_DOWN		0x20000000
#define GR_M_MIDDLE_DOWN	0x40000000
#define GR_M_DCLICK			0x80000000


/* variables generales */
COMMON_GLOBL int g_XorMode 			// = GR_XOR ou GR_NXOR selon couleur de fond
#ifdef EDA_BASE							// pour les tracés en mode XOR
= GR_NXOR
#endif
;
COMMON_GLOBL int g_DrawBgColor		// couleur de fond de la frame de dessin
#ifdef EDA_BASE
 = WHITE
#endif
;


typedef enum {		/* Line styles for Get/SetLineStyle. */
	GR_SOLID_LINE  = 0,
	GR_DOTTED_LINE = 1,
	GR_DASHED_LINE = 3
} GRLineStypeType;

typedef enum {		/* Line widths for Get/SetLineStyle. */
	GR_NORM_WIDTH  = 1,
	GR_THICK_WIDTH = 3
} GRLineWidthType;


/*******************************************************/
/* Prototypage des fonctions definies dans gr_basic.cc */
/*******************************************************/
int GRMapX(int x);
int GRMapY(int y);

class WinEDA_DrawPanel;
void GRMouseWarp(WinEDA_DrawPanel * panel, const wxPoint& pos); /* positionne la souris au point de coord pos */

/* routines generales */
void GRSetDrawMode(wxDC * DC, int mode);
int GRGetDrawMode(wxDC * DC);
void GRResetPenAndBrush(wxDC * DC);
void GRSetColorPen(wxDC * DC, int Color , int width = 1);
void GRSetBrush(wxDC * DC, int Color , int fill = 0);
void GRForceBlackPen(bool flagforce );
void SetPenMinWidth(int minwidth); /* ajustage de la largeur mini de plume */

void GRLine(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int Color);
void GRMixedLine(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int Color);
void GRSMixedLine(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int Color);
void GRDashedLine(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int Color);
void GRSDashedLine(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int Color);
void GRDashedLineTo(EDA_Rect * ClipBox,wxDC * DC, int x2, int y2, int Color);
void GRSDashedLineTo(EDA_Rect * ClipBox,wxDC * DC, int x2, int y2, int Color);
void GRBusLine(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int Color);
void GRSBusLine(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int Color);
void GRSLine(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int Color);
void GRMoveTo(int x, int y);
void GRSMoveTo(int x, int y);
void GRLineTo(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int Color);
void GRBusLineTo(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int Color);
void GRSLineTo(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int Color);
void GRMoveRel(int x, int y);
void GRSMoveRel(int x, int y);
void GRLineRel(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int Color);
void GRSLineRel(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int Color);
void GRPoly(EDA_Rect * ClipBox, wxDC * DC, int n, int *Points,
		int Fill, int Color, int BgColor);
void GRPolyLines(EDA_Rect * ClipBox, wxDC * DC, int n, int *Points,
		int Color, int BgColor, int width);
void GRClosedPoly(EDA_Rect * ClipBox, wxDC * DC, int n, int *Points,
		int Fill, int Color, int BgColor);
void GRSPoly(EDA_Rect * ClipBox, wxDC * DC, int n, int *Points,
		int Fill, int Color, int BgColor);
void GRSPolyLines(EDA_Rect * ClipBox, wxDC * DC, int n, int *Points,
		int Color, int BgColor, int width);
void GRSClosedPoly(EDA_Rect * ClipBox, wxDC * DC, int n, int *Points,
		int Fill, int Color, int BgColor);
void GRCircle(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int r, int Color);
void GRCircle(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int r, int width, int Color);
void GRFilledCircle(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int r,
					int Color, int BgColor);
void GRSCircle(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int r, int Color);
void GRSCircle(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int r, int width, int Color);
void GRSFilledCircle(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int r,
		int Color, int BgColor);
void GRArc(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int StAngle, int EndAngle, int r, int Color);
void GRArc(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int StAngle, int EndAngle, int r, int width, int Color);
void GRArc1(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int xc, int yc, int Color);
void GRArc1(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int xc, int yc, int width, int Color);
void GRSArc1(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int xc, int yc, int Color);
void GRSArc1(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int xc, int yc, int width, int Color);
void GRSArc(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int StAngle, int EndAngle, int r, int Color);
void GRSArc(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int StAngle, int EndAngle, int r, int width, int Color);
void GRFilledArc(EDA_Rect * ClipBox, wxDC * DC, int x, int y,
			int StAngle, int EndAngle, int r, int Color, int BgColor);
void GRSFilledArc(EDA_Rect * ClipBox, wxDC * DC, int x, int y,
		int StAngle, int EndAngle, int r, int Color, int BgColor);
void GRCSegm(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int width, int Color);
void GRFillCSegm(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int width, int Color);
void GRSCSegm(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int width, int Color);
void GRSFillCSegm(EDA_Rect * ClipBox, wxDC * DC,
			int x1, int y1, int x2, int y2, int width, int Color);

void GRSetColor(int Color);
void GRSetDefaultPalette(void);
int GRGetColor(void);
void GRPutPixel(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int color);
void GRSPutPixel(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int color);
int GRGetPixel(wxDC * DC, int x, int y);
void GRFilledRect(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1,
			int x2, int y2, int Color, int BgColor);
void GRSFilledRect(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1,
			int x2, int y2, int Color, int BgColor);
void GRSFilledRect(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int Color, int BgColor);
void GRRect(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1,
			int x2, int y2, int Color);
void GRSRect(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1,
			int x2, int y2, int Color);

/* Routines relatives a l'affichage des textes */
void GRSetFont(wxDC * DC, wxFont * Font);
void GRResetTextFgColor(wxDC * DC);
void GRSetTextFgColor(wxDC * DC, int Color);
void GRSetTextFgColor(wxDC * DC, wxFont * Font, int Color);
int GRGetTextFgColor(wxDC * DC, wxFont * Font);
void GRSetTextBgColor(wxDC * DC, int Color);
void GRSetTextBgColor(wxDC * DC, wxFont * Font, int Color);
int GRGetTextBgColor(wxDC * DC, wxFont * Font);
void GRGetTextExtent(wxDC * DC, const wxChar * Text, long * width, long * height);

#endif		/* define GR_BASIC */

