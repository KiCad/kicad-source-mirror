	/********************************/
	/* Low level graphics routines  */
	/********************************/


#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

#ifdef PCBNEW
#include "pcbnew.h"
#endif

#ifdef EESCHEMA
#include "program.h"
#include "libcmp.h"
#include "general.h"
#endif

#ifdef CVPCB
#include "pcbnew.h"
#include "cvpcb.h"
#endif

#include "trigo.h"

#ifndef FILLED
#define FILLED 1
#endif

#ifndef EXCHG
#define EXCHG(a,b) { int __temp__ = (a); (a) = (b); (b) = __temp__; }
#endif

/* Variables locales */
static int GRLastMoveToX , GRLastMoveToY;
static int Text_Color = LIGHTGRAY;

static int PenMinWidth = 1;		/* largeur minimum de la plume (DOIT etre > 0)
							(utile pour trace sur imprimante) */
static int ForceBlackPen;	/* si != 0 : traces en noir (utilise pour trace
							sur imprimante */
static int xcliplo = 0,
	ycliplo = 0,
	xcliphi = 2000,
	ycliphi = 2000;	/* coord de la surface de trace */
static int lastcolor = -1;
static int lastwidth = -1;
static wxDC * lastDC = NULL;

/*
	Macro de clipping du trace d'une ligne:
	la ligne (x1,y1 x2,y2) est clippee pour rester dans le cadre
	(xcliplo,ycliplo xcliphi,ycliphi) (variables globales,locales a ce fichier)
	Ceci est necessaire sous WIN95 car les coord de trace
	(bien que en int 32bits) sont tronquees en 16 bits (stupide BG)
*/
#ifndef us
#define us unsigned int
#endif

static inline int USCALE(us arg, us num, us den)
	{
	int ii;

	ii = (int)( ((float) arg * num) / den);
	return( ii );
	}


#ifdef WX_ZOOM
#define GET_ZOOM 1
#else
#define GET_ZOOM ActiveScreen->GetZoom()
#endif

static int inline ZoomValue(int value_to_zoom)
{
int zoom = GET_ZOOM;
	if ( value_to_zoom >= 0 )
		return( (value_to_zoom + (zoom >> 1 ) ) /zoom );
	else
		return( (value_to_zoom - (zoom >> 1 ) ) /zoom );
}	
	
/****************************************/
/* External reference for the mappings. */
/****************************************/
int GRMapX(int x)
{
int coord = x - ActiveScreen->m_DrawOrg.x;

#ifndef WX_ZOOM
 	coord = ZoomValue(coord);
	coord -= ActiveScreen->m_StartVisu.x;
#endif

	return coord;
}

int GRMapY(int y)
{
int coord = y - ActiveScreen->m_DrawOrg.y;

#ifndef WX_ZOOM
 	coord = ZoomValue(coord);
	coord -= ActiveScreen->m_StartVisu.y;
#endif

	return coord;
}



#define WHEN_OUTSIDE return
#define WHEN_INSIDE 

#define CLIP_LINE(x1,y1,x2,y2) \
{\
int temp;\
do {\
	if(x1 > x2) { EXCHG(x1,x2); EXCHG(y1,y2); }\
	if((x2 < xcliplo) || (x1 > xcliphi)) { WHEN_OUTSIDE; }\
	if(y1 < y2)\
		{\
		if((y2 < ycliplo) || (y1 > ycliphi)) { WHEN_OUTSIDE;}\
		if(y1 < ycliplo)\
			{\
			temp = USCALE((x2 - x1),(ycliplo - y1),(y2 - y1));\
			if((x1 += temp) > xcliphi) { WHEN_OUTSIDE; }\
			y1 = ycliplo;\
			WHEN_INSIDE;\
			}\
		if(y2 > ycliphi)\
			{\
			temp = USCALE((x2 - x1),(y2 - ycliphi),(y2 - y1));\
			if((x2 -= temp) < xcliplo) { WHEN_OUTSIDE; }\
			y2 = ycliphi;\
			WHEN_INSIDE;\
			}\
		if(x1 < xcliplo)\
			{\
			temp = USCALE((y2 - y1),(xcliplo - x1),(x2 - x1));\
			y1 += temp; x1 = xcliplo;\
			WHEN_INSIDE;\
			}\
		if(x2 > xcliphi)\
			{\
			temp = USCALE((y2 - y1),(x2 - xcliphi),(x2 - x1));\
			y2 -= temp; x2 = xcliphi;\
			WHEN_INSIDE;\
			}\
		}\
	else\
		{\
		if((y1 < ycliplo) || (y2 > ycliphi)) { WHEN_OUTSIDE; }\
		if(y1 > ycliphi)\
			{\
			temp = USCALE((x2 - x1),(y1 - ycliphi),(y1 - y2));\
			if((x1 += temp) > xcliphi) { WHEN_OUTSIDE; }\
			y1 = ycliphi;\
			WHEN_INSIDE;\
			}\
		if(y2 < ycliplo)\
			{\
			temp = USCALE((x2 - x1),(ycliplo - y2),(y1 - y2));\
			if((x2 -= temp) < xcliplo) { WHEN_OUTSIDE; }\
			y2 = ycliplo;\
			WHEN_INSIDE;\
			}\
		if(x1 < xcliplo)\
			{\
			temp = USCALE((y1 - y2),(xcliplo - x1),(x2 - x1));\
			y1 -= temp; x1 = xcliplo;\
			WHEN_INSIDE;\
			}\
		if(x2 > xcliphi)\
			{\
			temp = USCALE((y1 - y2),(x2 - xcliphi),(x2 - x1));\
			y2 += temp; x2 = xcliphi;\
			WHEN_INSIDE;\
			}\
		}\
	} while(0);\
}

