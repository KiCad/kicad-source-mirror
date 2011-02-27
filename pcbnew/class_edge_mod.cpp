/****************************************************/
/* class_module.cpp : EDGE_MODULE class definition. */
/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "wxstruct.h"
#include "common.h"
#include "trigo.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "colors_selection.h"

#include "pcbnew.h"
#include "class_board_design_settings.h"
#include "richio.h"

#define MAX_WIDTH 10000     /* Thickness (in 1 / 10000 ") of maximum reasonable
                             * features, text... */

/*********************/
/* class EDGE_MODULE */
/*********************/

EDGE_MODULE::EDGE_MODULE( MODULE* parent ) :
    BOARD_ITEM( parent, TYPE_EDGE_MODULE )
{
    m_Shape = S_SEGMENT;
    m_Angle = 0;
    m_Width = 120;
}


EDGE_MODULE::~EDGE_MODULE()
{
}


void EDGE_MODULE::Copy( EDGE_MODULE* source )
{
    if( source == NULL )
        return;

    m_Start  = source->m_Start;
    m_End    = source->m_End;
    m_Shape  = source->m_Shape;
    m_Start0 = source->m_Start0;
    m_End0   = source->m_End0;
    m_Angle  = source->m_Angle;
    m_Layer  = source->m_Layer;
    m_Width  = source->m_Width;

    m_PolyPoints = source->m_PolyPoints;    // std::vector copy
}


/**
 * Function GetBoundingBox
 * returns the orthogonal, bounding box of this object for display purposes.
 * This box should be an enclosing perimeter for visible components of this
 * object, and the units should be in the pcb or schematic coordinate system.
 * It is OK to overestimate the size by a few counts.
 */
EDA_Rect EDGE_MODULE::GetBoundingBox() const
{
    EDA_Rect bbox;

    bbox.SetOrigin( m_Start );

    switch( m_Shape )
    {
    case S_SEGMENT:
        bbox.SetEnd( m_End );
        bbox.Inflate( (m_Width / 2) + 1 );
        break;

    case S_CIRCLE:
        bbox.Inflate( GetRadius() + 1 );
        break;

    case S_ARC:
    {
        bbox.Inflate( GetRadius() + 1 );
    }
    break;

    case S_POLYGON:
    {
        // We must compute true coordinates from m_PolyPoints
        // which are relative to module position, orientation 0

        wxPoint p_end;
        MODULE* Module = (MODULE*) m_Parent;
        for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
        {
            wxPoint pt = m_PolyPoints[ii];

            if( Module )
            {
                RotatePoint( &pt, Module->m_Orient );
                pt += Module->m_Pos;
            }

            if( ii == 0 )
                p_end = pt;
            bbox.m_Pos.x = MIN( bbox.m_Pos.x, pt.x );
            bbox.m_Pos.y = MIN( bbox.m_Pos.y, pt.y );
            p_end.x   = MAX( p_end.x, pt.x );
            p_end.y   = MAX( p_end.y, pt.y );
        }

        bbox.SetEnd(p_end);
        bbox.Inflate( 1 );
        break;
    }
    }

    bbox.Inflate( (m_Width+1) / 2 );
    return bbox;
}


void EDGE_MODULE::SetDrawCoord()
{
    MODULE* Module = (MODULE*) m_Parent;

    m_Start = m_Start0;
    m_End   = m_End0;

    if( Module )
    {
        RotatePoint( &m_Start.x, &m_Start.y, Module->m_Orient );
        RotatePoint( &m_End.x, &m_End.y, Module->m_Orient );
        m_Start += Module->m_Pos;
        m_End   += Module->m_Pos;
    }
}


/* Draw EDGE_MODULE:
 * Entry: offset = offset trace
 * Draw_mode mode = trace (GR_OR, GR_XOR, GR_AND)
 * The contours are of different types:
 * - Segment
 * - Circles
 * - Arcs
 */
