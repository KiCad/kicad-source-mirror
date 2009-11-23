/**************/
/* gr_basic.h */
/**************/

#ifndef GR_BASIC
#define GR_BASIC

#include "colors.h"
#include <vector>
class EDA_Rect;


#define GR_COPY 0
#define GR_OR   0x01000000
#define GR_XOR  0x02000000
#define GR_AND  0x04000000
#define GR_NXOR 0x08000000

#define GR_SURBRILL 0x80000000

#define GR_M_LEFT_DOWN   0x10000000
#define GR_M_RIGHT_DOWN  0x20000000
#define GR_M_MIDDLE_DOWN 0x40000000
#define GR_M_DCLICK      0x80000000


extern int g_XorMode;
extern int g_DrawBgColor;


typedef enum {
    /* Line styles for Get/SetLineStyle. */
    GR_SOLID_LINE  = 0,
    GR_DOTTED_LINE = 1,
    GR_DASHED_LINE = 3
} GRLineStypeType;


int  GRMapX( int x );
int  GRMapY( int y );

class WinEDA_DrawPanel;

void GRSetDrawMode( wxDC* DC, int mode );
int  GRGetDrawMode( wxDC* DC );
void GRResetPenAndBrush( wxDC* DC );
void GRSetColorPen( wxDC* DC, int Color, int width = 1, int stype = wxSOLID );
void GRSetBrush( wxDC* DC, int Color, int fill = 0 );

/** function GRForceBlackPen
 * @param flagforce True to force a black pen whenever the asked color
 */
void GRForceBlackPen( bool flagforce );

/** function GetGRForceBlackPenState
 * @return ForceBlackPen (True if a black pen was forced)
 */
bool GetGRForceBlackPenState( void );
void SetPenMinWidth( int minwidth );

void GRLine( EDA_Rect* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd,
             int aWidth, int aColor );
void GRLine( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2,
             int y2, int width, int Color );
void GRMixedLine( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                  int width, int Color );
void GRSMixedLine( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                   int width, int Color );
void GRDashedLine( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int  y2,
                   int width, int Color );
void GRSDashedLine( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2,
                    int y2, int width, int Color );
void GRDashedLineTo( EDA_Rect* ClipBox, wxDC* DC, int x2, int y2, int width,
                     int Color );
void GRSDashedLineTo( EDA_Rect* ClipBox, wxDC* DC, int x2, int y2, int width,
                      int Color );
void GRSLine( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
              int width, int Color );
void GRMoveTo( int x, int y );
void GRSMoveTo( int x, int y );
void GRLineTo( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int width,
               int Color );
void GRSLineTo( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int width,
                int Color );
void GRMoveRel( int x, int y );
void GRSMoveRel( int x, int y );
void GRLineRel( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int width,
                int Color );
void GRSLineRel( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int width,
                 int Color );


void GRPoly( EDA_Rect* ClipBox, wxDC* DC, int n, wxPoint Points[], bool Fill,
             int width, int Color, int BgColor );

void GRBezier( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
               int x3, int y3, int width, int Color );
void GRBezier( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
               int x3, int y3, int x4, int y4, int width, int Color );

/**
 * Function GRClosedPoly
 * draws a closed polygon onto the drawing context \a aDC and optionally fills
 * and/or draws a border around it.
 * @param ClipBox defines a rectangular boundary outside of which no drawing
 *                will occur.
 * @param aDC the device context into which drawing should occur.
 * @param aPointCount the number of points in the array \a aPointArray.
 * @param aPointArray an array holding the wxPoints in the polygon.
 * @param doFill true if polygon is to be filled, else false and only the
 *               boundary is drawn.
 * @param aPenColor the color index of the border.
 * @param aFillColor the fill color of the polygon's interior.
 */
void GRClosedPoly( EDA_Rect* ClipBox,
                   wxDC  *   aDC,
                   int       aPointCount,
                   wxPoint   aPoints[],
                   bool      doFill,
                   int       aPenColor,
                   int       aFillColor );