static void WinClipAndDrawLine(EDA_Rect * ClipBox, wxDC * DC,
			int x1, int y1, int x2, int y2,
			int Color, int width = 1 )
{
	GRLastMoveToX = x2;
	GRLastMoveToY = y2;

	if ( ClipBox )
		{
		xcliplo = ClipBox->GetX();
		ycliplo = ClipBox->GetY();
		xcliphi = ClipBox->GetRight();
		ycliphi = ClipBox->GetBottom();

		xcliplo -= width;
		ycliplo -= width;

		xcliphi += width;
		ycliphi += width;

		CLIP_LINE(x1, y1, x2, y2);
		}

	GRSetColorPen(DC, Color, width);
	DC->DrawLine(x1, y1, x2, y2);
}


/* Routine de forcage de la reinit de la plume courante.
	Doit etre appelee par securite apres changement de contexte graphique
	avant tout trace
 */
void GRResetPenAndBrush(wxDC * DC)
{
	lastcolor = -1;
	GRSetBrush(DC, BLACK);	// Force no fill
	lastDC = NULL;
}

/* routine d'ajustage de la largeur mini de plume */
void SetPenMinWidth(int minwidth)
{
	PenMinWidth = minwidth;
	if( PenMinWidth < 1 ) PenMinWidth = 1;
}


/* Routine de changement de couleurs et epaisseur de la plume courante */
void GRSetColorPen(wxDC * DC, int Color , int width)
{
	Color  &= MASKCOLOR;	// Pour 32 couleurs Max

	if(width < PenMinWidth) width = PenMinWidth;

	if( ForceBlackPen && Color != WHITE ) Color = BLACK;

	if( (lastcolor != Color) || (lastwidth != width) || (lastDC != DC ) )
		{
		DrawPen->SetColour(
						ColorRefs[Color].m_Red,
						ColorRefs[Color].m_Green,
						ColorRefs[Color].m_Blue
						);

		DrawPen->SetWidth(width);
		if ( &DC->GetPen() != DrawPen ) DC->SetPen(*DrawPen);
		lastcolor = Color; lastwidth = width; lastDC = DC;
		}
}

/***********************************************/
void GRSetBrush(wxDC * DC, int Color , int fill)
/***********************************************/
{
	Color &= MASKCOLOR;	// Pour 32 couleurs Max
	if( ForceBlackPen && Color != WHITE ) Color = BLACK;
	DrawBrush->SetColour(
						ColorRefs[Color].m_Red,
						ColorRefs[Color].m_Green,
						ColorRefs[Color].m_Blue
						);

	if ( fill ) DrawBrush->SetStyle(wxSOLID);
	else DrawBrush->SetStyle(wxTRANSPARENT);
	if ( &DC->GetBrush() != DrawBrush ) DC->SetBrush(*DrawBrush);
}

/*************************************/
void GRForceBlackPen(bool flagforce )
/*************************************/
{
	ForceBlackPen = flagforce;
}


/************************************************************/
/* routines de controle et positionnement du curseur souris */
/************************************************************/
/* positionne la souris au point de coord pos */
void GRMouseWarp(WinEDA_DrawPanel * panel, const wxPoint& pos)
{
	if( panel == NULL ) return;
	panel->WarpPointer(pos.x, pos.y);
}

/**********************************************/
/* Routine pour selectionner le mode de trace */
/**********************************************/
void GRSetDrawMode(wxDC * DC, int draw_mode)
{
	if( draw_mode & GR_OR ) DC->SetLogicalFunction(wxOR);
	else if( draw_mode & GR_XOR ) DC->SetLogicalFunction(wxXOR);
	else if( draw_mode & GR_NXOR ) DC->SetLogicalFunction(wxEQUIV);
	else DC->SetLogicalFunction(wxCOPY);
}


/*********************************************************************/
void GRPutPixel(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int Color)
/*********************************************************************/
{
	GRSPutPixel(ClipBox, DC, GRMapX(x), GRMapY(y), Color);
}


/********************************************************************/
void GRSPutPixel(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int Color)
/********************************************************************/
{

	if ( ClipBox )	/* suppression des pixels hors ecran */
		{
		if ( x < ClipBox->GetX() ) return;
		if ( y < ClipBox->GetY() ) return;
		if ( x > (ClipBox->GetRight()) ) return;
		if ( y > (ClipBox->GetBottom()) ) return;
		}
	GRSetColorPen(DC, Color );
	DC->DrawPoint( x, y);
}

	/*******************************************/
	/* int GRGetPixel(wxDC * DC, int x, int y) */
	/*******************************************/

int GRGetPixel(wxDC * DC, int x, int y)
{
wxColour colour;
unsigned char r, g, b;
int ii;

	DC->GetPixel( (long)x, (long)y, &colour);
	r = colour.Red();
	b = colour.Blue();
	g = colour.Green();
	for ( ii = 0; ii < NBCOLOR; ii++ )
		{
		if( ( r == ColorRefs[ii].m_Red ) &&
			( g == ColorRefs[ii].m_Green ) &&
			( b == ColorRefs[ii].m_Blue ) )
			break;
		}

	return ii;
}



/****************************************************************************
* Routine to draw a line, in Object spaces.						  *
****************************************************************************/
void GRLine(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2, int Color)
{
	GRSLine(ClipBox, DC, GRMapX(x1), GRMapY(y1), GRMapX(x2), GRMapY(y2), Color);
}

/***************************************************/
/* Routine to draw a Dashed line, in Screen space. */
/***************************************************/
void GRSDashedLine(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2, int Color)
{
	GRLastMoveToX = x2;
	GRLastMoveToY = y2;
	lastcolor = -1;
	DrawPen->SetStyle(wxSHORT_DASH);
	GRSLine(ClipBox, DC, x1, y1, x2, y2, Color);
	lastcolor = -1;
	DrawPen->SetStyle(wxSOLID);
}