void EDGE_MODULE::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int draw_mode, const wxPoint& offset )
{
    int                  ux0, uy0, dx, dy, rayon, StAngle, EndAngle;
    int                  color, type_trace;
    int                  typeaff;
    PCB_SCREEN*          screen;
    WinEDA_BasePcbFrame* frame;
    MODULE* module = (MODULE*) m_Parent;

    if( module == NULL )
        return;

    BOARD * brd = GetBoard( );
    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    color = brd->GetLayerColor(m_Layer);

    frame = (WinEDA_BasePcbFrame*) panel->GetParent();

    screen = frame->GetScreen();

    type_trace = m_Shape;

    ux0 = m_Start.x - offset.x;
    uy0 = m_Start.y - offset.y;

    dx = m_End.x - offset.x;
    dy = m_End.y - offset.y;

    GRSetDrawMode( DC, draw_mode );
    typeaff = frame->m_DisplayModEdge;
    if( m_Layer <= LAST_COPPER_LAYER )
    {
        typeaff = frame->m_DisplayPcbTrackFill;
        if( !typeaff )
            typeaff = SKETCH;
    }

    if( DC->LogicalToDeviceXRel( m_Width ) < L_MIN_DESSIN )
        typeaff = FILAIRE;

    switch( type_trace )
    {
    case S_SEGMENT:
        if( typeaff == FILAIRE )
            GRLine( &panel->m_ClipBox, DC, ux0, uy0, dx, dy, 0, color );
        else if( typeaff == FILLED )
            GRLine( &panel->m_ClipBox, DC, ux0, uy0, dx, dy, m_Width, color );
        else
            // SKETCH Mode
            GRCSegm( &panel->m_ClipBox, DC, ux0, uy0, dx, dy, m_Width, color );
        break;

    case S_CIRCLE:
        rayon = (int) hypot( (double) (dx - ux0), (double) (dy - uy0) );
        if( typeaff == FILAIRE )
        {
            GRCircle( &panel->m_ClipBox, DC, ux0, uy0, rayon, color );
        }
        else
        {
            if( typeaff == FILLED )
            {
                GRCircle( &panel->m_ClipBox, DC, ux0, uy0, rayon,
                          m_Width, color );
            }
            else        // SKETCH Mode
            {
                GRCircle( &panel->m_ClipBox, DC, ux0, uy0,
                          rayon + (m_Width / 2), color );
                GRCircle( &panel->m_ClipBox, DC, ux0, uy0,
                          rayon - (m_Width / 2), color );
            }
        }
        break;

    case S_ARC:
        rayon    = (int) hypot( (double) (dx - ux0), (double) (dy - uy0) );
        StAngle  = (int) ArcTangente( dy - uy0, dx - ux0 );
        EndAngle = StAngle + m_Angle;
        if( StAngle > EndAngle )
            EXCHG( StAngle, EndAngle );
        if( typeaff == FILAIRE )
        {
            GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
                   rayon, color );
        }
        else if( typeaff == FILLED )
        {
            GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle, rayon,
                   m_Width, color );
        }
        else        // SKETCH Mode
        {
            GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
                   rayon + (m_Width / 2), color );
            GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
                   rayon - (m_Width / 2), color );
        }
        break;

    case S_POLYGON:

        // We must compute true coordinates from m_PolyPoints
        // which are relative to module position, orientation 0

        std::vector<wxPoint>        points = m_PolyPoints;

        for( unsigned ii = 0; ii < points.size(); ii++ )
        {
            wxPoint& pt = points[ii];

            RotatePoint( &pt.x, &pt.y, module->m_Orient );
            pt += module->m_Pos - offset;
        }

        GRPoly( &panel->m_ClipBox, DC, points.size(), &points[0],
                TRUE, m_Width, color, color );
        break;
    }
}


// see class_edge_mod.h
void EDGE_MODULE::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    wxString msg;

    MODULE*  module = (MODULE*) m_Parent;

    if( !module )
        return;

    BOARD* board = (BOARD*) module->GetParent();
    if( !board )
        return;

    frame->ClearMsgPanel();

    frame->AppendMsgPanel( _( "Graphic Item" ), wxEmptyString, DARKCYAN );

    frame->AppendMsgPanel( _( "Module" ), module->m_Reference->m_Text,
                           DARKCYAN );
    frame->AppendMsgPanel( _( "Value" ), module->m_Value->m_Text, BLUE );

    msg.Printf( wxT( "%8.8lX" ), module->m_TimeStamp );
    frame->AppendMsgPanel( _( "TimeStamp" ), msg, BROWN );

    frame->AppendMsgPanel( _( "Mod Layer" ),
                           board->GetLayerName( module->GetLayer() ), RED );

    frame->AppendMsgPanel( _( "Seg Layer" ),
                           board->GetLayerName( GetLayer() ), RED );

    valeur_param( m_Width, msg );
    frame->AppendMsgPanel( _( "Width" ), msg, BLUE );
}


