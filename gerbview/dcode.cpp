/***************************/
/**** class D_CODE ****/
/***************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "macros.h"
#include "trigo.h"

#include "gerbview.h"
#include "class_gerber_draw_item.h"
#include "class_GERBER.h"

#define DEFAULT_SIZE 100

/* Format Gerber: NOTES:
 * Tools and D_CODES
 *   tool number (identification of shapes)
 *   1 to 999
 *
 * D_CODES:
 *   D01 ... D9 = command codes:
 *      D01 = activating light (pen down) while moving
 *      D02 = light extinction (pen up) while moving
 *      D03 = Flash
 *      D04 to D09 = non used
 *   D10 ... D999 = Indentification Tool (Shape id)
 *
 * For tools defining a shape):
 * DCode min = D10
 * DCode max = 999
 */


/***************/
/* Class DCODE */
/***************/


D_CODE::D_CODE( int num_dcode )
{
    m_Num_Dcode = num_dcode;
    Clear_D_CODE_Data();
}


D_CODE::~D_CODE()
{
}


void D_CODE::Clear_D_CODE_Data()
{
    m_Size.x     = DEFAULT_SIZE;
    m_Size.y     = DEFAULT_SIZE;
    m_Shape      = APT_CIRCLE;
    m_Drill.x    = m_Drill.y = 0;
    m_DrillShape = APT_DEF_NO_HOLE;
    m_InUse      = FALSE;
    m_Defined    = FALSE;
    m_Macro      = NULL;
    m_Rotation   = 0.0;
    m_EdgesCount = 0;
    m_PolyCorners.clear();
}


const wxChar* D_CODE::ShowApertureType( APERTURE_T aType )
{
    const wxChar* ret;

    switch( aType )
    {
    case APT_CIRCLE:
        ret = wxT( "Round" );   break;

    case APT_RECT:
        ret = wxT( "Rect" );    break;

    case APT_OVAL:
        ret = wxT( "Oval" );    break;

    case APT_POLYGON:
        ret = wxT( "Poly" );    break;

    case APT_MACRO:
        ret = wxT( "Macro" );   break;

    default:
        ret = wxT( "???" );     break;
    }

    return ret;
}

/** GetShapeDim
 * Calculate a value that can be used to evaluate the size of text
 * when displaying the D-Code of an item
 * due to the complexity of some shapes,
 * one cannot calculate the "size" of a shape (only a bounding box)
 * but here, the "dimension" of the shape is the diameter of the primitive
 * or for lines the width of the line if the shape is a line
 * @param aParent = the parent GERBER_DRAW_ITEM which is actually drawn
 * @return a dimension, or -1 if no dim to calculate
 */
int D_CODE::GetShapeDim( GERBER_DRAW_ITEM* aParent )
{
    int dim = -1;
    switch( m_Shape )
    {
    case APT_CIRCLE:
        dim = m_Size.x;
        break;

    case APT_RECT:
    case APT_OVAL:
        dim = MIN( m_Size.x, m_Size.y );
        break;

    case APT_POLYGON:
        dim = MIN( m_Size.x, m_Size.y );
        break;

    case APT_MACRO:
        if( m_Macro )
            dim = m_Macro->GetShapeDim( aParent );
        break;

    default:
        break;
    }

    return dim;
}


/*
 * Function ReadDCodeDefinitionFile
 * Can be useful only with old RS274D Gerber file format.
 * Is not needed with RS274X files format.
 * These files need an auxiliary DCode file description. There is no defined file format for this.
 * This function read a file format I needed a long time ago.
 * reads in a dcode file assuming ALSPCB file format with ';' indicating comments.
 * Format is like CSV but with optional ';' delineated comments:
 * tool,     Horiz,       Vert,   drill, speed, accel. ,Type ; [DCODE (commentaire)]
 * ex:     1,         12,       12,     0,        0,     0,   3 ; D10
 */
