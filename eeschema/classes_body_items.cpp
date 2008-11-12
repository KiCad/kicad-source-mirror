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


/* Basic class (abstract) for components bodies items */
LibEDA_BaseStruct::LibEDA_BaseStruct( KICAD_T struct_type ) :
    EDA_BaseStruct( struct_type )
{
    m_Unit    = 0;  /* Unit identification (for multi part per package)
                     *  0 if the item is common to all units */
    m_Convert = 0;  /* Shape identification (for parts which have a convert shape)
                     *  0 if the item is common to all shapes */
    m_Width   = 0;       /* Default value to draw lines or arc ... */
    m_Fill    = NO_FILL; /* NO_FILL, FILLED_SHAPE or FILLED_WITH_BG_BODYCOLOR.
                          * has meaning only for some items */
}


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
    int     linewidth = MAX( m_Width, g_DrawMinimunLineWidth );

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
        EXCHG( pos1.x, pos2.x );
        EXCHG( pos1.y, pos2.y );
    }

    GRSetDrawMode( aDC, aDrawMode );

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRFilledArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
            m_Rayon, linewidth, color,
            ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE && !aData )
        GRFilledArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
            m_Rayon, color, color );
    else
#ifdef DRAW_ARC_WITH_ANGLE



        GRArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
            m_Rayon, linewidth, color );
#else



        GRArc1( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
            posc.x, posc.y, linewidth, color );
#endif
}


/*************************************************************************************************/
void LibDrawCircle::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                          int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/*************************************************************************************************/
{
    wxPoint pos1;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     linewidth = MAX( m_Width, g_DrawMinimunLineWidth );

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
            m_Rayon, linewidth, color,
            ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE )
        GRFilledCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y,
            m_Rayon, 0, color, color );
    else
        GRCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y,
            m_Rayon, linewidth, color );
}


/*************************************************************************************************/
void LibDrawText::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                        int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/*************************************************************************************************/
{
    wxPoint pos1, pos2;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     linewidth = MAX( m_Width, g_DrawMinimunLineWidth );

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
        GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, linewidth );
}


/*************************************************************************************************/
void LibDrawSquare::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                          int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/*************************************************************************************************/
{
    wxPoint pos1, pos2;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     linewidth = MAX( m_Width, g_DrawMinimunLineWidth );

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
            color, linewidth,
            ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( m_Fill == FILLED_SHAPE  && !aData )
        GRFilledRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
            color, color );
    else
        GRRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
            linewidth, color );
}


/*************************************************************************************************/
void LibDrawSegment::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                           int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/*************************************************************************************************/
{
    wxPoint pos1, pos2;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     linewidth = MAX( m_Width, g_DrawMinimunLineWidth );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( (m_Selected & IS_SELECTED) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    pos2 = TransformCoordinate( aTransformMatrix, m_End ) + aOffset;

    GRLine( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y, linewidth, color );
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
    int         linewidth = MAX( m_Width, g_DrawMinimunLineWidth );
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
            Buf_Poly_Drawings, 1, linewidth, color,
            ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE  )
        GRPoly( &aPanel->m_ClipBox, aDC, m_CornersCount,
            Buf_Poly_Drawings, 1, linewidth, color, color );
    else
        GRPoly( &aPanel->m_ClipBox, aDC, m_CornersCount,
            Buf_Poly_Drawings, 0, linewidth, color, color );
}


/*************************************************************************************************/
void LibDrawField::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                         int aDrawMode, void* aData, int aTransformMatrix[2][2] )
/*************************************************************************************************/

/* if aData not NULL, adat must point a wxString which is used instead of the m_Text
 */
{
    wxPoint text_pos;

    int     color     = aColor;
    int     linewidth = MAX( m_Width, g_DrawMinimunLineWidth );

    if( aColor < 0 )                                    // Used normal color or selected color
    {
        if( (m_Selected & IS_SELECTED) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    if( color < 0 )
    {
        switch( m_FieldId )
        {
        case REFERENCE:
            color = ReturnLayerColor( LAYER_REFERENCEPART );
            break;

        case VALUE:
            color = ReturnLayerColor( LAYER_VALUEPART );
            break;

        default:
            color = ReturnLayerColor( LAYER_FIELDS );
            break;
        }
    }
    text_pos = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;

    wxString* text = aData ? (wxString*) aData : &m_Text;
    GRSetDrawMode( aDC, aDrawMode );
    DrawGraphicText( aPanel, aDC, text_pos,
        color, text->GetData(),
        m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
        m_Size,
        m_HJustify, m_VJustify, linewidth );
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test, in Field coordinate system
 * @return bool - true if a hit, else false
 */
bool LibDrawField::HitTest( const wxPoint& refPos )
{
    EDA_Rect bbox;             // bounding box for the text

    bbox.SetOrigin( m_Pos );
    int      dx;                // X size for the full text
    if ( m_FieldId == REFERENCE )
        dx = m_Size.x * (m_Text.Len( ) + 1);   // The displayed text has one char more (U is displayed U?)
    else
        dx = m_Size.x * m_Text.Len( );
    dx = (int) ((double) dx * 10.0/9);      // spacing between char is 0.1 the char size
    int dy = m_Size.y;

    if( m_Orient )
        EXCHG( dx, dy );            // Swap X and Y size for a vertical text

    // adjust position of the left bottom corner according to the justification
    // pos is at this point correct for a left and top justified text
    // Horizontal justification
    if( m_HJustify == GR_TEXT_HJUSTIFY_CENTER )
        bbox.Offset( -dx / 2, 0 );
    else if( m_HJustify == GR_TEXT_HJUSTIFY_RIGHT )
        bbox.Offset( -dx, 0 );

    // Vertical justification
    if( m_VJustify == GR_TEXT_VJUSTIFY_CENTER )
        bbox.Offset( 0, -dy / 2 );
    else if( m_VJustify == GR_TEXT_VJUSTIFY_TOP )
        bbox.Offset( 0, -dy );

    bbox.SetSize( dx, dy );

    if( bbox.Inside( refPos ) )
        return true;

    return false;
}