void GRSDashedLineTo(EDA_Rect * ClipBox,wxDC * DC, int x2, int y2, int Color)
{
	lastcolor = -1;
	DrawPen->SetStyle(wxSHORT_DASH);
	GRSLine(ClipBox, DC, GRLastMoveToX, GRLastMoveToY, x2, y2, Color);
	lastcolor = -1;
	DrawPen->SetStyle(wxSOLID);
	GRLastMoveToX = x2;
	GRLastMoveToY = y2;
}
/****************************************************************************
* Routine to draw a Dashed line, in Object spaces.						  *
****************************************************************************/
void GRDashedLineTo(EDA_Rect * ClipBox,wxDC * DC,int x2, int y2, int Color)
{
	GRSDashedLineTo(ClipBox, DC, GRMapX(x2), GRMapY(y2), Color);
}

void GRDashedLine(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2, int Color)
{
	GRSDashedLine(ClipBox, DC, GRMapX(x1), GRMapY(y1), GRMapX(x2), GRMapY(y2), Color);
}


/*************************************************/
/* Routine to draw a Bus line, in Object spaces. */
/*************************************************/

void GRBusLine(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2, int Color)
{
	x1 = GRMapX(x1); x2 = GRMapX(x2);
	y1 = GRMapY(y1); y2 = GRMapY(y2);
	GRSBusLine(ClipBox, DC, x1, y1, x2, y2, Color);
}

/****************************************************************
* Routine to draw a Bus Line, in Screen (pixels) space.			 *
****************************************************************************/
void GRSBusLine(EDA_Rect * ClipBox, wxDC * DC, int x1, int y1, int x2, int y2, int Color)
{
	GRSFillCSegm(ClipBox, DC, x1, y1, x2, y2, 3*PenMinWidth, Color);
	GRLastMoveToX = x2;
	GRLastMoveToY = y2;
}


/****************************************************************************
* Routine to move to a new position, in Object space.				*
****************************************************************************/
void GRMoveTo(int x, int y)
{
	GRLastMoveToX = GRMapX(x);
	GRLastMoveToY = GRMapY(y);
}

/*******************************************************/
/* Routine to draw to a new position, in Object space. */
/*******************************************************/
void GRLineTo(EDA_Rect * ClipBox,wxDC * DC, int x, int y, int Color)
{
int GRLineToX, GRLineToY;

	GRLineToX = GRMapX(x); GRLineToY = GRMapY(y);
	GRSLine(ClipBox, DC, GRLastMoveToX, GRLastMoveToY, GRLineToX, GRLineToY, Color);
}

/*************************************************/
/* Routine to draw a Mixed line, in Object space */
/*************************************************/
void GRMixedLine(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2, int Color)
{
	GRSMixedLine(ClipBox, DC, GRMapX(x1), GRMapY(y1), GRMapX(x2), GRMapY(y2), Color);
}

/***********************************************************/
/* Routine to draw a Mixed line, in Screen (Pixels)  space */
/***********************************************************/
void GRSMixedLine(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2, int Color)
{
	DrawPen->SetStyle(wxDOT_DASH);
	GRSLine(ClipBox, DC, x1, y1, x2, y2, Color);
	DrawPen->SetStyle(wxSOLID);
}

/*******************************************************************/
/* Routine to draw a Bus line to a new position, in Object spaces. */
/*******************************************************************/
void GRBusLineTo(EDA_Rect * ClipBox,wxDC * DC, int x, int y, int Color)
{
int GRLineToX, GRLineToY;

	GRLineToX = GRMapX(x); GRLineToY = GRMapY(y);

	GRSBusLine(ClipBox, DC, GRLastMoveToX, GRLastMoveToY, GRLineToX, GRLineToY,
						Color);
	GRLastMoveToX = GRLineToX;
	GRLastMoveToY = GRLineToY;
}

/****************************************************************************
* Routine to move to a new position, in Screen (pixels) space.			*
****************************************************************************/
void GRSMoveTo(int x, int y)
{
	GRLastMoveToX = x;
	GRLastMoveToY = y;
}

/****************************************************************************
* Routine to draw to a new position, in Screen (pixels) space.			*
****************************************************************************/
void GRSLineTo(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int Color)
{
	GRSLine(ClipBox, DC, GRLastMoveToX, GRLastMoveToY, x, y, Color);
	GRLastMoveToX = x; GRLastMoveToY = y;
}

/****************************************************************************
* Routine to draw to a new position, in Screen (pixels) space.			*
****************************************************************************/
void GRSLine(EDA_Rect * ClipBox, wxDC *DC, int x1, int y1, int x2, int y2, int Color)
{
	WinClipAndDrawLine(ClipBox, DC, x1, y1, x2, y2, Color);
	GRLastMoveToX = x2; GRLastMoveToY = y2;
}

/****************************************************************************/
/* Routine to move to a new position relative to current one, as in Object	*/
/* space.																	*/
/****************************************************************************/
void GRMoveRel(int x, int y)
{
	GRLastMoveToX += ZoomValue(x);
	GRLastMoveToY += ZoomValue(y);
}

/****************************************************************************
* Routine to line to a new position relative to current one, as in Object	*
* space.									*
****************************************************************************/
void GRLineRel(EDA_Rect * ClipBox,wxDC * DC, int x, int y, int Color)
{
int GRLineToX = GRLastMoveToX,
	GRLineToY = GRLastMoveToY;

	GRLineToX += ZoomValue(x);
	GRLineToY += ZoomValue(y);

	GRSLine(ClipBox, DC, GRLastMoveToX, GRLastMoveToY, GRLineToX, GRLineToY, Color);
}

/****************************************************************************
* Routine to move to a new position relative to current one, as in Screen	*
* space (pixel coords.).							  *
****************************************************************************/
void GRSMoveRel(int x, int y)
{
	GRLastMoveToX += x;
	GRLastMoveToY += y;
}