/*******************************************/
bool EDGE_MODULE::Save( FILE* aFile ) const
/*******************************************/
{
    int ret = -1;

    switch( m_Shape )
    {
    case S_SEGMENT:
        ret = fprintf( aFile, "DS %d %d %d %d %d %d\n",
                       m_Start0.x, m_Start0.y,
                       m_End0.x, m_End0.y,
                       m_Width, m_Layer );
        break;

    case S_CIRCLE:
        ret = fprintf( aFile, "DC %d %d %d %d %d %d\n",
                       m_Start0.x, m_Start0.y,
                       m_End0.x, m_End0.y,
                       m_Width, m_Layer );
        break;

    case S_ARC:
        ret = fprintf( aFile, "DA %d %d %d %d %d %d %d\n",
                       m_Start0.x, m_Start0.y,
                       m_End0.x, m_End0.y,
                       m_Angle,
                       m_Width, m_Layer );
        break;

    case S_POLYGON:
        ret = fprintf( aFile, "DP %d %d %d %d %d %d %d\n",
                       m_Start0.x, m_Start0.y,
                       m_End0.x, m_End0.y,
                       (int) m_PolyPoints.size(),
                       m_Width, m_Layer );

        for( unsigned i = 0;  i<m_PolyPoints.size();  ++i )
            fprintf( aFile, "Dl %d %d\n", m_PolyPoints[i].x,
                     m_PolyPoints[i].y );

        break;

    default:

        // future: throw an exception here
#if defined(DEBUG)
        printf( "EDGE_MODULE::Save(): unexpected m_Shape: %d\n", m_Shape );
#endif
        break;
    }

    return ret > 5;
}


/* Read a description line like:
 *  DS 2600 0 2600 -600 120 21
 *  this description line is in Line
 *  EDGE_MODULE type can be:
 *  - Circle,
 *  - Segment (line)
 *  - Arc
 *  - Polygon
 *
 */