int GERBVIEW_FRAME::ReadDCodeDefinitionFile( const wxString& D_Code_FullFileName )
{
    int      current_Dcode, ii, dcode_scale;
    char*    ptcar;
    int      dimH, dimV, drill, dummy;
    float    fdimH, fdimV, fdrill;
    char     c_type_outil[256];
    char     line[GERBER_BUFZ];
    wxString msg;
    D_CODE*  dcode;
    FILE*    dest;
    int      layer = GetScreen()->m_Active_Layer;
    int      type_outil;

    if( g_GERBER_List[layer] == NULL )
        g_GERBER_List[layer] = new GERBER_IMAGE( this, layer );

    GERBER_IMAGE* gerber = g_GERBER_List[layer];


    /* Updating gerber scale: */
    dcode_scale   = 10; /* By uniting dCode = mil, internal unit = 0.1 mil
                         * -> 1 unite dcode = 10 unit PCB */
    current_Dcode = 0;

    if( D_Code_FullFileName.IsEmpty() )
        return 0;

    dest = wxFopen( D_Code_FullFileName, wxT( "rt" ) );
    if( dest == 0 )
    {
        msg = _( "File " ) + D_Code_FullFileName + _( " not found" );
        DisplayError( this, msg, 10 );
        return -1;
    }

    gerber->InitToolTable();

    while( fgets( line, sizeof(line) - 1, dest ) != NULL )
    {
        if( *line == ';' )
            continue;

        if( strlen( line ) < 10 )
            continue;                       /* Skip blank line. */

        dcode = NULL;
        current_Dcode = 0;

        /* Determine of the type of file from D_Code. */
        ptcar = line;
        ii    = 0;
        while( *ptcar )
            if( *(ptcar++) == ',' )
                ii++;

        if( ii >= 6 )   /* valeurs en mils */
        {
            sscanf( line, "%d,%d,%d,%d,%d,%d,%d", &ii,
                    &dimH, &dimV, &drill, &dummy, &dummy, &type_outil );

            dimH  = wxRound( dimH * dcode_scale );
            dimV  = wxRound( dimV * dcode_scale );
            drill = wxRound( drill * dcode_scale );
            if( ii < 1 )
                ii = 1;
            current_Dcode = ii - 1 + FIRST_DCODE;
        }
        else        /* Values in inches are converted to mils. */
        {
            fdrill = 0;
            current_Dcode = 0;

            sscanf( line, "%f,%f,%1s", &fdimV, &fdimH, c_type_outil );
            ptcar = line;
            while( *ptcar )
            {
                if( *ptcar == 'D' )
                {
                    sscanf( ptcar + 1, "%d,%f", &current_Dcode, &fdrill );
                    break;
                }
                else
                    ptcar++;
            }

            dimH  = wxRound( fdimH * dcode_scale * 1000 );
            dimV  = wxRound( fdimV * dcode_scale * 1000 );
            drill = wxRound( fdrill * dcode_scale * 1000 );

            if( strchr( "CLROP", c_type_outil[0] ) )
                type_outil = (APERTURE_T) c_type_outil[0];
            else
            {
                fclose( dest );
                return -2;
            }
        }

        /* Update the list of d_codes if consistant. */
        if( current_Dcode < FIRST_DCODE )
            continue;

        if( current_Dcode >= TOOLS_MAX_COUNT )
            continue;

        dcode = gerber->GetDCODE( current_Dcode );
        dcode->m_Size.x  = dimH;
        dcode->m_Size.y  = dimV;
        dcode->m_Shape   = (APERTURE_T) type_outil;
        dcode->m_Drill.x = dcode->m_Drill.y = drill;
        dcode->m_Defined = TRUE;
    }

    fclose( dest );

    return 1;
}


/* Set Size Items (Lines, Flashes) from DCodes List
 */