/****************************************************************************
* Routine to line to a new position relative to current one, as in Screen	*
* space (pixel coords.).							  *
****************************************************************************/
void GRSLineRel(EDA_Rect * ClipBox,wxDC * DC, int x, int y, int Color)
{
long GRLineToX = GRLastMoveToX + x,
	GRLineToY = GRLastMoveToY + y;

	GRSLine(ClipBox, DC, GRLastMoveToX, GRLastMoveToY, GRLineToX, GRLineToY, Color);
	GRLastMoveToX = GRLineToX;
	GRLastMoveToY = GRLineToY;
}

/**************************************************/
/* Routine de trace d'un segment a bouts arrondis */
/* Object space  = real coords.).				  */
/**************************************************/
void GRCSegm(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int width, int Color)
{
	GRSCSegm(ClipBox, DC, GRMapX(x1), GRMapY(y1), GRMapX(x2), GRMapY(y2),
				ZoomValue(width), Color);
}

/*******************************************************************
* Routine de trace d'un segment (plein) a bouts arrondis in Object *
* space (real coords.).												 *
********************************************************************/
void GRFillCSegm(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int width, int Color)
{
	GRSFillCSegm(ClipBox, DC, GRMapX(x1), GRMapY(y1), GRMapX(x2), GRMapY(y2),
				ZoomValue(width), Color);
}


	/**********************************************************/
	/* Routine de trace d'un segment (plein) a bouts arrondis */
	/* ( Screen  space = pixel coords.).					  */
	/**********************************************************/

void GRSFillCSegm(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2, int width, int Color)
{
	WinClipAndDrawLine(ClipBox, DC, x1,y1,x2,y2, Color, width);
}


/****************************************************************/
/* Routine de trace d'un segment a bouts arrondis (Mode SKETCH) */
/* Screen space (pixel coords.).								*/
/****************************************************************/
void GRSCSegm(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2, int width, int Color)
{
long rayon, dwx, dwy;
long dx, dy, dwx2, dwy2;
long sx1, sy1, ex1, ey1; /* coord du 1er bord */
long sx2, sy2, ex2, ey2; /* coord du 1eme bord */
bool swap_ends = FALSE;


	GRLastMoveToX = x2;
	GRLastMoveToY = y2;

	if ( ClipBox )
		{
		xcliplo = ClipBox->GetX();
		ycliplo = ClipBox->GetY();
		xcliphi = ClipBox->GetRight();
		ycliphi = ClipBox->GetHeight();

		xcliplo -= width;
		ycliplo -= width;

		xcliphi += width;
		ycliphi += width;

		CLIP_LINE(x1, y1, x2, y2);
		}


	if ( width <= 2 )	/* ligne simple ou epaisse de 2 pixels*/
		{
		GRSetColorPen(DC, Color, width );
		DC->DrawLine(x1, y1, x2, y2);
		return;
		}

	GRSetColorPen(DC, Color );
	GRSetBrush(DC,Color,FALSE);

	rayon = (width+1) >> 1;
	dx = x2 - x1; dy = y2 - y1;
	if ( dx == 0 )  /* segment vertical */
	{
		dwx = rayon;
		if ( dy >= 0 ) dwx = -dwx;
		sx1 = x1 - dwx; sy1 = y1;
		ex1 = x2 - dwx; ey1 = y2;
		DC->DrawLine(sx1 , sy1, ex1, ey1);

		sx2 = x1 + dwx; sy2 = y1;
		ex2 = x2 + dwx; ey2 = y2;
		DC->DrawLine(sx2, sy2, ex2, ey2);
	}

	else if ( dy == 0 ) /* segment horizontal */
	{
		dwy = rayon;
		if ( dx < 0 ) dwy = -dwy;
		sx1 = x1, sy1 = y1 - dwy;
		ex1 = x2; ey1 = y2 - dwy;
		DC->DrawLine(sx1, sy1, ex1 , ey1);

		sx2 = x1; sy2 = y1 + dwy;
		ex2 = x2; ey2 = y2 + dwy;
		DC->DrawLine(sx2, sy2, ex2 , ey2);
	}

	else
	{
		if ( ABS(dx) == ABS(dy) ) /* segment a 45 degre */
		{
			dwx = dwy = ((width * 5)+ 4) / 7;	// = width/2 * 0.707
			if ( dy < 0 )
			{
				if ( dx <= 0 )
				{
					dwx = -dwx; swap_ends = TRUE;
				}
			}
			else
			{
				if( dx > 0 )
				{
					dwy = -dwy; swap_ends = TRUE;
				}
			}
		}

		else
		{
			int delta_angle = ArcTangente(dy, dx);
			dwx = 0; dwy = width;
			RotatePoint( (int*)&dwx, (int*)&dwy, -delta_angle);
		}
		dwx2 = dwx >> 1; dwy2 = dwy >> 1;
		sx1 = x1 - dwx2; sy1 = y1 - dwy2;
		ex1 = x2 - dwx2; ey1 = y2 - dwy2;
		DC->DrawLine(sx1, sy1, ex1, ey1);

		sx2 = x1 + dwx2; sy2 = y1 + dwy2;
		ex2 = x2 + dwx2; ey2 = y2 + dwy2;
		DC->DrawLine(sx2, sy2, ex2, ey2);
	}

	if ( swap_ends )
	{
		DC->DrawArc(sx2, sy2, sx1 , sy1, x1, y1);
		DC->DrawArc(ex1, ey1, ex2, ey2, x2, y2);
	}
	else
	{
		DC->DrawArc(sx1, sy1, sx2 , sy2, x1, y1);
		DC->DrawArc(ex2, ey2, ex1, ey1, x2, y2);
	}
}