int EDGE_MODULE::ReadDescr( LINE_READER* aReader )
{
    int  ii;
    int  error = 0;
    char* Buf;
    char* Line;

    Line = aReader->Line();

    switch( Line[1] )
    {
    case 'S':
        m_Shape = S_SEGMENT;
        break;

    case 'C':
        m_Shape = S_CIRCLE;
        break;

    case 'A':
        m_Shape = S_ARC;
        break;

    case 'P':
        m_Shape = S_POLYGON;
        break;

    default:
        wxString msg;
        msg.Printf( wxT( "Unknown EDGE_MODULE type <%s>" ), Line );
        DisplayError( NULL, msg );
        error = 1;
        break;
    }

    switch( m_Shape )
    {
    case S_ARC:
        sscanf( Line + 3, "%d %d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &m_Angle, &m_Width, &m_Layer );
        NORMALIZE_ANGLE_360( m_Angle );
        break;

    case S_SEGMENT:
    case S_CIRCLE:
        sscanf( Line + 3, "%d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &m_Width, &m_Layer );
        break;

    case S_POLYGON:
        int pointCount;
        sscanf( Line + 3, "%d %d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &pointCount, &m_Width, &m_Layer );

        m_PolyPoints.clear();
        m_PolyPoints.reserve( pointCount );
        for( ii = 0;  ii<pointCount;  ii++ )
        {
            if( aReader->ReadLine() )
            {
                Buf = aReader->Line();
                if( strncmp( Buf, "Dl", 2 ) != 0 )
                {
                    error = 1;
                    break;
                }

                int x;
                int y;
                sscanf( Buf + 3, "%d %d\n", &x, &y );

                m_PolyPoints.push_back( wxPoint( x, y ) );
            }
            else
            {
                error = 1;
                break;
            }
        }

        break;

    default:
        sscanf( Line + 3, "%d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &m_Width, &m_Layer );
        break;
    }

    // Check for a reasonable width:
    if( m_Width <= 1 )
        m_Width = 1;
    if( m_Width > MAX_WIDTH )
        m_Width = MAX_WIDTH;

    // Check for a reasonable layer:
    // m_Layer must be >= FIRST_NON_COPPER_LAYER, but because microwave footprints
    // can use the copper layers m_Layer < FIRST_NON_COPPER_LAYER is allowed.
    // @todo: changes use of EDGE_MODULE these footprints and allows only m_Layer >= FIRST_NON_COPPER_LAYER
    if( (m_Layer < 0) || (m_Layer > LAST_NON_COPPER_LAYER) )
        m_Layer = SILKSCREEN_N_FRONT;
    return error;
}


wxPoint EDGE_MODULE::GetStart() const
{
    switch( m_Shape )
    {
    case S_ARC:
        return m_End;  // the start of the arc is held in field m_End, center point is in m_Start.

    case S_SEGMENT:
    default:
        return m_Start;
    }
}


wxPoint EDGE_MODULE::GetEnd() const
{
    wxPoint endPoint;         // start of arc

    switch( m_Shape )
    {
    case S_ARC:
        // rotate the starting point of the arc, given by m_End, through the
        // angle m_Angle to get the ending point of the arc.
        // m_Start is the arc centre
        endPoint  = m_End;         // m_End = start point of arc
        RotatePoint( &endPoint, m_Start, -m_Angle );
        return endPoint;   // after rotation, the end of the arc.
        break;

    case S_SEGMENT:
    default:
        return m_End;
    }
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool EDGE_MODULE::HitTest( const wxPoint& refPos )
{
    int rayon, dist;

    switch( m_Shape )
    {
    case S_SEGMENT:
        if( TestSegmentHit( refPos, m_Start, m_End, m_Width / 2 ) )
            return true;
        break;

    case S_CIRCLE:
        rayon = GetRadius();
        dist  = (int) hypot( (double) (refPos.x - m_Start.x), (double) (refPos.y - m_Start.y) );
        if( abs( rayon - dist ) <= (m_Width/2) )
            return true;
        break;

    case S_ARC:
        rayon = GetRadius();
        dist  = (int) hypot( (double) (refPos.x - m_Start.x), (double) (refPos.y - m_Start.y) );

        if( abs( rayon - dist ) > (m_Width/2) )
            break;

        int mouseAngle = ArcTangente( refPos.y - m_Start.y, refPos.x - m_Start.x );
        int stAngle    = ArcTangente( m_End.y - m_Start.y, m_End.x - m_Start.x );
        int endAngle   = stAngle + m_Angle;

        if( endAngle > 3600 )
        {
            stAngle  -= 3600;
            endAngle -= 3600;
        }

        if( (mouseAngle >= stAngle) && (mouseAngle <= endAngle) )
            return true;

        break;
    }

    return false;       // an unknown m_Shape also returns false
}


/**
 * Function HitTest (overlayed)
 * tests if the given EDA_Rect intersect this object.
 * For now, for arcs and segments, an ending point must be inside this rect.
 * @param refArea : the given EDA_Rect
 * @return bool - true if a hit, else false
 */
bool EDGE_MODULE::HitTest( EDA_Rect& refArea )
{
    switch(m_Shape)
    {
        case S_CIRCLE:
        {
            int radius = GetRadius();
            // Test if area intersects the circle:
            EDA_Rect area = refArea;
            area.Inflate(radius);
            if( area.Contains(m_Start) )
                return true;
        }
            break;

        case S_ARC:
        case S_SEGMENT:
            if( refArea.Contains( GetStart() ) )
                return true;
            if( refArea.Contains( GetEnd() ) )
                return true;
            break;
    }
    return false;
}

#if defined(DEBUG)


/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void EDGE_MODULE::Show( int nestLevel, std::ostream& os )
{
    wxString shape = ShowShape( (Track_Shapes) m_Shape );

    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " type=\"" << CONV_TO_UTF8( shape ) << "\">";

    os << " <start" << m_Start0 << "/>";
    os << " <end" << m_End0 << "/>";

    os << " </" << GetClass().Lower().mb_str() << ">\n";
}


#endif