void GERBVIEW_FRAME::CopyDCodesSizeToItems()
{
    static D_CODE dummy( 999 );   //Used if D_CODE not found in list

    BOARD_ITEM*   item = GetBoard()->m_Drawings;

    for( ; item; item = item->Next() )
    {
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
        D_CODE*           dcode     = gerb_item->GetDcodeDescr();
        wxASSERT( dcode );
        if( dcode == NULL )
            dcode = &dummy;

        dcode->m_InUse = TRUE;

        gerb_item->m_Size = dcode->m_Size;
        if(                                             // Line Item
            (gerb_item->m_Shape == GBR_SEGMENT )        /* rectilinear segment */
            || (gerb_item->m_Shape == GBR_ARC )         /* segment arc (rounded tips) */
            || (gerb_item->m_Shape == GBR_CIRCLE )      /* segment in a circle (ring) */
            )
        {
        }
        else        // Spots ( Flashed Items )
        {
            switch( dcode->m_Shape )
            {
            case APT_CIRCLE:        /* spot round */
                gerb_item->m_Shape = GBR_SPOT_CIRCLE;
                break;

            case APT_OVAL:          /* spot oval*/
                gerb_item->m_Shape = GBR_SPOT_OVAL;
                break;

            case APT_RECT:                /* spot rect*/
                gerb_item->m_Shape = GBR_SPOT_RECT;
                break;

            case APT_POLYGON:                /* spot regular polyg 3 to 1é edges */
                gerb_item->m_Shape = GBR_SPOT_POLY;
                break;

            case APT_MACRO:                /* spot defined by a macro */
                gerb_item->m_Shape = GBR_SPOT_MACRO;
                break;

            default:
                wxMessageBox( wxT( "GERBVIEW_FRAME::CopyDCodesSizeToItems() error" ) );
                break;
            }
        }
    }
}


/*
 * Function DrawFlashedShape
 * Draw the dcode shape for flashed items.
 * When an item is flashed, the DCode shape is the shape of the item
 */
void D_CODE::DrawFlashedShape(  GERBER_DRAW_ITEM* aParent,
                                EDA_Rect* aClipBox, wxDC* aDC, int aColor, int aAltColor,
                                wxPoint aShapePos, bool aFilledShape )
{
    int radius;

    switch( m_Shape )
    {
    case APT_MACRO:
        GetMacro()->DrawApertureMacroShape( aParent, aClipBox, aDC, aColor, aAltColor,
                                            aShapePos, aFilledShape);
        break;

    case APT_CIRCLE:
        radius = m_Size.x >> 1;
        if( !aFilledShape )
            GRCircle( aClipBox, aDC, aParent->GetABPosition(aShapePos),
                      radius, 0, aColor );
        else
            if( m_DrillShape == APT_DEF_NO_HOLE )
                GRFilledCircle( aClipBox, aDC, aParent->GetABPosition(aShapePos),
                                radius, aColor );
            else if( APT_DEF_ROUND_HOLE == 1 )    // round hole in shape
            {
                int width = (m_Size.x - m_Drill.x ) / 2;
                GRCircle( aClipBox, aDC,  aParent->GetABPosition(aShapePos),
                          radius - (width / 2), width, aColor );
            }
            else                            // rectangular hole
            {
                if( m_PolyCorners.size() == 0 )
                    ConvertShapeToPolygon();
                DrawFlashedPolygon( aParent, aClipBox, aDC, aColor, aFilledShape, aShapePos );
            }
        break;

    case APT_RECT:
    {
        wxPoint start;
        start.x = aShapePos.x - m_Size.x / 2;
        start.y = aShapePos.y - m_Size.y / 2;
        wxPoint end = start + m_Size;
        start = aParent->GetABPosition( start );
        end = aParent->GetABPosition( end );
        if( !aFilledShape )
        {
            GRRect( aClipBox, aDC, start.x, start.y, end.x, end.y,
                    0, aColor );
        }
        else if( m_DrillShape == APT_DEF_NO_HOLE )
        {
            GRFilledRect( aClipBox, aDC, start.x, start.y, end.x, end.y,
                          0, aColor, aColor );
        }
        else
        {
            if( m_PolyCorners.size() == 0 )
                ConvertShapeToPolygon();
            DrawFlashedPolygon( aParent, aClipBox, aDC, aColor, aFilledShape, aShapePos );
        }
    }
    break;

    case APT_OVAL:
    {
        wxPoint start = aShapePos;
        wxPoint end   = aShapePos;
        if( m_Size.x > m_Size.y )   // horizontal oval
        {
            int delta = (m_Size.x - m_Size.y) / 2;
            start.x -= delta;
            end.x   += delta;
            radius   = m_Size.y;
        }
        else   // horizontal oval
        {
            int delta = (m_Size.y - m_Size.x) / 2;
            start.y -= delta;
            end.y   += delta;
            radius   = m_Size.x;
        }
        start = aParent->GetABPosition( start );
        end = aParent->GetABPosition( end );
        if( !aFilledShape )
        {
            GRCSegm( aClipBox, aDC, start.x, start.y,
                     end.x, end.y, radius, aColor );
        }
        else if( m_DrillShape == APT_DEF_NO_HOLE )
        {
            GRFillCSegm( aClipBox, aDC, start.x,
                         start.y, end.x, end.y, radius, aColor );
        }
        else
        {
            if( m_PolyCorners.size() == 0 )
                ConvertShapeToPolygon();
            DrawFlashedPolygon( aParent, aClipBox, aDC, aColor, aFilledShape, aShapePos );
        }
    }
    break;

    case APT_POLYGON:
        if( m_PolyCorners.size() == 0 )
            ConvertShapeToPolygon();
        DrawFlashedPolygon( aParent, aClipBox, aDC, aColor, aFilledShape, aShapePos );
        break;
    }
}