static bool IsGRSPolyDrawable( EDA_Rect * ClipBox,int n, int *Points)
{
int ii;
int Xmin, Xmax, Ymin, Ymax;

	Xmin = Xmax = Points[0];
	Ymin = Ymax = Points[1];

	for ( ii = 1; ii < n; ii++) 	// calcul du rectangle d'encadrement
		{
		int jj = ii * 2;
		Xmin = MIN(Xmin,Points[jj]);
		Xmax = MAX(Xmax,Points[jj]);
		Ymin = MIN(Ymin,Points[jj+1]);
		Ymax = MAX(Ymax,Points[jj+1]);
		}

	xcliplo = ClipBox->GetX();
	ycliplo = ClipBox->GetY();
	xcliphi = ClipBox->GetRight();
	ycliphi = ClipBox->GetHeight();

	if ( Xmax < xcliplo ) return FALSE;
	if ( Xmin > xcliphi ) return FALSE;
	if ( Ymax < ycliplo ) return FALSE;
	if ( Ymin > ycliphi ) return FALSE;

	return TRUE;

}
/************************************************************************/
/* Routine to draw a new polyline and fill it if Fill, in screen space. */
/************************************************************************/
void GRSPoly(EDA_Rect * ClipBox,wxDC * DC, int n, int *Points, int Fill,
			int Color, int BgColor)
{
int startx, starty;

	if ( ! IsGRSPolyDrawable(ClipBox, n, Points) ) return;

	GRSetColorPen(DC, Color );

	if( Fill && ( n > 2 ) )
		{
		GRSetBrush(DC, BgColor, FILLED);
		DC->DrawPolygon(n, (wxPoint*)Points);
		}
	else
		{
		startx = Points[n * 2 - 2]; starty = Points[n * 2 - 1];
		GRSetBrush(DC, Color);
		DC->DrawLines(n, (wxPoint*)Points);
		}
}

/************************************************************************/
/* Routine to draw a new polyline (line width = Width), in screen space. */
/************************************************************************/
void GRSPolyLines(EDA_Rect * ClipBox,wxDC * DC, int n, int *Points,
			int Color, int BgColor, int Width)
{
int startx, starty;

	if ( ! IsGRSPolyDrawable(ClipBox, n, Points) ) return;

	GRSetColorPen(DC, Color, Width );

	startx = Points[n * 2 - 2]; starty = Points[n * 2 - 1];
	GRSetBrush(DC, Color);
	DC->DrawLines(n, (wxPoint*)Points);
}

/******************************************************************************/
/* Routine to draw a new closed polyline and fill it if Fill, in screen space */
/******************************************************************************/
void GRSClosedPoly(EDA_Rect * ClipBox,wxDC * DC, int n, int *Points,
	int Fill, int Color, int BgColor)
{
int startx, starty;

	if ( ! IsGRSPolyDrawable(ClipBox, n, Points) ) return;

	GRSetColorPen(DC, Color );

	if( Fill && ( n > 2 ) )
		{
		GRSMoveTo(Points[n * 2 - 2], Points[n * 2 - 1]);
		GRSetBrush(DC, BgColor, FILLED);
		DC->DrawPolygon(n, (wxPoint*) Points, 0, 0,wxODDEVEN_RULE );
		}
	else
		{
		startx = Points[n * 2 - 2]; starty = Points[n * 2 - 1];
		GRSetBrush(DC, BgColor);
		DC->DrawLines(n, (wxPoint*)Points);

		/* Fermeture du polygone */
		if( (startx != Points[0]) || (starty != Points[1]) )
			{
			GRSLine(ClipBox, DC, Points[0], Points[1], startx, starty, Color);
			}
		}
}


/************************************************************************/
/* Routine to draw a new polyline and fill it if Fill, in drawing space. */
/************************************************************************/
void GRPoly(EDA_Rect * ClipBox, wxDC * DC, int n, int *Points,
	int Fill, int Color, int BgColor)
{
int ii, jj;

	for (ii = 0; ii < n; ii++)
		{
		jj = ii << 1;
		Points[jj] = GRMapX(Points[jj]);
		jj++;
		Points[jj] = GRMapY(Points[jj]);
		}
	GRSPoly(ClipBox, DC, n, Points, Fill, Color, BgColor);
}

void GRPolyLines(EDA_Rect * ClipBox, wxDC * DC, int n, int *Points,
		int Color, int BgColor, int width)
{
int ii, jj;

	width = ZoomValue(width);
	
	for (ii = 0; ii < n; ii++)
		{
		jj = ii << 1;
		Points[jj] = GRMapX(Points[jj]);
		jj++;
		Points[jj] = GRMapY(Points[jj]);
		}
	if ( width <= 1 ) GRSPoly(ClipBox, DC, n, Points, 0, Color, BgColor);
	else GRSPolyLines(ClipBox, DC, n, Points, Color, BgColor, width);
}
/**************************************************************************/
/* Routine to draw a closed polyline and fill it if Fill, in object space */
/**************************************************************************/
void GRClosedPoly(EDA_Rect * ClipBox,wxDC * DC, int n, int *Points,
		int Fill, int Color, int BgColor)
{
int ii, jj;

	for (ii = 0; ii < n; ii++)
		{
		jj = ii << 1;
		Points[jj] = GRMapX(Points[jj]);
		jj++;
		Points[jj] = GRMapY(Points[jj]);
		}
	GRSClosedPoly(ClipBox, DC, n, Points, Fill, Color, BgColor);
}

/***********************************************/
/* Routine to draw a circle, in object space. */
/***********************************************/
void GRCircle(EDA_Rect * ClipBox,wxDC * DC, int x, int y, int r, int Color)
{
int cx = GRMapX(x);
int cy = GRMapY(y);
int rayon = ZoomValue(r);

	GRSCircle(ClipBox, DC, cx, cy, rayon, Color );
}

/*****************************************************/
/* Routine to draw a Filled circle, in object space. */
/*****************************************************/
void GRFilledCircle(EDA_Rect * ClipBox,wxDC * DC, int x, int y, int r,
		int Color, int BgColor)
{
	r = ZoomValue(r);
	GRSFilledCircle(ClipBox, DC, GRMapX(x), GRMapY(y), r, Color, BgColor );
}

