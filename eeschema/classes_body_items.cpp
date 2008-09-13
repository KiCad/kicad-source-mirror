/************************/
/* class_body_items.cpp */
/************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"


//#define DRAW_ARC_WITH_ANGLE		// Used to draw arcs


/** Function Draw (virtual)
 * Draw A body item
 * @param aPanel = DrawPanel to use (can be null) mainly used for clipping purposes
 * @param aDC = Device Context (can be null)
 * @param aOffset = offset to draw
 * @param aDrawMode = GR_OR, GR_XOR, ...
 * @param aDisplay_mode = FILL_T value ( has meaning only for items what can be filled )
 * @param aTransformMatrix = Transform Matrix
 */

/**********************************************************************************************/
void LibDrawArc::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                       int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/**********************************************************************************************/
{
    wxPoint pos1, pos2, posc;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     LineWidth = MAX( m_Width, g_DrawMinimunLineWidth );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( (m_Selected & IS_SELECTED) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_ArcEnd ) + aOffset;
    pos2 = TransformCoordinate( aTransformMatrix, m_ArcStart ) + aOffset;
    posc = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    int  pt1  = t1;
    int  pt2  = t2;
    bool swap = MapAngles( &pt1, &pt2, aTransformMatrix );
    if( swap )
    {
        EXCHG( pos1.x, pos2.x ); EXCHG( pos1.y, pos2.y )
    }

    GRSetDrawMode( aDC, aDrawMode );

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRFilledArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
            m_Rayon, LineWidth, color,
            ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE && !aData )
        GRFilledArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
            m_Rayon, color, color );
    else
#ifdef DRAW_ARC_WITH_ANGLE

        GRArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
            m_Rayon, LineWidth, color );
#else

        GRArc1( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
            posc.x, posc.y, LineWidth, color );
#endif
}


/*************************************************************************************************/
void LibDrawCircle::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                          int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/*************************************************************************************************/
{
    wxPoint pos1;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     LineWidth = MAX( m_Width, g_DrawMinimunLineWidth );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( (m_Selected & IS_SELECTED) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    GRSetDrawMode( aDC, aDrawMode );

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRFilledCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y,
            m_Rayon, LineWidth, color,
            ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE )
        GRFilledCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y,
            m_Rayon, 0, color, color );
    else
        GRCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y,
            m_Rayon, LineWidth, color );
}


/*************************************************************************************************/
void LibDrawText::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                        int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/*************************************************************************************************/
{
    wxPoint pos1, pos2;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     LineWidth = MAX( m_Width, g_DrawMinimunLineWidth );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( (m_Selected & IS_SELECTED) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;

    /* The text orientation may need to be flipped if the
     *  transformation matrix causes xy axes to be flipped. */
    int t1 = (aTransformMatrix[0][0] != 0) ^ (m_Horiz != 0);

    DrawGraphicText( aPanel, aDC, pos1, color, m_Text,
        t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT,
        m_Size,
        GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, LineWidth );
}


/*************************************************************************************************/
void LibDrawSquare::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                          int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/*************************************************************************************************/
{
    wxPoint pos1, pos2;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     LineWidth = MAX( m_Width, g_DrawMinimunLineWidth );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( (m_Selected & IS_SELECTED) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    pos2 = TransformCoordinate( aTransformMatrix, m_End ) + aOffset;

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    if( fill == FILLED_WITH_BG_BODYCOLOR && !aData )
        GRFilledRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
            color, LineWidth,
            ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( m_Fill == FILLED_SHAPE  && !aData )
        GRFilledRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
            color, color );
    else
        GRRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
            LineWidth, color );
}


/*************************************************************************************************/
void LibDrawSegment::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                           int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/*************************************************************************************************/
{
    wxPoint pos1, pos2;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     LineWidth = MAX( m_Width, g_DrawMinimunLineWidth );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( (m_Selected & IS_SELECTED) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    pos2 = TransformCoordinate( aTransformMatrix, m_End ) + aOffset;

    GRLine( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y, LineWidth, color );
}


/*************************************************************************************************/
void LibDrawPolyline::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                            const wxPoint& aOffset, int aColor,
                            int aDrawMode, void* aData,
                            int aTransformMatrix[2][2] )
/*************************************************************************************************/
{
    wxPoint     pos1;

    int         color     = ReturnLayerColor( LAYER_DEVICE );
    int         LineWidth = MAX( m_Width, g_DrawMinimunLineWidth );
    static int* Buf_Poly_Drawings = NULL;               // Buffer used to store current corners coordinates for drawings
    static int  Buf_Poly_Size = 0;                      // Buffer used to store current corners coordinates for drawings

    if( aColor < 0 )                                    // Used normal color or selected color
    {
        if( (m_Selected & IS_SELECTED) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    // Set the size of the buffer od coordinates
    if( Buf_Poly_Drawings == NULL )
    {
        Buf_Poly_Size     = m_CornersCount;
        Buf_Poly_Drawings = (int*) MyMalloc( sizeof(int) * 2 * Buf_Poly_Size );
    }
    else if( Buf_Poly_Size < m_CornersCount )
    {
        Buf_Poly_Size     = m_CornersCount;
        Buf_Poly_Drawings = (int*) realloc( Buf_Poly_Drawings,
            sizeof(int) * 2 * Buf_Poly_Size );
    }

    for( int ii = 0, jj = 0; ii < m_CornersCount; ii++, jj += 2 )
    {
        pos1.x = m_PolyList[jj];
        pos1.y = m_PolyList[jj + 1];

        pos1 = TransformCoordinate( aTransformMatrix, pos1 ) + aOffset;

        Buf_Poly_Drawings[jj]     = pos1.x;
        Buf_Poly_Drawings[jj + 1] = pos1.y;
    }

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRPoly( &aPanel->m_ClipBox, aDC, m_CornersCount,
            Buf_Poly_Drawings, 1, LineWidth, color,
            ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE  )
        GRPoly( &aPanel->m_ClipBox, aDC, m_CornersCount,
            Buf_Poly_Drawings, 1, LineWidth, color, color );
    else
        GRPoly( &aPanel->m_ClipBox, aDC, m_CornersCount,
            Buf_Poly_Drawings, 0, LineWidth, color, color );
}


/*************************************************************************************************/
void LibDrawField::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                         int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/*************************************************************************************************/
{
}