/*
 * Function DrawFlashedPolygon
 * a helper function used id ::Draw to draw the polygon stored ion m_PolyCorners
 * Draw some Apertures shapes when they are defined as filled polygons.
 * APT_POLYGON is always a polygon, but some complex shapes are also converted to
 * polygons (shapes with holes)
 */
void D_CODE::DrawFlashedPolygon( GERBER_DRAW_ITEM* aParent,
                                 EDA_Rect* aClipBox, wxDC* aDC,
                                 int aColor, bool aFilled,
                                 const wxPoint& aPosition )
{
    if( m_PolyCorners.size() == 0 )
        return;

    std::vector<wxPoint> points;
    points = m_PolyCorners;
    for( unsigned ii = 0; ii < points.size(); ii++ )
    {
        points[ii] += aPosition;
        points[ii] = aParent->GetABPosition( points[ii] );
    }

    GRClosedPoly( aClipBox, aDC, points.size(), &points[0], aFilled, aColor, aColor );
}


#define SEGS_CNT 32     // number of segments to approximate a circle

// A helper function for D_CODE::ConvertShapeToPolygon().
// Add a hole to a polygon
static void addHoleToPolygon( std::vector<wxPoint>& aBuffer,
                              APERTURE_DEF_HOLETYPE aHoleShape,
                              wxSize                aSize,
                              wxPoint               aAnchorPos );

/**
 * Function ConvertShapeToPolygon
 * convert a shape to an equivalent polygon.
 * Arcs and circles are approximated by segments
 * Useful when a shape is not a graphic primitive (shape with hole,
 * Rotated shape ... ) and cannot be easily drawn.
 */