/***********************************************************/
/* Routine to draw un anneau, epaisseur w, in object space. */
/***********************************************************/
void GRCircle(EDA_Rect * ClipBox,  wxDC * DC, int x, int y, int r, int width, int Color)
{
	r = ZoomValue(r);
	width = ZoomValue(width);
	GRSCircle(ClipBox, DC, GRMapX(x), GRMapY(y), r, width, Color);
}


/***********************************************/
/* Routine to draw a circle, in drawing space. */
/***********************************************/
void GRSCircle(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int r, int Color)
{
int d = r + r;

	/* suppression des cercles hors ecran */
	if ( ClipBox )
		{
		int x0, y0, xm, ym;
		x0 = ClipBox->GetX();
		y0 = ClipBox->GetY();
		xm = ClipBox->GetRight();
		ym = ClipBox->GetBottom();
		if ( x < (x0-r) ) return;
		if ( y < (y0-r) ) return;
		if ( x > (r+xm) ) return;
		if ( y > (r+ym) ) return;
		}

	GRSetColorPen(DC, Color);
	GRSetBrush(DC,Color,FALSE);
	DC->DrawEllipse(x-r,y-r, d, d);
}

/******************************************************/
/* Routine to draw a FILLED circle, in drawing space. */
/******************************************************/
void GRSFilledCircle(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int r,
	int Color, int BgColor)
{
	/* suppression des cercles hors ecran */
	if ( ClipBox )
		{
		int x0, y0, xm, ym;
		x0 = ClipBox->GetX();
		y0 = ClipBox->GetY();
		xm = ClipBox->GetRight();
		ym = ClipBox->GetBottom();
		if ( x < (x0-r) ) return;
		if ( y < (y0-r) ) return;
		if ( x > (r+xm) ) return;
		if ( y > (r+ym) ) return;
		}

	GRSetColorPen(DC, Color );
	GRSetBrush(DC, BgColor, FILLED);
	DC->DrawEllipse(x-r, y-r, r+r, r+r);
}

	/***********************************************************************/
	/* Routine de trace d'un cercle epais ( Screen  space = pixel coords.) */
	/***********************************************************************/

void GRSCircle(EDA_Rect * ClipBox,wxDC * DC, int xc, int yc, int r, int width, int Color)
{
	/* suppression des cercles hors ecran */
	if ( ClipBox )
		{
		int x0, y0, xm, ym;
		x0 = ClipBox->GetX();
		y0 = ClipBox->GetY();
		xm = ClipBox->GetRight();
		ym = ClipBox->GetBottom();
		if ( xc < (x0-r - width) ) return;
		if ( yc < (y0-r - width) ) return;
		if ( xc > (r+xm + width) ) return;
		if ( yc > (r+ym + width) ) return;
		}

	GRSetColorPen(DC, Color, width);
	GRSetBrush(DC, Color, FALSE);
	DC->DrawEllipse(xc-r, yc-r, r+r, r+r);
}


/************************************************/
/* Routine to draw an arc, in USER space.		*/
/* Debut et fin de l'arc donnes par leur coord. */
/************************************************/
void GRArc1(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int xc, int yc, int Color)
{
	GRSArc1(ClipBox, DC,
			GRMapX(x1), GRMapY(y1), GRMapX(x2), GRMapY(y2),
			GRMapX(xc), GRMapY(yc), Color);
}

/************************************************/
/* Routine to draw an arc, width = width in USER space.		*/
/* Debut et fin de l'arc donnes par leur coord. */
/************************************************/
void GRArc1(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int xc, int yc, int width, int Color)
{
	GRSArc1(ClipBox, DC,
			GRMapX(x1), GRMapY(y1), GRMapX(x2), GRMapY(y2),
			GRMapX(xc), GRMapY(yc), ZoomValue(width), Color);
}

/************************************************/
/* Routine to draw an arc, in screen space.		*/
/* Debut et fin de l'arc donnes par leur coord. */
/************************************************/
void GRSArc1(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int xc, int yc, int Color)
{
	/* suppression des cercles hors ecran */
	if ( ClipBox )
		{
		int x0, y0, xm, ym, r;
		x0 = ClipBox->GetX();
		y0 = ClipBox->GetY();
		xm = ClipBox->GetRight();
		ym = ClipBox->GetBottom();
		r = (int)hypot(x1-xc, y1-yc);
		if ( xc < (x0-r) ) return;
		if ( yc < (y0-r) ) return;
		if ( xc > (r+xm) ) return;
		if ( yc > (r+ym) ) return;
		}

	GRSetColorPen(DC, Color );
	GRSetBrush(DC,Color,FALSE);
	DC->DrawArc(x1, y1, x2, y2, xc, yc);
}

/************************************************/
/* Routine to draw an arc, width = width, in screen space.		*/
/* Debut et fin de l'arc donnes par leur coord. */
/************************************************/
void GRSArc1(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int xc, int yc, int width, int Color)
{
	/* suppression des cercles hors ecran */
	if ( ClipBox )
		{
		int x0, y0, xm, ym, r;
		x0 = ClipBox->GetX();
		y0 = ClipBox->GetY();
		xm = ClipBox->GetRight();
		ym = ClipBox->GetBottom();
		r = (int)hypot(x1-xc, y1-yc);
		if ( xc < (x0-r) ) return;
		if ( yc < (y0-r) ) return;
		if ( xc > (r+xm) ) return;
		if ( yc > (r+ym) ) return;
		}

	GRSetColorPen(DC, Color , width);
	GRSetBrush(DC, Color);
	DC->DrawArc(x1, y1, x2, y2, xc, yc);
}