// @todo could make these 2 closed polygons calls a single function and default
// the aPenWidth argument

/**
 * Function GRClosedPoly
 * draws a closed polygon onto the drawing context \a aDC and optionally fills
 * and/or draws a border around it.
 * @param ClipBox defines a rectangular boundary outside of which no drawing
 *                will occur.
 * @param aDC the device context into which drawing should occur.
 * @param aPointCount the number of points in the array \a aPointArray.
 * @param aPointArray an array holding the wxPoints in the polygon.
 * @param doFill true if polygon is to be filled, else false and only the
 *               boundary is drawn.
 * @param aPenWidth is the width of the pen to use on the perimeter, can be
 *                  zero.
 * @param aPenColor the color index of the border.
 * @param aFillColor the fill color of the polygon's interior.
 */
void GRClosedPoly( EDA_Rect* ClipBox,
                   wxDC*     aDC,
                   int       aPointCount,
                   wxPoint   aPoints[],
                   bool      doFill,
                   int       aPenWidth,
                   int       aPenColor,
                   int       aFillColor );


/**
 * Function GRCircle
 * draws a circle onto the drawing context \a aDC centered at the user
 * coordinates (x,y)
 *
 * @param ClipBox defines a rectangular boundary outside of which no drawing
 *                will occur.
 * @param aDC the device context into which drawing should occur.
 * @param x The x coordinate in user space of the center of the circle.
 * @param x The y coordinate in user space of the center of the circle.
 * @param aRadius is the radius of the circle.
 * @param aColor is an index into our color table of RGB colors.
 * @see EDA_Colors and colors.h
 */
void GRCircle( EDA_Rect* ClipBox, wxDC* aDC, int x, int y, int aRadius,
               int aColor );

void GRCircle( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int r, int  width,
               int Color );
void GRFilledCircle( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int r,
                     int width, int Color, int BgColor );
void GRSCircle( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int r, int width,
                int Color );
void GRSFilledCircle( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int r,
                      int width, int Color, int BgColor );
void GRArc( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int StAngle,
            int EndAngle, int r, int Color );
void GRArc( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int StAngle,
            int EndAngle, int r, int width, int Color );
void GRArc1( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
             int xc, int yc, int Color );
void GRArc1( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
             int xc, int yc, int width, int Color );
void GRSArc1( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
              int xc, int yc, int width, int Color );
void GRSArc( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int StAngle,
             int EndAngle, int r, int width, int Color );
void GRFilledArc( EDA_Rect* ClipBox, wxDC* DC, int x, int y,
                  int StAngle, int EndAngle, int r, int Color, int BgColor );
void GRFilledArc( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int StAngle,
                  int EndAngle, int r, int width, int Color, int BgColor );
void GRSFilledArc( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int StAngle,
                   int EndAngle, int r, int width, int Color, int BgColor );
void GRCSegm( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
              int width, int Color );
void GRFillCSegm( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                  int width, int Color );
void GRSCSegm( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
               int width, int Color );
void GRSFillCSegm( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                   int width, int Color );

void GRSetColor( int Color );
void GRSetDefaultPalette();
int  GRGetColor();
void GRPutPixel( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int color );
void GRSPutPixel( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int color );
int  GRGetPixel( wxDC* DC, int x, int y );
void GRFilledRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1,
                   int x2, int y2, int Color, int BgColor );
void GRFilledRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1,
                   int x2, int y2, int width, int Color, int BgColor );
void GRSFilledRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1,
                    int x2, int y2, int Color, int BgColor );
void GRSFilledRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1,
                    int x2, int y2, int width, int Color, int BgColor );
void GRRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1,
             int x2, int y2, int Color );
void GRRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1,
             int x2, int y2, int width, int Color );
void GRSRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1,
              int x2, int y2, int Color );
void GRSRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1,
              int x2, int y2, int width, int Color );


#endif      /* define GR_BASIC */