void D_CODE::ConvertShapeToPolygon()
{
    wxPoint initialpos;
    wxPoint currpos;

    m_PolyCorners.clear();

    switch( m_Shape )
    {
    case APT_CIRCLE:        // creates only a circle with rectangular hole
        currpos.x  = m_Size.x >> 1;
        initialpos = currpos;
        for( unsigned ii = 0; ii <= SEGS_CNT; ii++ )
        {
            currpos = initialpos;
            RotatePoint( &currpos, ii * 3600 / SEGS_CNT );
            m_PolyCorners.push_back( currpos );
        }

        addHoleToPolygon( m_PolyCorners, m_DrillShape, m_Drill, initialpos );
        break;

    case APT_RECT:
        currpos.x  = m_Size.x / 2;
        currpos.y  = m_Size.y / 2;
        initialpos = currpos;
        m_PolyCorners.push_back( currpos );
        currpos.x -= m_Size.x;
        m_PolyCorners.push_back( currpos );
        currpos.y -= m_Size.y;
        m_PolyCorners.push_back( currpos );
        currpos.x += m_Size.x;
        m_PolyCorners.push_back( currpos );
        currpos.y += m_Size.y;
        m_PolyCorners.push_back( currpos );    // close polygon

        addHoleToPolygon( m_PolyCorners, m_DrillShape, m_Drill, initialpos );
        break;

    case APT_OVAL:
    {
        int delta, radius;

        // we create an horizontal oval shape. then rotate if needed
        if( m_Size.x > m_Size.y )   // horizontal oval
        {
            delta  = (m_Size.x - m_Size.y) / 2;
            radius = m_Size.y / 2;
        }
        else   // vertical oval
        {
            delta  = (m_Size.y - m_Size.x) / 2;
            radius = m_Size.x / 2;
        }
        currpos.y  = radius;
        initialpos = currpos;
        m_PolyCorners.push_back( currpos );

        // build the right arc of the shape
        unsigned ii = 0;
        for( ; ii <= SEGS_CNT / 2; ii++ )
        {
            currpos = initialpos;
            RotatePoint( &currpos, ii * 3600 / SEGS_CNT );
            currpos.x += delta;
            m_PolyCorners.push_back( currpos );
        }

        // build the left arc of the shape
        for( ii = SEGS_CNT / 2; ii <= SEGS_CNT; ii++ )
        {
            currpos = initialpos;
            RotatePoint( &currpos, ii * 3600 / SEGS_CNT );
            currpos.x -= delta;
            m_PolyCorners.push_back( currpos );
        }

        m_PolyCorners.push_back( initialpos );      // close outline
        if( m_Size.y > m_Size.x )                   // vertical oval, rotate polygon.
        {
            for( unsigned jj = 0; jj < m_PolyCorners.size(); jj++ )
                RotatePoint( &m_PolyCorners[jj], 900 );
        }

        addHoleToPolygon( m_PolyCorners, m_DrillShape, m_Drill, initialpos );
    }
    break;

    case APT_POLYGON:
        currpos.x  = m_Size.x >> 1;     // first point is on X axis
        initialpos = currpos;

        // rs274x said: m_EdgesCount = 3 ... 12
        if( m_EdgesCount < 3 )
            m_EdgesCount = 3;
        if( m_EdgesCount > 12 )
            m_EdgesCount = 12;
        for( int ii = 0; ii <= m_EdgesCount; ii++ )
        {
            currpos = initialpos;
            RotatePoint( &currpos, ii * 3600 / m_EdgesCount );
            m_PolyCorners.push_back( currpos );
        }

        addHoleToPolygon( m_PolyCorners, m_DrillShape, m_Drill, initialpos );
        if( m_Rotation )                   // vertical oval, rotate polygon.
        {
            int angle = wxRound( m_Rotation * 10 );
            for( unsigned jj = 0; jj < m_PolyCorners.size(); jj++ )
            {
                RotatePoint( &m_PolyCorners[jj], -angle );
            }
        }
        break;

    case APT_MACRO:

        // TODO
        break;
    }
}


// The helper function for D_CODE::ConvertShapeToPolygon().
// Add a hole to a polygon
static void addHoleToPolygon( std::vector<wxPoint>& aBuffer,
                              APERTURE_DEF_HOLETYPE aHoleShape,
                              wxSize                aSize,
                              wxPoint               aAnchorPos )
{
    wxPoint currpos;

    if( aHoleShape == APT_DEF_ROUND_HOLE )                     // build a round hole
    {
        for( int ii = 0; ii <= SEGS_CNT; ii++ )
        {
            currpos.x = 0;
            currpos.y = aSize.x / 2;      // aSize.x / 2 is the radius of the hole
            RotatePoint( &currpos, ii * 3600 / SEGS_CNT );
            aBuffer.push_back( currpos );
        }

        aBuffer.push_back( aAnchorPos );        // link to outline
    }
    if( aHoleShape == APT_DEF_RECT_HOLE )       // Create rectangular hole
    {
        currpos.x = aSize.x / 2;
        currpos.y = aSize.y / 2;
        aBuffer.push_back( currpos );     // link to hole and begin hole
        currpos.x -= aSize.x;
        aBuffer.push_back( currpos );
        currpos.y -= aSize.y;
        aBuffer.push_back( currpos );
        currpos.x += aSize.x;
        aBuffer.push_back( currpos );
        currpos.y += aSize.y;
        aBuffer.push_back( currpos );       // close hole
        aBuffer.push_back( aAnchorPos );    // link to outline
    }
}