/********************************************************************/
/* Routine to draw an arc, in screen space.							*/
/* As the Y axe is inverted the Angles should be inverted as well. */
/********************************************************************/
void GRSArc(EDA_Rect * ClipBox,wxDC * DC, int xc, int yc,
			int StAngle, int EndAngle, int r, int Color)
{
int x1, y1, x2, y2;

	/* suppression des cercles hors ecran */
	if ( ClipBox )
		{
		int x0, y0, xm, ym;
		x0 = ClipBox->GetX();
		y0 = ClipBox->GetY();
		xm = ClipBox->GetRight();
		ym = ClipBox->GetBottom();
		if ( xc < (x0-r - 1) ) return;
		if ( yc < (y0-r - 1) ) return;
		if ( xc > (r+xm + 1) ) return;
		if ( yc > (r+ym + 1) ) return;
		}

	x1 = r; y1 = 0;
	RotatePoint( &x1, & y1, EndAngle);

	x2 = r; y2 = 0;
	RotatePoint( &x2, & y2, StAngle);

	GRSetColorPen(DC, Color);
	GRSetBrush(DC,Color,FALSE);
	DC->DrawArc(xc + x1, yc - y1, xc + x2, yc - y2, xc, yc);
}

/********************************************************************/
/* Routine to draw an arc, width = width, in screen space.			*/
/* As the Y axe is inverted the Angles should be inverted as well. */
/********************************************************************/
void GRSArc(EDA_Rect * ClipBox,wxDC * DC, int xc, int yc, int StAngle, int EndAngle,
					int r, int width, int Color)
{
int x1, y1, x2, y2;

	/* suppression des cercles hors ecran */
	if ( ClipBox )
		{
		int x0, y0, xm, ym;
		x0 = ClipBox->GetX();
		y0 = ClipBox->GetY();
		xm = ClipBox->GetRight();
		ym = ClipBox->GetBottom();
		if ( xc < (x0-r - width) ) return;
		if ( yc < (y0-r - width) ) return;
		if ( xc > (r+xm + width) ) return;
		if ( yc > (r+ym + width) ) return;
		}

	x1 = r; y1 = 0;
	RotatePoint( &x1, & y1, EndAngle);

	x2 = r; y2 = 0;
	RotatePoint( &x2, & y2, StAngle);

	GRSetColorPen(DC, Color , width);
	GRSetBrush(DC, Color);
	DC->DrawArc(xc + x1, yc - y1, xc + x2, yc - y2, xc, yc);
}

/********************************************************************/
/* Routine to draw an Filled arc, in screen space.					*/
/* As the Y axes is inverted the Angles should be inverted as well. */
/********************************************************************/
void GRSFilledArc(EDA_Rect * ClipBox,wxDC * DC, int xc, int yc,
				int StAngle, int EndAngle, int r, int Color, int BgColor)
{
int x1, y1, x2, y2;

	/* suppression des cercles hors ecran */
	if ( ClipBox )
		{
		int x0, y0, xm, ym;
		x0 = ClipBox->GetX();
		y0 = ClipBox->GetY();
		xm = ClipBox->GetRight();
		ym = ClipBox->GetBottom();
		if ( xc < (x0-r - 1) ) return;
		if ( yc < (y0-r - 1) ) return;
		if ( xc > (r+xm + 1) ) return;
		if ( yc > (r+ym + 1) ) return;
		}

	x1 = r; y1 = 0;
	RotatePoint( &x1, & y1, EndAngle);

	x2 = r; y2 = 0;
	RotatePoint( &x2, & y2, StAngle);

	GRSetBrush(DC, BgColor, FILLED);
	GRSetColorPen(DC, Color);
	DC->DrawArc(xc + x1, yc - y1, xc + x2, yc - y2, xc, yc);
}

/********************************************************************/
/* Routine to draw a Filled arc, in drawing space.						  */
/* As the Y axes is inverted the Angles should be inverted as well. */
/********************************************************************/
void GRFilledArc(EDA_Rect * ClipBox,wxDC * DC, int x, int y,
		int StAngle, int EndAngle, int r, int Color, int BgColor)
{
	GRSFilledArc(ClipBox, DC, GRMapX(x), GRMapY(y),
					StAngle, EndAngle,
					ZoomValue(r), Color, BgColor);
}

/********************************************************************/
/* Routine to draw an arc, in drawing space.						*/
/* As the Y axes is inverted the Angles should be inverted as well. */
/********************************************************************/
void GRArc(EDA_Rect * ClipBox,wxDC * DC, int xc, int yc, int StAngle,
			int EndAngle, int r, int Color)
{
int x1, y1, x2, y2;

	/* suppression des cercles hors ecran */
	if ( ClipBox )
		{
		int rayon = ZoomValue(r) + 1;
		int x0, y0, xm, ym, x, y;
		x0 = ClipBox->GetX();
		y0 = ClipBox->GetY();
		xm = ClipBox->GetRight();
		ym = ClipBox->GetBottom();
		x = GRMapX(xc); y = GRMapY(yc);
		if ( x < (x0 - rayon) ) return;
		if ( y < (y0 - rayon) ) return;
		if ( x > (xm + rayon) ) return;
		if ( y > (ym + rayon) ) return;
		}

	x1 = r; y1 = 0;
	RotatePoint( &x1, & y1, EndAngle);

	x2 = r; y2 = 0;
	RotatePoint( &x2, & y2, StAngle);

	GRSetColorPen(DC, Color);
	GRSetBrush(DC,Color,FALSE);
	DC->DrawArc(GRMapX(xc + x1), GRMapY(yc - y1),
				GRMapX(xc + x2), GRMapY(yc - y2),
				GRMapX(xc), GRMapY(yc) );
}

/********************************************************************/
/* Routine to draw an arc, width = width, in drawing space.						*/
/* As the Y axes is inverted the Angles should be inverted as well. */
/********************************************************************/
void GRArc(EDA_Rect * ClipBox, wxDC * DC, int x, int y, int StAngle, int EndAngle,
						 int r, int width, int Color)
{
	GRSArc(ClipBox, DC, GRMapX(x), GRMapY(y),
			StAngle, EndAngle,
			ZoomValue(r),
			ZoomValue(width),
			Color);
}

/**************************************************/
/* Routine to draw a Rectangle, in drawing space. */
/**************************************************/
void GRRect(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2, int Color)
{
	x1 = GRMapX(x1); y1 = GRMapY(y1);
	x2 = GRMapX(x2); y2 = GRMapY(y2);

	GRSRect(ClipBox, DC, x1, y1, x2, y2, Color );
}

/************************************************************************************/
void GRFilledRect(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
				int Color, int BgColor)
/************************************************************************************/
/* Routine to draw a Rectangle (filled with AreaColor), in drawing space. */
{
	x1 = GRMapX(x1); y1 = GRMapY(y1);
	x2 = GRMapX(x2); y2 = GRMapY(y2);

	GRSFilledRect(ClipBox, DC, x1, y1, x2, y2, Color, BgColor );
}


/*************************************************/
/* Routine to draw a Rectangle, in screen space. */
/*************************************************/
void GRSRect(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2, int Color)
{
	if(x1 > x2) EXCHG(x1,x2);
	if(y1 > y2) EXCHG(y1,y2);

	/* Clipping des coordonnees */
	if ( ClipBox )
		{
		int xmin = ClipBox->GetX();
		int ymin = ClipBox->GetY();
		int xmax = ClipBox->GetRight();
		int ymax = ClipBox->GetBottom();

		if ( x1 > xmax ) return;
		if ( x2 < xmin ) return;
		if ( y1 > ymax ) return;
		if ( y2 < ymin ) return;
		}

	GRSetColorPen(DC, Color );
	if ( (x1 == x2) || (y1 == y2) ) DC->DrawLine(x1, y1, x2, y2);
	else
	{
		GRSetBrush(DC, BLACK );
		DC->DrawRectangle(x1, y1, x2 - x1, y2 - y1);
	}
}


/***************************************************************************************/
void GRSFilledRect(EDA_Rect * ClipBox,wxDC * DC, int x1, int y1, int x2, int y2,
			int Color, int BgColor)
/***************************************************************************************/
/* Routine to draw a Filled Rectangle, in screen space. */
{
	if(x1 > x2) EXCHG(x1,x2);
	if(y1 > y2) EXCHG(y1,y2);
	if ( ClipBox )
		{
		int xmin = ClipBox->GetX();
		int ymin = ClipBox->GetY();
		int xmax = ClipBox->GetRight();
		int ymax = ClipBox->GetBottom();

		if ( x1 > xmax ) return;
		if ( x2 < xmin ) return;
		if ( y1 > ymax ) return;
		if ( y2 < ymin ) return;

		// Clipping des coordonnees
		if ( x1 < xmin )x1 = xmin -1;
		if ( y1 < ymin )y1 = ymin -1;
		if ( x2 > xmax ) x2 = xmax +1;
		if ( y2 > ymax ) y2 = ymax +1;
		}

	GRSetColorPen(DC, Color );
	if ( (x1 == x2) || (y1 == y2) ) DC->DrawLine(x1, y1, x2, y2);
	else
	{
		GRSetBrush(DC, BgColor, FILLED);
		DC->DrawRectangle(x1, y1, x2 - x1, y2 - y1);
	}
}


	/****************************************/
	/* Routines relatives au trace de texte */
	/****************************************/

	/*********************************************/
	/* Routine de selection de la fonte courante */
	/*********************************************/

void GRSetFont(wxDC * DC, wxFont * Font)
{
	DC->SetFont(*Font);
}

	/*********************************************************/
	/* void GRSetTextFgColor(wxFont * Font, int Color) */
	/*********************************************************/

/* Mise a la valeur Color des textes a afficher */
void GRSetTextFgColor(wxDC * DC, int Color)
{
	DC->SetTextForeground(wxColour(
						ColorRefs[Color].m_Red,
						ColorRefs[Color].m_Green,
						ColorRefs[Color].m_Blue)
						);
}

/* Mise a la valeur Color des textes a afficher */
void GRSetTextFgColor(wxDC * DC, wxFont *, int Color)
{
	DC->SetTextForeground(wxColour(
						ColorRefs[Color].m_Red,
						ColorRefs[Color].m_Green,
						ColorRefs[Color].m_Blue)
						);
}


/*****************************************************************************/
void GRGetTextExtent(wxDC * DC, const wxChar * Text, long * width, long * height)
/*****************************************************************************/
/* donne la taille du rectangle d'encadrement du texte Text
*/
{
long w = 0, h = 0;

	if ( Text )
		{
		DC->GetTextExtent(Text, &w, &h );
		}
	if ( width ) *width = w;
	if ( height ) * height = h;
}

	/********************************/
	/* void GRReseTextFgColor(void) */
	/********************************/

/* Mise a la couleur par defaut des textes a afficher */
void GRResetTextFgColor(wxDC * DC)
{
	GRSetTextFgColor(DC, Text_Color);
}


	/*********************************************************/
	/* void GRSetTextBgColor(wxFont * Font, int Color) */
	/*********************************************************/

/* Mise a la valeur Color du fond pour les textes a afficher */
void GRSetTextBgColor(wxDC * DC, int Color)
{
	Color &= MASKCOLOR;	// Pour 32 couleurs Max
	DC->SetTextBackground(wxColour(
						ColorRefs[Color].m_Red,
						ColorRefs[Color].m_Green,
						ColorRefs[Color].m_Blue)
						);
}

void GRSetTextBgColor(wxDC * DC, wxFont *, int Color)
{
	Color &= MASKCOLOR;	// Pour 32 couleurs Max
	DC->SetTextBackground(wxColour(
						ColorRefs[Color].m_Red,
						ColorRefs[Color].m_Green,
						ColorRefs[Color].m_Blue)
						);
}

